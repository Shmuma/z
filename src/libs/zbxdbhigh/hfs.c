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

#include <unistd.h>
#include <fcntl.h>


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


typedef enum {
	NK_ItemData,
	NK_ItemMeta,
	NK_HostState,
	NK_ItemValues,
	NK_ItemStatus,
	NK_ItemStderr,
} name_kind_t;


static hfs_meta_t* read_meta (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, time_t clock);
static void free_meta (hfs_meta_t* meta);
static char* get_name (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, time_t clock, name_kind_t kind);
static int store_value (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, time_t clock, int delay, void* value, int len, item_type_t type);
static off_t find_meta_ofs (int time, hfs_meta_t* meta);
static int get_next_data_ts (int ts);
static int get_prev_data_ts (int ts);
static void foldl_time (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int ts, void* init_res, fold_fn_t fn);
static void foldl_count (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int count, void* init_res, fold_fn_t fn);
static int is_valid_val (void* val);

static int obtain_lock (int fd, int write);
static int release_lock (int fd, int write);


static void write_str (int fd, const char* str);
static char* read_str (int fd);


/*
  Routine adds double value to HistoryFS storage.
*/
void HFSadd_history (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, double value, int clock)
{
    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history()");
    store_value (hfs_base_dir, siteid, itemid, clock, delay, &value, sizeof (double), IT_DOUBLE);
}

static void xfree(void *ptr)
{
	if (ptr != NULL)
		free(ptr);
}

static int xopen(char *fn, int flags, mode_t mode)
{
	int retval;
	if ((retval = open(fn, flags, mode)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: open: %s", fn, strerror(errno));
	return retval;
}

static int xclose(char *fn, int fd)
{
	int rc;
	if ((rc = close(fd)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: close: %s", fn, strerror(errno));
	return rc;
}

static ssize_t xwrite(char *fn, int fd, const void *buf, size_t count)
{
	ssize_t rc;
	if ((rc = write(fd, buf, count)) == -1) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: write: %s", fn, strerror(errno));
		xclose(fn, fd);
	}
	return rc;
}

static off_t xlseek(char *fn, int fd, off_t offset, int whence)
{
	off_t rc;
	if ((rc = lseek(fd, offset, whence)) == -1) {
		zabbix_log(LOG_LEVEL_CRIT,
			"HFS: %s: lseek(fd, %lld (%d), %d): %s",
			fn, offset, whence, strerror(errno));
		xclose(fn, fd);
	}
	return rc;
}

static int store_value (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, time_t clock, int delay, void* value, int len, item_type_t type)
{
    char *p_meta = NULL, *p_data = NULL;
    hfs_meta_item_t item, *ip;
    hfs_meta_t* meta;
    int fd;
    int i, j, r, extra;
    unsigned char v = 0xff;
    off_t eextra;
    off_t size, ofs;
    int retval = 1;

    zabbix_log(LOG_LEVEL_DEBUG, "HFS: store_value()");

    if ((meta = read_meta (hfs_base_dir, siteid, itemid, clock)) == NULL) {
	zabbix_log(LOG_LEVEL_CRIT, "HFS: store_value(): read_meta(%s, %s, %llu, %d) == NULL",
		    hfs_base_dir, siteid, itemid, clock);
	return retval;
    }

    p_meta = get_name (hfs_base_dir, siteid, itemid, clock, NK_ItemMeta);
    p_data = get_name (hfs_base_dir, siteid, itemid, clock, NK_ItemData);

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
	if ((fd = xopen (p_meta, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IWGRP | S_IWOTH)) == -1)
		goto err_exit;

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

	if ((xwrite (p_meta, fd, &meta->blocks, sizeof (meta->blocks)) == -1) ||
	    (xwrite (p_meta, fd, &meta->last_delay, sizeof (meta->last_delay)) == -1) ||
	    (xwrite (p_meta, fd, &meta->last_type, sizeof (meta->last_type)) == -1) ||
	    (xwrite (p_meta, fd, &meta->last_ofs, sizeof (meta->last_ofs)) == -1))
		goto err_exit;

	if (xlseek (p_meta, fd, sizeof (hfs_meta_item_t)*(meta->blocks-1), SEEK_CUR) == -1)
		goto err_exit;

	if (xwrite (p_meta, fd, &item, sizeof (item)) == -1)
		goto err_exit;

	if (xclose (p_meta, fd) == -1)
		goto err_exit;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS: block metadata updated %d, %d, %d: %u, %u, %d, %llu", meta->blocks, meta->last_delay, meta->last_type,
		   item.start, item.end, item.delay, item.ofs);

	/* append data */
	if ((fd = xopen (p_data, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
		goto err_exit;

	if (xlseek (p_data, fd, item.ofs, SEEK_SET) == -1)
		goto err_exit;

	if (xwrite (p_data, fd, value, len) == -1)
		goto err_exit;

	if (xclose (p_data, fd) == -1)
		goto err_exit;

	retval = 0;
    }
    else {
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: extending existing block for item %llu, last ofs %u", itemid, meta->last_ofs);

	/* check for gaps in block */
	ip = meta->meta + (meta->blocks-1);

	if ((fd = xopen (p_data, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
		goto err_exit;

	if (((size = xlseek (p_data, fd, 0, SEEK_END)) == -1) ||
	    (xlseek (p_data, fd, 0, SEEK_SET) == -1))
		goto err_exit;

	/* fill missing values with FFs */
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: check for gaps. Now %d, last %d, delay %d, delta %d", clock, ip->end, delay, clock - ip->end);

	meta->last_ofs += len;
	if (xlseek (p_data, fd, meta->last_ofs, SEEK_SET) == -1)
		goto err_exit;

	extra = (clock - ip->end) / delay;

	/* if value appeared before it's time */
	if (extra >= 1) {
            /* check for time-based items count differs from offset-based */
            eextra = (ip->end - ip->start) / delay - (meta->last_ofs - ip->ofs) / len;

            if (eextra > 0) {
                zabbix_log(LOG_LEVEL_CRIT, "HFS: there is disagree of time-based items count with offset based. Perform alignment. Count=%d", eextra);
		for (i = 0; i < eextra; i++)
		    for (j = 0; j < len; j++) {
			if (xwrite (p_data, fd, &v, sizeof (v)) == -1)
				goto err_exit;
		    }
                meta->last_ofs += eextra * len;
            }

	    if (extra > 1) {
		extra--;
		zabbix_log(LOG_LEVEL_DEBUG, "HFS: there are gaps of size %d items", extra);

		for (i = 0; i < extra; i++)
		    for (j = 0; j < len; j++) {
			if (xwrite (p_data, fd, &v, sizeof (v)) == -1)
				goto err_exit;
		    }
		meta->last_ofs += extra * len;
	    }            

	    if (xwrite (p_data, fd, value, len) == -1)
		goto err_exit;

	    if (xclose (p_data, fd) == -1)
		goto err_exit;

	    /* update meta */
	    ip->end = clock;

	    if ((fd = xopen (p_meta, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
		goto err_exit;

	    if (xlseek (p_meta, fd, sizeof (meta->blocks) * 3, SEEK_SET) == -1)
		goto err_exit;

	    if (xwrite (p_meta, fd, &meta->last_ofs, sizeof (meta->last_ofs)) == -1)
		goto err_exit;

	    if (xlseek (p_meta, fd, sizeof (hfs_meta_item_t) * (meta->blocks-1), SEEK_CUR) == -1)
		goto err_exit;

	    if (xwrite (p_meta, fd, ip, sizeof (hfs_meta_item_t)) == -1)
		goto err_exit;

	    if (xclose (p_meta, fd) == -1)
		goto err_exit;
	}
	else
	    zabbix_log(LOG_LEVEL_DEBUG, "HFS: value appeared earlier than expected. Now %d, but next must be at %d", clock, ip->end + delay);

	retval = 0;
    }

err_exit:

    free_meta (meta);
    xfree (p_meta);
    xfree (p_data);

    return retval;
}


/*
  Routine adds uint64 value to HistoryFS storage.
*/
void HFSadd_history_uint (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, zbx_uint64_t value, int clock)
{
    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history()");
    store_value (hfs_base_dir, siteid, itemid, clock, delay, &value, sizeof (zbx_uint64_t), IT_UINT64);
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
		    zabbix_log(LOG_LEVEL_ERR, "HFS: make_directories: mkdir(): %s", strerror(errno));
		    return errno;
		}
	    }
	    else {
		zabbix_log(LOG_LEVEL_ERR, "HFS: make_directories: stat(): %s", strerror(errno));
		return errno;
	    }
	}
	else {
	    if (!S_ISDIR (st.st_mode)) {
		zabbix_log(LOG_LEVEL_ERR, "HFS: make_directories: Object %s exists and this isn't a directory", buf);
		return errno;
	    }
	}
	p = pp+1;
    }

    return 0;
}


/*
   fcntl wrappers
 */
int obtain_lock (int fd, int write)
{
	struct flock fls;

	fls.l_type = write ? F_WRLCK : F_RDLCK;
	fls.l_whence = SEEK_SET;
	fls.l_start = 0;
	fls.l_len = 0;

	while (1) {
		if (fcntl (fd, write ? F_SETLKW : F_SETLK, &fls) < 0) {
			if (errno == EINTR)
				continue;
			zabbix_log(LOG_LEVEL_ERR, "HFS_obtain_lock: fcntl(): %s", strerror(errno));
			return 0;
		}
		else
			break;
	}

	return 1;
}


int release_lock (int fd, int write)
{
	struct flock fls;

	fls.l_type = F_UNLCK;
	fls.l_whence = SEEK_SET;
	fls.l_start = 0;
	fls.l_len = 0;

	while (1) {
		if (fcntl (fd, write ? F_SETLKW : F_SETLK, &fls) < 0) {
			if (errno == EINTR)
				continue;
			zabbix_log(LOG_LEVEL_ERR, "HFS_release_lock: fcntl(): %s", strerror(errno));
			return 0;
		}
		else
			break;
	}

	return 1;
}


void write_str (int fd, const char* str)
{
	int len = str ? strlen (str) : 0;

	write (fd, &len, sizeof (len));
	if (len)
		write (fd, str, len+1);
}


char* read_str (int fd)
{
	int len;
	char* res = NULL;

	if (read (fd, &len, sizeof (len)) < sizeof (len))
		return NULL;

	if (fd) {
		res = (char*)calloc (len+1, 1);
		if (res)
			read (fd, res, len+1);
	}

	return res;
}


/*
  Performs folding of values of historical data into some state. We filter values according to time.
*/
static void foldl_time (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int ts, void* init_res, fold_fn_t fn)
{
    char *p_data;
    hfs_meta_t* meta;
    off_t ofs;
    int fd;
    zbx_uint64_t value;

    zabbix_log(LOG_LEVEL_DEBUG, "HFS_foldl_time (%s, %llu, %u)", hfs_base_dir, itemid, ts);

    while (1) {
	meta = read_meta (hfs_base_dir, siteid, itemid, ts);

	if (!meta)
	    break;

	if (!meta->blocks) {
	    free_meta (meta);
	    break;
	}

	ofs = find_meta_ofs (ts, meta);

	zabbix_log(LOG_LEVEL_DEBUG, "Offset for TS %u is %u (%x) (%u)", ts, ofs, ofs, time(NULL)-ts);

	/* open data file and count amount of valid items */
	p_data = get_name (hfs_base_dir, siteid, itemid, ts, NK_ItemData);
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
static void foldl_count (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int count, void* init_res, fold_fn_t fn)
{
    char *p_data;
    int fd, ts = time (NULL)-1;
    zbx_uint64_t value;
    off_t ofs;

    zabbix_log(LOG_LEVEL_DEBUG, "HFS_foldl_count (%s, %llu, %u)", hfs_base_dir, itemid, count);

    while (count > 0) {
	p_data = get_name (hfs_base_dir, siteid, itemid, ts, NK_ItemData);
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


static hfs_meta_t* read_meta (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, time_t clock)
{
    char* path = get_name (hfs_base_dir, siteid, itemid, clock, NK_ItemMeta);
    hfs_meta_t* res;
    FILE* f;

    if (!path)
	return NULL;

    res = (hfs_meta_t*)malloc (sizeof (hfs_meta_t));

    if (!res) {
	free (path);
	return NULL;
    }

    /* if we have no file, create empty one */
    if ((f = fopen (path, "rb")) == NULL) {
	zabbix_log(LOG_LEVEL_DEBUG, "%s: file open failed: %s", path, strerror(errno));
	res->blocks = 0;
	res->last_delay = 0;
	res->last_type = IT_DOUBLE;
	res->last_ofs = 0;
	res->meta = NULL;
	free (path);
	return res;
    }

    free (path);

    if (fread (&res->blocks, sizeof (res->blocks), 1, f) != 1 ||
	fread (&res->last_delay, sizeof (res->last_delay), 1, f) != 1 ||
	fread (&res->last_type, sizeof (res->last_type), 1, f) != 1 ||
	fread (&res->last_ofs, sizeof (res->last_ofs), 1, f) != 1)
    {
	fclose (f);
	free (res);
	return NULL;
    }

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


static char* get_name (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, time_t clock, name_kind_t kind)
{
    char* res;
    int len = strlen (hfs_base_dir) + strlen (siteid) + 100;

    res = (char*)malloc (len);

    if (!res)
	return NULL;

    switch (kind) {
    case NK_ItemData:
    case NK_ItemMeta:
	    snprintf (res, len, "%s/%s/items/%llu/%u.%s", hfs_base_dir, siteid, itemid, (unsigned int)(clock / (time_t)1000000),
		      kind == NK_ItemMeta ? "meta" : "data");
	    break;
    case NK_HostState:
	    snprintf (res, len, "%s/%s/hosts/%llu.state", hfs_base_dir, siteid, itemid);
	    break;
    case NK_ItemValues:
	    snprintf (res, len, "%s/%s/items/%llu/values.dat", hfs_base_dir, siteid, itemid);
	    break;
    case NK_ItemStatus:
	    snprintf (res, len, "%s/%s/items/%llu/status.dat", hfs_base_dir, siteid, itemid);
	    break;
    case NK_ItemStderr:
	    snprintf (res, len, "%s/%s/items/%llu/stderr.dat", hfs_base_dir, siteid, itemid);
	    break;
    }

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


zbx_uint64_t HFS_get_count (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    (*(zbx_uint64_t*)state)++;
    }

    zbx_uint64_t val = 0;
    foldl_time (hfs_base_dir, siteid, itemid, from, &val, functor);
    return val;
}


zbx_uint64_t HFS_get_count_u64_eq (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, zbx_uint64_t value)
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

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_ne (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, zbx_uint64_t value)
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

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_gt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, zbx_uint64_t value)
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

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_lt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, zbx_uint64_t value)
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

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_ge (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, zbx_uint64_t value)
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

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_le (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, zbx_uint64_t value)
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

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_eq (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, double value)
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

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_ne (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, double value)
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

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_gt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, double value)
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

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_lt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, double value)
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

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_ge (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, double value)
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

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_le (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, double value)
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

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_sum_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    *(zbx_uint64_t*)state += *(zbx_uint64_t*)db;
    }

    zbx_uint64_t sum = 0;

    if (seconds)
	foldl_time (hfs_base_dir, siteid, itemid, period, &sum, functor);
    else
	foldl_count (hfs_base_dir, siteid, itemid, period, &sum, functor);

    return sum;
}


double HFS_get_sum_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    *(double*)state += *(double*)db;
    }

    double sum = 0;

    if (seconds)
	foldl_time (hfs_base_dir, siteid, itemid, period, &sum, functor);
    else
	foldl_count (hfs_base_dir, siteid, itemid, period, &sum, functor);

    return sum;
}


double HFS_get_avg_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds)
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
	foldl_time (hfs_base_dir, siteid, itemid, period, &state, functor);
    else
	foldl_count (hfs_base_dir, siteid, itemid, period, &state, functor);

    return (double)state.sum / (double)state.count;
}


double HFS_get_avg_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds)
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
	foldl_time (hfs_base_dir, siteid, itemid, period, &state, functor);
    else
	foldl_count (hfs_base_dir, siteid, itemid, period, &state, functor);

    return (double)state.sum / (double)state.count;
}


zbx_uint64_t HFS_get_min_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    if (*(zbx_uint64_t*)state > *(zbx_uint64_t*)db)
		*(zbx_uint64_t*)state = *(zbx_uint64_t*)db;
    }

    zbx_uint64_t min = ULLONG_MAX;

    if (seconds)
	foldl_time (hfs_base_dir, siteid, itemid, period, &min, functor);
    else
	foldl_count (hfs_base_dir, siteid, itemid, period, &min, functor);

    return min;
}


double HFS_get_min_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    if (*(double*)state > *(double*)db)
		*(double*)state = *(double*)db;
    }

    double min = 1e300;

    if (seconds)
	foldl_time (hfs_base_dir, siteid, itemid, period, &min, functor);
    else
	foldl_count (hfs_base_dir, siteid, itemid, period, &min, functor);

    return min;
}


zbx_uint64_t HFS_get_max_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    if (*(zbx_uint64_t*)state < *(zbx_uint64_t*)db)
		*(zbx_uint64_t*)state = *(zbx_uint64_t*)db;
    }

    zbx_uint64_t max = 0;

    if (seconds)
	foldl_time (hfs_base_dir, siteid, itemid, period, &max, functor);
    else
	foldl_count (hfs_base_dir, siteid, itemid, period, &max, functor);

    return max;
}


double HFS_get_max_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db))
	    if (*(double*)state < *(double*)db)
		*(double*)state = *(double*)db;
    }

    double max = 0;

    if (seconds)
	foldl_time (hfs_base_dir, siteid, itemid, period, &max, functor);
    else
	foldl_count (hfs_base_dir, siteid, itemid, period, &max, functor);

    return max;
}


zbx_uint64_t HFS_get_delta_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds)
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
	foldl_time (hfs_base_dir, siteid, itemid, period, &state, functor);
    else
	foldl_count (hfs_base_dir, siteid, itemid, period, &state, functor);

    return state.max - state.min;
}

double HFS_get_delta_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds)
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
	foldl_time (hfs_base_dir, siteid, itemid, period, &state, functor);
    else
	foldl_count (hfs_base_dir, siteid, itemid, period, &state, functor);

    return state.max - state.min;
}

static int
HFS_find_meta(const char *hfs_base_dir, const char* siteid, zbx_uint64_t itemid, time_t from_ts, hfs_meta_t **res) {
	int i, block = 0;
	time_t ts = from_ts;
	hfs_meta_t *meta = NULL;
	hfs_meta_item_t *ip = NULL;

#ifdef DEBUG_legion
	fprintf(stderr, "It HFS_find_meta(hfs_base_dir=%s, siteid=%s, itemid=%lld, from=%d, res)\n", hfs_base_dir, siteid, itemid, from_ts);
	fflush(stderr);
#endif
	(*res) = NULL;

	while (ts > 0) {
		char *path;

		i = -1;
		if ((path = get_name (hfs_base_dir, siteid, itemid, ts, NK_ItemMeta)) != NULL) {
			i = access(path, R_OK);
			free(path);
		}

		if (i == -1) {
			ts = get_next_data_ts(ts);
			continue;
		}

		if ((meta = read_meta(hfs_base_dir, siteid, itemid, ts)) == NULL)
			return -1; // Somethig real bad happend :(

		if (meta->blocks > 0)
			break;

		free_meta(meta);
		meta = NULL;

		ts = get_next_data_ts(ts);
	}

	if ((meta == NULL) || (meta->blocks == 0)) {
		free_meta(meta);
		return -1;
	}

	(*res) = meta;
	if (from_ts <= ts)
		return block;

	for (i = 0; i < meta->blocks; i++) {
		ip = meta->meta + i;
		if (ip->start <= ts && ts <= ip->end)
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
HFSread_item (const char *hfs_base_dir, const char* siteid, size_t sizex, zbx_uint64_t itemid, time_t graph_from_ts, time_t graph_to_ts, time_t from_ts, time_t to_ts, hfs_item_value_t **result)
{
	item_value_u max, min, val;
	time_t ts = from_ts;
	size_t items = 0, result_size = 0;
	int z, p, x, finish_loop = 0, block;
	long values = 0, count = 0, cur_group, group = -1;

	p = (graph_to_ts - graph_from_ts);
	z = ((p - graph_from_ts) % p);
	x = (sizex - 1);

	max.d = 0.0;
	min.d = 0.0;

	zabbix_log(LOG_LEVEL_DEBUG,
		"In HFSread_item(hfs_base_dir=%s, sizex=%d, itemid=%lld, graph_from=%d, graph_to=%d, from=%d, to=%d)\n",
		hfs_base_dir, x, itemid, graph_from_ts, graph_to_ts, from_ts, to_ts);

	while (!finish_loop) {
		int fd = -1;
		char *p_data = NULL;
		hfs_meta_t *meta = NULL;
		hfs_meta_item_t *ip;
		off_t ofs;

		if ((block = HFS_find_meta(hfs_base_dir, siteid, itemid, ts, &meta)) == -1)
			break;

		ip = meta->meta + block;

		if (graph_to_ts < ip->start) {
			/*      graph_from_ts   graph_to_ts
			   ----|---------------|----------------> Time
			                            ^ ip->start
			*/
			finish_loop = 1;
			goto nextloop;
		}

		if ((p_data = get_name (hfs_base_dir, siteid, itemid, ts, NK_ItemData)) == NULL) {
			zabbix_log(LOG_LEVEL_CRIT, "HFS: unable to get file name");
			finish_loop = 1;
			goto nextloop;
		}

		if ((ofs = find_meta_ofs (ts, meta)) == -1) {
			zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: %d: unable to get offset in file", p_data, ts);
			finish_loop = 1;
			goto nextloop;
		}

		if ((fd = open (p_data, O_RDONLY)) == -1) {
			zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: file open failed: %s", p_data, strerror(errno));
			finish_loop = 1;
			goto nextloop;
		}

		if (lseek (fd, ofs, SEEK_SET) == -1) {
			zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: unable to change file offset: %s", p_data, strerror(errno));
			finish_loop = 1;
			goto nextloop;
		}

		while (read (fd, &val.l, sizeof (val.l)) > 0) {
			ts += ip->delay;
			values++;

			if (!is_valid_val(&val.l))
				continue;

			if (result_size <= items) {
				result_size += alloc_item_values;
				*result = (hfs_item_value_t *) realloc(*result, (sizeof(hfs_item_value_t) * result_size));
			}

			cur_group = (long) (x * ((ts + z) % p) / p);
			if (group == -1)
				group = cur_group;

			if (group != cur_group) {
				(*result)[items].type  = ip->type;
				(*result)[items].group = group;
				(*result)[items].count = count;
				(*result)[items].clock = ts;
				(*result)[items].max = max;
				(*result)[items].min = min;

				if (ip->type == IT_DOUBLE)
					(*result)[items].avg.d = ((max.d + min.d) / 2);
				else
					(*result)[items].avg.l = ((max.l + min.l) / 2);

				group = cur_group;
				count = 0;
				max.d = 0.0;
				min.d = 0.0;
				items++;
			}
			count++;

			if (ip->type == IT_DOUBLE) {
				max.d = ((max.d > val.d) ? max.d : val.d);
				min.d = ((max.d < val.d) ? max.d : val.d);
			}
			else {
				max.l = ((max.l > val.l) ? max.l : val.l);
				min.l = ((max.l < val.l) ? max.l : val.l);
			}

			if (ts >= to_ts) {
				finish_loop = 1;
				break;
			}

			if (ts >= ip->end) {
				block++;

				if (block >= meta->blocks)
					/* We have read all blocks and we need another meta. */
					break;

				ip = meta->meta + block;
			}
    		}
nextloop:
		if (fd != -1 && close(fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: file open failed: %s", p_data, strerror(errno));

		if (p_data) {
			free (p_data);
			p_data = NULL;
		}

		free_meta (meta);
		meta = NULL;

		ts = get_next_data_ts (ts);
	}

	if (result_size > items)
		*result = (hfs_item_value_t *) realloc(*result, (sizeof(hfs_item_value_t) * items));

	zabbix_log(LOG_LEVEL_DEBUG,
		"HFS: HFSread_item: Out items=%d values=%d\n",
		items, values);

	return items;
}

int
HFSread_count(const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int count, void* init_res, read_count_fn_t fn)
{
	int i;
	char *p_data = NULL;
	int fd = -1, ts = time (NULL)-1;
	hfs_meta_item_t *ip = NULL;
	hfs_meta_t *meta = NULL;
	item_value_u val;
	off_t ofs;

#ifdef DEBUG_legion
	fprintf(stderr, "In HFSread_count(hfs_base_dir=%s, itemid=%lld, count=%d, res, func)\n",
			hfs_base_dir, itemid, count);
	fflush(stderr);
#endif

	while (count > 0 && ts > 0) {
		if ((meta = read_meta(hfs_base_dir, siteid, itemid, ts)) == NULL)
			return -1; // Somethig real bad happend :(

		if (meta->blocks == 0)
			break;

		if ((p_data = get_name(hfs_base_dir, siteid, itemid, ts, NK_ItemData)) == NULL) {
			zabbix_log(LOG_LEVEL_CRIT, "HFS: unable to get file name");
			break; // error
		}

		if ((fd = open (p_data, O_RDONLY)) == -1) {
			zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: file open failed: %s", p_data, strerror(errno));
			break; // error
		}

		if ((ofs = lseek(fd, meta->last_ofs, SEEK_SET)) == -1) {
			zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: unable to change file offset = %u: %s", p_data, meta->last_ofs, strerror(errno));
			break; // error
		}

		for (i = (meta->blocks-1); i >= 0; i--) {
			ip = meta->meta + i;
			ts = ip->end;

			while (count > 0 && ip->start <= ts) {
				if ((ofs = lseek(fd, (ofs - sizeof(val.l)), SEEK_SET)) == -1) {
					zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: unable to change file offset: %s", p_data, strerror(errno));
					goto end;
				}

				if (read(fd, &val.l, sizeof (val.l)) == -1) {
					zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: unable to read data: %s", p_data, strerror(errno));
					goto end;
				}

				if (is_valid_val(&val.l)) {
					fn (ip->type, val, ts, init_res);
					count--;
				}

				ts = (ts - ip->delay);
			}

			if (count == 0)
				goto end;
		}

		if (close(fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: file open failed: %s", p_data, strerror(errno));
		fd = -1;

		if (p_data) {
			free(p_data);
			p_data = NULL;
		}

		free_meta(meta);
		meta = NULL;

		ts = get_prev_data_ts (ts);
	}
end:
	if (fd != -1 && close(fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: file open failed: %s", p_data, strerror(errno));

	xfree(p_data);
	free_meta(meta);

	return count;
}


/*
   Stores host availability in FS.
 */
void HFS_update_host_availability (const char* hfs_base_dir, const char* siteid, zbx_uint64_t hostid, int available, int clock, const char* error)
{
	char* name = get_name (hfs_base_dir, siteid, hostid, 0, NK_HostState);
	int fd;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_host_availability entered");

	if (!name)
		return;

	zabbix_log(LOG_LEVEL_DEBUG, "got name %s", name);

	make_directories (name);

	/* open file for writing */
	fd = open (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_host_availability: open(): %s: %s", name, strerror(errno));
		xfree (name);
		return;
	}
	xfree (name);

	/* place write lock on that file or wait for unlock */
	if (!obtain_lock (fd, 1)) {
		close (fd);
		return;
	}

	/* lock obtained, write data */
	if (write (fd, &available, sizeof (available)) == -1 ||
	    write (fd, &clock, sizeof (clock)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_host_availability: write(): %s",  strerror(errno));

	write_str (fd, error);

	/* truncate file */
	if (ftruncate (fd, lseek (fd, 0, SEEK_CUR)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_host_availability: ftruncate(): %s", strerror(errno));

	/* release lock */
	release_lock (fd, 1);

	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_host_availability: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_host_availability leave");
}


/* Obtain host attributes stored in HFS. If successfull, return 1. If something goes wrong, return 0. */
int HFS_get_host_availability (const char* hfs_base_dir, const char* siteid, zbx_uint64_t hostid, int* available, int* clock, char** error)
{
	char* name = get_name (hfs_base_dir, siteid, hostid, 0, NK_HostState);
	int fd;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_host_availability entered");

	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);

	if (fd < 0) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_host_availability: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	/* obtain read lock */
	if (!obtain_lock (fd, 0)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_get_host_availability: close(): %s", strerror(errno));
		return 0;
	}

	/* reading data */
	if (read (fd, available, sizeof (*available)) == -1 ||
	    read (fd, clock, sizeof (*clock)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_host_availability: read(): %s", strerror(errno));
	*error = read_str (fd);

	/* release read lock */
	release_lock (fd, 0);
	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_host_availability: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_host_availability leave");
	return 1;
}


void HFS_update_item_values_dbl (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid,
			     int lastclock, int nextcheck, double prevvalue, double lastvalue, double prevorgvalue)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, 0, NK_ItemValues);
	int fd, kind;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_values_dbl entered");

	if (!name)
		return;

	zabbix_log(LOG_LEVEL_DEBUG, "got name %s", name);

	make_directories (name);

	/* open file for writing */
	fd = open (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_dbl: open(): %s: %s", name, strerror(errno));
		free (name);
		return;
	}
	free (name);

	/* place write lock on that file or wait for unlock */
	if (!obtain_lock (fd, 1)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_dbl: close(): %s", strerror(errno));
		return;
	}

	/* lock obtained, write data */
	if (write (fd, &lastclock, sizeof (lastclock)) == -1 ||
	    write (fd, &nextcheck, sizeof (nextcheck)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_dbl: write(): %s", strerror(errno));

	kind = 0;
	if (write (fd, &kind, sizeof (kind)) == -1 ||
	    write (fd, &prevvalue, sizeof (prevvalue)) == -1 ||
	    write (fd, &lastvalue, sizeof (lastvalue)) == -1 ||
	    write (fd, &prevorgvalue, sizeof (prevorgvalue)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_dbl: write(): %s", strerror(errno));

	/* truncate file */
	if (ftruncate (fd, lseek (fd, 0, SEEK_CUR)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_dbl: ftruncate(): %s", strerror(errno));

	/* release lock */
	release_lock (fd, 1);

	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_dbl: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_values_dbl leave");
}



void HFS_update_item_values_int (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid,
				 int lastclock, int nextcheck, zbx_uint64_t prevvalue, zbx_uint64_t lastvalue, zbx_uint64_t prevorgvalue)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, 0, NK_ItemValues);
	int fd, kind;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_values_int entered");

	if (!name)
		return;

	zabbix_log(LOG_LEVEL_DEBUG, "got name %s", name);

	make_directories (name);

	/* open file for writing */
	fd = open (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_int: open(): %s: %s", name, strerror(errno));
		free (name);
		return;
	}
	free (name);

	/* place write lock on that file or wait for unlock */
	if (!obtain_lock (fd, 1)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_int: close(): %s", strerror(errno));
		return;
	}

	/* lock obtained, write data */
	if (write (fd, &lastclock, sizeof (lastclock)) == -1 ||
	    write (fd, &nextcheck, sizeof (nextcheck)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_int: write(): %s", strerror(errno));

	kind = 1;
	if (write (fd, &kind, sizeof (kind)) == -1 ||
	    write (fd, &prevvalue, sizeof (prevvalue)) == -1 ||
	    write (fd, &lastvalue, sizeof (lastvalue)) == -1 ||
	    write (fd, &prevorgvalue, sizeof (prevorgvalue)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_int: write(): %s", strerror(errno));

	/* truncate file */
	if (ftruncate (fd, lseek (fd, 0, SEEK_CUR)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_int: ftruncate(): %s", strerror(errno));

	/* release lock */
	release_lock (fd, 1);

	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_int: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_values_int leave");
}



void HFS_update_item_values_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid,
				 int lastclock, int nextcheck, const char* prevvalue, const char* lastvalue, const char* prevorgvalue)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, 0, NK_ItemValues);
	int fd, kind;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_values_str entered");

	if (!name)
		return;

	zabbix_log(LOG_LEVEL_DEBUG, "got name %s", name);

	make_directories (name);

	/* open file for writing */
	fd = open (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_str: open(): %s: %s", name, strerror(errno));
		free (name);
		return;
	}
	free (name);

	/* place write lock on that file or wait for unlock */
	if (!obtain_lock (fd, 1)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_str: close(): %s", strerror(errno));
		return;
	}

	/* lock obtained, write data */
	if (write (fd, &lastclock, sizeof (lastclock)) == -1 ||
	    write (fd, &nextcheck, sizeof (nextcheck)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_str: write(): %s", strerror(errno));
	
	kind = 2;
	if (write (fd, &kind, sizeof (kind)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_str: write(): %s", strerror(errno));

	write_str (fd, prevvalue);
	write_str (fd, lastvalue);
	write_str (fd, prevorgvalue);

	/* truncate file */
	if (ftruncate (fd, lseek (fd, 0, SEEK_CUR)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_str: ftruncate(): %s", strerror(errno));

	/* release lock */
	release_lock (fd, 1);

	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_str: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_values_str leave");
}


int HFS_get_item_values_dbl (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int* lastclock,
			     int* nextcheck, double* prevvalue, double* lastvalue, double* prevorgvalue)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, 0, NK_ItemValues);
	int fd, kind;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_values_dbl entered");
	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);
	if (fd < 0) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_dbl: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	/* obtain read lock */
	if (!obtain_lock (fd, 0)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_dbl: close(): %s", strerror(errno));
		return 0;
	}

	/* reading data */
	if (read (fd, lastclock, sizeof (*lastclock)) == -1 ||
	    read (fd, nextcheck, sizeof (*nextcheck)) == -1 ||
	    read (fd, &kind, sizeof (kind)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_dbl: read(): %s", strerror(errno));
	
	if (kind != 0) {
		release_lock (fd, 0);
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_dbl: close(): %s", strerror(errno));
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_dbl error. Incorrect type (%d)", kind);
		return 0;
	}

	if (read (fd, prevvalue, sizeof (*prevvalue)) == -1 ||
	    read (fd, lastvalue, sizeof (*lastvalue)) == -1 ||
	    read (fd, prevorgvalue, sizeof (*prevorgvalue)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_dbl: read(): %s", strerror(errno));

	/* release read lock */
	release_lock (fd, 0);
	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_dbl: close(): %s", strerror(errno));
	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_values_dbl leave");
	return 1;
}


int HFS_get_item_values_int (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int* lastclock,
			     int* nextcheck, zbx_uint64_t* prevvalue, zbx_uint64_t* lastvalue, zbx_uint64_t* prevorgvalue)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, 0, NK_ItemValues);
	int fd, kind;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_values_int entered");
	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);
	if (fd < 0) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_int: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	/* obtain read lock */
	if (!obtain_lock (fd, 0)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_int: close(): %s", strerror(errno));
		return 0;
	}

	/* reading data */
	if (read (fd, lastclock, sizeof (*lastclock)) == -1 ||
	    read (fd, nextcheck, sizeof (*nextcheck)) == -1 ||
	    read (fd, &kind, sizeof (kind)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_dbl: read(): %s", strerror(errno));
	
	if (kind != 1) {
		release_lock (fd, 0);
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_int: close(): %s", strerror(errno));
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_values_int error. Incorrect type (%d)", kind);	
		return 0;
	}

	if (read (fd, prevvalue, sizeof (*prevvalue)) == -1 ||
	    read (fd, lastvalue, sizeof (*lastvalue)) == -1 ||
	    read (fd, prevorgvalue, sizeof (*prevorgvalue)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_int: read(): %s", strerror(errno));

	/* release read lock */
	release_lock (fd, 0);

	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_int: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_values_int leave");
	return 1;
}



int HFS_get_item_values_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid,
			 int* lastclock, int* nextcheck, char** prevvalue, char** lastvalue, char** prevorgvalue)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, 0, NK_ItemValues);
	int fd, kind;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_values_str entered");
	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);
	if (fd < 0) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_str: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	/* obtain read lock */
	if (!obtain_lock (fd, 0)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_str: close(): %s", strerror(errno));
		return 0;
	}

	/* reading data */
	if (read (fd, lastclock, sizeof (*lastclock)) == -1 ||
	    read (fd, nextcheck, sizeof (*nextcheck)) == -1 ||
	    read (fd, &kind, sizeof (kind)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_str: read(): %s", strerror(errno));
	
	if (kind != 2) {
		release_lock (fd, 0);
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_str: close(): %s", strerror(errno));
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_values_str error. Incorrect type (%d)", kind);	
		return 0;
	}

	*prevvalue = read_str (fd);
	*lastvalue = read_str (fd);
	*prevorgvalue = read_str (fd);

	/* release read lock */
	release_lock (fd, 0);

	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_str: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_values_str leave");
	return 1;
}



void HFS_update_item_status (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int status, const char* error)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, 0, NK_ItemStatus);
	int fd, kind;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_status entered");

	if (!name)
		return;

	zabbix_log(LOG_LEVEL_DEBUG, "got name %s", name);

	make_directories (name);

	/* open file for writing */
	fd = open (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_status: open(): %s: %s", name, strerror(errno));
		free (name);
		return;
	}
	free (name);

	/* place write lock on that file or wait for unlock */
	if (!obtain_lock (fd, 1)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_status: close(): %s", strerror(errno));
		return;
	}

	/* lock obtained, write data */
	if (write (fd, &status, sizeof (status)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_status: write(): %s", strerror(errno));

	write_str (fd, error);

	/* truncate file */
	if (ftruncate (fd, lseek (fd, 0, SEEK_CUR)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_status: ftruncate(): %s", strerror(errno));

	/* release lock */
	release_lock (fd, 1);

	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_status: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_status leave");
}


void HFS_update_item_stderr (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, const char* stderr)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, 0, NK_ItemStderr);
	int fd, kind;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_stderr entered");

	if (!name)
		return;

	zabbix_log(LOG_LEVEL_DEBUG, "got name %s", name);

	make_directories (name);

	/* open file for writing */
	fd = open (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_stderr: open(): %s: %s", name, strerror(errno));
		free (name);
		return;
	}
	free (name);

	/* place write lock on that file or wait for unlock */
	if (!obtain_lock (fd, 1)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_stderr: close(): %s", strerror(errno));
		return;
	}

	/* lock obtained, write data */
	write_str (fd, stderr);

	/* truncate file */
	if (ftruncate (fd, lseek (fd, 0, SEEK_CUR)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_stderr: write(): %s", strerror(errno));

	/* release lock */
	release_lock (fd, 1);

	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_stderr: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_stderr leave");
}


int HFS_get_item_status (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int* status, char** error)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, 0, NK_ItemStatus);
	int fd;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_status entered");
	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);
	if (fd < 0) {
		if (errno != ENOENT)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_status: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	/* obtain read lock */
	if (!obtain_lock (fd, 0)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_status: close(): %s", strerror(errno));
		return 0;
	}

	/* reading data */
	if (read (fd, status, sizeof (*status)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_status: read(): %s", strerror(errno));
	*error = read_str (fd);

	/* release read lock */
	release_lock (fd, 0);

	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_status: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_status leave");
	return 1;
}


int HFS_get_item_stderr (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, char** stderr)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, 0, NK_ItemStderr);
	int fd;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_stderr entered");
	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);
	if (fd < 0) {
		if (errno != ENOENT)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_stderr: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	/* obtain read lock */
	if (!obtain_lock (fd, 0)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_stderr: close(): %s", strerror(errno));
		return 0;
	}

	/* reading data */
	*stderr = read_str (fd);

	/* release read lock */
	release_lock (fd, 0);

	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_stderr: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_stderr leave");
	return 1;
}
