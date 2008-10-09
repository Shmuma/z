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


extern char* CONFIG_SERVER_SITE;
extern char* CONFIG_HFS_PATH;



void	child_feeder_main(int i, zbx_sock_t *s)
{
	zabbix_log( LOG_LEVEL_DEBUG, "In child_feeder_main()");

	zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Feeder]", i);

	DBconnect(ZBX_DB_CONNECT_NORMAL);
	for(;;)
	{
		zbx_setproctitle("waiting for connection");
/* 		if (zbx_tcp_accept(s) != SUCCEED) */
/* 			zabbix_log(LOG_LEVEL_ERR, "trapper failed to accept connection"); */
/* 		else { */
/* 			zbx_setproctitle("processing data"); */
/* 			process_trapper_child(s); */

/* 			zbx_tcp_unaccept(s); */
/*                 } */
		sleep (100);
	}
	DBclose();
}

