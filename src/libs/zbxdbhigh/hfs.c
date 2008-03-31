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

static char* calc_coords (const char* hfs_base_dir, zbx_uint64_t itemid, unsigned int delay, time_t clock, unsigned int* ofs_items);
static int make_directories (const char* path);
static int store_value (char* val, int len, char* path, unsigned int ofs);


/*
  Routine adds double value to HistoryFS storage.
*/
void HFSadd_history (const char* hfs_base_dir, zbx_uint64_t itemid, unsigned int delay, double value, int clock)
{
    char* path;
    unsigned int ofs_items;
    int err;

    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history()");
    
    /* make directory which contain data */
    path = calc_coords (hfs_base_dir, itemid, delay, (time_t)clock, &ofs_items);

    if (!path) {
	zabbix_log(LOG_LEVEL_ERR, "HFS: Calculation of item's path in HFS failed");
	return;
    }
	
    zabbix_log(LOG_LEVEL_DEBUG, "HFS: Value would be placed in %s, ofs = %u (ts=%u, %u)", path, ofs_items, clock);

    /* make directory entries for this path */
    if (err = make_directories (path)) {
	zabbix_log(LOG_LEVEL_ERR, "HFS: Cannot make directories for path %s, err = %d", path, err);
	free (path);
	return;
    }

    store_value ((char*)&value, sizeof (double), path, ofs_items);

    free (path);
}


/*
  Routine adds uint64 value to HistoryFS storage.
*/
void HFSadd_history_uint (const char* hfs_base_dir, zbx_uint64_t itemid, unsigned int delay, unsigned long long value, int clock)
{
    char* path;
    unsigned int ofs_items;
    int err;

    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history_uint()");
    
    /* make directory which contain data */
    path = calc_coords (hfs_base_dir, itemid, delay, (time_t)clock, &ofs_items);

    if (!path) {
	zabbix_log(LOG_LEVEL_ERR, "HFS: Calculation of item's path in HFS failed");
	return;
    }
	
    zabbix_log(LOG_LEVEL_DEBUG, "HFS: Value would be placed in %s, ofs = %u (ts=%u, %u)", path, ofs_items, clock);

    /* make directory entries for this path */
    if (err = make_directories (path)) {
	zabbix_log(LOG_LEVEL_ERR, "HFS: Cannot make directories for path %s, err = %d", path, err);
	free (path);
	return;
    }

    store_value ((char*)&value, sizeof (unsigned long long), path, ofs_items);

    free (path);
}



static char* calc_coords (const char* hfs_base_dir, zbx_uint64_t itemid, unsigned int delay, time_t clock, unsigned int* ofs_items)
{
    char* res;
    int len = strlen (hfs_base_dir) + 20 + 10;
    char buf[100];

    res = (char*)malloc (len);

    if (!res)
	return NULL;

    zbx_snprintf (res, len, "%s/%llu/%u/%u.hfs", hfs_base_dir, itemid, (unsigned int)(clock / (time_t)1000000), delay);
    *ofs_items = (clock % 1000000L) / delay;

    return res;
}



static int make_directories (const char* path)
{
    /* parse path */
    const char *p = path, *pp;
    char buf[PATH_MAX+1];
    int len = 0;
    struct stat st;
    int err;

    buf[0] = 0;

    zabbix_log(LOG_LEVEL_DEBUG, "make_directories: %s", path);

    while (*p) {
	while (*p == '/')
	    p++;
	if (!*p)
	    break;
	pp = strchr (p, '/');
	if (pp == p)
	    break;
	if (!pp)
	    break;

	buf[len] = '/';
	memcpy (buf+len+1, p, pp-p+1);
	len += pp-p+1;
	buf[len] = 0;
	if (stat (buf, &st)) {
	    if (errno == ENOENT) {
		if (mkdir (buf, 0755)) {
		    zabbix_log(LOG_LEVEL_ERR, "HFS: Error creating directory, error = %d", errno);
		    return errno;
		}
	    }
	    else {
		zabbix_log(LOG_LEVEL_ERR, "HFS: stat() error = %d", errno);
		return errno;
	    }
	} 
	else {
	    if (!S_ISDIR (st.st_mode)) {
		zabbix_log(LOG_LEVEL_ERR, "HFS: Object %s exists and this isn't a directory", buf);
		return errno;
	    }
	}
	p = pp+1;
    }

    return 0;
}


static int store_value (char* val, int len, char* path, unsigned int ofs)
{
    int fd;

    /* we have a valid path and value. Store it. */
    fd = open (path, O_WRONLY | O_CREAT, 0600);

    if (fd < 0) {
	zabbix_log(LOG_LEVEL_ERR, "HFS: Error opening file %s, err = %d", path, errno);
	return 1;
    }

    lseek (fd, ofs * len, SEEK_SET);
    write (fd, val, len);
    close (fd);

    return 0;
}
