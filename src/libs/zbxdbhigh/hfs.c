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
static void calc_coords_int (zbx_uint64_t itemid, time_t clock, unsigned int* p_a, unsigned int* p_b);

/* internal structures */
typedef struct hfs_meta_item {
    time_t start, end;
    int delay;
    off_t ofs;
} hfs_meta_item_t;


typedef struct hfs_meta {
    int blocks;
    int last_delay;
    hfs_meta_item_t* meta;
} hfs_meta_t;


static hfs_meta_t* read_meta (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock);
static void free_meta (hfs_meta_t* meta);
static char* get_name (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock, int meta);
static int store_value (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock, int delay, void* value, int len);
static off_t find_meta_ofs (int time, hfs_meta_t* meta);
static int get_next_data_ts (int ts);


/*
  Routine adds double value to HistoryFS storage.
*/
void HFSadd_history (const char* hfs_base_dir, zbx_uint64_t itemid, unsigned int delay, double value, int clock)
{
    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history()");
    store_value (hfs_base_dir, itemid, clock, delay, &value, sizeof (double));
}


static int store_value (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock, int delay, void* value, int len)
{
    char *p_meta, *p_data;
    hfs_meta_item_t item, *ip;
    hfs_meta_t* meta;
    int fd;
    int i, j, r;
    unsigned char v = 0xff;

    zabbix_log(LOG_LEVEL_DEBUG, "HFS: store_value()");

    p_meta = get_name (hfs_base_dir, itemid, clock, 1);
    p_data = get_name (hfs_base_dir, itemid, clock, 0);

    meta = read_meta (hfs_base_dir, itemid, clock);

    make_directories (p_meta);
    zabbix_log(LOG_LEVEL_DEBUG, "HFS: meta read: delays: %d %d, blocks %d", meta->last_delay, delay, meta->blocks);

    /* should we start a new block? */
    if (meta->last_delay != delay) {
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: appending new block for item %llu", itemid); 
	item.start = item.end = clock;

	/* analyze current value */
	item.delay = delay;
	if (meta->blocks) {
	    ip = meta->meta + (meta->blocks-1);
	    zabbix_log(LOG_LEVEL_DEBUG, "HFS: there is another block on way: %u, %u, %d, %llu", ip->start, ip->end, ip->delay, ip->ofs);
	    item.ofs = (1 + (ip->end - ip->start) / ip->delay) * len;
	}
	else
	    item.ofs = 0;

	/* append block to meta */
	fd = open (p_meta, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	meta->blocks++;
	meta->last_delay = delay;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS: blocks <- %u", meta->blocks);
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: delay <- %u", meta->last_delay);
	
	write (fd, &meta->blocks, sizeof (meta->blocks));
	write (fd, &meta->last_delay, sizeof (meta->last_delay));
	lseek (fd, sizeof (hfs_meta_item_t)*(meta->blocks-1), SEEK_CUR);
	write (fd, &item, sizeof (item));
	close (fd);

	zabbix_log(LOG_LEVEL_DEBUG, "HFS: block metadata updated %d, %d: %u, %u, %d, %llu", meta->blocks, meta->last_delay,
		   item.start, item.end, item.delay, item.ofs);

	/* append data */
	fd = open (p_data, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	lseek (fd, item.ofs, SEEK_SET);
	write (fd, value, len);
	close (fd);
    } 
    else {
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: extending existing block for item %llu", itemid);

	/* check for gaps in block */
	ip = meta->meta + (meta->blocks-1);
	
	fd = open (p_data, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	lseek (fd, 0, SEEK_END);
	
	/* fill missing values with FFs */
	if (clock - ip->end > delay) {
	    zabbix_log(LOG_LEVEL_DEBUG, "HFS: there are gaps of size %d items (%d bytes per item)", (clock - ip->end) / delay, len);
	    
	    for (i = 0; i < (clock - ip->end) / delay; i++)
		for (j = 0; j < len; j++)
		    write (fd, &v, sizeof (v));
	}

	write (fd, value, len);
	close (fd);

	/* update meta */
	ip->end = clock;
	
	fd = open (p_meta, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	
	lseek (fd, sizeof (meta->blocks) * 2 + sizeof (hfs_meta_item_t) * (meta->blocks-1), SEEK_SET);
	write (fd, ip, sizeof (hfs_meta_item_t));
	close (fd);
}

    free_meta (meta);
    free (p_meta);
    free (p_data);

    return 0;
}



/*
  Routine adds uint64 value to HistoryFS storage.
*/
void HFSadd_history_uint (const char* hfs_base_dir, zbx_uint64_t itemid, unsigned int delay, unsigned long long value, int clock)
{
    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history()");
    store_value (hfs_base_dir, itemid, clock, delay, &value, sizeof (unsigned long long));
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



/*
  Routine calculates amount of items stored since timestamp
*/
unsigned long long HFS_get_count (const char* hfs_base_dir, zbx_uint64_t itemid, int from)
{
    int p_a, p_b;
    char *p_meta, *p_data;
    hfs_meta_t* meta;
    off_t ofs;
    int fd;
    unsigned long long val, res = 0;

    zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_count (%s, %llu, %u)", hfs_base_dir, itemid, from);

    while (1) {
	p_meta = get_name (hfs_base_dir, itemid, from, 1);

	meta = read_meta (hfs_base_dir, itemid, from);
	
	if (!meta)
	    break;

	if (!meta->blocks) {
	    free_meta (meta);
	    break;
	}

	ofs = find_meta_ofs (from, meta);

	zabbix_log(LOG_LEVEL_DEBUG, "Offset for TS %u is %llu", from, ofs);

	/* open data file and count amount of valid items */
	p_data = get_name (hfs_base_dir, itemid, from, 0);
	fd = open (p_data, O_RDONLY);
	
	if (fd >= 0) {
	    lseek (fd, ofs, SEEK_SET);
	    while (read (fd, &val, sizeof (val)) > 0) {
		if (val != (unsigned long long)0xffffffffffffffff)
		    res++;
	    }
	    close (fd);
	}

	free_meta (meta);
	free (p_meta);
	free (p_data);

	from = get_next_data_ts (from);
    }

    zabbix_log(LOG_LEVEL_CRIT, "HFS_get_count (%s, %llu) = %llu", hfs_base_dir, itemid, res);

    return res;
}


static hfs_meta_t* read_meta (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock)
{
    char* path = get_name (hfs_base_dir, itemid, clock, 1);
    hfs_meta_t* res;
    FILE* f;

    if (!path)
	return NULL;

    res = (hfs_meta_t*)malloc (sizeof (hfs_meta_t));
    
    if (!res) {
	free (path);
	return NULL;
    }
    
    f = fopen (path, "rb");

    /* if we have no file, create empty one */
    if (!f) {
	zabbix_log(LOG_LEVEL_DEBUG, "file open failed: %s", path);
	res->blocks = 0;
	res->last_delay = 0;
	res->meta = NULL;
	free (path);
	return res;
    }

    free (path);
   
    fread (&res->blocks, sizeof (res->blocks), 1, f);
    fread (&res->last_delay, sizeof (res->last_delay), 1, f);

    res->meta = (hfs_meta_item_t*)malloc (sizeof (hfs_meta_item_t)*res->blocks);

    if (!res->meta) {
	fclose (f);
	free (res);
	return NULL;
    }

    fread (res->meta, sizeof (hfs_meta_item_t), res->blocks, f);

    fclose (f);

    return res;
}


static void free_meta (hfs_meta_t* meta)
{
    if (!meta)
	return;

    if (meta->meta)
	free (meta->meta);

    free (meta);
}


static char* get_name (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock, int meta)
{
    char* res;
    int len = strlen (hfs_base_dir) + 100;

    res = (char*)malloc (len);

    if (!res)
	return NULL;

    zbx_snprintf (res, len, "%s/%llu/%u.%s", hfs_base_dir, itemid, (unsigned int)(clock / (time_t)1000000), 
		  meta ? "meta" : "data");

    return res;
}


static off_t find_meta_ofs (int time, hfs_meta_t* meta)
{
    int i;
    int f = 0;

    for (i = 0; i < meta->blocks; i++) {
	if (!f) {
	    f = (meta->meta[i].end > time);
	    if (!f)
		continue;
	}

	if (meta->meta[i].start > time)
	    return meta->meta[i].ofs;

	return meta->meta[i].ofs + sizeof (double) * (time - meta->meta[i].start) / meta->meta[i].delay;
    }

    return (off_t)(-1);
}


static int get_next_data_ts (int ts)
{
    return (ts / 1000000+1) * 1000000;
}
