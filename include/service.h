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

#ifndef ZABBIX_SERVICE_H
#define ZABBIX_SERVICE_H

#if !defined(_WINDOWS)
#	error "This module allowed only for Windows OS"
#endif /* _WINDOWS */

#include "threads.h"

extern ZBX_THREAD_HANDLE	*threads;

#define ZABBIX_SERVICE_NAME   APPLICATION_NAME
#define ZABBIX_EVENT_SOURCE   APPLICATION_NAME

void service_start(void);

int ZabbixCreateService(char *execName);
int ZabbixRemoveService(void);
int ZabbixStartService(void);
int ZabbixStopService(void);

void	init_main_process(void);

/* APPLICATION running status                    */
/* requred for closing application from service  */
int application_status;

#define ZBX_APP_STOPPED 0
#define ZBX_APP_RUNNED 1

/* ask for running application of closing status */
#define ZBX_IS_RUNNING (ZBX_APP_RUNNED == application_status)

/* ask for application closing status            */
#define ZBX_DO_EXIT() application_status = ZBX_APP_STOPPED

#define START_MAIN_ZABBIX_ENTRY(a)	service_start()

#endif /* ZABBIX_SERVICE_H */
