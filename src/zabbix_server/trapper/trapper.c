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

#include "nodesync.h"
#include "nodeevents.h"
#include "nodehistory.h"
#include "trapper.h"
#include "active.h"

#include "daemon.h"

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
					ret = process_data(sock,server,key,value_string, NULL, NULL, NULL, NULL, NULL, timestamp);
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
			ret=process_data(sock,server,key,value_string,error,lastlogsize,timestamp,source,severity, NULL);
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

void	process_trapper_child(zbx_sock_t *sock)
{
	char	*data;

/* suseconds_t is not defined under HP-UX */
/*	struct timeval tv;
	suseconds_t    msec;
	gettimeofday(&tv, NULL);
	msec = tv.tv_usec;*/

/*	alarm(CONFIG_TIMEOUT);*/

	if(zbx_tcp_recv(sock, &data) != SUCCEED)
	{
            zabbix_log (LOG_LEVEL_DEBUG, "zbx_tcp_recv faield. Error: %s (%s), code %d", 
                        zbx_tcp_strerror (), strerror_from_system(zbx_sock_last_error ()), zbx_sock_last_error ());
/*		alarm(0);*/
		return;
	}

	process_trap(sock, data, sizeof(data));
/*	alarm(0);*/

/*	gettimeofday(&tv, NULL);
	zabbix_log( LOG_LEVEL_DEBUG, "Trap processed in " ZBX_FS_DBL " seconds",
		(double)(tv.tv_usec-msec)/1000000 );*/
}

void	child_trapper_main(int i, zbx_sock_t *s)
{
	zabbix_log( LOG_LEVEL_DEBUG, "In child_trapper_main()");

	zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Trapper]", i);

	key_values  = metric_register ("trapper_values",  i);
	key_checks  = metric_register ("trapper_checks",  i);
	key_history = metric_register ("trapper_history", i);
	key_reqs    = metric_register ("trapper_reqs", i);

	DBconnect(ZBX_DB_CONNECT_NORMAL);

	for(;;)
	{
		zbx_setproctitle("waiting for connection");
		if (zbx_tcp_accept(s) != SUCCEED)
			zabbix_log(LOG_LEVEL_ERR, "trapper failed to accept connection");
		else {
			zbx_setproctitle("processing data");
			mtr_reqs++;
			metric_update (key_reqs, mtr_reqs);
			process_trapper_child(s);

			zbx_tcp_unaccept(s);
                }
	}
	DBclose();
}

/*
pid_t	child_trapper_make(int i,int listenfd, int addrlen)
{
	pid_t	pid;

	if((pid = zbx_fork()) >0)
	{
		return (pid);
	}

	child_trapper_main(i, listenfd, addrlen);

	return 0;
}*/
