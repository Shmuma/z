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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>

#include <string.h>

#include <time.h>

#include <sys/socket.h>
#include <errno.h>

/* Functions: pow(), round() */
#include <math.h>

#include "common.h"
#include "db.h"
#include "log.h"
#include "zlog.h"

#include "active.h"

/******************************************************************************
 *                                                                            *
 * Function: send_list_of_active_checks                                       *
 *                                                                            *
 * Purpose: send list of active checks to the host                            *
 *                                                                            *
 * Parameters: sockfd - open socket of server-agent connection                *
 *             host - hostname                                                *
 *                                                                            *
 * Return value:  SUCCEED - list of active checks sent succesfully            *
 *                FAIL - an error occured                                     *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments: format of the list: key:delay:last_log_size                      *
 *                                                                            *
 ******************************************************************************/
int	send_list_of_active_checks(zbx_sock_t *sock, const char *host)
{
	char	s[MAX_STRING_LEN];
	DB_RESULT result;
	DB_ROW	row;
	int have_checks = 0;

#if !defined(HAVE_IPV6)
	ZBX_SOCKADDR    name;
	struct          hostent *hp;
	char            *sip;
	int             i[4], j[4];
	socklen_t       nlen;
#endif
	char		sname[MAX_STRING_LEN];

	zabbix_log( LOG_LEVEL_DEBUG, "In send_list_of_active_checks()");

	if (0 != CONFIG_REFRESH_UNSUPPORTED) {
		result = DBselect("select i.key_,i.delay,i.lastlogsize from items i,hosts h "
			"where i.hostid=h.hostid and h.status=%d and i.type=%d and h.host='%s' "
			"and (i.status=%d or (i.status=%d and i.nextcheck<=%d))",
			HOST_STATUS_MONITORED,
			ITEM_TYPE_ZABBIX_ACTIVE,
			host,
			ITEM_STATUS_ACTIVE, ITEM_STATUS_NOTSUPPORTED, time(NULL));
	} else {
		result = DBselect("select i.key_,i.delay,i.lastlogsize from items i,hosts h "
			"where i.hostid=h.hostid and h.status=%d and i.type=%d and h.host='%s' "
			"and i.status=%d",
			HOST_STATUS_MONITORED,
			ITEM_TYPE_ZABBIX_ACTIVE,
			host,
			ITEM_STATUS_ACTIVE);
	}

	while((row=DBfetch(result)))
	{
		zbx_snprintf(s,sizeof(s),"%s:%s:%s\n",
			row[0],
			row[1],
			row[2]);
		zabbix_log( LOG_LEVEL_DEBUG, "Sending [%s]",
			s);

		if( zbx_tcp_send_raw(sock,s) != SUCCEED )
		{
			zabbix_log( LOG_LEVEL_WARNING, "Error while sending list of active checks");
			return  FAIL;
		}
		have_checks++;
	}
	DBfree_result(result);
	
	if (!have_checks) {
#if defined(HAVE_IPV6)
	    sname = "IPV6_TODO";
#else
	    nlen = sizeof(name);
	    if(ZBX_TCP_ERROR == getpeername(sock->socket, (struct sockaddr*)&name, &nlen)) {
		zabbix_log(LOG_LEVEL_WARNING, "Unable to get peer name.");
	    }
	    strcpy(sname, inet_ntoa(name.sin_addr));
	    if(sscanf(sname, "%d.%d.%d.%d", &i[0], &i[1], &i[2], &i[3]) != 4)
	    {
		zabbix_log(LOG_LEVEL_WARNING, "IP address not valid: [%s]", sname);
	    }
#endif /*HAVE_IPV6*/
	    zabbix_log(LOG_LEVEL_WARNING, "No service definitions for '%s' requested by '%s'.", host, sname);
	}
	
	zbx_snprintf(s,sizeof(s),"%s\n",
		"ZBX_EOF");
	zabbix_log( LOG_LEVEL_DEBUG, "Sending [%s]",
		s);

	if( zbx_tcp_send_raw(sock,s) != SUCCEED )
	{
		zabbix_log( LOG_LEVEL_WARNING, "Error while sending list of active checks");
		return  FAIL;
	}

	return  SUCCEED;
}
