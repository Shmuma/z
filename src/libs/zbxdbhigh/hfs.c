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
#include <stdarg.h>

#include "common.h"
#include "log.h"
#include "hfs.h"
#include "hfs_internal.h"
#include "memcache_php.h"
#include "tpl.h"

#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <aio.h>
#include <stdlib.h>
#include <errno.h>

# ifndef ULLONG_MAX
#  define ULLONG_MAX    18446744073709551615ULL
# endif


/* global file descriptors */
static int function_val_fd = -1;


int is_trend_type (item_type_t type)
{
    switch (type) {
    case IT_DOUBLE:
    case IT_UINT64:
        return 0;

    case IT_TRENDS:
        return 1;

    default:
        return 0;
    }
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


/*
  Routine adds double value to HistoryFS storage.
*/
void HFSadd_history (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, double value, hfs_time_t clock)
{
    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history()");
    store_values (hfs_base_dir, siteid, itemid, clock, delay, &value, sizeof (double), 1, IT_DOUBLE);
}



/*
  Routine adds uint64 value to HistoryFS storage.
*/
void HFSadd_history_uint (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, zbx_uint64_t value, hfs_time_t clock)
{
    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history_uint()");   
    store_values (hfs_base_dir, siteid, itemid, clock, delay, &value, sizeof (zbx_uint64_t), 1, IT_UINT64);
}



/*
  Routine adds array of double values to HistoryFS storage.
*/
void HFSadd_history_vals (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay,
			  double* values, int count, hfs_time_t clock)
{
    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history()");
    store_values (hfs_base_dir, siteid, itemid, clock, delay, values, sizeof (double), count, IT_DOUBLE);
}



/*
  Routine adds array of uint64 value to HistoryFS storage.
*/
void HFSadd_history_vals_uint (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay,
			       zbx_uint64_t* values, int count, hfs_time_t clock)
{
    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history_uint()");
    store_values (hfs_base_dir, siteid, itemid, clock, delay, values, sizeof (zbx_uint64_t), count, IT_UINT64);
}



void recalculate_trend (hfs_trend_t* new, hfs_trend_t old, item_type_t type)
{
	if (new->max.d < old.max.d)
		new->max.d = old.max.d;

	if (new->min.d > old.min.d)
		new->min.d = old.min.d;

	new->avg.d = (old.avg.d * old.count + new->avg.d * new->count) / (new->count + old.count);
	new->count += old.count;
}



void *xfree(void *ptr)
{
	if (ptr != NULL)
		free(ptr);
	return NULL;
}

int xopen(const char *fn, int flags, mode_t mode)
{
	int retval;
	if ((retval = open(fn, flags, mode)) == -1) {
		if (!make_directories (fn)) {
			if ((retval = open(fn, flags, mode)) == -1)
				zabbix_log(LOG_LEVEL_DEBUG, "HFS: %s: open: %s", fn, strerror(errno));
                }
		else
			return -1;
	}
	return retval;
}


int store_values (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t clock, int delay, void* value, int len, int count, item_type_t type)
{
    char *p_meta = NULL, *p_data = NULL;
    int is_trend = is_trend_type (type);
    int res;

    p_meta = get_name (hfs_base_dir, siteid, itemid, is_trend ? NK_TrendItemMeta : NK_ItemMeta);
    p_data = get_name (hfs_base_dir, siteid, itemid, is_trend ? NK_TrendItemData : NK_ItemData);

    res = hfs_store_values (p_meta, p_data, clock, delay, value, len, count, type);
    free (p_meta);
    free (p_data);

    return res;
}


int hfs_store_values (const char* p_meta, const char* p_data, hfs_time_t clock, int delay, void* values, int len, int count, item_type_t type)
{
    hfs_meta_item_t item, *ip;
    hfs_meta_t* meta;
    int fd;
    int i, j, res;
    unsigned char v = 0xff;
    hfs_off_t extra;
    hfs_off_t size, ofs;
    int retval = 1;

    if ((meta = read_metafile (p_meta, NULL)) == NULL) {
	zabbix_log(LOG_LEVEL_CRIT, "HFS: store_values(): read_metafile(%s) == NULL", p_meta);
	return retval;
    }

    clock -= clock % delay;

    /* should we start a new block? */
    if (meta->blocks == 0 || meta->last_delay != delay || type != meta->last_type) {
	item.start = clock;
        item.end = clock + (count-1)*delay;
	item.type = type;

	/* analyze current value */
	item.delay = delay;
	if (meta->blocks) {
	    ip = meta->meta + (meta->blocks-1);
            ofs = meta->last_ofs + len;
	    item.ofs = meta->last_ofs + len;
	}
	else
	    item.ofs = ofs = 0;

	meta->blocks++;
	meta->last_delay = delay;
	meta->last_type = type;

	if (!write_metafile (p_meta, meta, &item))
		goto err_exit;

	/* append data */
	if ((fd = xopen (p_data, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
		goto err_exit;

	if (lseek (fd, ofs, SEEK_SET) == -1)
		goto err_exit;

	/* TODO: change this to AIO */
	if (write (fd, values, len * count) == -1)
		goto err_exit;

	close (fd);
	fd = -1;

	retval = 0;
    }
    else {
	/* check for gaps in block */
	ip = meta->meta + (meta->blocks-1);

        if (clock < ip->start) {
            zabbix_log (LOG_LEVEL_ERR, "HFS: cannot write outside last block boundaries");
            goto err_exit;
        }

	if ((fd = xopen (p_data, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
		goto err_exit;

	/* round start and end clock according to delay */
	ip->start -= ip->start % delay;
	ip->end -= ip->end % delay;

	ofs = ip->ofs + ((clock - ip->start) / delay) * len;

	extra = ofs - meta->last_ofs - len;
	extra -= extra % len;

	if (extra > 0) {
		char* buf;
		lseek (fd, meta->last_ofs + len, SEEK_SET);
		buf = (char*)malloc (extra + len*count);
		memset (buf, 0xFF, extra);
		memcpy (buf+extra, values, len*count);
		res = write (fd, buf, extra + len*count);
		free (buf);
	}
	else {
		lseek (fd, ofs, SEEK_SET);
		/* we're ready to write */
		/* TODO: change this to AIO */
		res = write (fd, values, len*count);
	}
        if (meta->last_ofs < ofs + len*(count-1))
		meta->last_ofs = ofs + len*(count-1);

        close (fd);
        fd = -1;

        /* update meta */
        if (ip->end < clock + delay*(count-1))
            ip->end = clock + delay*(count-1);

	write_metafile (p_meta, meta, NULL);
	retval = 0;
    }

err_exit:
    if (fd != -1)
        close (fd);
    free_meta (meta);

    return retval;
}



int make_directories (const char* path)
{
    /* parse path */
    const char *p = path, *pp;
    char buf[PATH_MAX+1];
    int len = 0;
    struct stat st;

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
		if (mkdir (buf, 0775)) {
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


void write_str (int fd, const char* str)
{
	int len = str ? strlen (str) : 0;

	write (fd, &len, sizeof (len));
	if (len)
		write (fd, str, len+1);
}


void write_str_wo_len (int fd, const char* str)
{
	int len = str ? strlen (str) : 0;

	if (len)
		write (fd, str, len+1);
}



int str_buffer_length (const char* str)
{
	int len = str ? strlen (str) : 0;
	if (len)
		len += 1;
	return len + sizeof (int);
}


char* buffer_str (char* buf, const char* str, int buf_size)
{
	int len = str ? strlen (str) : 0;

	if (buf_size < sizeof (len) + len + (len ? 1 : 0))
		return NULL;

	*(int*)buf = len;
	buf += sizeof (len);
	if (len) {
		memcpy (buf, str, len+1);
		return buf + len + 1;
	}
	else
		return buf;
}


char* read_str (int fd)
{
	int len;
	char* res = NULL;

	if (read (fd, &len, sizeof (len)) < sizeof (len))
		return NULL;

	if (len) {
		res = (char*)malloc (len+1);
		if (res) {
			res[0] = 0;
			read (fd, res, len+1);
		}
	}

	return res;
}


char* read_str_wo_len (int fd, int len)
{
	char* res = NULL;

	if (len) {
		res = (char*)malloc (len+1);
		if (res) {
			res[0] = 0;
			read (fd, res, len+1);
		}
	}

	return res;
}



char* unbuffer_str (char **buf)
{
	char* b = *buf;
	int len = *(int*)b;
	char* res = NULL;

	b += sizeof (len);
	*buf = b;

	if (len) {
		res = (char*)malloc (len+1);
		memcpy (res, b, len+1);
		b += len+1;
		*buf = b;
	}

	return res;
}


/*
   Performs folding on values from specified time interval
 */
int HFSread_interval(const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, hfs_time_t to, void* init_res, read_count_fn_t fn)
{
    char *p_data;
    hfs_meta_t* meta;
    hfs_off_t ofs;
    int fd, block;
    item_value_u value;
    hfs_time_t ts;
    int count = 0, total = 0;

    zabbix_log(LOG_LEVEL_DEBUG, "HFSread_interval (%s, %llu, %lld, %lld)", hfs_base_dir, itemid, from, to);

    meta = read_meta (hfs_base_dir, siteid, itemid, 0);
    if (!meta)
	return count;

    if (!meta->blocks) {
	free_meta (meta);
	return count;
    }

    /* open data file and count amount of valid items */
    p_data = get_name (hfs_base_dir, siteid, itemid, NK_ItemData);

    if ((fd = open (p_data, O_RDONLY)) == -1) {
	free_meta (meta);
	free (p_data);
	return count;
    }
    free (p_data);

    ofs = find_meta_ofs (from, meta, &block);

    if (ofs != -1) {
	    /* find correct timestamp */
	    ts = meta->meta[block].start + ((ofs - meta->meta[block].ofs) / sizeof (double)) * meta->meta[block].delay;
	    lseek (fd, ofs, SEEK_SET);

	    while (read (fd, &value, sizeof (value)) > 0) {
		    if (is_valid_val (&value, sizeof (value))) {
			    fn (meta->meta[block].type, value, ts, init_res);
			    count++;
			    total++;
		    }
		    ts += meta->meta[block].delay;

		    if (ts >= to)
			    break;

		    if (ts >= meta->meta[block].end) {
			    block++;
			    if (block >= meta->blocks)
				    break;
		    }
	    }
    }

    free_meta (meta);
    close (fd);
    return count;
}



/*
  Performs folding of values of historical data into some state. We filter values according to time.
*/
void foldl_time (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t ts, void* init_res, fold_fn_t fn)
{
    char *p_data;
    hfs_meta_t* meta;
    hfs_off_t ofs;
    int fd, count, i;
    zbx_uint64_t value[128];
    int total = 0;

    zabbix_log(LOG_LEVEL_DEBUG, "HFS_foldl_time (%s, %llu, %lld)", hfs_base_dir, itemid, ts);

    meta = read_meta (hfs_base_dir, siteid, itemid, 0);
    if (!meta)
	return;

    if (!meta->blocks) {
	free_meta (meta);
	return;
    }

    /* open data file and count amount of valid items */
    p_data = get_name (hfs_base_dir, siteid, itemid, NK_ItemData);

    if ((fd = open (p_data, O_RDONLY)) == -1) {
	free_meta (meta);
	free (p_data);
	return;
    }
    free (p_data);

    ofs = find_meta_ofs (ts, meta, NULL);

    if (ofs != -1) {
	    lseek (fd, ofs, SEEK_SET);
	    while (1) {
		    count = read (fd, value, sizeof (value)) / sizeof (value[0]);
		    if (!count)
			    break;
		    for (i = 0; i < count; i++)
			    if (is_valid_val (value+i, sizeof (value[i]))) {
				    fn (value+i, init_res);
				    total++;
			    }
	    }
    }

    free_meta (meta);
    close (fd);
}

int is_valid_val (void* val, size_t len)
{
    int i;
    for (i = 0; i < len; i++)
	    if (((unsigned char *)val)[i] != 0xff)
		    return 1;
    return 0;
}

/*
  Performs folding of values of historical data into some state. We filter values according to count of values.
*/
void foldl_count (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t count, void* init_res, fold_fn_t fn)
{
    char *p_data;
    int fd;
    hfs_time_t ts = time (NULL)-1;
    zbx_uint64_t value;
    hfs_off_t ofs;

    zabbix_log(LOG_LEVEL_DEBUG, "HFS_foldl_count (%s, %llu, %lld)", hfs_base_dir, itemid, count);

    p_data = get_name (hfs_base_dir, siteid, itemid, NK_ItemData);
    fd = open (p_data, O_RDONLY);
    free (p_data);

    if (fd < 0)
        return;

    ofs = lseek (fd, 0, SEEK_END);

    while (ofs && count > 0) {
        ofs = lseek (fd, ofs - sizeof (double), SEEK_SET);
        read (fd, &value, sizeof (value));
        if (is_valid_val (&value, sizeof (value))) {
	    fn (&value, init_res);
	    count--;
	}
    }

/*     release_lock (fd, 0); */
    close (fd);
}

hfs_meta_t *read_metafile(const char *metafile, const char* siteid)
{
	hfs_meta_t *res;
	int fd = -1;
	char* buf = NULL;
	char* p;
	size_t len;
	struct stat st;

	if (!metafile)
		return NULL;

	if ((res = (hfs_meta_t *) malloc(sizeof(hfs_meta_t))) == NULL)
		return NULL;

#ifdef HAVE_MEMCACHE
	buf = memcache_zbx_read_val (siteid, metafile, &len);
#endif

	if (!buf) {
		/* if we have no file, create empty one */
		if ((fd = open(metafile, O_RDONLY)) < 0) {
			res->blocks = 0;
			res->last_delay = 0;
			res->last_type = IT_DOUBLE;
			res->last_ofs = 0;
			res->meta = NULL;
			return res;
		}

		if (fstat (fd, &st) < 0)
			st.st_size = 1024*4;
		buf = (char*)malloc (st.st_size);

		/* read whole file into buffer (we hope) */
		len = read (fd, buf, st.st_size);
	}

	if (len < sizeof (hfs_meta_t) - sizeof (res->meta)) {
		if (fd >= 0)
			close (fd);
		free (res);
		free (buf);
		return NULL;
	}

	p = buf;
	memcpy (res, p, sizeof (hfs_meta_t) - sizeof (res->meta));
	p += sizeof (hfs_meta_t) - sizeof (res->meta);
	len -= sizeof (hfs_meta_t) - sizeof (res->meta);

	res->meta = (hfs_meta_item_t *) malloc(sizeof(hfs_meta_item_t)*res->blocks);
        if (!res->meta) {
		if (fd >= 0)
			close (fd);
		free (res);
		free (buf);
		return NULL;
	}

	if (len == sizeof (hfs_meta_item_t) * res->blocks)
		memcpy (res->meta, p, sizeof (hfs_meta_item_t) * res->blocks);

	if (fd >= 0)
		close (fd);
	free (buf);
	return res;
}


hfs_meta_t* read_meta (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int trend)
{
    char* path = get_name (hfs_base_dir, siteid, itemid, trend ? NK_TrendItemMeta : NK_ItemMeta);
    hfs_meta_t* res = read_metafile(path, siteid);
    free(path);

    return res;
}


/* Write metafile. If extra argument passed, it also will be
   written. Warning! If extra is not NULL, meta->blocks must be
   already incremented.  */
int write_metafile (const char* filename, hfs_meta_t* meta, hfs_meta_item_t* extra)
{
	int fd;
	unsigned char* buf = NULL, *p;
	int len, i;

        if ((fd = xopen (filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
		return 0;

	len = sizeof (hfs_meta_t) - sizeof (meta->meta) + meta->blocks * sizeof (hfs_meta_item_t);
	buf = (unsigned char*)malloc (len);

	if (!buf) {
		close (fd);
		return 0;
	}

	memcpy (buf, meta, sizeof (hfs_meta_t) - sizeof (meta->meta));
	p = buf + sizeof (hfs_meta_t) - sizeof (meta->meta);

	for (i = 0; i < meta->blocks - (extra ? 1 : 0); i++) {
		memcpy (p, meta->meta+i, sizeof (hfs_meta_item_t));
		p += sizeof (hfs_meta_item_t);
	}

	if (extra)
		memcpy (p, extra, sizeof (hfs_meta_item_t));

	write (fd, buf, len);
	close (fd);

#ifdef HAVE_MEMCACHE
	memcache_zbx_save_val (filename, buf, len, 0);
#endif

	free (buf);
	return 1;
}


void free_meta (hfs_meta_t* meta)
{
    if (!meta)
	return;

    if (meta->meta)
	free (meta->meta);

    free (meta);
}


char* get_name (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, name_kind_t kind)
{
    char* res;
    int len = strlen (hfs_base_dir) + strlen (siteid) + 100;
    zbx_uint64_t item_ord;

    res = (char*)malloc (len);

    if (!res)
	return NULL;

    item_ord = itemid / 1000;

    switch (kind) {
    case NK_ItemData:
    case NK_ItemMeta:
            snprintf (res, len, "%s/%s/items/%llu/%llu/history.%s", hfs_base_dir, siteid, item_ord, itemid,
		      kind == NK_ItemMeta ? "meta" : "data");
            break;
    case NK_ItemString:
            snprintf (res, len, "%s/%s/items/%llu/%llu/strings.data", hfs_base_dir, siteid, item_ord, itemid);
	    break;
    case NK_ItemLog:
            snprintf (res, len, "%s/%s/items/%llu/%llu/logs.data", hfs_base_dir, siteid, item_ord, itemid);
	    break;
    case NK_ItemLogDir:
            snprintf (res, len, "%s/%s/items/%llu/%llu/logs.dir", hfs_base_dir, siteid, item_ord, itemid);
	    break;
    case NK_TrendItemData:
    case NK_TrendItemMeta:
            snprintf (res, len, "%s/%s/items/%llu/%llu/trends.%s", hfs_base_dir, siteid, item_ord, itemid,
		      kind == NK_TrendItemMeta ? "meta" : "data");
	    break;
    case NK_HostState:
	    snprintf (res, len, "%s/%s/hosts/hosts.state", hfs_base_dir, siteid);
	    break;
    case NK_HostError:
	    snprintf (res, len, "%s/%s/hosts/%llu/%llu.state", hfs_base_dir, siteid, item_ord, itemid);
	    break;
    case NK_ItemValues:
            snprintf (res, len, "%s/%s/items/%llu/%llu/values.data", hfs_base_dir, siteid, item_ord, itemid);
	    break;
    case NK_ItemStatus:
            snprintf (res, len, "%s/%s/items/%llu/%llu/status.data", hfs_base_dir, siteid, item_ord, itemid);
	    break;
    case NK_TriggerStatus:
	    snprintf (res, len, "%s/%s/misc/triggers.data", hfs_base_dir, siteid);
	    break;
    case NK_Alert:
	    snprintf (res, len, "%s/%s/misc/alerts.data", hfs_base_dir, siteid);
	    break;
    case NK_EventTrigger:
	    snprintf (res, len, "%s/%s/triggers/%llu/%llu/events.data", hfs_base_dir, siteid, item_ord, itemid);
	    break;
    case NK_EventHost:
	    snprintf (res, len, "%s/%s/hosts/%llu/%llu/events.data", hfs_base_dir, siteid, item_ord, itemid);
	    break;
    case NK_FunctionVals:
	    snprintf (res, len, "%s/%s/misc/functions.data", hfs_base_dir, siteid);
	    break;
    case NK_AggrSlaveVal:
            snprintf (res, len, "%s/%s/items/%llu/%llu/aggr.data", hfs_base_dir, siteid, item_ord, itemid);
	    break;
    }

    return res;
}


hfs_off_t find_meta_ofs (hfs_time_t time, hfs_meta_t* meta, int* block)
{
	int i;
	int f = 0;

	for (i = 0; i < meta->blocks; i++) {
		if (block)
			*block = i;

		zabbix_log(LOG_LEVEL_DEBUG, "check block %d[%lld,%lld], ts %lld, ofs %llu", i,
			   meta->meta[i].start, meta->meta[i].end, time, meta->meta[i].ofs);

		if (!f) {
			f = (meta->meta[i].end >= time);
			if (!f)
				continue;
		}

		if (meta->meta[i].start >= time)
			return meta->meta[i].ofs;

		if (is_trend_type (meta->meta[i].type))
			return meta->meta[i].ofs + sizeof (hfs_trend_t) * ((time - meta->meta[i].start) / meta->meta[i].delay);
		else
			return meta->meta[i].ofs + sizeof (double) * ((time - meta->meta[i].start) / meta->meta[i].delay);
	}

	return -1;
}


hfs_time_t get_data_index_from_ts (hfs_time_t ts)
{
	return ts / (zbx_uint64_t)1000000;
}


zbx_uint64_t HFS_get_count (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(zbx_uint64_t)))
	    (*(zbx_uint64_t*)state)++;
    }

    zbx_uint64_t val = 0;
    foldl_time (hfs_base_dir, siteid, itemid, from, &val, functor);
    return val;
}


zbx_uint64_t HFS_get_count_u64_eq (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, zbx_uint64_t value)
{
    typedef struct {
	zbx_uint64_t val, count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(zbx_uint64_t)) && *(zbx_uint64_t*)db == ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_ne (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, zbx_uint64_t value)
{
    typedef struct {
	zbx_uint64_t val, count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(zbx_uint64_t)) && *(zbx_uint64_t*)db != ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_gt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, zbx_uint64_t value)
{
    typedef struct {
	zbx_uint64_t val, count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(zbx_uint64_t)) && *(zbx_uint64_t*)db > ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_lt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, zbx_uint64_t value)
{
    typedef struct {
	zbx_uint64_t val, count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(zbx_uint64_t)) && *(zbx_uint64_t*)db < ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_ge (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, zbx_uint64_t value)
{
    typedef struct {
	zbx_uint64_t val, count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(zbx_uint64_t)) && *(zbx_uint64_t*)db >= ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_u64_le (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, zbx_uint64_t value)
{
    typedef struct {
	zbx_uint64_t val, count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(zbx_uint64_t)) && *(zbx_uint64_t*)db <= ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_eq (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, double value)
{
    typedef struct {
	double val;
	zbx_uint64_t count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(double)) && *(double*)db+0.00001 > ((state_t*)state)->val && *(double*)db-0.00001 < ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_ne (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, double value)
{
    typedef struct {
	double val;
	zbx_uint64_t count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(double)) && (*(double*)db+0.00001 < ((state_t*)state)->val || *(double*)db-0.00001 > ((state_t*)state)->val))
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_gt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, double value)
{
    typedef struct {
	double val;
	zbx_uint64_t count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(double)) && *(double*)db > ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_lt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, double value)
{
    typedef struct {
	double val;
	zbx_uint64_t count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(double)) && *(double*)db < ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_ge (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, double value)
{
    typedef struct {
	double val;
	zbx_uint64_t count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(double)) && *(double*)db >= ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_count_float_le (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, double value)
{
    typedef struct {
	double val;
	zbx_uint64_t count;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(double)) && *(double*)db <= ((state_t*)state)->val)
	    ((state_t*)state)->count++;
    }

    state_t state;
    state.val = value;
    state.count = 0;

    foldl_time (hfs_base_dir, siteid, itemid, from, &state, functor);
    return state.count;
}


zbx_uint64_t HFS_get_sum_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(zbx_uint64_t)))
	    *(zbx_uint64_t*)state += *(zbx_uint64_t*)db;
    }

    zbx_uint64_t sum = 0;

    if (seconds)
	foldl_time (hfs_base_dir, siteid, itemid, period, &sum, functor);
    else
	foldl_count (hfs_base_dir, siteid, itemid, period, &sum, functor);

    return sum;
}


double HFS_get_sum_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(double)))
	    *(double*)state += *(double*)db;
    }

    double sum = 0;

    if (seconds)
	foldl_time (hfs_base_dir, siteid, itemid, period, &sum, functor);
    else
	foldl_count (hfs_base_dir, siteid, itemid, period, &sum, functor);

    return sum;
}


double HFS_get_avg_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds)
{
    typedef struct {
	zbx_uint64_t count, sum;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(zbx_uint64_t))) {
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


double HFS_get_avg_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds)
{
    typedef struct {
	zbx_uint64_t count;
	double sum;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(double))) {
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


zbx_uint64_t HFS_get_min_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(zbx_uint64_t)))
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


double HFS_get_min_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(double)))
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


zbx_uint64_t HFS_get_max_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(zbx_uint64_t)))
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


double HFS_get_max_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds)
{
    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(double)))
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


zbx_uint64_t HFS_get_delta_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds)
{
    typedef struct {
	zbx_uint64_t min, max;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(zbx_uint64_t))) {
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

double HFS_get_delta_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds)
{
    typedef struct {
	double min, max;
    } state_t;

    void functor (void* db, void* state)
    {
	if (is_valid_val (db, sizeof(double))) {
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

int HFS_find_meta(const char *hfs_base_dir, const char* siteid, int trend,
                  zbx_uint64_t itemid, zbx_uint64_t from_ts, hfs_meta_t **res)
{
	int i, block = 0;
	hfs_meta_t *meta = NULL;
	hfs_meta_item_t *ip = NULL;
	char *path;

#ifdef DEBUG_legion
	fprintf(stderr, "It HFS_find_meta(hfs_base_dir=%s, siteid=%s, itemid=%lld, from=%d, res)\n",
		hfs_base_dir, siteid, itemid, from_ts);
	fflush(stderr);
#endif
	(*res) = NULL;

	if ((meta = read_meta(hfs_base_dir, siteid, itemid, trend)) == NULL)
		return -1; // Somethig real bad happend :(

	if ((meta == NULL) || (meta->blocks == 0)) {
		free_meta(meta);
		return -1;
	}

	(*res) = meta;
	ip = meta->meta;

	if (from_ts <= ip->start)
		return 0;

	for (i = 0; i < meta->blocks; i++) {
		ip = meta->meta + i;
		if (from_ts <= ip->end)
			return i;
	}

	/* Interesting situation:
	   Meta file was found, from_ts not in past, but timestamp not found in blocks
	*/
	free_meta(meta);
	(*res) = NULL;
	return -1;
}

void HFS_init_trend_value(int is_trend, item_type_t type, void *val, hfs_trend_t *res)
{
	if (!is_trend) {
		res->count = 1;
		res->max = *((item_value_u *) val);
		res->min = *((item_value_u *) val);
		res->avg = *((item_value_u *) val);
	}
	else {
		*res = *((hfs_trend_t *) val);
	}

	if (type == IT_UINT64) {
		res->max.d = res->max.l;
		res->min.d = res->min.l;
		res->avg.d = res->avg.l;
	}
}

//!!!!!!
size_t HFSread_item (const char *hfs_base_dir, const char* siteid,
                     int is_trend,             zbx_uint64_t itemid,
                     size_t sizex,
                     hfs_time_t graph_from_ts, hfs_time_t graph_to_ts,
                     hfs_time_t from_ts,       hfs_time_t to_ts,
                     hfs_item_value_t **result)
{
	void 		*val;
	item_value_u 	 val_history;
	hfs_trend_t  	 val_trends;
	hfs_trend_t  	 val_temp;

	hfs_time_t ts = from_ts;
	size_t val_len, items = 0, result_size = 0;
	int finish_loop = 0;
	long count = 0, cur_group, group = -1;
	long z, p, x;

	int block, fd = -1;
	char *p_data = NULL;
	hfs_meta_t *meta = NULL;
	hfs_off_t ofs;
	hfs_meta_item_t *ip;

	char* buffer = NULL;
	const int buf_size = 1024; /* amount of items can be stored in buffer */
	int buf_items;             /* amount of items read from file */
	int i;

	p = (graph_to_ts - graph_from_ts);
	z = (p - graph_from_ts % p);
	x = (sizex - 1);

	zabbix_log(LOG_LEVEL_DEBUG,
		"In HFSread_item(hfs_base_dir=%s, trend=%d, itemid=%lld, sizex=%d, graph_from=%lld, graph_to=%lld, from=%lld, to=%lld)\n",
		hfs_base_dir, is_trend, itemid, x, graph_from_ts, graph_to_ts, from_ts, to_ts);

	zabbix_log(LOG_LEVEL_DEBUG,
		"HFS: HFSread_item: Magic numbers: p=%d, z=%d, x=%d\n",
		p, z, x);

	if (is_trend == 0) {
		val = &val_history;
		val_len = sizeof(val_history);
	}
	else {
		val = &val_trends;
		val_len = sizeof(val_trends);
	}

	buffer = (char*)malloc (buf_size * val_len);
        if (!buffer) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS: memory allocation error");
		return 0;
	}

	if ((block = HFS_find_meta(hfs_base_dir, siteid, is_trend, itemid, ts, &meta)) == -1) {
		zabbix_log(LOG_LEVEL_WARNING, "HFS: unable to find position in metafile");
		return 0;
	}

	ip = meta->meta + block;

	if (graph_to_ts < ip->start) {
		/*      graph_from_ts   graph_to_ts
		   ----|---------------|----------------> Time
		                            ^ ip->start
		*/
		free_meta(meta);
		return 0;
	}

	if (ip->start > ts) {
		/*          graph_from_ts   graph_to_ts
		   --------|---------------|------------> Time
			^ ip->start
		*/
		ts = ip->start;
	}

	if ((p_data = get_name (hfs_base_dir, siteid, itemid, is_trend ? NK_TrendItemData : NK_ItemData)) == NULL) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS: unable to get datafile");
		free_meta(meta);
		return 0;
	}

	if ((fd = open (p_data, O_RDONLY)) == -1) {
		zabbix_log(LOG_LEVEL_DEBUG, "HFS: %s: file open failed: %s", p_data, strerror(errno));
		goto out;
	}

	if ((ofs = find_meta_ofs (ts, meta, NULL)) == -1) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: %lld: unable to get offset in file", p_data, ts);
		goto out;
	}

	if (lseek (fd, ofs, SEEK_SET) == -1) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: unable to change file offset: %s", p_data, strerror(errno));
		goto out;
	}

	while ((buf_items = read (fd, buffer, buf_size * val_len)) > 0) {
		buf_items /= val_len;

		for (i = 0; i < buf_items; i++) {
			val = buffer + i * val_len;

			cur_group = (long) (x * ((ts + z) % p) / p);
			if (group == -1)
				group = cur_group;

			ts += ip->delay;

			if (!is_valid_val(val, val_len))
				continue;

			if (result_size <= items) {
				result_size += alloc_item_values;
				*result = (hfs_item_value_t *) realloc(*result, (sizeof(hfs_item_value_t) * result_size));
				HFS_init_trend_value(is_trend, ip->type, val, &((*result)[items].value));
			}

			if (group != cur_group) {
				(*result)[items].type  = ip->type;
				(*result)[items].group = cur_group;
				(*result)[items].count = count;
				(*result)[items].clock = ts;

				group = cur_group;
				count = 0;
				items++;

				if (result_size <= items) {
					result_size += alloc_item_values;
					*result = (hfs_item_value_t *) realloc(*result, (sizeof(hfs_item_value_t) * result_size));
				}

				HFS_init_trend_value(is_trend, ip->type, val, &((*result)[items].value));
			}

			if (count > 0) {
				HFS_init_trend_value(is_trend, ip->type, val, &val_temp);
				recalculate_trend(&val_temp,
						  (*result)[items].value,
						  ip->type);
				(*result)[items].value = val_temp;
			}

			if (ts >= to_ts)
				goto out;

			if (ts >= ip->end) {
				block++;

				if (block >= meta->blocks)
					goto out;

				ip = meta->meta + block;
			}

			count++;
		}
	}

out:
	free_meta (meta);
	xfree(p_data);
	if (fd != -1) {
		close(fd);
	}
	if (result_size > items)
		*result = (hfs_item_value_t *) realloc(*result, (sizeof(hfs_item_value_t) * items));
	if (buffer)
		free (buffer);
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: HFSread_item: Out items=%d\n", items);

	return items;
}

int HFSread_count(const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid,
                  int count, void* init_res, read_count_fn_t fn)
{
	int i;
	char *p_data = NULL;
	int fd = -1;
	hfs_time_t ts = time (NULL)-1;
	hfs_meta_item_t *ip = NULL;
	hfs_meta_t *meta = NULL;
	item_value_u val;
	hfs_off_t ofs;

#ifdef DEBUG_legion
	fprintf(stderr, "In HFSread_count(hfs_base_dir=%s, itemid=%lld, count=%d, res, func)\n",
			hfs_base_dir, itemid, count);
	fflush(stderr);
#endif

        if ((meta = read_meta(hfs_base_dir, siteid, itemid, 0)) == NULL)
		return -1; // Somethig real bad happend :(

	if (meta->blocks == 0)
		goto end;

	if ((p_data = get_name(hfs_base_dir, siteid, itemid, NK_ItemData)) == NULL) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS: unable to get file name");
		goto end; // error
	}

	if ((fd = open (p_data, O_RDONLY)) == -1) {
		zabbix_log(LOG_LEVEL_DEBUG, "HFS: %s: file open failed: %s", p_data, strerror(errno));
		goto end; // error
	}

	ofs = meta->last_ofs;

	for (i = (meta->blocks-1); i >= 0; i--) {
		ip = meta->meta + i;
		ts = ip->end;

		while (count > 0 && ip->start <= ts) {
			if ((ofs = lseek(fd, ofs, SEEK_SET)) == -1) {
				zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: unable to change file offset: %s", p_data, strerror(errno));
				goto end;
			}

			if (read(fd, &val.l, sizeof (val.l)) == -1) {
				zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: unable to read data: %s", p_data, strerror(errno));
				goto end;
			}

			if (is_valid_val(&val.l, sizeof(zbx_uint64_t))) {
				fn (ip->type, val, ts, init_res);
				count--;
			}

			ts = (ts - ip->delay);
			ofs -= sizeof(val.l);
		}

		if (count == 0)
			break;
	}
end:
	if (fd != -1)
		close(fd);

	xfree(p_data);
	free_meta(meta);

	return count;
}


/*
   Stores host availability in FS.
 */
void HFS_update_host_availability (const char* hfs_base_dir, const char* siteid, zbx_uint64_t hostid, int available, hfs_time_t clock, const char* error)
{
	char* name = get_name (hfs_base_dir, siteid, hostid, NK_HostState);
	int fd;

	if (!name)
		return;

	/* open file for writing */
	fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_host_availability: open(): %s: %s", name, strerror(errno));
		xfree (name);
		return;
	}
	xfree (name);

	lseek (fd, (sizeof (available) + sizeof (clock))*hostid, SEEK_SET);
	if (write (fd, &available, sizeof (available)) == -1 ||
	    write (fd, &clock, sizeof (clock)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_host_availability: write(): %s",  strerror(errno));

	close (fd);
}


/* Obtain host attributes stored in HFS. If successfull, return 1. If something goes wrong, return 0. */
int HFS_get_host_availability (const char* hfs_base_dir, const char* siteid, zbx_uint64_t hostid, int* available, hfs_time_t* clock, char** error)
{
	char* name = get_name (hfs_base_dir, siteid, hostid, NK_HostState);
	int fd;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_host_availability entered");

	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);

	if (fd < 0) {
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_host_availability: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);
	
	/* reading data */
	lseek (fd, (sizeof (*available) + sizeof (*clock))*hostid, SEEK_SET);
	if (read (fd, available, sizeof (*available)) == -1 ||
	    read (fd, clock, sizeof (*clock)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_host_availability: read(): %s", strerror(errno));
	*error = NULL;

	/* release read lock */
	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_host_availability: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_host_availability leave");
	return 1;
}


int HFS_get_hosts_statuses (const char* hfs_base_dir, const char* siteid, hfs_host_status_t** statuses)
{
	char* name = get_name (hfs_base_dir, siteid, 0, NK_HostState);
	int fd, count = 0, buf_size = 0;
	hfs_host_status_t status;
	zbx_uint64_t hostid;

	typedef struct {
		int available;
		hfs_time_t clock;
	} __attribute__ ((packed)) status_t ;

	status_t *buf = NULL;
	int buf_s, buf_c, i;

	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);
	if (fd < 0) {
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_hosts_statuses: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	buf_s = 1024;
	buf = (status_t*)malloc (buf_s * sizeof (status_t));

	hostid = 0;
	*statuses = NULL;

	while ((buf_c = read (fd, buf, buf_s * sizeof (status_t))) > 0) {
		buf_c /= sizeof (status_t);
		for (i = 0; i < buf_c; i++)  {
			status.clock = buf[i].clock;
			status.hostid = hostid++;
			if (!status.clock)
				continue;
			status.available = buf[i].available;
			if (count == buf_size)
				*statuses = (hfs_host_status_t*)realloc (*statuses, (buf_size += 256) * sizeof (hfs_host_status_t));
			(*statuses)[count++] = status;
		}
	}

	close (fd);
	free (buf);
	return count;
}


static void aio_completion_handler (sigval_t sigval)
{
	struct aiocb *req;
	req = (struct aiocb *)sigval.sival_ptr;
	close (req->aio_fildes);
	free ((char*)req->aio_buf);
	free (req);
}



static void aio_item_value (int fd, void* val, int len, const char* stderr)
{
	struct aiocb* cb;
	int buf_len = len + str_buffer_length (stderr);

	cb = calloc (1, sizeof (struct aiocb));

	cb->aio_fildes = fd;
	cb->aio_buf = malloc (buf_len);
	cb->aio_nbytes = buf_len;
	cb->aio_offset = 0;

	memcpy ((char*)cb->aio_buf, val, len);
	buffer_str ((char*)cb->aio_buf + len, stderr, buf_len-len);

	cb->aio_sigevent.sigev_notify = SIGEV_THREAD;
	cb->aio_sigevent.sigev_notify_function = aio_completion_handler;
	cb->aio_sigevent.sigev_notify_attributes = NULL;
	cb->aio_sigevent.sigev_value.sival_ptr = cb;

	if (aio_write (cb)) {
		close (fd);
		free ((char*)cb->aio_buf);
		free (cb);
	}
}



void HFS_update_item_values_dbl (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid,
				 hfs_time_t lastclock, double prevvalue, double lastvalue, double prevorgvalue,
				 const char* stderr)
{
#ifdef HAVE_MEMCACHE	
	const char* key;
	char* buf;
	int len;
	item_value_dbl_t val;

	key = memcache_get_key (MKT_LAST_DOUBLE, itemid);
	len = sizeof (item_value_dbl_t) + str_buffer_length (stderr);
	buf = (char*)malloc (len);
	
	((item_value_dbl_t*)buf)->lastclock = lastclock;
	((item_value_dbl_t*)buf)->kind = 0;
	((item_value_dbl_t*)buf)->prevvalue = prevvalue;
	((item_value_dbl_t*)buf)->lastvalue = lastvalue;
	((item_value_dbl_t*)buf)->prevorgvalue = prevorgvalue;
	buffer_str (buf+sizeof (item_value_dbl_t), stderr, len-sizeof (item_value_dbl_t));

	memcache_zbx_save_val (key, buf, len, 0);
	free (buf);
#else
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemValues);
	int fd;
	item_value_dbl_t val;

	if (!name)
		return;

	/* open file for writing */
	fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_values_dbl: open(): %s: %s", name, strerror(errno));
		free (name);
		return;
	}
	free (name);

	val.lastclock = lastclock;
	val.kind = 0;
	val.prevvalue = prevvalue;
	val.lastvalue = lastvalue;
	val.prevorgvalue = prevorgvalue;

	aio_item_value (fd, &val, sizeof (val), stderr);
#endif
}



void HFS_update_item_values_int (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid,
				 hfs_time_t lastclock, zbx_uint64_t prevvalue, zbx_uint64_t lastvalue, 
				 zbx_uint64_t prevorgvalue, const char* stderr)
{
#ifdef HAVE_MEMCACHE	
	const char* key;
	char* buf;
	int len;
	item_value_int_t val;

	key = memcache_get_key (MKT_LAST_UINT64, itemid);
	len = sizeof (item_value_int_t) + str_buffer_length (stderr);
	buf = (char*)malloc (len);
	
	((item_value_int_t*)buf)->lastclock = lastclock;
	((item_value_int_t*)buf)->kind = 0;
	((item_value_int_t*)buf)->prevvalue = prevvalue;
	((item_value_int_t*)buf)->lastvalue = lastvalue;
	((item_value_int_t*)buf)->prevorgvalue = prevorgvalue;
	buffer_str (buf+sizeof (item_value_int_t), stderr, len-sizeof (item_value_int_t));

	memcache_zbx_save_val (key, buf, len, 0);
	free (buf);
#else
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemValues);
	int fd;
	item_value_int_t val;

	if (!name)
		return;

	/* open file for writing */
	fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_values_int: open(): %s: %s", name, strerror(errno));
		free (name);
		return;
	}
	free (name);

	val.lastclock = lastclock;
	val.kind = 1;
	val.prevvalue = prevvalue;
	val.lastvalue = lastvalue;
	val.prevorgvalue = prevorgvalue;

	aio_item_value (fd, &val, sizeof (val), stderr);
#endif
}



void HFS_update_item_values_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid,
				 hfs_time_t lastclock, const char* prevvalue, const char* lastvalue, 
				 const char* prevorgvalue, const char* stderr)
{
#ifdef HAVE_MEMCACHE	
	const char* key;
	int len, rest;
	char *buf, *p;

	key = memcache_get_key (MKT_LAST_STRING, itemid);

	/* make buffer */
	len = sizeof (hfs_time_t)*2 + str_buffer_length (prevvalue) + 
		str_buffer_length (lastvalue) + str_buffer_length (prevorgvalue) + str_buffer_length (stderr);

	buf = malloc (len);
	if (!buf)
		return;

	p = buf;
	rest = len;
	((hfs_time_t*)p)[0] = lastclock;
	p += sizeof (hfs_time_t);
	rest -= sizeof (hfs_time_t);

	p = buffer_str (p, prevvalue, rest);
	rest -= str_buffer_length (prevvalue);

	p = buffer_str (p, lastvalue, rest);
	rest -= str_buffer_length (lastvalue);

	p = buffer_str (p, prevorgvalue, rest);
	rest -= str_buffer_length (prevorgvalue);

	p = buffer_str (p, stderr, rest);
	memcache_zbx_save_val (key, buf, len, 0);
#else
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemValues);
	int fd, kind;

	if (!name)
		return;

	/* open file for writing */
	fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		/* it's normal situation when file not exists */
		zabbix_log(LOG_LEVEL_WARNING, "HFS_update_item_values_str: open(): %s: %s", name, strerror(errno));
		free (name);
		return;
	}
	free (name);

	/* lock obtained, write data */
	if (write (fd, &lastclock, sizeof (lastclock)) == -1) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_str: write(): %s", strerror(errno));
	
	kind = 2;
	if (write (fd, &kind, sizeof (kind)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_values_str: write(): %s", strerror(errno));

	write_str (fd, prevvalue);
	write_str (fd, lastvalue);
	write_str (fd, prevorgvalue);
	write_str (fd, stderr);

	close (fd);
#endif
}


void HFS_update_item_values_log (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t lastclock,
				 const char* prevvalue, const char* lastvalue,
				 hfs_time_t timestamp, const char* eventlog_source, int eventlog_severity,
				 const char* stderr)
{
#ifdef HAVE_MEMCACHE
	const char* key;
	hfs_log_last_t entry;
	tpl_node* tpl;
	char* buf;
	size_t len;

	key = memcache_get_key (MKT_LAST_LOG, itemid);

	/* fill structure */
	entry.clock = lastclock;
	entry.prev = prevvalue;
	entry.last = lastvalue;
	entry.timestamp = timestamp;
	entry.source = eventlog_source;
	entry.severity = eventlog_severity;
	entry.stderr = stderr;

	tpl = tpl_map (TPL_HFS_LOG_LAST, &entry);
	tpl_pack (tpl, 0);
	if (!tpl_dump (tpl, TPL_MEM, &buf, &len)) {
		/* save in memcache */
		memcache_zbx_save_val (key, buf, len, 0);
		free (buf);
	}

	tpl_free (tpl);
#endif
}



int HFS_get_item_values_dbl (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t* lastclock,
			     double* prevvalue, double* lastvalue, double* prevorgvalue,
			     char** stderr)
{
#ifdef HAVE_MEMCACHE
	const char* key;
	char* buf, *p;
	size_t len;
	item_value_dbl_t val;

	key = memcache_get_key (MKT_LAST_DOUBLE, itemid);
	if (!key)
		return 0;

	buf = memcache_zbx_read_val (siteid, key, &len);
	if (!buf)
		return 0;

	if (len < sizeof (item_value_dbl_t)) {
		free (buf);
		return 0;
	}

	*lastclock = ((item_value_dbl_t*)buf)->lastclock;
	*prevvalue = ((item_value_dbl_t*)buf)->prevvalue;
	*lastvalue = ((item_value_dbl_t*)buf)->lastvalue;
	*prevorgvalue = ((item_value_dbl_t*)buf)->prevorgvalue;

	p = buf + sizeof (item_value_dbl_t);
	*stderr = unbuffer_str (&p);
	free (buf);
	return 1;
#else
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemValues);
	int fd;
	item_value_dbl_t val;

	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);
	if (fd < 0) {
		/* it's normal situation when file not exists */
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_values_dbl: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	/* reading data */
	if (read (fd, &val, sizeof (val)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_dbl: read(): %s", strerror(errno));
	
	if (val.kind != 0) {
		close (fd);
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_dbl error. Incorrect type (%d)", val.kind);
		return 0;
	}

	*lastclock = val.lastclock;
	*prevvalue = val.prevvalue;
	*lastvalue = val.lastvalue;
	*prevorgvalue = val.prevorgvalue;
	*stderr = read_str (fd);

	close (fd);
	return 1;
#endif
}


int HFS_get_item_values_int (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t* lastclock,
			     zbx_uint64_t* prevvalue, zbx_uint64_t* lastvalue, zbx_uint64_t* prevorgvalue,
			     char** stderr)
{
#ifdef HAVE_MEMCACHE
	const char* key;
	char* buf, *p;
	size_t len;
	item_value_int_t val;

	key = memcache_get_key (MKT_LAST_UINT64, itemid);
	if (!key)
		return 0;

	buf = memcache_zbx_read_val (siteid, key, &len);
	if (!buf)
		return 0;

	if (len < sizeof (item_value_int_t)) {
		free (buf);
		return 0;
	}

	*lastclock = ((item_value_int_t*)buf)->lastclock;
	*prevvalue = ((item_value_int_t*)buf)->prevvalue;
	*lastvalue = ((item_value_int_t*)buf)->lastvalue;
	*prevorgvalue = ((item_value_int_t*)buf)->prevorgvalue;
	p = buf + sizeof (item_value_int_t);
	*stderr = unbuffer_str (&p);
	free (buf);
	return 1;
#else
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemValues);
	int fd;
	item_value_int_t val;

	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);
	if (fd < 0) {
		/* it's normal situation when file not exists */
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_values_int: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	/* reading data */
	if (read (fd, &val, sizeof (val)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_dbl: read(): %s", strerror(errno));
	
	if (val.kind != 1) {
		close (fd);
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_values_int error. Incorrect type (%d)", val.kind);
		return 0;
	}

	*lastclock = val.lastclock;
	*prevvalue = val.prevvalue;
	*lastvalue = val.lastvalue;
	*prevorgvalue = val.prevorgvalue;
	*stderr = read_str (fd);
	close (fd);

	return 1;
#endif
}



int HFS_get_item_values_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid,
			     hfs_time_t* lastclock, char** prevvalue, char** lastvalue, 
			     char** prevorgvalue, char** stderr)
{
#ifdef HAVE_MEMCACHE
	const char* key;
	char *buf, *p;
	size_t len;

	key = memcache_get_key (MKT_LAST_STRING, itemid);
	if (!key)
		return 0;

	buf = memcache_zbx_read_val (siteid, key, &len);
	if (!buf)
		return 0;

	p = buf;
	*lastclock = ((hfs_time_t*)p)[0];
	p += sizeof (hfs_time_t);
	
	*prevvalue = unbuffer_str (&p);
	*lastvalue = unbuffer_str (&p);
	*prevorgvalue = unbuffer_str (&p);

	if (!*prevvalue)
		*prevvalue = strdup ("");
	if (!*lastvalue)
		*lastvalue = strdup ("");
	if (!*prevorgvalue)
		*prevorgvalue = strdup ("");

	*stderr = unbuffer_str (&p);
	free (buf);
	return 1;
#else
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemValues);
	int fd, kind;

	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);
	if (fd < 0) {
		/* it's normal situation when file not exists */
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_values_str: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	/* reading data */
	if (read (fd, lastclock, sizeof (*lastclock)) == -1 || read (fd, &kind, sizeof (kind)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_values_str: read(): %s", strerror(errno));
	
	if (kind != 2) {
		close (fd);
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_values_str error. Incorrect type (%d)", kind);	
		return 0;
	}

	*prevvalue = read_str (fd);
	*lastvalue = read_str (fd);
	*prevorgvalue = read_str (fd);
	*stderr = read_str (fd);
	close (fd);

	return 1;
#endif
}



int HFS_get_item_values_log (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t* lastclock,
			     char** prevvalue, char** lastvalue, hfs_time_t* timestamp, 
			     char** eventlog_source, int* eventlog_severity, char** stderr)
{
#ifdef HAVE_MEMCACHE
	const char* key;
	size_t len;
	char* buf;
	hfs_log_last_t entry;
	tpl_node* tpl;

	key = memcache_get_key (MKT_LAST_LOG, itemid);

	buf = memcache_zbx_read_val (siteid, key, &len);
	if (!buf)
		return 0;

	tpl = tpl_map (TPL_HFS_LOG_LAST, &entry);
	
	if (!tpl) {
		free (buf);
		return 0;
	}

	tpl_load (tpl, TPL_MEM, buf, len);

	if (tpl_unpack (tpl, 0) <= 0) {
		free (buf);
		tpl_free (tpl);
		return 0;
	}

	/* assign structure values */
	*lastclock = entry.clock;
	*prevvalue = entry.prev;
	*lastvalue = entry.last;
	*timestamp = entry.timestamp;
	*eventlog_source = entry.source;
	*eventlog_severity = entry.severity;
	*stderr = entry.stderr;

	tpl_free (tpl);
	free (buf);
	return 1;
#else
	return 0;
#endif
}



void HFS_update_item_status (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int status, const char* error)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemStatus);
	int fd;

	if (!name)
		return;

	/* open file for writing */
	/* S_IWGRP is not a mistake! We need this file writeable by web server. */
	fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

	if (fd < 0) {
		/* it's normal situation when file not exists */
		zabbix_log(LOG_LEVEL_WARNING, "HFS_update_item_status: open(): %s: %s", name, strerror(errno));
		free (name);
		return;
	}
	free (name);

	/* lock obtained, write data */
	if (write (fd, &status, sizeof (status)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_item_status: write(): %s", strerror(errno));

	write_str (fd, error);

	close (fd);
}


int HFS_get_item_status (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int* status, char** error)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemStatus);
	int fd;

	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);
	if (fd < 0) {
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_item_status: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	/* reading data */
	if (read (fd, status, sizeof (*status)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_item_status: read(): %s", strerror(errno));
	*error = read_str (fd);

	close (fd);
	return 1;
}



/* routine appends string to item's  string table */
void HFSadd_history_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t clock, const char* value)
{
	int len = 0, buf_len, fd;
	char* p_name = get_name (hfs_base_dir, siteid, itemid, NK_ItemString);
	char *buf, *p;

	if (value)
		len = strlen (value);

	if ((fd = xopen (p_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
		zabbix_log (LOG_LEVEL_DEBUG, "Canot open file %s", p_name);
		free (p_name);
		return;
	}

	lseek (fd, 0, SEEK_END);
	free (p_name);

	buf_len = sizeof (clock) + str_buffer_length (value) + sizeof (len);
	buf = (char*)malloc (buf_len);

	*(hfs_time_t*)buf = clock;
	p = buffer_str (buf+sizeof (clock), value, buf_len - sizeof (clock));
	if (!p) {
		free (buf);
		close (fd);
		return;
	}

	write (fd, buf, buf_len);
	free (buf);
	close (fd);
	return;
}



void HFSadd_history_log (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t clock, const char* value, 
			 hfs_time_t timestamp, char* eventlog_source, int eventlog_severity)
{
	char* p_name = get_name (hfs_base_dir, siteid, itemid, NK_ItemLog);
	int fd, res;
	hfs_log_entry_t entry;
	tpl_node* tpl;
	hfs_off_t ofs;
	hfs_log_dir_t dir_entry;

	if ((fd = xopen (p_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
		zabbix_log (LOG_LEVEL_DEBUG, "Canot open file %s", p_name);
		free (p_name);
		return;
	}

	free (p_name);
	ofs = lseek (fd, 0, SEEK_END);

	entry.clock = clock;
	entry.entry = value;
	entry.timestamp = timestamp;
	entry.source = eventlog_source;
	entry.severity = eventlog_severity;

	/* pack structure using TPL */
	tpl = tpl_map (TPL_HFS_LOG_ENTRY, &entry);
	tpl_pack (tpl, 0);
	res = tpl_dump (tpl, TPL_FD, fd);
	tpl_free (tpl);
	close (fd);

	if (res < 0)
		return;

	/* write offset of entry in directory */
	p_name = get_name (hfs_base_dir, siteid, itemid, NK_ItemLogDir);

	if ((fd = xopen (p_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
		zabbix_log (LOG_LEVEL_DEBUG, "Canot open file %s", p_name);
		free (p_name);
		return;
	}

	free (p_name);
	lseek (fd, 0, SEEK_END);
	dir_entry.clock = clock;
	dir_entry.ofs = ofs;

	write (fd, &dir_entry, sizeof (dir_entry));
	close (fd);
}



/* Read all values inside given time region. */
size_t HFSread_item_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, hfs_time_t to, hfs_item_str_value_t **result)
{
	int len = 0, fd, eof = 0;
	hfs_time_t clock;
	char* p_name = get_name (hfs_base_dir, siteid, itemid, NK_ItemString);
	size_t count = 0;
	hfs_item_str_value_t* tmp;
	struct __attribute__ ((packed)) {
		int len;
		hfs_time_t clock;
		int len2;
	} tmp_str;

	*result = NULL;

	if ((fd = open (p_name, O_RDONLY)) == -1) {
		zabbix_log (LOG_LEVEL_DEBUG, "HFSread_item_str: Canot open file %s", p_name);
		free (p_name);
		return 0;
	}

	free (p_name);

	/* search for start position */
	while (1) {
		if (read (fd, &clock, sizeof (clock)) != sizeof (clock)) {
			eof = 1;
			break;
		}

		/* if we run out of data, exit */
		if (clock > to) {
			eof = 1;
			break;
		}

		/* skip string value */
		if (read (fd, &len, sizeof (len)) != sizeof (len)) {
			eof = 1;
			break;
		}

		/* we find it */
		if (clock >= from)
			break;

		if (lseek (fd, len + (len ? 1 : 0) + sizeof (len), SEEK_CUR) == (off_t)-1) {
			eof = 1;
			break;
		}
	}

	if (!eof) {
		/* read values */
		while (1) {
			count++;
			tmp = (hfs_item_str_value_t*)realloc (*result, count * sizeof (hfs_item_str_value_t));

			/* we have no memory for this, return as is */
			if (!tmp) {
				count--;
				break;
			}

			*result = tmp;
			tmp[count-1].clock = clock;
			if (len) {
				tmp[count-1].value = (char*)malloc (len+1);

				if (!tmp[count-1].value)
					break;
				if (read (fd, tmp[count-1].value, len+1) != len+1) {
					free (tmp[count-1].value);
					count--;
					break;
				}
			}
			else
				tmp[count-1].value = NULL;

			/* read metadata of next string */
			if (read (fd, &tmp_str, sizeof (tmp_str)) != sizeof (tmp_str))
				break;

			clock = tmp_str.clock;
			len = tmp_str.len2;
			if (clock > to)
				break;
		}
	}

	close (fd);

	return count;
}


/* Read specified amount of latest entries. */
size_t HFSread_count_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int count, hfs_item_str_value_t **result)
{
	int len = 0, fd;
	hfs_time_t clock;
	char* p_name = get_name (hfs_base_dir, siteid, itemid, NK_ItemString);
	size_t res_count = 0;
	hfs_item_str_value_t* tmp;
	hfs_off_t ofs;

	*result = NULL;

	if ((fd = open (p_name, O_RDONLY)) == -1) {
		zabbix_log (LOG_LEVEL_DEBUG, "HFSread_item_str: Canot open file %s", p_name);
		free (p_name);
		return 0;
	}

	free (p_name);

	/* data is empty */
	ofs = lseek (fd, 0, SEEK_END);
	if (ofs == -1)
		return 0;
	ofs = lseek (fd, ofs-sizeof (len), SEEK_SET);
	if (ofs == -1)
		return 0;

	while (res_count < count) {
		if (read (fd, &len, sizeof (len)) != sizeof (len))
			break;

		res_count++;
		tmp = (hfs_item_str_value_t*)realloc (*result, res_count * sizeof (hfs_item_str_value_t));

		/* we have no memory for this, return as is */
		if (!tmp) {
			res_count--;
			break;
		}

		*result = tmp;

		if (!len) {
			/* empty string - special case */
			if (lseek (fd, ofs - (sizeof (len) + sizeof (clock)), SEEK_SET) == -1) {
				res_count--;
				break;
			}

			tmp[res_count-1].value = NULL;
			read (fd, &tmp[res_count-1].clock, sizeof (tmp[res_count-1].clock));
			ofs = lseek (fd, ofs - (sizeof (len)*2 + sizeof (clock)), SEEK_SET);
		}
		else {
			if (lseek (fd, ofs - (sizeof (len) + sizeof (clock) + len + 1), SEEK_SET) == -1) {
				res_count--;
				break;
			}

			tmp[res_count-1].value = (char*)malloc (len + 1);
			if (!tmp[res_count-1].value) {
				res_count--;
				break;
			}

			read (fd, &tmp[res_count-1].clock, sizeof (tmp[res_count-1].clock));
			read (fd, &len, sizeof (len));
			read (fd, tmp[res_count-1].value, len + 1);
			ofs = lseek (fd, ofs - (sizeof (len)*2 + sizeof (clock) + len + 1), SEEK_SET);
			if (ofs == -1)
				break;
		}
	}

/* 	release_lock (fd, 0); */
	close (fd);

	return res_count;
}

void HFS_update_trigger_value(const char* hfs_path, const char* siteid, zbx_uint64_t triggerid, int new_value, hfs_time_t now)
{
	char* name = get_name (hfs_path, siteid, triggerid, NK_TriggerStatus);
	int fd;
	trigger_value_t val = { new_value, now };
	const char* key;
	size_t len;

	if (!name)
		return;

	/* open file for writing */
	fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_trigger_value: open(): %s: %s", name, strerror(errno));
		free (name);
		return;
	}
	free (name);

	/* seek at trigger's ID */
	if (lseek (fd, triggerid*sizeof (val), SEEK_SET) < 0) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_trigger_value: lseek(): %s", strerror(errno));
	} else {
		if (write (fd, &val, sizeof (val)) < 0)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_update_trigger_value: lseek(): %s", strerror(errno));
	}

	close (fd);

#ifdef HAVE_MEMCACHE
	key = memcache_get_key (MKT_TRIGGER, triggerid);
	len = sizeof (val);
	memcache_zbx_save_val (key, &val, len, 0);
#endif
	return;
}



int HFS_get_triggers_values (const char* hfs_path, const char* siteid, hfs_trigger_value_t** res)
{
	char* name = get_name (hfs_path, siteid, 0, NK_TriggerStatus);
	int fd;
	int buf_s, buf_c, count = 0, buf_size = 0, i;
	trigger_value_t* buf;
	zbx_uint64_t triggerid = 0;

	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);
	if (fd < 0) {
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_trigger_value: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	buf_s = 1024;
	buf = (trigger_value_t*)malloc (buf_s * sizeof (trigger_value_t));
	*res = NULL;

	while ((buf_c = read (fd, buf, buf_s * sizeof (trigger_value_t))) > 0) {
		buf_c /= sizeof (trigger_value_t);
		for (i = 0; i < buf_c; i++) {
			triggerid++;
			if (!buf[i].when)
				continue;
			if (count == buf_size)
				*res = (hfs_trigger_value_t*)realloc (*res, (buf_size += 256) * sizeof (hfs_trigger_value_t));
			(*res)[count].when = buf[i].when;
			(*res)[count].triggerid = triggerid-1;
			(*res)[count].value = buf[i].value;
			count++;
		}
	}

	close (fd);
	return count;
}



int HFS_get_trigger_value (const char* hfs_path, const char* siteid, zbx_uint64_t triggerid, hfs_trigger_value_t* res)
{
	char* name;
	int fd;
	trigger_value_t val;

	/* trying to fetch value from cache first */
#ifdef HAVE_MEMCACHE
	int cached = 0;
	const char* key;
	char* buf;
	size_t len;

	/* trying to obtain value from memcache */
	key = memcache_get_key (MKT_TRIGGER, triggerid);

	if (key) {
		buf = memcache_zbx_read_val (siteid, key, &len);

		if (buf) {
			if (len == sizeof (hfs_trigger_value_t)) {
				memcpy (res, buf, len);
				cached = 1;
			}
			free (buf);
		}
	}

	if (cached && res->when)
		return 1;
#endif

	name = get_name (hfs_path, siteid, 0, NK_TriggerStatus);

	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);
	if (fd < 0) {
		zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_trigger_value: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	if (lseek (fd, sizeof (trigger_value_t) * triggerid, SEEK_SET) < 0) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_trigger_value: lseek(): %s", strerror (errno));
		close (fd);
		return 0;
	}

        if (read (fd, &val, sizeof (val)) < sizeof (val)) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_trigger_value: read(): %s", strerror (errno));
		close (fd);
		return 0;
        }

        res->triggerid = triggerid;
        res->when = val.when;
        res->value = val.value;

	if (!res->when)
		res->value = TRIGGER_VALUE_UNKNOWN;

	close (fd);

#ifdef HAVE_MEMCACHE
#ifndef ZABBIX_PHP_MODULE
	/* save read value to memcache for future hits */
	/* we already have initialized key, so just use it */
	len = sizeof (*res);
	memcache_zbx_save_val (key, res, len, 0);
#endif
#endif
	return 1;
}



void HFS_add_alert(const char* hfs_path, const char* siteid, hfs_time_t clock, zbx_uint64_t userid, zbx_uint64_t triggerid, zbx_uint64_t actionid, 
		   zbx_uint64_t mediatypeid, int status, int retries, char *sendto, char *subject, char *message)
{
	int len = 0, fd;
	char* p_name = get_name (hfs_path, siteid, 0, NK_Alert);
	hfs_alert_value_t alert;

	if ((fd = xopen (p_name, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
		zabbix_log (LOG_LEVEL_ERR, "HFS_add_alert: Canot open file %s", p_name);
		free (p_name);
		return;
	}

	free (p_name);

	alert.clock = clock;
	alert.userid = userid;
	alert.triggerid = triggerid;
	alert.mediatypeid = mediatypeid;
	alert.actionid = actionid;
	alert.userid = userid;
	alert.status = status;
	alert.retries = retries;

	write (fd, &alert, sizeof (alert) - 3*sizeof (char*));
	write_str (fd, sendto);
	write_str (fd, subject);
	write_str (fd, message);

	len = sizeof (alert) - 3*sizeof (char*) + str_buffer_length (sendto) + 
		str_buffer_length (subject) + str_buffer_length (message);

	/* write len twice for backward reading */
	write (fd, &len, sizeof (len));

	close (fd);
	return;
}


int HFS_get_alerts (const char* hfs_path, const char* siteid, int skip, int count, hfs_alert_value_t** alerts)
{
	char* f_name = get_name (hfs_path, siteid, 0, NK_Alert);
	int fd, len, res = 0;
	hfs_off_t ofs;

	if ((fd = open (f_name, O_RDONLY)) == -1) {
		zabbix_log (LOG_LEVEL_ERR, "HFS_get_alerts: Canot open file %s", f_name);
		free (f_name);
		return 0;
	}

	free (f_name);

	ofs = lseek (fd, 0, SEEK_END);

	if (ofs == (hfs_off_t)-1) {
		close (fd);
		return 0;
	}

	while (skip > 0) {
		ofs = lseek (fd, ofs - sizeof (len), SEEK_SET);
		if ((ofs == (hfs_off_t)-1) || (read (fd, &len, sizeof (len)) != sizeof (len))) {
			close (fd);
			return 0;
		}

		ofs = lseek (fd, ofs-len, SEEK_SET);
		if (ofs == (hfs_off_t)-1) {
			close (fd);
			return -1;
		}

		skip--;
	}

	/* allocate memory for result */
	*alerts = (hfs_alert_value_t*)malloc (sizeof (hfs_alert_value_t) * count);

	if (*alerts == NULL) {
		close (fd);
		return 0;
	}

	while (count > 0) {
		ofs = lseek (fd, ofs - sizeof (len), SEEK_SET);
		if ((ofs == (hfs_off_t)-1) || (read (fd, &len, sizeof (len)) != sizeof (len))) {
			close (fd);
			return res;
		}

		ofs = lseek (fd, ofs - len, SEEK_SET);

		if (ofs == (hfs_off_t)-1) {
			close (fd);
			return res;
		}

		/* read alert data */
		read (fd, *alerts + res, sizeof (hfs_alert_value_t) - 3*sizeof (char*));
		(*alerts)[res].sendto = read_str (fd);
		(*alerts)[res].subject = read_str (fd);
		(*alerts)[res].message = read_str (fd);

		lseek (fd, ofs, SEEK_SET);

		res++;
		count--;
	}

	close (fd);
	return res;
}


void HFS_add_event (const char* hfs_path, const char* siteid, zbx_uint64_t eventid, zbx_uint64_t triggerid, 
		    hfs_time_t clock, int val, int ack, zbx_uint64_t hostid)
{
	char* f_name;
	int fd;
	hfs_event_value_t value;

	/* prepare event structure */
	value.eventid 	= eventid;
	value.triggerid = triggerid;
	value.clock 	= clock;
	value.val 	= val;
	value.ack 	= ack;

	/* events storeed in two places:
	   1. per-trigger file to be able get all trigger's events at once,
	   2. per-host file
	 */
	f_name = get_name (hfs_path, siteid, triggerid, NK_EventTrigger);

	if (!f_name)
		return;

	if ((fd = xopen (f_name, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
		zabbix_log (LOG_LEVEL_ERR, "HFS_add_event: Canot open file %s", f_name);
		free (f_name);
		return;
	}

	free (f_name);

	write (fd, &value, sizeof (value));
	close (fd);

	/* per-host file */
	f_name = get_name (hfs_path, siteid, hostid, NK_EventHost);

	if (!f_name)
		return;

	if ((fd = xopen (f_name, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
		zabbix_log (LOG_LEVEL_ERR, "HFS_add_event: Canot open file %s", f_name);
		free (f_name);
		return;
	}

	free (f_name);

	write (fd, &value, sizeof (value));
	close (fd);
}


int HFS_get_trigger_events (const char* hfs_path, const char* siteid, zbx_uint64_t triggerid, int count, hfs_event_value_t** res)
{
	char* f_name = get_name (hfs_path, siteid, triggerid, NK_EventTrigger);
	struct stat st;
	int total, fd;
	hfs_off_t ofs, res_count;

	if (!f_name)
		return 0;

	if (stat (f_name, &st)) {
		free (f_name);
		return 0;
	}

	total = st.st_size / sizeof (hfs_event_value_t);
	if (count == 0)
		count = total;
	if (total < count)
		count = total;
	ofs = (total-count)*sizeof (hfs_event_value_t);
	fd = open (f_name, O_RDONLY);

	if (fd < 0) {
		zabbix_log (LOG_LEVEL_ERR, "HFS_get_trigger_events: Canot open file %s", f_name);
		free (f_name);
		return 0;
	}
	free (f_name);

	lseek (fd, ofs, SEEK_SET);

	*res = (hfs_event_value_t*)malloc (sizeof (hfs_event_value_t) * count);

	if (!*res) {
		zabbix_log (LOG_LEVEL_ERR, "HFS_get_trigger_events: Memory allocation error");
		close (fd);
		return 0;
	}

	res_count = read (fd, *res, count * sizeof (hfs_event_value_t)) / sizeof (hfs_event_value_t);
	close (fd);

	return res_count;
}



int HFS_get_host_events (const char* hfs_path, const char* siteid, zbx_uint64_t hostid, int skip, int count, hfs_event_value_t** res)
{
	char* f_name = get_name (hfs_path, siteid, hostid, NK_EventHost);
	struct stat st;
	int total, fd;
	hfs_off_t ofs, res_count;

	if (!f_name)
		return 0;

	if (stat (f_name, &st)) {
		free (f_name);
		return 0;
	}

	total = st.st_size / sizeof (hfs_event_value_t);
	if (total-skip < count)
		count = total-skip;
	ofs = (total-skip-count)*sizeof (hfs_event_value_t);
	fd = open (f_name, O_RDONLY);

	if (fd < 0) {
		zabbix_log (LOG_LEVEL_ERR, "HFS_get_host_events: Canot open file %s", f_name);
		free (f_name);
		return 0;
	}
	free (f_name);

	lseek (fd, ofs, SEEK_SET);

	*res = (hfs_event_value_t*)malloc (sizeof (hfs_event_value_t) * count);

	if (!*res) {
		zabbix_log (LOG_LEVEL_ERR, "HFS_get_host_events: Memory allocation error");
		close (fd);
		return 0;
	}

	res_count = read (fd, *res, count * sizeof (hfs_event_value_t)) / sizeof (hfs_event_value_t);
	close (fd);

	return res_count;
}


void HFS_clear_item_history (const char* hfs_path, const char* siteid, zbx_uint64_t itemid)
{
	name_kind_t kinds[] = {
		NK_ItemMeta, NK_ItemData, NK_TrendItemMeta, NK_TrendItemData, NK_ItemValues
	};
	char* f_name;
	int i;

	for (i = 0; i < sizeof (kinds) / sizeof (kinds[0]); i++) {
		f_name = get_name (hfs_path, siteid, itemid, kinds[i]);
		unlink (f_name);
		free (f_name);
	}

#ifdef HAVE_MEMCACHE
	/* clear meta from memcache */
	f_name = get_name (hfs_path, siteid, itemid, NK_ItemMeta);
	memcache_zbx_del_val (siteid, f_name);
	free (f_name);
#endif
}



/* optimized routines which get latest values */
double		HFS_get_item_last_dbl (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid)
{
#ifdef HAVE_MEMCACHE
	hfs_time_t lastclock;
	double prev, last, prevorg, res = 0.0;
	char* stderr = NULL;
	if (HFS_get_item_values_dbl (hfs_base_dir, siteid, itemid, &lastclock, &prev, &last, &prevorg, &stderr))
		res = last;
	if (stderr)
		free (stderr);
	return res;
#else
    char *p_data;
    int fd;
    hfs_off_t ofs;
    double res;

    p_data = get_name (hfs_base_dir, siteid, itemid, NK_ItemData);
    if ((fd = open (p_data, O_RDONLY)) == -1) {
	free (p_data);
	return 0.0;
    }
    free (p_data);

    ofs = lseek (fd, 0, SEEK_END);
    ofs -= sizeof (res);
    lseek (fd, ofs, SEEK_SET);

    if (read (fd, &res, sizeof (res)) != sizeof (res))
	    res = 0.0;
    close (fd);

    return res;
#endif
}


zbx_uint64_t	HFS_get_item_last_int (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid)
{
#ifdef HAVE_MEMCACHE
	hfs_time_t lastclock;
	zbx_uint64_t prev, last, prevorg, res = 0;
	char* stderr = NULL;
	if (HFS_get_item_values_int (hfs_base_dir, siteid, itemid, &lastclock, &prev, &last, &prevorg, &stderr))
		res = last;
	if (stderr)
		free (stderr);
	return res;
#else
    char *p_data;
    int fd;
    hfs_off_t ofs;
    zbx_uint64_t res;

    p_data = get_name (hfs_base_dir, siteid, itemid, NK_ItemData);
    if ((fd = open (p_data, O_RDONLY)) == -1) {
	free (p_data);
	return 0;
    }
    free (p_data);

    ofs = lseek (fd, 0, SEEK_END);
    ofs -= sizeof (res);
    lseek (fd, ofs, SEEK_SET);

    if (read (fd, &res, sizeof (res)) != sizeof (res))
	    res = 0;
    close (fd);

    return res;
#endif
}


/* -------------------------------------------------- */
/* Function values */
/* -------------------------------------------------- */

/* trying to convert string value to apropriate value */
int HFS_convert_function_str2val (const char* value, hfs_function_value_t* result)
{
	int dig = 0, dots = 0, sign = 0, space = 0, other = 0;
	const char* p = value;

	/* empty value */
	if (!value || !value[0]) {
		result->type = FVT_NULL;
		result->value.d = 0;
		return 1;
	}

	while (*p) {
		if (isspace (*p))
			space++;
		else if (isdigit (*p))
			dig++;
		else if (*p == '.')
			dots++;
		else if (*p == '+' || *p == '-')
			sign++;
		else
			other++;
		p++;
	}

	/* this is an integer value */
	if (!dots && !sign && !other) {
		result->type = FVT_UINT64;
		ZBX_STR2UINT64 (result->value.l, value);
		return 1;
	}

	if (!other) {
		result->type = FVT_DOUBLE;
		result->value.d = atof (value);
		return 1;
	}

	return 0;
}



char* HFS_convert_function_val2str (hfs_function_value_t* result)
{
	static char buf[256];

	switch (result->type) {
	case FVT_NULL:
		return strdup ("");
	case FVT_UINT64:
		snprintf (buf, sizeof (buf), ZBX_FS_UI64, result->value.l);
		return strdup (buf);
	case FVT_DOUBLE:
		snprintf (buf, sizeof (buf), "%lf", result->value.d);
		return strdup (buf);
	default:
		return NULL;
	}
}



static int global_fd_function_open (const char* hfs_path, const char* siteid)
{
	char *name = get_name (hfs_path, siteid, 0, NK_FunctionVals);

	if ((function_val_fd = xopen (name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
		zabbix_log (LOG_LEVEL_ERR, "global_fd_function_open: Cannot open function value file");
		free (name);
		return 0;
	}
	free (name);

	return 1;
}



int HFS_save_function_value (const char* hfs_path, const char* siteid, zbx_uint64_t functionid, hfs_function_value_t* value)
{
#ifdef HAVE_MEMCACHE
	const char* key = memcache_get_key (MKT_FUNCTION, functionid);

	memcache_zbx_save_val (key, value, sizeof (hfs_function_value_t), 0);
#endif

	if (function_val_fd == -1)
		if (!global_fd_function_open (hfs_path, siteid))
			return 0;

	if (lseek (function_val_fd, sizeof (hfs_function_value_t) * functionid, SEEK_SET) == (off_t)-1) {
		zabbix_log (LOG_LEVEL_ERR, "HFS_save_function_value: Cannot seek to %lld offset", sizeof (hfs_function_value_t) * functionid);
		close (function_val_fd);
		function_val_fd = -1;
		return 0;
	}

	if (write (function_val_fd, value, sizeof (hfs_function_value_t)) != sizeof (hfs_function_value_t))
		zabbix_log (LOG_LEVEL_ERR, "HFS_save_function_value: Error updating value");

	return 1;
}


int HFS_get_function_value (const char* hfs_path, const char* siteid, zbx_uint64_t functionid, hfs_function_value_t* value)
{
#ifdef HAVE_MEMCACHE
	const char* key = memcache_get_key (MKT_FUNCTION, functionid);
	void* buf;
	size_t len;

	buf = memcache_zbx_read_val (siteid, key, &len);
	if (buf) {
		if (len == sizeof (hfs_function_value_t))
			memcpy (value, buf, sizeof (hfs_function_value_t));
		free (buf);
		return 1;
	}
#endif

	if (function_val_fd == -1)
		if (!global_fd_function_open (hfs_path, siteid))
			return 0;

	if (lseek (function_val_fd, sizeof (hfs_function_value_t) * functionid, SEEK_SET) == (off_t)-1) {
		value->type = FVT_NULL;
		close (function_val_fd);
		function_val_fd = -1;
		return 1;
	}

	if (read (function_val_fd, value, sizeof (hfs_function_value_t)) != sizeof (hfs_function_value_t))
		zabbix_log (LOG_LEVEL_ERR, "HFS_get_function_value: Error reading value for function %lld", functionid);

#ifdef HAVE_MEMCACHE
	memcache_zbx_save_val (key, value, sizeof (hfs_function_value_t), 0);
#endif
	return 1;
}


typedef struct __attribute__ ((packed)) {
	zbx_uint64_t ts;
	int valid;
	double value;
	int stderr_len;
} hfs_aggr_value_t;


void HFS_save_aggr_slave_value (const char* hfs_path, const char* siteid, zbx_uint64_t itemid, hfs_time_t ts, int valid, double value, const char* stderr)
{
	char *name = get_name (hfs_path, siteid, itemid, NK_AggrSlaveVal);
	int fd;
	hfs_aggr_value_t aggr;

	if ((fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
		zabbix_log (LOG_LEVEL_ERR, "HFS_save_function_value: Cannot open function value file");
		free (name);
		return;
	}
	free (name);

	aggr.ts = ts;
	aggr.valid = valid;
	aggr.value = value;
	aggr.stderr_len = stderr ? strlen (stderr) : 0;

	write (fd, &aggr, sizeof (aggr));
	if (aggr.stderr_len)
		write_str_wo_len (fd, stderr);
	close (fd);
}



void HFS_get_aggr_slave_value (const char* hfs_path, const char* siteid, zbx_uint64_t itemid, hfs_time_t* ts, int* valid, double* value, char** stderr)
{
	char *name = get_name (hfs_path, siteid, itemid, NK_AggrSlaveVal);
	int fd;
	hfs_aggr_value_t aggr;

	*ts = 0;
	*valid = 0;
	*stderr = NULL;

	if ((fd = open (name, O_RDONLY)) < 0) {
		free (name);
		return;
	}
	free (name);

	if (read (fd, &aggr, sizeof (aggr)) < sizeof (aggr)) {
		close (fd);
		return;
	}

	*ts = aggr.ts;
	*valid = aggr.valid;
	*value = aggr.value;
	if (aggr.stderr_len)
		*stderr = read_str_wo_len (fd, aggr.stderr_len);
	else
		*stderr = NULL;
	close (fd);
}
