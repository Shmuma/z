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
#include "hfs_internal.h"

#include <unistd.h>
#include <fcntl.h>

# ifndef ULLONG_MAX
#  define ULLONG_MAX    18446744073709551615ULL
# endif

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
  Routine adds double value to HistoryFS storage.
*/
void HFSadd_history (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, double value, hfs_time_t clock)
{
    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history()");
    store_value (hfs_base_dir, siteid, itemid, clock, delay, &value, sizeof (double), IT_DOUBLE);
}



/*
  Routine adds uint64 value to HistoryFS storage.
*/
void HFSadd_history_uint (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, zbx_uint64_t value, hfs_time_t clock)
{
    zabbix_log(LOG_LEVEL_DEBUG, "In HFSadd_history_uint()");   
    store_value (hfs_base_dir, siteid, itemid, clock, delay, &value, sizeof (zbx_uint64_t), IT_UINT64);
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
				zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: open: %s", fn, strerror(errno));
                }
		else
			return -1;
	}
	return retval;
}

int xclose(const char *fn, int fd)
{
	int rc;
	if ((rc = close(fd)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: close: %s", fn, strerror(errno));
	return rc;
}

ssize_t xwrite(const char *fn, int fd, const void *buf, size_t count)
{
	ssize_t rc;
	if ((rc = write(fd, buf, count)) == -1) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: write: %s", fn, strerror(errno));
		xclose(fn, fd);
	}
	return rc;
}

hfs_off_t xlseek(const char *fn, int fd, hfs_off_t offset, int whence)
{
	hfs_off_t rc;
	if ((rc = lseek(fd, offset, whence)) == -1) {
		zabbix_log(LOG_LEVEL_CRIT,
			"HFS: %s: lseek(fd, %lld (%d), %d): %s",
			fn, offset, whence, strerror(errno));
		xclose(fn, fd);
	}
	return rc;
}

int store_value (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t clock, int delay, void* value, int len, item_type_t type)
{
    char *p_meta = NULL, *p_data = NULL;
    int is_trend = is_trend_type (type);
    int res;

    p_meta = get_name (hfs_base_dir, siteid, itemid, is_trend ? NK_TrendItemMeta : NK_ItemMeta);
    p_data = get_name (hfs_base_dir, siteid, itemid, is_trend ? NK_TrendItemData : NK_ItemData);

    res = hfs_store_value (p_meta, p_data, clock, delay, value, len, type);
    free (p_meta);
    free (p_data);

    return res;
}

int hfs_store_value (const char* p_meta, const char* p_data, hfs_time_t clock, int delay, void* value, int len, item_type_t type)
{
    hfs_meta_item_t item, *ip;
    hfs_meta_t* meta;
    int fd;
    int i, j;
    unsigned char v = 0xff;
    hfs_off_t eextra, extra;
    hfs_off_t size;
    int retval = 1;
    hfs_trend_t trend;

    zabbix_log(LOG_LEVEL_DEBUG, "HFS: store_value()");

    if ((meta = read_metafile (p_meta)) == NULL) {
	zabbix_log(LOG_LEVEL_CRIT, "HFS: store_value(): read_metafile(%s) == NULL", p_meta);
	return retval;
    }

    zabbix_log(LOG_LEVEL_DEBUG, "HFS: meta read: delays: %d %d, blocks %d, ofs %u", meta->last_delay, delay, meta->blocks, meta->last_ofs);

    /* should we start a new block? */
    if (meta->blocks == 0 || meta->last_delay != delay || type != meta->last_type) {
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: appending new block for data %s", p_data);
	item.start = item.end = clock;
	item.type = type;

	/* analyze current value */
	item.delay = delay;
	if (meta->blocks) {
	    ip = meta->meta + (meta->blocks-1);
	    zabbix_log(LOG_LEVEL_DEBUG, "HFS: there is another block on way: %llu, %llu, %d, %llu", ip->start, ip->end, ip->delay, ip->ofs);
	    item.ofs = meta->last_ofs + len;
	}
	else
	    item.ofs = 0;

	/* append block to meta */
	if ((fd = xopen (p_meta, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
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
	fd = -1;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS: block metadata updated %d, %d, %d: %lld, %lld, %d, %llu", meta->blocks, meta->last_delay, meta->last_type,
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
	fd = -1;

	retval = 0;
    }
    else {
	zabbix_log(LOG_LEVEL_DEBUG, "HFS: extending existing block for data %s, last ofs %u", p_data, meta->last_ofs);

	/* check for gaps in block */
	ip = meta->meta + (meta->blocks-1);

	if ((fd = xopen (p_data, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
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

	/* if value appeared before it's time, drop it. Overwise fill gaps with empty entries and write actual data. */
	if (extra >= 1) {
            /* check for time-based items count differs from offset-based */
            eextra = (ip->end - ip->start) / delay - (meta->last_ofs - ip->ofs) / len;

            if (eextra > 0) {
                zabbix_log(LOG_LEVEL_DEBUG, "HFS: there is disagree of time-based items count with offset based. Perform alignment. Count=%d", eextra);
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
	    fd = -1;

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
	    fd = -1;
	}
	else
	    zabbix_log(LOG_LEVEL_DEBUG, "HFS: value appeared earlier than expected. Now %d, but next must be at %d", clock, ip->end + delay);

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
	/*
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
	*/
	return 1;
}


int release_lock (int fd, int write)
{
	/*
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
	*/
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
   Performs folding on values from specified time interval
 */
int HFSread_interval(const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, hfs_time_t to, void* init_res, read_count_fn_t fn);
{
    char *p_data;
    hfs_meta_t* meta;
    hfs_off_t ofs;
    int fd, block;
    item_value_u value;
    hfs_time_t ts;
    int count = 0;

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
		    fn (meta->meta[block].type, value, ts, init_res);
		    count++;
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
    int fd;
    zbx_uint64_t value;

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
	    while (read (fd, &value, sizeof (value)) > 0)
		    fn (&value, init_res);
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

    close (fd);
}

hfs_meta_t *read_metafile(const char *metafile)
{
	hfs_meta_t *res;
	FILE *f;

	if (!metafile)
		return NULL;

	if ((res = (hfs_meta_t *) malloc(sizeof(hfs_meta_t))) == NULL)
		return NULL;

	/* if we have no file, create empty one */
	if ((f = fopen(metafile, "rb")) == NULL) {
		zabbix_log(LOG_LEVEL_DEBUG, "%s: file open failed: %s",
			   metafile, strerror(errno));
		res->blocks = 0;
		res->last_delay = 0;
		res->last_type = IT_DOUBLE;
		res->last_ofs = 0;
		res->meta = NULL;
		return res;
	}

	if (fread(&res->blocks, sizeof(res->blocks), 1, f) != 1 ||
	    fread(&res->last_delay, sizeof(res->last_delay), 1, f) != 1 ||
	    fread(&res->last_type, sizeof(res->last_type), 1, f) != 1 ||
	    fread(&res->last_ofs, sizeof(res->last_ofs), 1, f) != 1)
	{
		fclose(f);
		free(res);
		return NULL;
	}

	res->meta = (hfs_meta_item_t *) malloc(sizeof(hfs_meta_item_t)*res->blocks);
        if (!res->meta) {
		fclose (f);
		free (res);
		return NULL;
	}

        if (fread (res->meta, sizeof (hfs_meta_item_t), res->blocks, f) != res->blocks) {
		fclose(f);
		free(res->meta);
		free(res);
		return NULL;
	}

	fclose (f);
	return res;
}

hfs_meta_t* read_meta (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int trend)
{
    char* path = get_name (hfs_base_dir, siteid, itemid, trend ? NK_TrendItemMeta : NK_ItemMeta);
    hfs_meta_t* res = read_metafile(path);
    free(path);

    return res;
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
    case NK_TrendItemData:
    case NK_TrendItemMeta:
            snprintf (res, len, "%s/%s/items/%llu/%llu/trends.%s", hfs_base_dir, siteid, item_ord, itemid,
		      kind == NK_TrendItemMeta ? "meta" : "data");
	    break;
    case NK_HostState:
	    snprintf (res, len, "%s/%s/hosts/%llu.state", hfs_base_dir, siteid, itemid);
	    break;
    case NK_ItemValues:
            snprintf (res, len, "%s/%s/items/%llu/%llu/values.data", hfs_base_dir, siteid, item_ord, itemid);
	    break;
    case NK_ItemStatus:
            snprintf (res, len, "%s/%s/items/%llu/%llu/status.data", hfs_base_dir, siteid, item_ord, itemid);
	    break;
    case NK_ItemStderr:
            snprintf (res, len, "%s/%s/items/%llu/%llu/stderr.data", hfs_base_dir, siteid, item_ord, itemid);
	    break;
    case NK_TriggerStatus:
	    snprintf (res, len, "%s/%s/triggers/%llu/status.data", hfs_base_dir, siteid, itemid);
	    break;
    case NK_Alert:
	    snprintf (res, len, "%s/%s/misc/alerts.data", hfs_base_dir, siteid);
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
	int ts = (int)from_ts;
	hfs_meta_t *meta = NULL;
	hfs_meta_item_t *ip = NULL;
	char *path;

#ifdef DEBUG_legion
	fprintf(stderr, "It HFS_find_meta(hfs_base_dir=%s, siteid=%s, itemid=%lld, from=%d, res)\n",
		hfs_base_dir, siteid, itemid, from_ts);
	fflush(stderr);
#endif
	(*res) = NULL;

	path = get_name (hfs_base_dir, siteid, itemid, trend ? NK_TrendItemMeta : NK_ItemMeta);
	free(path);

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

	if ((block = HFS_find_meta(hfs_base_dir, siteid, is_trend, itemid, ts, &meta)) == -1) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS: unable to get metafile");
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
		zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: file open failed: %s", p_data, strerror(errno));
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

	while (read (fd, val, val_len) > 0) {

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
			break;

		if (ts >= ip->end) {
			block++;

			if (block >= meta->blocks)
				break;

			ip = meta->meta + block;
		}

		count++;
	}

out:
	free_meta (meta);
	xfree(p_data);
	if (fd != -1)
		close(fd);

	if (result_size > items)
		*result = (hfs_item_value_t *) realloc(*result, (sizeof(hfs_item_value_t) * items));

	zabbix_log(LOG_LEVEL_DEBUG,
		"HFS: HFSread_item: Out items=%d\n", items);

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
		zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: file open failed: %s", p_data, strerror(errno));
		goto end; // error
	}

	if ((ofs = lseek(fd, meta->last_ofs, SEEK_SET)) == -1) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: unable to change file offset = %u: %s", p_data, meta->last_ofs, strerror(errno));
		goto end; // error
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

			if (is_valid_val(&val.l, sizeof(zbx_uint64_t))) {
				fn (ip->type, val, ts, init_res);
				count--;
			}

			ts = (ts - ip->delay);
		}

		if (count == 0)
			break;
	}
end:
	if (fd != -1 && close(fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS: %s: close(): %s", p_data, strerror(errno));

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

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_host_availability entered");

	if (!name)
		return;

	zabbix_log(LOG_LEVEL_DEBUG, "got name %s", name);

	/* open file for writing */
	fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

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
			     hfs_time_t lastclock, int nextcheck, double prevvalue, double lastvalue, double prevorgvalue)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemValues);
	int fd, kind;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_values_dbl entered");

	if (!name)
		return;

	zabbix_log(LOG_LEVEL_DEBUG, "got name %s", name);

	/* open file for writing */
	fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

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
				 hfs_time_t lastclock, int nextcheck, zbx_uint64_t prevvalue, zbx_uint64_t lastvalue, zbx_uint64_t prevorgvalue)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemValues);
	int fd, kind;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_values_int entered");

	if (!name)
		return;

	zabbix_log(LOG_LEVEL_DEBUG, "got name %s", name);

	/* open file for writing */
	fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

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
				 hfs_time_t lastclock, int nextcheck, const char* prevvalue, const char* lastvalue, const char* prevorgvalue)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemValues);
	int fd, kind;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_values_str entered");

	if (!name)
		return;

	zabbix_log(LOG_LEVEL_DEBUG, "got name %s", name);

	/* open file for writing */
	fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

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


int HFS_get_item_values_dbl (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t* lastclock,
			     int* nextcheck, double* prevvalue, double* lastvalue, double* prevorgvalue)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemValues);
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


int HFS_get_item_values_int (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t* lastclock,
			     int* nextcheck, zbx_uint64_t* prevvalue, zbx_uint64_t* lastvalue, zbx_uint64_t* prevorgvalue)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemValues);
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
			 hfs_time_t* lastclock, int* nextcheck, char** prevvalue, char** lastvalue, char** prevorgvalue)
{
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemValues);
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
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemStatus);
	int fd;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_status entered");

	if (!name)
		return;

	zabbix_log(LOG_LEVEL_DEBUG, "got name %s", name);

	/* open file for writing */
	fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

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
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemStderr);
	int fd;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_item_stderr entered");

	if (!name)
		return;

	zabbix_log(LOG_LEVEL_DEBUG, "got name %s", name);

	/* open file for writing */
	fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

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
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemStatus);
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
	char* name = get_name (hfs_base_dir, siteid, itemid, NK_ItemStderr);
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


/* routine appends string to item's  string table */
void HFSadd_history_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t clock, const char* value)
{
	zabbix_log (LOG_LEVEL_DEBUG, "In HFSadd_history_str()");
	store_value_str (hfs_base_dir, siteid, itemid, clock, value, IT_STRING);
}



int store_value_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t clock, const char* value, item_type_t type)
{
	int len = 0, fd;
	char* p_name = get_name (hfs_base_dir, siteid, itemid, NK_ItemString);

	if (value)
		len = strlen (value);

	if ((fd = xopen (p_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
		zabbix_log (LOG_LEVEL_DEBUG, "Canot open file %s", p_name);
		free (p_name);
		return 0;
	}

	if (!obtain_lock (fd, 1)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "hfs: store_value_str: close(): %s", strerror(errno));
		return 0;
	}

	lseek (fd, 0, SEEK_END);

	free (p_name);

	write (fd, &clock, sizeof (clock));
	write (fd, &len, sizeof (len));
	if (len)
		write (fd, value, len+1);
	/* write len twice for backward reading */
	write (fd, &len, sizeof (len));

	release_lock (fd, 1);

	close (fd);
	return 0;
}


/* Read all values inside given time region. */
size_t HFSread_item_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, hfs_time_t to, hfs_item_str_value_t **result)
{
	int len = 0, fd, eof = 0;
	hfs_time_t clock;
	char* p_name = get_name (hfs_base_dir, siteid, itemid, NK_ItemString);
	size_t count = 0;
	hfs_item_str_value_t* tmp;

	*result = NULL;

	if ((fd = open (p_name, O_RDONLY)) == -1) {
		zabbix_log (LOG_LEVEL_DEBUG, "HFSread_item_str: Canot open file %s", p_name);
		free (p_name);
		return 0;
	}

	if (!obtain_lock (fd, 0)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "hfs: HFSread_item_str: close(): %s", strerror(errno));
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

		if (lseek (fd, len + 1 + sizeof (len), SEEK_CUR) == (off_t)-1) {
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
			tmp[count-1].value = (char*)malloc (len+1);

			if (!tmp[count-1].value)
				break;

			if (read (fd, tmp[count-1].value, len+1) != len+1) {
				free (tmp[count-1].value);
				count--;
				break;
			}

			/* read metadata of next string */
			if (read (fd, &len, sizeof (len)) != sizeof (len) || 
			    read (fd, &clock, sizeof (clock)) != sizeof (clock) ||
			    read (fd, &len, sizeof (len)) != sizeof (len))
				break;

			if (clock > to)
				break;
		}
	}

	release_lock (fd, 0);
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

	if (!obtain_lock (fd, 0)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "hfs: HFSread_item_str: close(): %s", strerror(errno));
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

	release_lock (fd, 0);
	close (fd);

	return res_count;
}



void HFS_update_trigger_value(const char* hfs_path, const char* siteid, zbx_uint64_t triggerid, int new_value, hfs_time_t now)
{
	char* name = get_name (hfs_path, siteid, triggerid, NK_TriggerStatus);
	int fd;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_trigger_value entered");

	if (!name)
		return;

	/* open file for writing */
	fd = xopen (name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd < 0) {
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_trigger_value: open(): %s: %s", name, strerror(errno));
		free (name);
		return;
	}
	free (name);

	/* place write lock on that file or wait for unlock */
	if (!obtain_lock (fd, 1)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_update_trigger_value: close(): %s", strerror(errno));
		return;
	}

	/* lock obtained, write data */
	if (write (fd, &new_value, sizeof (new_value)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_trigger_value: write(): %s", strerror(errno));
	if (write (fd, &now, sizeof (now)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_trigger_value: write(): %s", strerror(errno));

	/* release lock */
	release_lock (fd, 1);

	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_update_trigger_value: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_update_trigger_value leave");
	return;
}



int HFS_get_trigger_value (const char* hfs_path, const char* siteid, zbx_uint64_t triggerid, int* value, hfs_time_t* when)
{
	char* name = get_name (hfs_path, siteid, triggerid, NK_TriggerStatus);
	int fd;

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_trigger_value entered");
	if (!name)
		return 0;

	/* open file for reading */
	fd = open (name, O_RDONLY);
	if (fd < 0) {
		if (errno != ENOENT)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_get_trigger_value: open(): %s: %s", name, strerror(errno));
		free (name);
		return 0;
	}
	free (name);

	/* obtain read lock */
	if (!obtain_lock (fd, 0)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "HFS_get_trigger_value: close(): %s", strerror(errno));
		return 0;
	}

	/* reading data */
	if (read (fd, value, sizeof (*value)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_trigger_value: read(): %s", strerror(errno));
	if (read (fd, when, sizeof (*when)) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_trigger_value: read(): %s", strerror(errno));

	/* release read lock */
	release_lock (fd, 0);

	if (close (fd) == -1)
		zabbix_log(LOG_LEVEL_CRIT, "HFS_get_trigger_value: close(): %s", strerror(errno));

	zabbix_log(LOG_LEVEL_DEBUG, "HFS_get_trigger_value leave");
	return 1;	
}



void HFS_add_alert(const char* hfs_path, const char* siteid, hfs_time_t clock, zbx_uint64_t actionid, zbx_uint64_t userid, 
		   zbx_uint64_t triggerid,  zbx_uint64_t mediatypeid, char *sendto, char *subject, char *message)
{
	int len = 0, fd;
	char* p_name = get_name (hfs_path, siteid, 0, NK_Alert);

	if ((fd = xopen (p_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
		zabbix_log (LOG_LEVEL_DEBUG, "Canot open file %s", p_name);
		free (p_name);
		return;
	}

	if (!obtain_lock (fd, 1)) {
		if (close (fd) == -1)
			zabbix_log(LOG_LEVEL_CRIT, "hfs: HFS_add_alert: close(): %s", strerror(errno));
		return;
	}

	lseek (fd, 0, SEEK_END);

	free (p_name);

	write (fd, &clock, sizeof (clock));
	write (fd, &actionid, sizeof (actionid));
	write (fd, &userid, sizeof (userid));
	write (fd, &triggerid, sizeof (triggerid));
	write (fd, &mediatypeid, sizeof (mediatypeid));
	write_str (fd, sendto);
	write_str (fd, subject);
	write_str (fd, message);

	len = sizeof (clock) + sizeof (actionid) + sizeof (userid) + sizeof (triggerid) + sizeof (mediatypeid) + 
		strlen (sendto) + 1 + strlen (subject) + 1 + strlen (subject) + 1;

	/* write len twice for backward reading */
	write (fd, &len, sizeof (len));

	release_lock (fd, 1);

	close (fd);
	return;
}
