/* 
** ZABBIX
** Copyright (C) 2000-2006 SIA Zabbix
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
#include "db.h"
#include "log.h"
#include "zlog.h"

#include "comms.h"
#include "nodecomms.h"

/******************************************************************************
 *                                                                            *
 * Function: send_to_node                                                     *
 *                                                                            *
 * Purpose: send configuration changes to required node                       *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: SUCCESS - processed succesfully                              * 
 *               FAIL - an error occured                                      *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
int send_to_node(char *name, int dest_nodeid, int nodeid, char *data)
{
	char	ip[MAX_STRING_LEN];
	int	port;
	int	ret = FAIL;

	DB_RESULT	result;
	DB_ROW		row;

	zbx_sock_t	sock;
	char		*answer;

	zabbix_log( LOG_LEVEL_WARNING, "NODE: Sending %s to node %s:%d datalen %d",
		name,
		CONFIG_MASTER_IP, CONFIG_MASTER_PORT,
		strlen(data));
/*	zabbix_log( LOG_LEVEL_WARNING, "Data [%s]", data);*/

	if( FAIL == zbx_tcp_connect(&sock, CONFIG_MASTER_IP, CONFIG_MASTER_PORT, 0, NULL))
	{
		zabbix_log(LOG_LEVEL_DEBUG, "Unable to connect to Node [%s:%d] error: %s", CONFIG_MASTER_IP, CONFIG_MASTER_PORT, zbx_tcp_strerror());
		return  FAIL;
	}


	if( FAIL == zbx_tcp_send(&sock, data))
	{
		zabbix_log( LOG_LEVEL_WARNING, "Error while sending data to Node [%s:%d]",
			CONFIG_MASTER_IP, CONFIG_MASTER_PORT);
		zbx_tcp_close(&sock);
		return  FAIL;
	}

	if( FAIL == zbx_tcp_recv(&sock, &answer))
	{
		zabbix_log( LOG_LEVEL_WARNING, "Error while receiving answer from Node [%s:%d]",
			CONFIG_MASTER_IP, CONFIG_MASTER_PORT);
		zbx_tcp_close(&sock);
		return  FAIL;
	}

	zabbix_log( LOG_LEVEL_DEBUG, "Answer [%s]",
		answer);

	if(strcmp(answer,"OK") == 0)
	{
		zabbix_log( LOG_LEVEL_DEBUG, "OK");
		ret = SUCCEED;
	}
	else
	{
		zabbix_log( LOG_LEVEL_WARNING, "NOT OK");
	}

	zbx_tcp_close(&sock);

	return ret;
}
