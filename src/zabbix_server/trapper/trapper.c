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
#include "active.h"

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
static int 		queue_idx = 0;
/* queue offset */
static hfs_off_t	queue_ofs = 0;
/* queue file descriptor */
static int		queue_fd = -1;

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
	name = queue_get_name (QNK_Position, process_id, 0);

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
	name = queue_get_name (QNK_Position, process_id, 0);

	fd = open (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd >= 0) {
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
		name = queue_get_name (QNK_File, process_id, queue_idx + attempts);
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
	name = queue_get_name (QNK_File, process_id, queue_idx);
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
	static int buf[16384];
	int req_len, len;
	int count = 0;
	struct inotify_event ie;

	*entries = req_buf;

	if (queue_fd < 0) {
		/* if queue cannot be opened or found, sleep for some time */
		if (!trapper_open_queue ())
			return count;
	}

	while (count < QUEUE_CHUNK) {
		/* get request length */
		while (!read (queue_fd, &req_len, sizeof (req_len))) {
			if (queue_ofs > QUEUE_SIZE_LIMIT)
				if (!trapper_open_next_queue ())
					return count;
			read (queue_inotify_fd, &ie, sizeof (ie));
		}
		len = 0;
		while (len < req_len) {
			len += read (queue_fd, buf+len, req_len - len);
			if (len < req_len)
				read (queue_inotify_fd, &ie, sizeof (ie));
		}

		queue_ofs += sizeof (req_len) + req_len;

		/* decode request data */
		queue_decode_entry (req_buf+count, buf, req_len);
		count++;

		if (queue_ofs > QUEUE_SIZE_LIMIT)
			if (!trapper_open_next_queue ())
				return count;
	}

	/* update queue_ofs */
	trapper_update_ofs ();

	return count;
}



static int	process_trap(zbx_sock_t	*sock,char *s, int max_len)
{
	char	*line,*host;
	char	*server,*key = NULL,*value_string, *error = NULL;
	char	copy[MAX_STRING_LEN];
	char	host_dec[MAX_STRING_LEN],key_dec[MAX_STRING_LEN],value_dec[MAX_STRING_LEN],error_dec[MAX_STRING_LEN];
	char	lastlogsize[MAX_STRING_LEN];
	char	timestamp[MAX_STRING_LEN];
	char	source[MAX_STRING_LEN];
	char	severity[MAX_STRING_LEN];

	int	ret=SUCCEED;

	zbx_rtrim(s, " \r\n\0");
	zabbix_log( LOG_LEVEL_DEBUG, "Trapper got [%s] len %d", s, strlen(s));

/* Request for list of active checks */
	switch (s[0]) {
	case 'Z':
		if(strncmp (s,"ZBX_GET_ACTIVE_CHECKS", 20) == 0)
		{
			line=strtok(s,"\n");
			host=strtok(NULL,"\n");
			if(host)
				ret = send_list_of_active_checks(sock, host);
			mtr_checks++;
			metric_update (key_checks, mtr_checks);
		}
		break;

	case '<':
		zabbix_log( LOG_LEVEL_DEBUG, "XML received [%s]", s);

		if (strncmp (s, "<req>", 5) == 0)
		{
			comms_parse_response(s,host_dec,key_dec,value_dec,error_dec,lastlogsize,timestamp,source,severity,sizeof(host_dec)-1);
			server=host_dec;
			value_string=value_dec;
			error = error_dec;
			key=key_dec;
			mtr_values++;
			metric_update (key_values, mtr_values);
		}

		if (strncmp (s, "<reqs>", 6) == 0)
		{
			/* TODO: improve history performance */
			ret = SUCCEED;
			value_string = NULL;

			void* token = NULL;
			void* history_token = NULL;

			if (CONFIG_HFS_PATH) {
				/* perform ultra-fast history addition */
				while (comms_parse_multi_response (s,host_dec,key_dec,value_dec,lastlogsize,timestamp,source,severity,
								   sizeof(host_dec)-1, &token) == SUCCEED)
				{
					append_history (host_dec, key_dec, value_dec, timestamp, &history_token);
					mtr_history++;
				}
				flush_history (&history_token);
				metric_update (key_history, mtr_history);
			}
			else {
				DBbegin();
				while (comms_parse_multi_response (s,host_dec,key_dec,value_dec,lastlogsize,timestamp,source,severity,
								   sizeof(host_dec)-1, &token) == SUCCEED)
				{
					server = host_dec;
					value_string = value_dec;
					key = key_dec;
					/* insert history value. It doesn't support  */
/* 					ret = process_data(sock,server,key,value_string, NULL, NULL, NULL, NULL, NULL, timestamp); */
					if (ret != SUCCEED)
						break;
				}
				DBcommit();
			}
			key = NULL;
		}

		if (value_string)
			zabbix_log( LOG_LEVEL_DEBUG, "Value [%s]", value_string);

		if (key)
		{
			DBbegin();
/* 			ret=process_data(sock,server,key,value_string,error,lastlogsize,timestamp,source,severity, NULL); */
			DBcommit();
		}
		
		if( zbx_tcp_send_raw(sock, SUCCEED == ret ? "OK" : "NOT OK") != SUCCEED)
		{
			zabbix_log( LOG_LEVEL_WARNING, "Error sending result back");
			zabbix_syslog("Trapper: error sending result back");
		}
		zabbix_log( LOG_LEVEL_DEBUG, "After write()");

		break;
	}
	return ret;
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
	trapper_initialize_queue ();

	DBconnect(ZBX_DB_CONNECT_NORMAL);

	for(;;)
	{
		zbx_setproctitle("Trapper waiting for new queue data");

		/* dequeue data block to process */
		if ((count = trapper_dequeue_requests (&entries)) > 0) {
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

