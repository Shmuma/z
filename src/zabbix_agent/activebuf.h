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

#ifndef ZABBIX_ACTIVEBUF_H
#define ZABBIX_ACTIVEBUF_H

#include "zbxconf.h"
#include "active.h"


typedef struct {
    char* key;
    int size, refresh;
    unsigned long* ts;
    char** values;
} active_buffer_item_t;



void init_active_buffer ();
void free_active_buffer ();
void update_active_buffer (ZBX_ACTIVE_METRIC* active);
void store_in_active_buffer (const char* key, const char* value);
int  active_buffer_is_empty ();
active_buffer_item_t* take_active_buffer_item ();
void free_active_buffer_item (active_buffer_item_t* item);


typedef struct  {
    int size;
    char** keys;
    int* refreshes;
} active_buffer_checks_t;

active_buffer_checks_t* get_buffer_checks_list ();
void free_buffer_checks_list (active_buffer_checks_t* checks);

#endif /* ZABBIX_ACTIVEBUF_H */
