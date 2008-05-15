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

# ifndef ULLONG_MAX
#  define ULLONG_MAX    18446744073709551615ULL
# endif

static int make_directories (const char* path);

/* internal structures */
typedef struct hfs_meta_item {
    time_t start, end;
    int delay;
    item_type_t type;
    off_t ofs;
} hfs_meta_item_t;


typedef struct hfs_meta {
    int blocks;
    int last_delay;
    item_type_t last_type;
    off_t last_ofs;
    hfs_meta_item_t* meta;
} hfs_meta_t;

typedef void (*fold_fn_t) (void* db_val, void* state);


static hfs_meta_t* read_meta (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock);
static void free_meta (hfs_meta_t* meta);
static char* get_name (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock, int meta);
static int store_value (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock, int delay, void* value, int len, item_type_t type);
static off_t find_meta_ofs (int time, hfs_meta_t* meta);
static int get_next_data_ts (int ts);
static int get_prev_data_ts (int ts);
static void foldl_time (const char* hfs_base_dir, zbx_uint64_t itemid, int ts, void* init_res, fold_fn_t fn);
static void foldl_count (const char* hfs_base_dir, zbx_uint64_t itemid, int count, void* init_res, fold_fn_t fn);
static int is_valid_val (void* val);


/*
  Routine adds double value to HistoryFS storage.
*/
void HFSadd_history (const char* hfs_base_dir, zbx_uint64_t itemid, unsigned int delay, double value, int clock)
{
    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history()");
    store_value (hfs_base_dir, itemid, clock, delay, &value, sizeof (double), IT_DOUBLE);
}


static int store_value (const char* hfs_base_dir, zbx_uint64_t itemid, time_t clock, int delay, void* value, int len, item_type_t type)
{
    char *p_meta, *p_data;
    hfs_meta_item_t item, *ip;
    hfs_meta_t* meta;
    int fd;
    int i, j, r, extra, eextra;
    unsigned char v = 0xff;
    off_t size, ofs;

    zabbix_log(LOG_LEVEL_DEBUG, "HFS: store_value()");

    p_meta = get_name (hfs_base_dir, itemid, clock, 1);
    p_data = get_name (hfs_base_dir, itemid, clock, 0);

    meta = read_meta (hfs_base_dir, itemid, clock);

    make_directories (p_meta);
    zabbix_log(LOG_LEVEL_DEBUG, "HFS: meta read: delays: %d %d, blocks %d, ofs %u", meta->last_delay, delay, meta->blocks, meta->last_ofs);

    /* should we start a new block? */
    if (meta->last_delay != delay || type != meta->last_type) {
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: appending new block for item %llu", itemid);
	item.start = item.end = clock;
	item.type = type;

	/* analyze current value */
	item.delay = delay;
	if (meta->blocks) {
	    ip = meta->meta + (meta->blocks-1);
	    zabbix_log(LOG_LEVEL_DEBUG, "HFS: there is another block on way: %u, %u, %d, %llu", ip->start, ip->end, ip->delay, ip->ofs);
	    item.ofs = meta->last_ofs + len;
	}
	else
	    item.ofs = 0;

	/* append block to meta */
	fd = open (p_meta, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IWGRP | S_IWOTH);
	meta->blocks++;
	meta->last_delay = delay;
	meta->last_type = type;

	/* when new data file is created, last_ofs is zero */
	if (meta->last_ofs)
	    meta->last_ofs += len;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS: blocks <- %u", meta->blocks);
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: delay <- %u", meta->last_delay);
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: type <- %u", meta->last_type);
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: last_ofs <- %u", meta->last_ofs);

	write (fd, &meta->blocks, sizeof (meta->blocks));
	write (fd, &meta->last_delay, sizeof (meta->last_delay));
	write (fd, &meta->last_type, sizeof (meta->last_type));
	write (fd, &meta->last_ofs, sizeof (meta->last_type));
	lseek (fd, sizeof (hfs_meta_item_t)*(meta->blocks-1), SEEK_CUR);
	write (fd, &item, sizeof (item));
	close (fd);

	zabbix_log(LOG_LEVEL_DEBUG, "HFS: block metadata updated %d, %d, %d: %u, %u, %d, %llu", meta->blocks, meta->last_delay, meta->last_type,
		   item.start, item.end, item.delay, item.ofs);

	/* append data */
	fd = open (p_data, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	lseek (fd, item.ofs, SEEK_SET);
	write (fd, value, len);
	close (fd);
    }
    else {
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: extending existing block for item %llu, last ofs %u", itemid, meta->last_ofs);

	/* check for gaps in block */
	ip = meta->meta + (meta->blocks-1);

	fd = open (p_data, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	size = lseek (fd, 0, SEEK_END);
	lseek (fd, 0, SEEK_SET);

	/* fill missing values with FFs */
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: check for gaps. Now %d, last %d, delay %d, delta %d", clock, ip->end, delay, clock - ip->end);

	meta->last_ofs += len;
	lseek (fd, meta->last_ofs, SEEK_SET);

	extra = (clock - ip->end) / delay;

	/* if value appeared before it's time */
	if (extra >= 1) {
            /* check for time-based items count differs from offset-based */
            eextra = (ip->end - ip->start) / delay - (meta->last_ofs - ip->ofs) / len;

            if (eextra > 0) {
                zabbix_log(LOG_LEVEL_CRIT, "HFS: there is disagree of time-based items count with offset based. Perform alignment. Count=%d", eextra);
		for (i = 0; i < eextra; i++)
		    for (j = 0; j < len; j++)
			write (fd, &v, sizeof (v));
                meta->last_ofs += eextra * len;
            }

	    if (extra > 1) {
		extra--;
		zabbix_log(LOG_LEVEL_DEBUG, "HFS: there are gaps of size %d items", extra);

		for (i = 0; i < extra; i++)
		    for (j = 0; j < len; j++)
			write (fd, &v, sizeof (v));
		meta->last_ofs += extra * len;
	    }            

	    write (fd, value, len);
	    close (fd);

	    /* update meta */
	    ip->end = clock;

	    fd = open (p_meta, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

	    lseek (fd, sizeof (meta->blocks) * 3, SEEK_SET);
	    write (fd, &meta->last_ofs, sizeof (meta->last_ofs));

	    lseek (fd, sizeof (hfs_meta_item_t) * (meta->blocks-1), SEEK_CUR);
	    write (fd, ip, sizeof (hfs_meta_item_t));
	    close (fd);
	}
	else
	    zabbix_log(LOG_LEVEL_DEBUG, "HFS: value appeared earlier than expected. Now %d, but next must be at %d", clock, ip->end + delay);
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
    store_value (hfs_base_dir, itemid, clock, delay, &value, sizeof (zbx_uint64_t), IT_UINT64);
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

	if (fd != -1) {
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
	res->last_type = IT_DOUBLE;
	res->last_ofs = 0;
	res->meta = NULL;
	free (path);
	return res;
    }

    free (path);

    fread (&res->blocks, sizeof (res->blocks), 1, f);
    fread (&res->last_delay, sizeof (res->last_delay), 1, f);
    fread (&res->last_type, sizeof (res->last_type), 1, f);
    fread (&res->last_ofs, sizeof (res->last_ofs), 1, f);

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


zbx_uint64_t HFS_get_sum_u64 (const char* hfs_base_dir, zbx_uint64_t itemid, int period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    *(zbx_uint64_t*)state += *(zbx_uint64_t*)db;
    }

    zbx_uint64_t sum = 0;

    if (seconds)
	foldl_time (hfs_base_dir, itemid, period, &sum, functor);
    else
	foldl_count (hfs_base_dir, itemid, period, &sum, functor);

    return sum;
}


double HFS_get_sum_float (const char* hfs_base_dir, zbx_uint64_t itemid, int period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    *(double*)state += *(double*)db;
    }

    double sum = 0;

    if (seconds)
	foldl_time (hfs_base_dir, itemid, period, &sum, functor);
    else
	foldl_count (hfs_base_dir, itemid, period, &sum, functor);

    return sum;
}


double HFS_get_avg_u64 (const char* hfs_base_dir, zbx_uint64_t itemid, int period, int seconds)
{
    typedef struct {
	zbx_uint64_t count, sum;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db)) {
	    ((state_t*)state)->count++;
	    ((state_t*)state)->sum += (*(zbx_uint64_t*)db);
	}
    }

    state_t state;
    state.sum = state.count = 0;

    if (seconds)
	foldl_time (hfs_base_dir, itemid, period, &state, functor);
    else
	foldl_count (hfs_base_dir, itemid, period, &state, functor);

    return (double)state.sum / (double)state.count;
}


double HFS_get_avg_float (const char* hfs_base_dir, zbx_uint64_t itemid, int period, int seconds)
{
    typedef struct {
	zbx_uint64_t count;
	double sum;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db)) {
	    ((state_t*)state)->count++;
	    ((state_t*)state)->sum += (*(double*)db);
	}
    }

    state_t state;
    state.sum = state.count = 0;

    if (seconds)
	foldl_time (hfs_base_dir, itemid, period, &state, functor);
    else
	foldl_count (hfs_base_dir, itemid, period, &state, functor);

    return (double)state.sum / (double)state.count;
}


zbx_uint64_t HFS_get_min_u64 (const char* hfs_base_dir, zbx_uint64_t itemid, int period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    if (*(zbx_uint64_t*)state > *(zbx_uint64_t*)db)
		*(zbx_uint64_t*)state = *(zbx_uint64_t*)db;
    }

    zbx_uint64_t min = ULLONG_MAX;

    if (seconds)
	foldl_time (hfs_base_dir, itemid, period, &min, functor);
    else
	foldl_count (hfs_base_dir, itemid, period, &min, functor);

    return min;
}


double HFS_get_min_float (const char* hfs_base_dir, zbx_uint64_t itemid, int period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    if (*(double*)state > *(double*)db)
		*(double*)state = *(double*)db;
    }

    double min = 1e300;

    if (seconds)
	foldl_time (hfs_base_dir, itemid, period, &min, functor);
    else
	foldl_count (hfs_base_dir, itemid, period, &min, functor);

    return min;
}


zbx_uint64_t HFS_get_max_u64 (const char* hfs_base_dir, zbx_uint64_t itemid, int period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    if (*(zbx_uint64_t*)state < *(zbx_uint64_t*)db)
		*(zbx_uint64_t*)state = *(zbx_uint64_t*)db;
    }

    zbx_uint64_t max = 0;

    if (seconds)
	foldl_time (hfs_base_dir, itemid, period, &max, functor);
    else
	foldl_count (hfs_base_dir, itemid, period, &max, functor);

    return max;
}


double HFS_get_max_float (const char* hfs_base_dir, zbx_uint64_t itemid, int period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    if (*(double*)state < *(double*)db)
		*(double*)state = *(double*)db;
    }

    double max = 0;

    if (seconds)
	foldl_time (hfs_base_dir, itemid, period, &max, functor);
    else
	foldl_count (hfs_base_dir, itemid, period, &max, functor);

    return max;
}


zbx_uint64_t HFS_get_delta_u64 (const char* hfs_base_dir, zbx_uint64_t itemid, int period, int seconds)
{
    typedef struct {
	zbx_uint64_t min, max;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db)) {
	    if (((state_t*)state)->min > *(zbx_uint64_t*)db)
		((state_t*)state)->min = *(zbx_uint64_t*)db;
	    if (((state_t*)state)->max < *(zbx_uint64_t*)db)
		((state_t*)state)->max = *(zbx_uint64_t*)db;
	}
    }

    state_t state;
    state.min = ULLONG_MAX;
    state.max = 0;

    if (seconds)
	foldl_time (hfs_base_dir, itemid, period, &state, functor);
    else
	foldl_count (hfs_base_dir, itemid, period, &state, functor);

    return state.max - state.min;
}

double HFS_get_delta_float (const char* hfs_base_dir, zbx_uint64_t itemid, int period, int seconds)
{
    typedef struct {
	double min, max;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db)) {
	    if (((state_t*)state)->min > *(double*)db)
		((state_t*)state)->min = *(double*)db;
	    if (((state_t*)state)->max < *(double*)db)
		((state_t*)state)->max = *(double*)db;
	}
    }

    state_t state;
    state.min = 1e300;
    state.max = 0;

    if (seconds)
	foldl_time (hfs_base_dir, itemid, period, &state, functor);
    else
	foldl_count (hfs_base_dir, itemid, period, &state, functor);

    return state.max - state.min;
}

static int
HFS_find_meta(const char *hfs_base_dir, zbx_uint64_t itemid, time_t from_ts, hfs_meta_t **res) {
	int i, block = 0;
	time_t ts = from_ts;
	hfs_meta_t *meta = NULL;
	hfs_meta_item_t *ip = NULL;

#ifdef DEBUG_legion
	fprintf(stderr, "It HFS_find_meta(hfs_base_dir=%s, itemid=%lld, from=%d, res)\n", hfs_base_dir, itemid, from_ts);
	fflush(stderr);
#endif
	(*res) = NULL;

	while (ts > 0) {
		char *path = get_name (hfs_base_dir, itemid, ts, 1);
		i = access(path, R_OK);
		free(path);

		if (i == -1)
			break;

		if ((meta = read_meta(hfs_base_dir, itemid, ts)) == NULL)
			return -1; // Somethig real bad happend :(

		if (meta->blocks > 0)
			break;

		free_meta(meta);
		ts = get_next_data_ts(ts);
	}

	if ((meta == NULL) || (meta->blocks == 0)) {
		free_meta(meta);
		return -1;
	}

	(*res) = meta;
	if (from_ts < ts)
		return block;

	for (i = 0; i < meta->blocks; i++) {
		ip = meta->meta + i;

		if (ip->start <= ts && ip->end >= ts)
			return i;
	}

	/* Interesting situation:
	   Meta file was found, from_ts not in past, but timestamp not found in blocks
	*/
	free_meta(meta);
	(*res) = NULL;
	return -1;
}

size_t
HFSread_item (const char *hfs_base_dir, size_t sizex, zbx_uint64_t itemid, time_t graph_from_ts, time_t graph_to_ts, time_t from_ts, time_t to_ts, hfs_item_value_t **result)
{
	item_value_u max, min, val;
	time_t ts = from_ts;
	size_t items = 0, result_size = 0;
	int z, p, x, finish_loop = 0, block;
	long cur_group, group = -1;

	p = (graph_to_ts - graph_from_ts);
	z = (p - graph_from_ts % p);
	x = (sizex - 1);

	max.d = 0.0;
	min.d = 0.0;

#ifdef DEBUG_legion
	fprintf(stderr, "In HFSread_item(hfs_base_dir=%s, sizex=%d, itemid=%lld, from=%d, to=%d)\n", hfs_base_dir, x, itemid, from_ts, to_ts);
	fprintf(stderr, "Magic variables: p=%d z=%d x=%d\n", p, z, x);
#endif
	while (!finish_loop) {
		int fd;
		char *p_data = NULL;
		hfs_meta_t *meta;
		hfs_meta_item_t *ip;
		off_t ofs;
#ifdef DEBUG_legion
		fprintf(stderr, "HFSread_item in main loop\n");
		fflush(stderr);
#endif

		if ((block = HFS_find_meta(hfs_base_dir, itemid, ts, &meta)) == -1)
			break;

		ip = meta->meta + block;

		if (group == -1)
			group = (long) (x * ((ts + z) % p) / p);

#ifdef DEBUG_legion
		fprintf(stderr, "hfs_meta_item meta->meta[%d] {\n"
			"\tstart=%d\n"
			"\tend=%d\n"
			"\tdelay=%d\n"
			"\ttype=%d\n"
			"\tofs=%d\n"
			"};\n",
			block,
			ip->start, ip->end, ip->delay, ip->type, ip->ofs);
		fflush(stderr);
#endif

		if ((p_data = get_name (hfs_base_dir, itemid, ts, 0)) == NULL) {
			zabbix_log(LOG_LEVEL_CRIT, "HFS: unable to get file name");
			goto nextloop;
		}
#ifdef DEBUG_legion
		fprintf(stderr, "data file = [%s]\n", p_data);
		fflush(stderr);
#endif
		if ((fd = open (p_data, O_RDONLY)) == -1) {
			zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: file open failed: %s", p_data, strerror(errno));
			goto nextloop;
		}

		if ((ofs = find_meta_ofs (ts, meta)) == -1) {
			zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: unable to get offset in file", p_data);
			goto nextloop;
		}
#ifdef DEBUG_legion
		fprintf(stderr, "run find_meta_ofs (%d, meta) = %d\n", ts, ofs);
		fflush(stderr);
#endif
		if (lseek (fd, ofs, SEEK_SET) == -1) {
			zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: unable to change file offset: %s", p_data, strerror(errno));
			goto nextloop;
		}

		while (read (fd, &val.l, sizeof (val.l)) > 0) {
			ts += ip->delay;

			if (!is_valid_val(&val.l))
				continue;

			if (result_size <= items) {
				result_size += alloc_item_values;
				*result = (hfs_item_value_t *) realloc(*result, (sizeof(hfs_item_value_t) * result_size));
			}

			cur_group = (long) (x * ((ts + z) % p) / p);

			if (group != cur_group) {
				(*result)[items].type  = ip->type;
				(*result)[items].group = group;
				(*result)[items].clock = ts;
				(*result)[items].max = max;
				(*result)[items].min = min;

				if (ip->type == IT_DOUBLE)
					(*result)[items].avg.d = ((max.d + min.d) / 2);
				else
					(*result)[items].avg.l = ((max.l + min.l) / 2);

				group = cur_group;
				max.d = 0.0;
				min.d = 0.0;
				items++;
			}

			if (ip->type == IT_DOUBLE) {
				max.d = ((max.d > val.d) ? max.d : val.d);
				min.d = ((max.d < val.d) ? max.d : val.d);
			}
			else {
				max.l = ((max.l > val.l) ? max.l : val.l);
				min.l = ((max.l < val.l) ? max.l : val.l);
			}

			if (ts >= to_ts) {
#ifdef DEBUG_legion
				fprintf(stderr, "ts >= to_ts (%d >= %d)\n", ts, to_ts);
				fflush(stderr);
#endif
				finish_loop = 1;
				break;
			}

			if (ts >= ip->end) {
				block++;

				if (block >= meta->blocks)
					/* We have read all blocks and we need another meta. */
					break;

				ip = meta->meta + block;
#ifdef DEBUG_legion
				fprintf(stderr, "read next block %d\n", block);
				fflush(stderr);
#endif
			}
    		}
nextloop:
		if (p_data)
			free (p_data);
		free_meta (meta);
		ts = get_next_data_ts (ts);
	}

	if (result_size > items)
		*result = (hfs_item_value_t *) realloc(*result, (sizeof(hfs_item_value_t) * items));
#ifdef DEBUG_legion
	fprintf(stderr, "Out HFSread_item()\n");
	fflush(stderr);
#endif
	return items;
}
