#include "common.h"
#include "cfg.h"
#include "db.h"

#include "zlog.h"
#include "log.h"

#include "aggr_slave.h"


extern char* CONFIG_SERVER_SITE;
extern char* CONFIG_HFS_PATH;


int proc_num;


#define PLAN_PERIOD 120


typedef struct {
	zbx_uint64_t itemid;
	int delay;
	char* key;
} plan_item_t;


typedef struct {
	time_t ts;
	int* items;
	int items_count;
} plan_entry_t;


typedef struct {
	plan_item_t* items;
	int items_count;
	plan_entry_t* entries;
	int entries_count;
} plan_t;



typedef struct {
	zbx_uint64_t itemid;
	zbx_item_value_type_t value_type;
} aggregate_item_info_t;


typedef struct {
	const char* itemfunc;
	double* (*hook) (aggregate_item_info_t* items, int items_count, const char* param);
} aggregate_gather_hook_t;


typedef struct {
	const char* grpfunc;
	double (*hook) (double* vals, int count, int* info_index);
} aggregate_reduce_hook_t;



static void plan_append_entry_item (plan_entry_t* entry, int item)
{
	entry->items = (int*)realloc (entry->items, (entry->items_count+1)*sizeof (int));
	entry->items[entry->items_count] = item;
	entry->items_count++;
}


static int plan_sort_func (const void* a, const void* b)
{
	return ((const plan_entry_t*)a)->ts - ((const plan_entry_t*)b)->ts;
}


static void plan_free_entries (plan_entry_t* entries, int entries_count)
{
	int i;

	for (i = 0; i < entries_count; i++)
		if (entries[i].items)
			free (entries[i].items);

	if (entries)
		free (entries);
}


static void plan_free_items (plan_item_t* items, int items_count)
{
	int i;

	for (i = 0; i < items_count; i++)
		if (items[i].key)
			free (items[i].key);

	if (items)
		free (items);
}


/* build plan with all needed information for 2 minutes of activity */
static plan_t* build_checks_plan (int period)
{
	DB_RESULT result;
	DB_ROW row;
	plan_t* plan;
	plan_item_t* items = NULL;
	int items_count = 0, items_buf = 0;
	time_t time_now, time_fin, ts;
	int delay;

	plan_entry_t* entries = NULL;
	int entries_count = 0, entries_buf = 0, i, j, k;

	time_now = time (NULL);
	time_fin = time_now + period;

	plan = (plan_t*)malloc (sizeof (plan_t));

	result = DBselect ("select i.itemid, i.delay, i.key_ from items i, hosts h where " ZBX_SQL_MOD(i.itemid,%d) 
			   "=%d and i.status=%d and i.type=%d and h.hostid=i.hostid and h.status=%d",
			   CONFIG_AGGR_SLAVE_FORKS,
			   proc_num-1,
			   ITEM_STATUS_ACTIVE,
			   ITEM_TYPE_AGGREGATE,
			   HOST_STATUS_MONITORED);

	while (row = DBfetch (result)) {
		/* append item to items table */
		if (items_count == items_buf) {
			items_buf += 16;
			items = (plan_item_t*)realloc (items, sizeof (plan_item_t) * items_buf);

			if (!items) {
				free (plan);
				return NULL;
			}
		}

		ZBX_STR2UINT64 (items[items_count].itemid, row[0]);
		delay = items[items_count].delay = atoi (row[1]);
		items[items_count].key = strdup (row[2]);

		/* append item's timespots */
		ts = time_now - time_now % delay;
		while (ts <= time_fin) {
			if (ts >= time_now) {
				/* append timestamp to entries */
				if (entries_count == entries_buf) {
					entries_buf += 16;
					entries = (plan_entry_t*)realloc (entries, sizeof (plan_entry_t) * entries_buf);

					if (!entries) {
						free (plan);
						free (items);
						return NULL;
					}
				}

				entries[entries_count].ts = ts;
				entries[entries_count].items = NULL;
				entries[entries_count].items_count = 0;

				plan_append_entry_item (entries+entries_count, items_count);
				entries_count++;
			}

			ts += delay;
		}

		items_count++;
	}

	DBfree_result (result);

	/* simplify plan */
	/* sort it */
	qsort (entries, entries_count, sizeof (plan_entry_t), plan_sort_func);

	/* calculate needed amount of entries */
	ts = -1;
	plan->entries_count = 0;

	for (i = 0; i < entries_count; i++) {
		if (entries[i].ts != ts) {
			ts = entries[i].ts;
			plan->entries_count++;
		}
	}

	plan->entries = (plan_entry_t*)malloc (sizeof (plan_entry_t) * plan->entries_count);

	j = ts = -1;

	for (i = 0; i < entries_count; i++) {
		if (entries[i].ts != ts) {
			j++;
			plan->entries[j].ts = ts = entries[i].ts;
			plan->entries[j].items = NULL;
			plan->entries[j].items_count = 0;
		}

		for (k = 0; k < entries[i].items_count; k++)
			plan_append_entry_item (plan->entries + j, entries[i].items[k]);
	}

	plan->items = items;
	plan->items_count = items_count;

	plan_free_entries (entries, entries_count);

	return plan;
}



static int parse_aggr_key (const char* key, char** grp_func, char** group, char** itemkey, char** item_func, char** param)
{
	char *p, *p2;
	char** args[] = { group, itemkey, item_func, param };
	int i, j, err;

	if( (p = strchr (key, '[')) != NULL) {
		*grp_func = (char*)malloc (p-key+1);
		(*grp_func)[p-key] = 0;
		zbx_strlcpy (*grp_func, key, p-key+1);
		p++;
	}
	else
		return 0;

	for (i = 0; i < sizeof (args) / sizeof (args[0]); i++)
		*args[i] = NULL;

	err = 0;

	/* loop over group function arguments */
	for (i = 0; i < sizeof (args) / sizeof (args[0]); i++) {
		p2 = strchr (p, '"');
		if (!p2) {
			err = 1;
			break;
		}
		p2++;

		if ((p = strchr (p2, '"')) != NULL) {
			*args[i] = (char*)malloc (p-p2+1);
			(*args[i])[p-p2] = 0;
			zbx_strlcpy (*args[i], p2, p-p2+1);
			p++;
		}
		else {
			err = 1;
			break;
		}
	}

	if (err) {
		free (*grp_func);
		for (i = 0; i < sizeof (args) / sizeof (args[0]); i++)
			if (*args[i])
				free (*args[i]);
	}

	return 1;
}



static aggregate_item_info_t* get_aggregate_items (const char* hostgroup, const char* itemkey, int* count)
{
	DB_RESULT result;
	DB_ROW row;
	char hostgroup_esc[MAX_STRING_LEN], itemkey_esc[MAX_STRING_LEN];
	aggregate_item_info_t* res = NULL, *tmp;
	int buf_count = 0;

	DBescape_string (itemkey,itemkey_esc,MAX_STRING_LEN);
	DBescape_string (hostgroup,hostgroup_esc,MAX_STRING_LEN);

	result = DBselect ("select i.itemid,i.value_type from items i, hosts h, hosts_groups hg, groups g "
			   " where i.hostid=h.hostid and h.hostid=hg.hostid and g.groupid=hg.groupid "
			   " and g.name='%s' and i.key_='%s' and i.status=%d and h.status=%d and i.value_type in (%d,%d) "
			   " and h.siteid=%d",
			   hostgroup_esc, itemkey_esc,
			   ITEM_STATUS_ACTIVE, HOST_STATUS_MONITORED,
			   ITEM_VALUE_TYPE_FLOAT, ITEM_VALUE_TYPE_UINT64,
			   getSiteCondition ());
	*count = 0;

	while ((row = DBfetch (result))) {
		if (*count == buf_count) {
			tmp = res;
			res = (aggregate_item_info_t*)realloc (res, (buf_count += 16)*sizeof (aggregate_item_info_t));
			if (!res) {
				zabbix_log (LOG_LEVEL_WARNING, "get_aggregate_items: Memory allocation error");
				free (tmp);
				DBfree_result (result);
				*count = 0;
				return NULL;
			}
		}

		res[*count].itemid = zbx_atoui64 (row[0]);
		res[*count].value_type = atoi (row[1]);
		(*count)++;
	}

	DBfree_result (result);

	return res;
}


static char* find_hostname_of_itemid (zbx_uint64_t itemid)
{
	DB_RESULT result;
	DB_ROW row;
	char* res = NULL;

	result = DBselect ("select h.host from items i, hosts h where i.itemid=" ZBX_FS_UI64 " and h.hostid=i.hostid", itemid);

	row = DBfetch (result);

	if (row && row[0])
		res = strdup (row[0]);
	DBfree_result (result);

	return res;
}


/* ---------------------------------------- */
/* HFS aggregate hooks                      */
static double* aggr_gather_last (aggregate_item_info_t* items, int items_count, const char* param)
{
	double* res = (double*)malloc (sizeof (double) * items_count);
	int i;

	if (!res)
		return res;

	for (i = 0; i < items_count; i++) {
		switch (items[i].value_type) {
		case ITEM_VALUE_TYPE_FLOAT:
			res[i] = HFS_get_item_last_dbl (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, items[i].itemid);
			break;

		case ITEM_VALUE_TYPE_UINT64:
			res[i] = HFS_get_item_last_int (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, items[i].itemid);
			break;
		}
	}

	return res;
}

static double* aggr_gather_sum (aggregate_item_info_t* items, int items_count, const char* param)
{
	double* res = (double*)malloc (sizeof (double) * items_count);
	int i, seconds;
	hfs_time_t arg;

	if (!res)
		return res;

	if (param[0] == '#') {
		seconds = 0;
		arg = atoi (param+1);
	} else {
		seconds = 1;
		arg = time (NULL) - atoi (param) + 1;
	}

	for (i = 0; i < items_count; i++) {
		switch (items[i].value_type) {
		case ITEM_VALUE_TYPE_FLOAT:
			res[i] = HFS_get_sum_float (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, items[i].itemid, arg, seconds);
			break;
		case ITEM_VALUE_TYPE_UINT64:
			res[i] = HFS_get_sum_u64 (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, items[i].itemid, arg, seconds);
			break;
		}
	}

	return res;
}


static double* aggr_gather_min (aggregate_item_info_t* items, int items_count, const char* param)
{
	double* res = (double*)malloc (sizeof (double) * items_count);
	int i, seconds;
	hfs_time_t arg;

	if (!res)
		return res;

	if (param[0] == '#') {
		seconds = 0;
		arg = atoi (param+1);
	} else {
		seconds = 1;
		arg = time (NULL) - atoi (param) + 1;
	}

	for (i = 0; i < items_count; i++) {
		switch (items[i].value_type) {
		case ITEM_VALUE_TYPE_FLOAT:
			res[i] = HFS_get_min_float (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, items[i].itemid, arg, seconds);
			break;
		case ITEM_VALUE_TYPE_UINT64:
			res[i] = HFS_get_min_u64 (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, items[i].itemid, arg, seconds);
			break;
		}
	}

	return res;
}


static double* aggr_gather_max (aggregate_item_info_t* items, int items_count, const char* param)
{
	double* res = (double*)malloc (sizeof (double) * items_count);
	int i, seconds;
	hfs_time_t arg;

	if (!res)
		return res;

	if (param[0] == '#') {
		seconds = 0;
		arg = atoi (param+1);
	} else {
		seconds = 1;
		arg = time (NULL) - atoi (param) + 1;
	}

	for (i = 0; i < items_count; i++) {
		switch (items[i].value_type) {
		case ITEM_VALUE_TYPE_FLOAT:
			res[i] = HFS_get_max_float (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, items[i].itemid, arg, seconds);
			break;
		case ITEM_VALUE_TYPE_UINT64:
			res[i] = HFS_get_max_u64 (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, items[i].itemid, arg, seconds);
			break;
		}
	}

	return res;
}


static double* aggr_gather_delta (aggregate_item_info_t* items, int items_count, const char* param)
{
	double* res = (double*)malloc (sizeof (double) * items_count);
	int i, seconds;
	hfs_time_t arg;

	if (!res)
		return res;

	if (param[0] == '#') {
		seconds = 0;
		arg = atoi (param+1);
	} else {
		seconds = 1;
		arg = time (NULL) - atoi (param) + 1;
	}

	for (i = 0; i < items_count; i++) {
		switch (items[i].value_type) {
		case ITEM_VALUE_TYPE_FLOAT:
			res[i] = HFS_get_delta_float (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, items[i].itemid, arg, seconds);
			break;
		case ITEM_VALUE_TYPE_UINT64:
			res[i] = HFS_get_delta_u64 (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, items[i].itemid, arg, seconds);
			break;
		}
	}

	return res;
}


static double* aggr_gather_count (aggregate_item_info_t* items, int items_count, const char* param)
{
	double* res = (double*)malloc (sizeof (double) * items_count);
	int i;
	hfs_time_t arg;

	if (!res)
		return res;

	arg = time (NULL) - atoi (param) + 1;

	for (i = 0; i < items_count; i++)
		res[i] = HFS_get_count (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, items[i].itemid, arg);

	return res;
}


static aggregate_gather_hook_t hfs_gather_hooks[] = {
	{ "last",   aggr_gather_last },
	{ "sum",    aggr_gather_sum  },
	{ "min",    aggr_gather_min  },
	{ "max",    aggr_gather_max  },
	{ "delta",  aggr_gather_delta },
	{ "count",  aggr_gather_count },
};


/* ---------------------------------------- */
/* HFS gather hooks                          */
/* info_index argument is used when we calculate min or max values. In
   it we'll store index of item with resulting max/min value */
static double aggr_reduce_max (double* vals, int count, int* info_index)
{
	int i;
	double res = 0;

	if (count > 0) {
		res = vals[0];
		*info_index = 0;
	}

	for (i = 1; i < count; i++)
		if (vals[i] > res) {
			res = vals[i];
			*info_index = i;
		}

	return res;
}


static double aggr_reduce_min (double* vals, int count, int* info_index)
{
	int i;
	double res = 0;

	if (count > 0) {
		res = vals[0];
		*info_index = 0;
	}

	for (i = 1; i < count; i++)
		if (vals[i] < res) {
			res = vals[i];
			*info_index = i;
		}

	return res;
}


static double aggr_reduce_sum (double* vals, int count, int* info_index)
{
	int i;
	double res = 0;

	for (i = 0; i < count; i++)
		res += vals[i];
	return res;
}


static double aggr_reduce_avg (double* vals, int count, int* info_index)
{
	return aggr_reduce_sum (vals, count, info_index) / count;
}



static aggregate_reduce_hook_t hfs_reduce_hooks[] = {
	{ "grpmax", aggr_reduce_max },
	{ "grpsum", aggr_reduce_sum },
	{ "grpavg", aggr_reduce_avg },
	{ "grpmin", aggr_reduce_min },
};



static void process_aggr_entry (plan_item_t* item)
{
	char *grp_func, *group, *itemkey, *item_func, *param;
	aggregate_item_info_t* items = NULL;
	int items_count, found = 0, i, info_index, value_got = 0;
	double* items_values = NULL;
	double result;
	char* stderr = NULL;

	if (!parse_aggr_key (item->key, &grp_func, &group, &itemkey, &item_func, &param))
		return;

	/* process item's key */
	/* find items' IDs for our group and site */
	items = get_aggregate_items (group, itemkey, &items_count);

/* 	zabbix_log (LOG_LEVEL_ERR, "Aggr Slave: process item %lld with key %s. Count of values are %d", item->itemid, item->key, items_count); */

	if (items_count) {
		/* process items */
		/* call apropriate hook to obtain per-item value */
		for (i = 0; i < sizeof (hfs_gather_hooks) / sizeof (hfs_gather_hooks[0]) && !found; i++)
			if (strcmp (item_func, hfs_gather_hooks[i].itemfunc) == 0) {
				found = 1;
				items_values = hfs_gather_hooks[i].hook (items, items_count, param);
			}

		if (!found)
			goto exit;
		if (!items_values)
			goto exit;

/* 		for (i = 0; i < items_count; i++) */
/* 			zabbix_log (LOG_LEVEL_ERR, "Aggr Slave: value %d = %f", i, items_values[i]); */

		/* calculate value of this site */
		found = 0;
		info_index = -1;
		for (i = 0; i < sizeof (hfs_reduce_hooks) / sizeof (hfs_reduce_hooks[0]) && !found; i++)
			if (strcmp (grp_func, hfs_reduce_hooks[i].grpfunc) == 0) {
				found = 1;
				result = hfs_reduce_hooks[i].hook (items_values, items_count, &info_index);
				value_got = 1;
			}

		if (info_index >= 0 && info_index < items_count)
			/* lookup hostname of winning item. It will be stderr of value. */
			stderr = find_hostname_of_itemid (items[info_index].itemid);
	}

 exit:
/* 	if (value_got) */
/* 		zabbix_log (LOG_LEVEL_ERR, "Aggr Slave: item %lld finished, value = %f, stderr = %s", item->itemid, result, stderr); */
/* 	else */
/* 		zabbix_log (LOG_LEVEL_ERR, "Aggr Slave: item %lld finished, no value is calculated", item->itemid); */

	/* save result of calculations */
	if (value_got)
		HFS_save_aggr_slave_value (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, item->itemid, time (NULL), value_got, result, stderr);
	if (items)
		free (items);
	if (items_values)
		free (items_values);
	if (stderr)
		free (stderr);
	free (grp_func);
	free (group);
	free (itemkey);
	free (item_func);
	free (param);
}


/* Tasks for aggregate slave are simple: calculate aggregate values
   for hosts local to server's site and save it's value to HFS. */
void main_aggregate_slave_loop (int procnum)
{
	plan_t* plan = NULL;
	int plan_pos = 0, i;
	time_t now;

	proc_num = procnum;

	DBconnect(ZBX_DB_CONNECT_NORMAL);

	while (1) {
		now = time (NULL);
		if (!plan || plan_pos >= plan->entries_count) {
			zbx_setproctitle ("Aggregate slave [building plan]");
			if (plan) {
				plan_free_entries (plan->entries, plan->entries_count);
				plan_free_items (plan->items, plan->items_count);
				free (plan);
			}

			/* build checks plan for 2 minutes */
			plan = build_checks_plan (PLAN_PERIOD);
			plan_pos = 0;
		}

		if (!plan || !plan->entries_count) {
			zbx_setproctitle ("Aggregate slave [nothing to do, sleep for %d seconds]", PLAN_PERIOD);
			sleep (PLAN_PERIOD);
		}
		else {
			zbx_setproctitle ("Aggregate slave [process items]");
			while (plan_pos < plan->entries_count && plan->entries[plan_pos].ts <= now) {
				/* process planned items */
				for (i = 0; i < plan->entries[plan_pos].items_count; i++)
					process_aggr_entry (plan->items + plan->entries[plan_pos].items[i]);
				plan_pos++;
			}

			if (plan_pos < plan->entries_count) {
				zbx_setproctitle ("Aggregate slave [sleep for %d seconds]", plan->entries[plan_pos].ts - now);
				sleep (plan->entries[plan_pos].ts - now);
			}
		}
	}
}

