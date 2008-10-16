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

static zbx_uint64_t mtr_checks = 0;
static metric_key_t key_checks;

static zbx_uint64_t mtr_history = 0;
static metric_key_t key_history;

static zbx_uint64_t mtr_reqs = 0;
static metric_key_t key_reqs;

static zbx_uint64_t mtr_idle = 0;
static metric_key_t key_idle;

static int 		process_id = 0;

/* queue index (part of file name) */
static int 		queue_idx[2] = { 0, 0 };
/* queue offset */
static hfs_off_t	queue_ofs[2] = { 0, 0 };
/* queue file descriptor */
static int		queue_fd[2] =  { -1, -1 };

static int		queue_inotify_fd[2] = { -1, -1 };
static int		queue_inotify_wd[2] = { -1, -1 };

#define QUEUE_CHUNK 10

static queue_entry_t req_buf[QUEUE_CHUNK];


static void trapper_initialize_queue (int index)
{
	const char* name;
	static char buf[256];
	int fd;

	/* obtain trapper queue index and position */
	name = queue_get_name (QNK_Position, index, process_id, 0);

	fd = open (name, O_RDONLY);
	if (fd >= 0) {
		read (fd, buf, sizeof (buf));
		if (sscanf (buf, "%d %lld", &queue_idx, &queue_ofs) != 2) {
			queue_idx[index] = 0;
			queue_ofs[index] = 0;
		}
		close (fd);
	}

	/* initialize inotify instance */
	queue_inotify_fd[index] = inotify_init ();
}


static void trapper_update_ofs (int index)
{
	const char* name;
	static char buf[256];
	int fd;

	/* obtain trapper queue index and position */
	name = queue_get_name (QNK_Position, index, process_id, 0);

	fd = open (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd >= 0) {
		snprintf (buf, sizeof (buf), "%d %lld\n", queue_idx[index], queue_ofs[index]);
		write (fd, buf, strlen (buf));
		close (fd);
	}
}


/* Trying to open queue file according to current index. If filed, try with next index until correct file not found. */
static int trapper_open_queue (int index)
{
	const char* name;
	int attempts = 0;

	while (attempts < 1000) {
		name = queue_get_name (QNK_File, index, process_id, queue_idx + attempts);
		queue_fd[index] = open (name, O_RDONLY);
		attempts++;
		if (queue_fd[index] >= 0) {
			lseek (queue_fd[index], queue_ofs, SEEK_SET);
			queue_inotify_wd[index] = inotify_add_watch (queue_inotify_fd[index], name, IN_MODIFY);
			break;
		}
	}

	return queue_fd[index] >= 0;
}


/* unlink old queue file (if opened) and open queue file with next index */
static int trapper_open_next_queue (int index)
{
	const char* name;

	close (queue_fd);
	name = queue_get_name (QNK_File, index, process_id, queue_idx);
	inotify_rm_watch (queue_inotify_fd[index], queue_inotify_wd[index]);
	unlink (name);
	queue_idx[index]++;
	queue_ofs[index] = 0;
	return trapper_open_queue (index);
}


/* Here we dequeue predefined amount of requests from queue file. If file is not open so far or
   suddenly finished, we trying to reopen next file according to index. */
static int	trapper_dequeue_requests (queue_entry_t** entries, int index)
{
	static int buf[16384];
	int req_len, len;
	int count = 0;
	struct inotify_event ie;

	*entries = req_buf;

	if (queue_fd[index] < 0) {
		/* if queue cannot be opened or found, sleep for some time */
		if (!trapper_open_queue (index))
			return count;
	}

	while (count < QUEUE_CHUNK) {
		/* get request length */
		while (!read (queue_fd[index], &req_len, sizeof (req_len))) {
			if (queue_ofs[index] > QUEUE_SIZE_LIMIT)
				if (!trapper_open_next_queue (index))
					return count;
			read (queue_inotify_fd[index], &ie, sizeof (ie));
		}
		len = 0;
		while (len < req_len) {
			len += read (queue_fd[index], buf+len, req_len - len);
			if (len < req_len)
				read (queue_inotify_fd[index], &ie, sizeof (ie));
		}

		queue_ofs[index] += sizeof (req_len) + req_len;

		/* decode request data */
		queue_decode_entry (req_buf+count, buf, req_len);
		count++;

		if (queue_ofs[index] > QUEUE_SIZE_LIMIT)
			if (!trapper_open_next_queue (index))
				return count;
	}

	/* update queue_ofs */
	trapper_update_ofs (index);

	return count;
}



void	child_trapper_main(int i)
{
	int count;
	queue_entry_t* entries;
	hfs_time_t now;

	zabbix_log( LOG_LEVEL_DEBUG, "In child_trapper_main()");

	zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Trapper]", i);

	key_values  = metric_register ("trapper_values",  i);
	key_checks  = metric_register ("trapper_checks",  i);
	key_history = metric_register ("trapper_history", i);
	key_reqs    = metric_register ("trapper_reqs", i);
	key_idle    = metric_register ("trapper_idle", i);

/*  	sleep (120);  */

	process_id = i;

	/* initialize queue */
	trapper_initialize_queue (0);
	trapper_initialize_queue (1);

	DBconnect(ZBX_DB_CONNECT_NORMAL);

	for(;;)
	{
		zbx_setproctitle("Trapper waiting for new queue data");

		/* dequeue data block to process */
		if ((count = trapper_dequeue_requests (&entries, 0)) > 0) {
			now = time (NULL);

			/* handle data block */
			zbx_setproctitle("processing queue data block");
			for (i = 0; i < count; i++) {
				process_data (entries[i].ts, entries[i].server, entries[i].key, entries[i].value, 
					      entries[i].error, entries[i].lastlogsize, entries[i].timestamp, 
					      entries[i].source, entries[i].severity);
				free (entries[i].buf);
			}
		}
		else {
			metric_update (key_idle, ++mtr_idle);
			zbx_setproctitle("Trapper sleeping for 1 second, waiting for queue to appear");
			sleep (1);
		}
	}
	DBclose();
}

