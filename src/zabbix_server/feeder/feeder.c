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
#include "active.h"
#include "../metrics.h"
#include "../queue.h"


extern char* CONFIG_SERVER_SITE;
extern char* CONFIG_HFS_PATH;


static int	process_id = 0;


static char	host_dec[MAX_STRING_LEN],key_dec[MAX_STRING_LEN],value_dec[MAX_STRING_LEN],error_dec[MAX_STRING_LEN];
static char	lastlogsize[MAX_STRING_LEN];
static char	timestamp[MAX_STRING_LEN];
static char	source[MAX_STRING_LEN];
static char	severity[MAX_STRING_LEN];

static metric_key_t	key_reqs = -1;
static zbx_uint64_t	mtr_reqs = 0;

static int	queue_idx[2] = { 0, 0 };
static int	queue_fd[2] = { -1, -1 };


static void    feeder_initialize_queue (int queue)
{
	int fd, len;
	const char* buf;
	static char idx_buf[256];

	/* read current index of queue */
	buf = queue_get_name (QNK_Index, queue, process_id, 0);
	fd = open (buf, O_RDONLY);

	if (fd < 0)
		queue_idx[queue] = 0;
	else {
		if ((len = read (fd, idx_buf, sizeof (idx_buf))) >= 0) {
			idx_buf[len] = 0;
			if (!sscanf (idx_buf, "%d", &queue_idx))
				queue_idx[queue] = 0;
		}
		close (fd);
	}

	/* open queue file */
	buf = queue_get_name (QNK_File, queue, process_id, queue_idx);
	queue_fd[queue] = xopen (buf, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}


static void    feeder_switch_queue (int queue)
{
	const char* buf;
	int fd;
	static char idx_buf[256];

	/* close queue file */
	close (queue_fd[queue]);

	/* update queue index */
	buf = queue_get_name (QNK_Index, queue, process_id, 0);
	fd = xopen (buf, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	ftruncate (fd, 0);
	
	queue_idx[queue]++;
	snprintf (idx_buf, sizeof (idx_buf), "%d\n", queue_idx);
	write (fd, idx_buf, strlen (idx_buf)-1);
	close (fd);

	/* reopen queue file */
	feeder_initialize_queue (queue);
}


static void    feeder_queue_data (queue_entry_t *entry, int queue)
{
	static char buf[16384];
	char* buf_p;
	int len;
	off_t ofs;

	buf_p = queue_encode_entry (entry, buf, sizeof (buf));

	/* request is too large, discard it */
	if (!buf_p) {
		zabbix_log(LOG_LEVEL_ERR, "feeder_queue_data: Request is too large and won't fit in %d bytes buffer", sizeof (buf));
		return;
	}

	len = buf_p-buf;
	write (queue_fd[queue], &len, sizeof (len));
	write (queue_fd[queue], buf, buf_p-buf);

	/* switch queue if current file grown too large */
	ofs = lseek (queue_fd[queue], 0, SEEK_CUR);
	if (ofs > QUEUE_SIZE_LIMIT)
		feeder_switch_queue (queue);
}


void	process_feeder_child(zbx_sock_t *sock)
{
	char	*data, *line, *host;
	char	*server,*key = NULL,*value_string, *error = NULL;
	int	ret = SUCCEED;
	queue_entry_t entry;

	if(zbx_tcp_recv(sock, &data) != SUCCEED)
		return;

	zbx_rtrim(data, " \r\n\0");

	switch (data[0]) {
	case 'Z':
		if(strncmp (data, "ZBX_GET_ACTIVE_CHECKS", 20) == 0)
		{
			line = strtok(data,"\n");
			host = strtok(NULL,"\n");
			if(host)
				ret = send_list_of_active_checks(sock, host);
		}
		break;	
	case '<':
		if (strncmp (data, "<req>", 5) == 0)
		{
			comms_parse_response(data, host_dec, key_dec, value_dec, error_dec, lastlogsize, timestamp, source, severity, sizeof(host_dec)-1);
			entry.ts = time (NULL);
			entry.server = host_dec;
			entry.value = value_dec;
			entry.error = error_dec;
			entry.key = key_dec;
			entry.lastlogsize = lastlogsize;
			entry.timestamp = timestamp;
			entry.source = source;
			entry.severity = severity;

			/* append to queue for trapper */
			feeder_queue_data (&entry, 0);
		}

		if( zbx_tcp_send_raw(sock, SUCCEED == ret ? "OK" : "NOT OK") != SUCCEED)
			zabbix_log(LOG_LEVEL_WARNING, "Error sending result back");
		break;
	}

	metric_update (key_reqs, mtr_reqs++);
}


void	child_feeder_main(int i, zbx_sock_t *s)
{
	zabbix_log( LOG_LEVEL_DEBUG, "In child_feeder_main()");

	zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Feeder]", i);

	process_id = i;
	key_reqs  = metric_register ("feeder_reqs",  i);

	feeder_initialize_queue (0);
	feeder_initialize_queue (1);

	DBconnect(ZBX_DB_CONNECT_NORMAL);
	for(;;)
	{
		zbx_setproctitle("Feeder waiting for connection");
		if (zbx_tcp_accept(s) != SUCCEED)
			zabbix_log(LOG_LEVEL_ERR, "feeder failed to accept connection");
		else {
			zbx_setproctitle("Feeder processing data");
			process_feeder_child(s);
			zbx_tcp_unaccept(s);
                }
	}
	DBclose();
}

