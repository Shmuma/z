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


// define our off_t and time_t types which are always 64-bit
typedef long long int hfs_off_t;
typedef long long int hfs_time_t;


typedef enum {
    IT_UINT64 = 0,
    IT_DOUBLE,
    IT_TRENDS,
    IT_STRING,
} item_type_t;

typedef union {
	zbx_uint64_t	l;
	double		d;
} item_value_u;

typedef struct hfs_trend {
    int count;
    item_value_u	min;
    item_value_u	max;
    item_value_u	avg;
} hfs_trend_t;

#define alloc_item_values 1000
typedef struct hfs_item_value {
	item_type_t	type;
        hfs_time_t	clock;
	long		group;
	long		count;
	hfs_trend_t	value;
} hfs_item_value_t;


typedef struct {
	hfs_time_t clock;
	char* value;
} hfs_item_str_value_t;


typedef void (*read_count_fn_t) (item_type_t type, item_value_u val, hfs_time_t timestamp, void *res);

void		HFSadd_history (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, double value, hfs_time_t clock);
void		HFSadd_history_uint (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, zbx_uint64_t value, hfs_time_t clock);
void		HFSadd_history_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t clock, const char* value);
size_t		HFSread_item (const char* hfs_base_dir, const char* siteid,
				int trend,		zbx_uint64_t itemid,
				size_t x,
				hfs_time_t graph_from,	hfs_time_t graph_to,
				hfs_time_t from,	hfs_time_t to,
				hfs_item_value_t **result);
int		HFSread_count(const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int count, void* init_res, read_count_fn_t fn);

size_t		HFSread_item_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, hfs_time_t to, hfs_item_str_value_t **result);
size_t		HFSread_count_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int count, hfs_item_str_value_t **result);

zbx_uint64_t	HFS_get_count (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from);
zbx_uint64_t	HFS_get_count_u64_eq (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_ne (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_gt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_lt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_ge (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_le (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, zbx_uint64_t value);

zbx_uint64_t	HFS_get_count_float_eq (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, double value);
zbx_uint64_t	HFS_get_count_float_ne (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, double value);
zbx_uint64_t	HFS_get_count_float_gt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, double value);
zbx_uint64_t	HFS_get_count_float_lt (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, double value);
zbx_uint64_t	HFS_get_count_float_ge (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, double value);
zbx_uint64_t	HFS_get_count_float_le (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, double value);

zbx_uint64_t	HFS_get_sum_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds);
double		HFS_get_sum_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds);

double		HFS_get_avg_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds);
double		HFS_get_avg_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds);

zbx_uint64_t	HFS_get_min_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds);
double		HFS_get_min_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds);

zbx_uint64_t	HFS_get_max_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds);
double		HFS_get_max_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds);

zbx_uint64_t	HFS_get_delta_u64 (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds);
double		HFS_get_delta_float (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t period, int seconds);

/* HFS per-object statuses which updated on monitoring pair and should be visible on ztops */
void		HFS_update_host_availability (const char* hfs_base_dir, const char* siteid, zbx_uint64_t hostid, 
					      int available, hfs_time_t clock, const char* error);
int		HFS_get_host_availability (const char* hfs_base_dir, const char* siteid, zbx_uint64_t hostid, 
					   int* available, hfs_time_t* clock, char** error);

void		HFS_update_item_values_dbl (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t lastclock,
					int nextcheck, double prevvalue, double lastvalue, double prevorgvalue);
void		HFS_update_item_values_int (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t lastclock,
					int nextcheck, zbx_uint64_t prevvalue, zbx_uint64_t lastvalue, zbx_uint64_t prevorgvalue);
void		HFS_update_item_values_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t lastclock,
					int nextcheck, const char* prevvalue, const char* lastvalue, const char* prevorgvalue);
int		HFS_get_item_values_dbl (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t* lastclock,
					 int* nextcheck, double* prevvalue, double* lastvalue, double* prevorgvalue);
int		HFS_get_item_values_int (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t* lastclock,
					 int* nextcheck, zbx_uint64_t* prevvalue, zbx_uint64_t* lastvalue, zbx_uint64_t* prevorgvalue);
int		HFS_get_item_values_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t* lastclock,
					 int* nextcheck, char** prevvalue, char** lastvalue, char** prevorgvalue);

void		HFS_update_item_status (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int status, const char* error);
void		HFS_update_item_stderr (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, const char* stderr);

int		HFS_get_item_status (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int* status, char** error);
int		HFS_get_item_stderr (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, char** stderr);

/* trigger statuses */
void		HFS_update_trigger_value(const char* hfs_path, const char* siteid, zbx_uint64_t triggerid, int new_value, hfs_time_t now);
int		HFS_get_trigger_value (const char* hfs_path, const char* siteid, zbx_uint64_t triggerid, int* value, hfs_time_t* when);

/* alerts history */
void 		HFS_add_alert(const char* hfs_path, const char* siteid, hfs_time_t clock, zbx_uint64_t actionid, zbx_uint64_t userid, 
			      zbx_uint64_t triggerid,  zbx_uint64_t mediatypeid, char *sendto, char *subject, char *message);


#endif
