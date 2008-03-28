/*
** ZABBIX
** Copyright (C) 2008 Max Lapan <max.lapan@gmail.com>
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
#include "log.h"
#include "hfs.h"

static char* calc_coords (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock, unsigned int* ofs_items);


/*
  Routine adds doubel value to HistoryFS storage.
*/
void HFSadd_history (const char* hfs_base_dir, zbx_uint64_t itemid, double value, int clock)
{
    char* path;
    unsigned int ofs_items;

    zabbix_log(LOG_LEVEL_CRIT, "In HFSadd_history()");
    
    /* make directory which contain data */
    path = calc_coords (hfs_base_dir, itemid, (time_t)clock, &ofs_items);

    if (!path) {
	zabbix_log(LOG_LEVEL_ERR, "Calculation of item's path in HFS failed");
	return;
    }
	
    zabbix_log(LOG_LEVEL_CRIT, "Value would be placed in %s, ofs = %u", path, ofs_items);

    free (path);
}



static char* calc_coords (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock, unsigned int* ofs_items)
{
    char* res;

    res = (char*)malloc (strlen (hfs_base_len) + 20 + 10);

    if (!res)
	return NULL;

    sprintf (res, "%s/%lu/%u.hfs", hfs_base_dir, itemid, clock / 10000000L);
    *ofs_items = clock % 10000000L;

    return res;
}

