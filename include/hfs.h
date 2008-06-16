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
#ifndef __HISTORY_FS__
#define __HISTORY_FS__

#include "common.h"

typedef enum {
    IT_UINT64 = 0,
    IT_DOUBLE,
} item_type_t;

typedef union {
	zbx_uint64_t	l;
	double		d;
} item_value_u;

#define alloc_item_values 1000
typedef struct hfs_item_value {
	item_type_t	type;
        time_t		clock;
	long		group;
        item_value_u 	max;
        item_value_u	min;
	item_value_u	avg;
} hfs_item_value_t;

typedef void (*read_count_fn_t) (item_type_t type, item_value_u val, time_t timestamp, void *res);

void		HFSadd_history (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, double value, int clock);
void		HFSadd_history_uint (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, zbx_uint64_t value, int clock);
size_t		HFSread_item (const char* hfs_base_dir, const char* siteid, size_t x, zbx_uint64_t itemid, time_t graph_from, time_t graph_to, time_t from, time_t to, hfs_item_value_t **result);
int		HFSread_count(const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int count, void* init_res, read_count_fn_t fn);


zbx_uint64_t	HFS_get_count (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from);
zbx_uint64_t	HFS_get_count_u64_eq (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_ne (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_gt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_lt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_ge (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_le (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, zbx_uint64_t value);

zbx_uint64_t	HFS_get_count_float_eq (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, double value);
zbx_uint64_t	HFS_get_count_float_ne (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, double value);
zbx_uint64_t	HFS_get_count_float_gt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, double value);
zbx_uint64_t	HFS_get_count_float_lt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, double value);
zbx_uint64_t	HFS_get_count_float_ge (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, double value);
zbx_uint64_t	HFS_get_count_float_le (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int from, double value);

zbx_uint64_t	HFS_get_sum_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds);
double		HFS_get_sum_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds);

double		HFS_get_avg_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds);
double		HFS_get_avg_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds);

zbx_uint64_t	HFS_get_min_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds);
double		HFS_get_min_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds);

zbx_uint64_t	HFS_get_max_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds);
double		HFS_get_max_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds);

zbx_uint64_t	HFS_get_delta_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds);
double		HFS_get_delta_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int period, int seconds);

/* HFS per-object statuses which updated on monitoring pair and should be visible on ztops */
void		HFS_update_host_availability (const char* hfs_base_dir, const char* siteid, zbx_uint64_t hostid, int available, int clock, const char* error);
int		HFS_get_host_availability (const char* hfs_base_dir, const char* siteid, zbx_uint64_t hostid, int* available, int* clock, char** error);

#endif
