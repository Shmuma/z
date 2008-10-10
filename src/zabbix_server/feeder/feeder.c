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
#include "../metrics.h"


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

static int	queue_fd = -1;



static void    feeder_queue_data (const char* server, const char* key, const char* value, const char* error, 
				  const char* lastlogsize, const char* timestamp, const char* source, const char* severity)
{
	static char name[256];
	static char dir[256];
	int len;

	if (queue_fd == -1) {
		snprintf (name, sizeof (name), "%s/%s/queue/q_feeder.%d", CONFIG_HFS_PATH, CONFIG_SERVER_SITE, process_id);
		queue_fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (queue_fd == -1)
			return;
	}

	write_str (queue_fd, server);
	write_str (queue_fd, key);
	write_str (queue_fd, value);
	write_str (queue_fd, error);
	write_str (queue_fd, lastlogsize);
	write_str (queue_fd, timestamp);
	write_str (queue_fd, source);
	write_str (queue_fd, severity);
}


void	process_feeder_child(zbx_sock_t *sock)
{
	char	*data, *line, *host;
	char	*server,*key = NULL,*value_string, *error = NULL;
	int	ret = SUCCEED;

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
			server=host_dec;
			value_string=value_dec;
			error = error_dec;
			key=key_dec;
			/* append to queue for trapper */
			feeder_queue_data (host_dec, key_dec, value_dec, error_dec, lastlogsize, timestamp, source, severity);
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

