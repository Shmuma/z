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
    zbx_uint64_t	count;
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
    hfs_time_t ts;
    item_type_t type;
    int delay;
    item_value_u val;
} hfs_data_item_t;


typedef struct {
	hfs_time_t clock;
	char* value;
} hfs_item_str_value_t;


/* item values structs */
typedef struct __attribute__ ((packed)) {
	hfs_time_t lastclock;
	int kind;
	double prevvalue, lastvalue, prevorgvalue;
} item_value_dbl_t;


typedef struct __attribute__ ((packed)) {
	hfs_time_t lastclock;
	int kind;
	zbx_uint64_t prevvalue, lastvalue, prevorgvalue;
} item_value_int_t;


typedef struct {
	hfs_time_t clock;
	char* entry;
	hfs_time_t timestamp;
	char* source;
	int severity;
} hfs_log_entry_t;

#define TPL_HFS_LOG_ENTRY "S(UsUsi)"



typedef void (*read_count_fn_t) (item_type_t type, item_value_u val, hfs_time_t timestamp, void *res);

int 		xopen(const char *fn, int flags, mode_t mode);
void 		write_str (int fd, const char* str);
char* 		read_str (int fd);
char*		buffer_str (char* buf, const char* str, int buf_size);
char*		unbuffer_str (char** buf);
int		str_buffer_length (const char* str);

void		HFSadd_history (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, double value, hfs_time_t clock);
void		HFSadd_history_uint (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, zbx_uint64_t value, hfs_time_t clock);

void		HFSadd_history_vals (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, double* values, int count, hfs_time_t clock);
void		HFSadd_history_vals_uint (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, unsigned int delay, zbx_uint64_t* values, int count, hfs_time_t clock);

void		HFSadd_history_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t clock, const char* value);
void		HFSadd_history_log (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t clock, const char* value, 
				    hfs_time_t timestamp, char* eventlog_source, int eventlog_severity);

size_t		HFSread_item (const char* hfs_base_dir, const char* siteid,
				int trend,		zbx_uint64_t itemid,
				size_t x,
				hfs_time_t graph_from,	hfs_time_t graph_to,
				hfs_time_t from,	hfs_time_t to,
				hfs_item_value_t **result);
int		HFSread_count(const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int count, void* init_res, read_count_fn_t fn);
int		HFSread_interval(const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, hfs_time_t to, void* init_res, read_count_fn_t fn);

size_t		HFSread_item_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, hfs_time_t to, 
				  hfs_item_str_value_t **result);
size_t		HFSread_count_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int count, hfs_item_str_value_t **result);

size_t		HFSread_items_log (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t from, hfs_time_t to,
				   const char* filter, int filter_include, hfs_log_entry_t** result);
size_t		HFSread_count_log (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int count, int start, const char* filter,
				   int filer_include, hfs_log_entry_t** result);

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
typedef struct {
	zbx_uint64_t hostid;
	int available;
	hfs_time_t clock;
} hfs_host_status_t;


void		HFS_update_host_availability (const char* hfs_base_dir, const char* siteid, zbx_uint64_t hostid, 
					      int available, hfs_time_t clock, const char* error);
int		HFS_get_host_availability (const char* hfs_base_dir, const char* siteid, zbx_uint64_t hostid, 
					   int* available, hfs_time_t* clock, char** error);
int		HFS_get_hosts_statuses (const char* hfs_base_dir, const char* siteid, hfs_host_status_t** statuses);


void		HFS_update_item_values_dbl (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t lastclock,
					    double prevvalue, double lastvalue, double prevorgvalue, const char* stderr);
void		HFS_update_item_values_int (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t lastclock,
					    zbx_uint64_t prevvalue, zbx_uint64_t lastvalue, zbx_uint64_t prevorgvalue,
					    const char* stderr);
void		HFS_update_item_values_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t lastclock,
					    const char* prevvalue, const char* lastvalue, const char* prevorgvalue,
					    const char* stderr);
void		HFS_update_item_values_log (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t lastclock,
					    const char* prevvalue, const char* lastvalue, hfs_time_t timestamp, 
					    const char* eventlog_source, int eventlog_severity, const char* stderr);
int		HFS_get_item_values_dbl (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t* lastclock,
					 double* prevvalue, double* lastvalue, double* prevorgvalue, char** stderr);
int		HFS_get_item_values_int (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t* lastclock,
					 zbx_uint64_t* prevvalue, zbx_uint64_t* lastvalue, zbx_uint64_t* prevorgvalue,
					 char** stderr);
int		HFS_get_item_values_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t* lastclock,
					 char** prevvalue, char** lastvalue, char** prevorgvalue, char** stderr);
int		HFS_get_item_values_log (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t* lastclock,
					 char** prevvalue, char** lastvalue, hfs_time_t* timestamp, 
					 char** eventlog_source, int* eventlog_severity, char** stderr);

void		HFS_update_item_status (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int status, const char* error);
int		HFS_get_item_status (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int* status, char** error);


double		HFS_get_item_last_dbl (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, char** stderr);
zbx_uint64_t	HFS_get_item_last_int (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, char** stderr);


/* trigger statuses */
typedef struct {
	zbx_uint64_t triggerid;
	int value;
	hfs_time_t when;
} hfs_trigger_value_t;


void		HFS_update_trigger_value(const char* hfs_path, const char* siteid, zbx_uint64_t triggerid, int new_value, hfs_time_t now);
int		HFS_get_triggers_values (const char* hfs_path, const char* siteid, hfs_trigger_value_t** res);
int		HFS_get_trigger_value (const char* hfs_path, const char* siteid, zbx_uint64_t triggerid, hfs_trigger_value_t* res);

/* alerts history */
typedef struct __attribute__ ((packed)) {
	hfs_time_t clock;
	zbx_uint64_t mediatypeid;
	zbx_uint64_t triggerid;
	zbx_uint64_t actionid;
	zbx_uint64_t userid;
	int status;
	int retries;
	char* sendto;
	char* subject;
	char* message;
} hfs_alert_value_t;


void HFS_add_alert(const char* hfs_path, const char* siteid, hfs_time_t clock, zbx_uint64_t userid, zbx_uint64_t triggerid, 
		   zbx_uint64_t actionid, zbx_uint64_t mediatypeid, int status, int retries, char *sendto, char *subject, char *message);
int HFS_get_alerts (const char* hfs_path, const char* siteid, int skip, int count, hfs_alert_value_t** alerts);


/* trigger events  */
typedef struct __attribute__ ((packed)) {
	zbx_uint64_t eventid;
	zbx_uint64_t triggerid;
	hfs_time_t clock;
	unsigned char val;
	unsigned char ack;
} hfs_event_value_t;

void		HFS_add_event (const char* hfs_path, const char* siteid, zbx_uint64_t eventid, zbx_uint64_t triggerid, 
			       hfs_time_t clock, int val, int ack, zbx_uint64_t hostid);

int		HFS_get_trigger_events (const char* hfs_path, const char* siteid, zbx_uint64_t triggerid, int count, hfs_event_value_t** res);
int		HFS_get_host_events (const char* hfs_path, const char* siteid, zbx_uint64_t hostid, int skip, int count, hfs_event_value_t** res);

void		HFS_clear_item_history (const char* hfs_path, const char* siteid, zbx_uint64_t itemid);


/* lastvalue from functions table */
typedef enum {
	FVT_NULL,
	FVT_UINT64,
	FVT_DOUBLE,
} hfs_function_val_type_t;


typedef struct __attribute__ ((packed)) {
	hfs_function_val_type_t type;
	item_value_u value;
} hfs_function_value_t;

int		HFS_convert_function_str2val (const char* value, hfs_function_value_t* result);
char*		HFS_convert_function_val2str (hfs_function_value_t* result);
int		HFS_save_function_value (const char* hfs_path, const char* siteid, zbx_uint64_t functionid, hfs_function_value_t* value);
int		HFS_get_function_value (const char* hfs_path, const char* siteid, zbx_uint64_t functionid, hfs_function_value_t* value);

void HFS_save_aggr_slave_value (const char* hfs_path, const char* siteid, zbx_uint64_t itemid, hfs_time_t ts, int valid, double value, const char* stderr);
void HFS_get_aggr_slave_value  (const char* hfs_path, const char* siteid, zbx_uint64_t itemid, hfs_time_t* ts, int* valid, double* value, char** stderr);

#endif
