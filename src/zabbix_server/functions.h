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

#ifndef ZABBIX_FUNCTIONS_H
#define ZABBIX_FUNCTIONS_H

#include "common.h"
#include "comms.h"
#include "db.h"
#include "sysinfo.h"

#ifdef HAVE_MEMCACHE
#include "memcache.h"
#endif

/* filter used in SQL queries. Sites table (s alias) and hosts table
   (h alias) must be joined for this to work */
#define ZBX_COND_SITE " s.siteid = h.siteid and %s "

const char* getSiteCondition ();

void    update_triggers (zbx_uint64_t itemid);
void	update_functions(DB_ITEM *item);
int	process_data(int history, hfs_time_t ts, char *server,char *key, char *value, char* error, 
		     char *lastlogsize,char *timestamp, char *source, char *severity);
void	process_new_value(int history, DB_ITEM *item, AGENT_RESULT *value, time_t timestamp, const char* stderr);

void	append_history (char* server, char* key, char* value, char* clock, void** token);
void	flush_history (void** token);


#endif
