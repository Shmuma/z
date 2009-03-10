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


#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <errno.h>
#include <string.h>

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

static zbx_uint64_t mtr_skipped = 0;
static metric_key_t key_skipped;


static int 		process_id = 0;
static int		history = 0;

/* queue file descriptor */
static int		queue_fd =  -1;


/* message buffer */
typedef struct {
	long type;
	char buf[1];
} msg_hdr_t;

static int msg_size = 0;
static msg_hdr_t* msg = NULL;



static void trapper_initialize_queue ()
{
	key_t key;

	key = ftok ("/tmp", process_id*2 + history);

	if (key < 0) {
		zabbix_log (LOG_LEVEL_ERR, "trapper_initialize_queue: Cannot obtain IPC queue ID: %s", strerror (errno));
		return;
	}

	queue_fd = msgget (key, IPC_CREAT | 0666);

	if (queue_fd < 0) {
		zabbix_log (LOG_LEVEL_ERR, "trapper_initialize_queue: Cannot initialize IPC queue: %s", strerror (errno));
		return;
	}

	msg_size = 1024;
	msg = (msg_hdr_t*)malloc (msg_size);
}


static ssize_t dequeue_next_message ()
{
	ssize_t len;

	while (1) {
		len = msgrcv (queue_fd, msg, msg_size, 0, 0);

		if (len < 0 && errno == E2BIG) {
			/* message buffer is too small, allocate a little larger buffer */
			msg_size += 1024;
			msg = (msg_hdr_t*)realloc (msg, msg_size);
			if (!msg)
				msg_size = 0;
			continue;
		}

		return len;
	}
}



static int	trapper_dequeue_request (queue_entry_t* entry)
{
	ssize_t len;

	while (1) {
		len = dequeue_next_message ();

		if (len < 0) {
			/* something really bad happen with our queue */
			return 0;
		}

		if (queue_decode_entry (entry, msg->buf, len-sizeof (long)))
			break;
	}

	return 1;
}


static int trapper_dequeue_history (queue_history_entry_t* entry)
{
	ssize_t len;

	while (1) {
		len = dequeue_next_message ();

		if (len < 0) {
			/* something really bad happen with our queue */
			return 0;
		}

		if (queue_decode_history (entry, msg->buf, len-sizeof (long)))
			break;
	}

	return 1;
}



void	child_trapper_main(int i)
{
	int count, history;
	queue_entry_t entry;

	zabbix_log( LOG_LEVEL_DEBUG, "In child_trapper_main()");
	zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Trapper]", i);

	process_id = i;
	history = 0;

	/* initialize queue */
	trapper_initialize_queue ();

	/* initialize metrics */
	key_values = metric_register ("trapper_data_values",  i);
	key_skipped = metric_register ("trapper_data_skipped_bytes",  i);
	metric_update (key_skipped, mtr_skipped);

	DBconnect(ZBX_DB_CONNECT_NORMAL);

	for(;;)
	{
		zbx_setproctitle("Trapper waiting for new queue data");

		/* First we try to get data from primary queue */
		if (trapper_dequeue_request (&entry)) {
			zbx_setproctitle("processing queue data block");

			if (!strcmp (entry.key, "profile"))
				process_profile_value (entry.server, entry.error);
			process_data (0, entry.ts, entry.server, entry.key, entry.value,
				      entry.error, entry.lastlogsize, entry.timestamp,
				      entry.source, entry.severity);
			free (entry.buf);
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
	key_skipped = metric_register ("trapper_history_skipped_bytes",  i);
	metric_update (key_skipped, mtr_skipped);

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
			free (entry.items);
		}
	}

	DBclose();
}
