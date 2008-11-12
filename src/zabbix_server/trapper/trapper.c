/* 
** ZABBIX
** Copyright (C) 2000-2005 SIA Zabbix
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**/

#include "common.h"

#include "cfg.h"
#include "comms.h"
#include "pid.h"
#include "db.h"
#include "log.h"
#include "zlog.h"
#include "hfs.h"

#include "../functions.h"
#include "../expression.h"
#include "../metrics.h"
#include "../queue.h"

#include "nodesync.h"
#include "nodeevents.h"
#include "nodehistory.h"
#include "trapper.h"

#include "daemon.h"

#include <sys/inotify.h>

extern char* CONFIG_SERVER_SITE;
extern char* CONFIG_HFS_PATH;

static zbx_uint64_t mtr_values = 0;
static metric_key_t key_values;

static int 		process_id = 0;
static int		history = 0;

/* queue index (part of file name) */
static int 		queue_idx = 0;
/* queue offset */
static hfs_off_t	queue_ofs = 0;
/* queue file descriptor */
static int		queue_fd =  -1;

static int		queue_inotify_fd = -1;
static int		queue_inotify_wd = -1;

#define QUEUE_CHUNK 10

static queue_entry_t req_buf[QUEUE_CHUNK];


static void trapper_initialize_queue ()
{
	const char* name;
	static char buf[256];
	int fd;

	/* obtain trapper queue index and position */
	name = queue_get_name (QNK_Position, history, process_id, 0);

	fd = open (name, O_RDONLY);
	if (fd >= 0) {
		read (fd, buf, sizeof (buf));
		if (sscanf (buf, "%d %lld", &queue_idx, &queue_ofs) != 2) {
			queue_idx = 0;
			queue_ofs = 0;
		}
		close (fd);
	}

	/* initialize inotify instance */
	queue_inotify_fd = inotify_init ();
}


static void trapper_update_ofs ()
{
	const char* name;
	static char buf[256];
	int fd;

	/* obtain trapper queue index and position */
	name = queue_get_name (QNK_Position, history, process_id, 0);

	fd = open (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd >= 0) {
		ftruncate (fd, 0);
		snprintf (buf, sizeof (buf), "%d %lld\n", queue_idx, queue_ofs);
		write (fd, buf, strlen (buf));
		close (fd);
	}
}


/* Trying to open queue file according to current index. If filed, try with next index until correct file not found. */
static int trapper_open_queue ()
{
	const char* name;
	int attempts = 0;

	while (attempts < 1000) {
		name = queue_get_name (QNK_File, history, process_id, queue_idx + attempts);
		queue_fd = open (name, O_RDONLY);
		attempts++;
		if (queue_fd >= 0) {
			lseek (queue_fd, queue_ofs, SEEK_SET);
			queue_inotify_wd = inotify_add_watch (queue_inotify_fd, name, IN_MODIFY);
			break;
		}
	}

	return queue_fd >= 0;
}


/* unlink old queue file (if opened) and open queue file with next index */
static int trapper_open_next_queue ()
{
	const char* name;

	close (queue_fd);
	name = queue_get_name (QNK_File, history, process_id, queue_idx);
	inotify_rm_watch (queue_inotify_fd, queue_inotify_wd);
	unlink (name);
	queue_idx++;
	queue_ofs = 0;
	return trapper_open_queue ();
}


/* Here we dequeue predefined amount of requests from queue file. If file is not open so far or
   suddenly finished, we trying to reopen next file according to index. */
static int	trapper_dequeue_requests (queue_entry_t** entries)
{
	static char buf[16384];
	int req_len, len, res;
	int count = 0;
	struct inotify_event ie;
	fd_set fds;
	struct timeval tv;

	*entries = req_buf;

	if (queue_fd < 0) {
		/* if queue cannot be opened or found, sleep for some time */
		if (!trapper_open_queue ())
			return 0;
	}

	while (count < QUEUE_CHUNK) {
		/* get request length */
		while (read (queue_fd, &req_len, sizeof (req_len)) <= 0) {
			if (queue_ofs > QUEUE_SIZE_LIMIT)
				if (!trapper_open_next_queue ())
					return count;

			/* Wait for new data portion to appear, not
			   more than 5 seconds. This is needed to
			   prevent situation when some agent send data
			   too slow until socket timeout occured. If
			   this happened, we'll wait for data and
			   already read entries remain unprocessed
			   until timeout expired.*/
			FD_ZERO (&fds);
			FD_SET (queue_inotify_fd, &fds);
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			if (select (queue_inotify_fd + 1, &fds, NULL, NULL, &tv) > 0)
				read (queue_inotify_fd, &ie, sizeof (ie));
			else {
				if (count > 0)
					return count;
			}
		}
		len = 0;
		while (len < req_len) {
			res = read (queue_fd, buf+len, req_len - len);
			if (res > 0)
				len += res;
			if (len < req_len)
				read (queue_inotify_fd, &ie, sizeof (ie));
		}

		queue_ofs += sizeof (req_len) + req_len;

		/* decode request data */
		if (queue_decode_entry (req_buf+count, buf, req_len))
			count++;

		if (queue_ofs > QUEUE_SIZE_LIMIT)
			if (!trapper_open_next_queue ())
				return count;
	}

	/* update queue_ofs */
	trapper_update_ofs ();
	return count;
}


static int trapper_dequeue_history (queue_history_entry_t* entry)
{
	char* buf;
	struct inotify_event ie;
	int req_len, len, res, count = 0;

	if (queue_fd < 0) {
		/* if queue cannot be opened or found, sleep for some time */
		if (!trapper_open_queue ())
			return count;
	}

	while (read (queue_fd, &req_len, sizeof (len)) <= 0) {
		if (queue_ofs > QUEUE_SIZE_LIMIT)
			if (!trapper_open_next_queue ())
				return 0;

		read (queue_inotify_fd, &ie, sizeof (ie));
	}

	/* allocate buffer for history data */
	buf = (char*)malloc (req_len);
	len = 0;

	while (len < req_len) {
		res = read (queue_fd, buf+len, req_len - len);
		if (res > 0)
			len += res;
		if (len < req_len)
			read (queue_inotify_fd, &ie, sizeof (ie));
	}

	queue_ofs += sizeof (req_len) + req_len;

	/* decode request data */
	if (queue_decode_history (entry, buf, req_len))
		count = 1;
	else
		free (buf);

	if (queue_ofs > QUEUE_SIZE_LIMIT)
		if (!trapper_open_next_queue ())
			return count;

	trapper_update_ofs ();
	return count;
}



void	child_trapper_main(int i)
{
	int count, history;
	queue_entry_t* entries;
	hfs_time_t now;

	zabbix_log( LOG_LEVEL_DEBUG, "In child_trapper_main()");
	zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Trapper]", i);

	process_id = i;
	history = 0;

	/* initialize queue */
	trapper_initialize_queue ();

	/* initialize metrics */
	key_values = metric_register ("trapper_data_values",  i);

	DBconnect(ZBX_DB_CONNECT_NORMAL);

	for(;;)
	{
		zbx_setproctitle("Trapper waiting for new queue data");

		/* First we try to get data from primary queue */
		count = trapper_dequeue_requests (&entries);

		/* dequeue data block to process */
		if (count) {
			now = time (NULL);
			/* handle data block */
			zbx_setproctitle("processing queue data block");
			mtr_values += count;
			metric_update (key_values, mtr_values);
			for (i = 0; i < count; i++) {
				process_data ((now - entries[i].ts) > 60, entries[i].ts, entries[i].server, entries[i].key, entries[i].value,
					      entries[i].error, entries[i].lastlogsize, entries[i].timestamp,
					      entries[i].source, entries[i].severity);
				free (entries[i].buf);
			}
		}
	}
	DBclose();
}



void	child_hist_trapper_main (int i)
{
	int count;
	queue_history_entry_t entry;
	void* history_token;

	zabbix_log( LOG_LEVEL_DEBUG, "In child_trapper_main()");
	zabbix_log( LOG_LEVEL_WARNING, "server #%d started [HistTrapper]", i);

	process_id = i;
	history = 1;

	trapper_initialize_queue ();

	key_values = metric_register ("trapper_history_values",  i);

	DBconnect(ZBX_DB_CONNECT_NORMAL);

	for (;;) {
		zbx_setproctitle("Trapper waiting for new history data");

		/* First we try to get data from primary queue */
 		count = trapper_dequeue_history (&entry);

		if (count) {
			history_token = NULL;

			mtr_values += entry.count;
			metric_update (key_values, mtr_values);

			for (i = 0; i < entry.count; i++)
				append_history (entry.server, entry.key, entry.items[i].value, entry.items[i].ts, &history_token);
			flush_history (&history_token);
			free (entry.buf);
			free (entry.items);
		}
	}

	DBclose();
}
