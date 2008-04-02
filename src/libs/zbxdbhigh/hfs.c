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

static int make_directories (const char* path);

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

typedef int (*pred_fn_t) (void*, void*);
typedef void (*fold_fn_t) (void* db_val, void* state);


static hfs_meta_t* read_meta (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock);
static void free_meta (hfs_meta_t* meta);
static char* get_name (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock, int meta);
static int store_value (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock, int delay, void* value, int len);
static off_t find_meta_ofs (int time, hfs_meta_t* meta);
static int get_next_data_ts (int ts);
static int get_prev_data_ts (int ts);
static zbx_uint64_t get_count_generic (const char* hfs_base_dir, zbx_uint64_t itemid, int from, void* val, pred_fn_t pred);
static void foldl_time (const char* hfs_base_dir, zbx_uint64_t itemid, int ts, void* init_res, fold_fn_t fn);
static void foldl_count (const char* hfs_base_dir, zbx_uint64_t itemid, int count, void* init_res, fold_fn_t fn);
static int is_valid_val (void* val);


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
    off_t size, ofs;

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
	    item.ofs = ip->ofs + (1 + (ip->end - ip->start) / ip->delay) * len;
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
	size = lseek (fd, 0, SEEK_END);
	lseek (fd, 0, SEEK_SET);
	
	/* fill missing values with FFs */
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: check for gaps. Now %d, last %d, delay %d, delta %d", clock, ip->end, delay, clock - ip->end);
	
	if (clock - ip->end >= delay*2) {
	    ofs = lseek (fd, ip->ofs + ((ip->end - ip->start + 2*ip->delay)/ip->delay)*len, SEEK_SET);
	    zabbix_log(LOG_LEVEL_DEBUG, "HFS: there are gaps of size %d items (%d bytes per item)", (clock - ip->end - delay) / delay, len);
	    zabbix_log(LOG_LEVEL_DEBUG, "HFS gaps: cur %u, size %u", ofs, size);
	    
	    for (i = 0; i < (clock - ip->end - ip->delay) / delay; i++)
		for (j = 0; j < len; j++)
		    write (fd, &v, sizeof (v));
	}
	else {
	    ofs = lseek (fd, ip->ofs + ((clock - ip->start + ip->delay)/ip->delay)*len, SEEK_SET);
	    zabbix_log(LOG_LEVEL_DEBUG, "HFS: cur %u, size %u", ofs, size);
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
void HFSadd_history_uint (const char* hfs_base_dir, zbx_uint64_t itemid, unsigned int delay, zbx_uint64_t value, int clock)
{
    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history()");
    store_value (hfs_base_dir, itemid, clock, delay, &value, sizeof (zbx_uint64_t));
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
  Performs folding of values of historical data into some state. We filter values according to time.
*/
static void foldl_time (const char* hfs_base_dir, zbx_uint64_t itemid, int ts, void* init_res, fold_fn_t fn)
{
    char *p_data;
    hfs_meta_t* meta;
    off_t ofs;
    int fd;
    zbx_uint64_t value;

    zabbix_log(LOG_LEVEL_DEBUG, "HFS_foldl_time (%s, %llu, %u)", hfs_base_dir, itemid, ts);

    while (1) {
	meta = read_meta (hfs_base_dir, itemid, ts);
	
	if (!meta)
	    break;

	if (!meta->blocks) {
	    free_meta (meta);
	    break;
	}

	ofs = find_meta_ofs (ts, meta);

	zabbix_log(LOG_LEVEL_DEBUG, "Offset for TS %u is %u (%x) (%u)", ts, ofs, ofs, time(NULL)-ts);

	/* open data file and count amount of valid items */
	p_data = get_name (hfs_base_dir, itemid, ts, 0);
	fd = open (p_data, O_RDONLY);
	
	if (fd >= 0) {
	    lseek (fd, ofs, SEEK_SET);
	    while (read (fd, &value, sizeof (value)) > 0)
		fn (&value, init_res);
	    close (fd);
	}

	free_meta (meta);
	free (p_data);

	ts = get_next_data_ts (ts);
    }
}


/*
  Performs folding of values of historical data into some state. We filter values according to count of values.
*/
static void foldl_count (const char* hfs_base_dir, zbx_uint64_t itemid, int count, void* init_res, fold_fn_t fn)
{
    char *p_data;
    int fd, ts = time (NULL)-1;
    zbx_uint64_t value;
    off_t ofs;

    zabbix_log(LOG_LEVEL_DEBUG, "HFS_foldl_count (%s, %llu, %u)", hfs_base_dir, itemid, count);

    while (count > 0) {
	p_data = get_name (hfs_base_dir, itemid, ts, 0);
	fd = open (p_data, O_RDONLY);
	
	if (fd < 0)
	    return;

	ofs = lseek (fd, 0, SEEK_END);

	while (ofs && count > 0) {
	    ofs = lseek (fd, ofs - sizeof (double), SEEK_SET);
	    read (fd, &value, sizeof (value));
	    if (is_valid_val (&value)) {
		fn (&value, init_res);
		count--;
	    }
	}

	close (fd);
	free (p_data);

	ts = get_prev_data_ts (ts);
    }
}



static int is_valid_val (void* val)
{
    return *(zbx_uint64_t*)val != (zbx_uint64_t)0xffffffffffffffffLLU;
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
	zabbix_log(LOG_LEVEL_DEBUG, "check block %d[%d,%d], ts %d, ofs %u", i, meta->meta[i].start, meta->meta[i].end, time, meta->meta[i].ofs);
	
	if (!f) {
	    f = (meta->meta[i].end > time);
	    if (!f)
		continue;
	}
	
	if (meta->meta[i].start > time)
	    return meta->meta[i].ofs;

	return meta->meta[i].ofs + sizeof (double) * ((time - meta->meta[i].start) / meta->meta[i].delay);
    }

    return (off_t)(-1);
}


static int get_next_data_ts (int ts)
{
    return (ts / 1000000 + 1) * 1000000;
}


static int get_prev_data_ts (int ts)
{
    return (ts / 1000000 - 1) * 1000000;
}


zbx_uint64_t HFS_get_count (const char* hfs_base_dir, zbx_uint64_t itemid, int from)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    (*(zbx_uint64_t*)state)++;
    }

    zbx_uint64_t val = 0;
    foldl_time (hfs_base_dir, itemid, from, &val, functor);
    return val;
}


zbx_uint64_t HFS_get_count_u64_eq (const char* hfs_base_dir, zbx_uint64_t itemid, int from, zbx_uint64_t value)
{
    typedef struct {
	zbx_uint64_t val, count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db) && *(zbx_uint64_t*)db == ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_ne (const char* hfs_base_dir, zbx_uint64_t itemid, int from, zbx_uint64_t value)
{
    typedef struct {
	zbx_uint64_t val, count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db) && *(zbx_uint64_t*)db != ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_gt (const char* hfs_base_dir, zbx_uint64_t itemid, int from, zbx_uint64_t value)
{
    typedef struct {
	zbx_uint64_t val, count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db) && *(zbx_uint64_t*)db > ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_lt (const char* hfs_base_dir, zbx_uint64_t itemid, int from, zbx_uint64_t value)
{
    typedef struct {
	zbx_uint64_t val, count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db) && *(zbx_uint64_t*)db < ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_ge (const char* hfs_base_dir, zbx_uint64_t itemid, int from, zbx_uint64_t value)
{
    typedef struct {
	zbx_uint64_t val, count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db) && *(zbx_uint64_t*)db >= ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_le (const char* hfs_base_dir, zbx_uint64_t itemid, int from, zbx_uint64_t value)
{
    typedef struct {
	zbx_uint64_t val, count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db) && *(zbx_uint64_t*)db <= ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_eq (const char* hfs_base_dir, zbx_uint64_t itemid, int from, double value)
{
    typedef struct {
	double val;
	zbx_uint64_t count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db) && *(double*)db+0.00001 > ((state_t*)state)->val && *(double*)db-0.00001 < ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_ne (const char* hfs_base_dir, zbx_uint64_t itemid, int from, double value)
{
    typedef struct {
	double val;
	zbx_uint64_t count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db) && (*(double*)db+0.00001 < ((state_t*)state)->val || *(double*)db-0.00001 > ((state_t*)state)->val))
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_gt (const char* hfs_base_dir, zbx_uint64_t itemid, int from, double value)
{
    typedef struct {
	double val;
	zbx_uint64_t count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db) && *(double*)db > ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_lt (const char* hfs_base_dir, zbx_uint64_t itemid, int from, double value)
{
    typedef struct {
	double val;
	zbx_uint64_t count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db) && *(double*)db < ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_ge (const char* hfs_base_dir, zbx_uint64_t itemid, int from, double value)
{
    typedef struct {
	double val;
	zbx_uint64_t count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db) && *(double*)db >= ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_le (const char* hfs_base_dir, zbx_uint64_t itemid, int from, double value)
{
    typedef struct {
	double val;
	zbx_uint64_t count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db) && *(double*)db <= ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_sum_sec_u64 (const char* hfs_base_dir, zbx_uint64_t itemid, int from)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    *(zbx_uint64_t*)state += *(zbx_uint64_t*)db;
    }

    zbx_uint64_t sum = 0;

    foldl_time (hfs_base_dir, itemid, from, &sum, functor);
    return sum;
}


double HFS_get_sum_sec_float (const char* hfs_base_dir, zbx_uint64_t itemid, int from)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    *(double*)state += *(double*)db;
    }

    double sum = 0;

    foldl_time (hfs_base_dir, itemid, from, &sum, functor);
    return sum;
}


zbx_uint64_t HFS_get_sum_vals_u64 (const char* hfs_base_dir, zbx_uint64_t itemid, int count)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    *(zbx_uint64_t*)state += *(zbx_uint64_t*)db;
    }

    zbx_uint64_t sum = 0;

    foldl_count (hfs_base_dir, itemid, count, &sum, functor);
    return sum;
}


double HFS_get_sum_vals_float (const char* hfs_base_dir, zbx_uint64_t itemid, int count)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    *(double*)state += *(double*)db;
    }

    double sum = 0;

    foldl_count (hfs_base_dir, itemid, count, &sum, functor);
    return sum;
}
