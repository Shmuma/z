/* 
** ZABBIX
** Copyright (C) 2000-2005 SIA Zabbix
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
#include "checks_aggregate.h"

#include "hfs.h"


extern char* CONFIG_HFS_PATH;


static	int	evaluate_one(double *result, int *num, char *grpfunc, char const *value_str, int valuetype)
{
	int	ret = SUCCEED;
	double	value = 0;

	if(valuetype == ITEM_VALUE_TYPE_FLOAT)
	{
		value = zbx_atod(value_str);
	}
	else if(valuetype == ITEM_VALUE_TYPE_UINT64)
	{
		value = (double)zbx_atoui64(value_str);
	}

	if(strcmp(grpfunc,"grpsum") == 0)
	{
		*result+=value;
		*num+=1;
	}
	else if(strcmp(grpfunc,"grpavg") == 0)
	{
		*result+=value;
		*num+=1;
	}
	else if(strcmp(grpfunc,"grpmin") == 0)
	{
		if(*num==0)
		{
			*result=value;
		}
		else if(value<*result)
		{
			*result=value;
		}
		*num+=1;
	}
	else if(strcmp(grpfunc,"grpmax") == 0)
	{
		if(*num==0)
		{
			*result=value;
		}
		else if(value>*result)
		{
			*result=value;
		}
		*num+=1;
	}
	else
	{
		zabbix_log( LOG_LEVEL_WARNING, "Unsupported group function [%s])",
			grpfunc);
		ret = FAIL;
	}

	return ret;
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
			res[i] = HFS_get_item_last_dbl (CONFIG_HFS_PATH, items[i].site, items[i].itemid);
			break;

		case ITEM_VALUE_TYPE_UINT64:
			res[i] = HFS_get_item_last_int (CONFIG_HFS_PATH, items[i].site, items[i].itemid);
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
			res[i] = HFS_get_sum_float (CONFIG_HFS_PATH, items[i].site, items[i].itemid, arg, seconds);
			break;
		case ITEM_VALUE_TYPE_UINT64:
			res[i] = HFS_get_sum_u64 (CONFIG_HFS_PATH, items[i].site, items[i].itemid, arg, seconds);
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
			res[i] = HFS_get_min_float (CONFIG_HFS_PATH, items[i].site, items[i].itemid, arg, seconds);
			break;
		case ITEM_VALUE_TYPE_UINT64:
			res[i] = HFS_get_min_u64 (CONFIG_HFS_PATH, items[i].site, items[i].itemid, arg, seconds);
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
			res[i] = HFS_get_max_float (CONFIG_HFS_PATH, items[i].site, items[i].itemid, arg, seconds);
			break;
		case ITEM_VALUE_TYPE_UINT64:
			res[i] = HFS_get_max_u64 (CONFIG_HFS_PATH, items[i].site, items[i].itemid, arg, seconds);
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
			res[i] = HFS_get_delta_float (CONFIG_HFS_PATH, items[i].site, items[i].itemid, arg, seconds);
			break;
		case ITEM_VALUE_TYPE_UINT64:
			res[i] = HFS_get_delta_u64 (CONFIG_HFS_PATH, items[i].site, items[i].itemid, arg, seconds);
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
		res[i] = HFS_get_count (CONFIG_HFS_PATH, items[i].site, items[i].itemid, arg);

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


static aggregate_item_info_t* get_aggregate_items (const char* hostgroup, const char* itemkey, int* count)
{
	DB_RESULT result;
	DB_ROW row;
	char hostgroup_esc[MAX_STRING_LEN], itemkey_esc[MAX_STRING_LEN];
	aggregate_item_info_t* res = NULL, *tmp;
	int buf_count = 0;

	DBescape_string (itemkey,itemkey_esc,MAX_STRING_LEN);
	DBescape_string (hostgroup,hostgroup_esc,MAX_STRING_LEN);

	result = DBselect ("select i.itemid,i.value_type,s.name as siteid from items i, hosts h, hosts_groups hg, groups g, sites s "
			   " where i.hostid=h.hostid and h.hostid=hg.hostid and g.groupid=hg.groupid and s.siteid=h.siteid "
			   " and g.name='%s' and i.key_='%s' and i.status=%d and h.status=%d and i.value_type in (%d,%d)",
			   hostgroup_esc, itemkey_esc,
			   ITEM_STATUS_ACTIVE, HOST_STATUS_MONITORED,
			   ITEM_VALUE_TYPE_FLOAT, ITEM_VALUE_TYPE_UINT64);
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
		strcpy (res[*count].site, row[2]);
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


/*
 * grpfunc: grpmax, grpmin, grpsum, grpavg
 * itemfunc: last, min, max, avg, sum,count
 */
static int	evaluate_aggregate_hfs(AGENT_RESULT *res,char *grpfunc, char *hostgroup, char *itemkey, char *itemfunc, char *param)
{
	int i, result = NOTSUPPORTED, found = 0;
	aggregate_item_info_t* items;
	int items_count, info_index;
	double* items_values = NULL;

	/* obtain list of items with sites */
	items = get_aggregate_items (hostgroup, itemkey, &items_count);

	if (!items) {
		/* I don't know, said Ivan Susanin... */
		SET_DBL_RESULT (res, 0);
		return SUCCEED;
	}

	/* call apropriate hook to obtain per-item value */
	for (i = 0; i < sizeof (hfs_gather_hooks) / sizeof (hfs_gather_hooks[0]) && !found; i++)
		if (strcmp (itemfunc, hfs_gather_hooks[i].itemfunc) == 0) {
			found = 1;
			items_values = hfs_gather_hooks[i].hook (items, items_count, param);
		}

	if (!found) {
		zabbix_log( LOG_LEVEL_WARNING, "evaluate_aggregate_hfs: Unsupported itemfunc function [%s])", itemfunc);
		free (items);
		return FAIL;
	}

	if (!items_values) {
		zabbix_log( LOG_LEVEL_WARNING, "evaluate_aggregate_hfs: memory allocation error)");
		free (items);
		return FAIL;
	}

	/* we have item's data. Perform calculations. */
	found = 0;
	info_index = -1;
	for (i = 0; i < sizeof (hfs_reduce_hooks) / sizeof (hfs_reduce_hooks[0]) && !found; i++)
		if (strcmp (grpfunc, hfs_reduce_hooks[i].grpfunc) == 0) {
			found = 1;
			SET_DBL_RESULT (res, hfs_reduce_hooks[i].hook (items_values, items_count, &info_index));
		}

	if (info_index >= 0 && info_index < items_count) {
		/* lookup hostname of winning item. It will be stderr of value. */
		char* hostname = find_hostname_of_itemid (items[info_index].itemid);

		if (hostname)
			SET_ERR_RESULT (res, hostname);
	}

	free (items);
	free (items_values);

	if (!found)  {
		zabbix_log( LOG_LEVEL_WARNING, "evaluate_aggregate_hfs: Unsupported grpfunc function [%s])", grpfunc);
		return FAIL;
	}

	return SUCCEED;
}


/*
 * grpfunc: grpmax, grpmin, grpsum, grpavg
 * itemfunc: last, min, max, avg, sum,count
 */
static int	evaluate_aggregate(AGENT_RESULT *res,char *grpfunc, char *hostgroup, char *itemkey, char *itemfunc, char *param)
{
	char		sql[MAX_STRING_LEN];
	char		sql2[MAX_STRING_LEN];
	char		hostgroup_esc[MAX_STRING_LEN],itemkey_esc[MAX_STRING_LEN];
 
	DB_RESULT	result;
	DB_ROW		row;

	int		valuetype;
	double		d = 0;
	const		char		*value;
	int		num = 0;
	int		now;
	char		items[MAX_STRING_LEN],items2[MAX_STRING_LEN];

	now=time(NULL);

	zabbix_log( LOG_LEVEL_DEBUG, "In evaluate_aggregate('%s','%s','%s','%s','%s')",
		grpfunc,
		hostgroup,
		itemkey,
		itemfunc,
		param);

	init_result(res);

	DBescape_string(itemkey,itemkey_esc,MAX_STRING_LEN);
	DBescape_string(hostgroup,hostgroup_esc,MAX_STRING_LEN);
/* Get list of affected item IDs */
	strscpy(items,"0");
	result = DBselect("select itemid from items i,hosts_groups hg,hosts h,groups g where hg.groupid=g.groupid and i.hostid=h.hostid and hg.hostid=h.hostid and g.name='%s' and i.key_='%s' and i.status=%d and h.status=%d",
		hostgroup_esc,
		itemkey_esc,
		ITEM_STATUS_ACTIVE,
		HOST_STATUS_MONITORED);

	while((row=DBfetch(result)))
	{
		zbx_snprintf(items2,sizeof(items2),"%s,%s",
			items,
			row[0]);
/*		zabbix_log( LOG_LEVEL_WARNING, "ItemIDs items2[%s])",items2);*/
		strscpy(items,items2);
/*		zabbix_log( LOG_LEVEL_WARNING, "ItemIDs items[%s])",items2);*/
	}
	DBfree_result(result);

	if(strcmp(itemfunc,"last") == 0)
	{
		zbx_snprintf(sql,sizeof(sql),"select itemid,value_type,lastvalue from items where lastvalue is not NULL and items.itemid in (%s)",
			items);
		zbx_snprintf(sql2,sizeof(sql2),"select itemid,value_type,lastvalue from items where 0=1");
	}
		/* The SQL works very very slow on MySQL 4.0. That's why it has been split into two. */
/*		zbx_snprintf(sql,sizeof(sql),"select items.itemid,items.value_type,min(history.value) from items,hosts_groups,hosts,groups,history where history.itemid=items.itemid and hosts_groups.groupid=groups.groupid and items.hostid=hosts.hostid and hosts_groups.hostid=hosts.hostid and groups.name='%s' and items.key_='%s' and history.clock>%d group by 1,2",hostgroup_esc, itemkey_esc, now - atoi(param));*/
	else if( (strcmp(itemfunc,"min") == 0) ||
		(strcmp(itemfunc,"max") == 0) ||
		(strcmp(itemfunc,"avg") == 0) ||
		(strcmp(itemfunc,"count") == 0) ||
		(strcmp(itemfunc,"sum") == 0)
	)
	{
		zbx_snprintf(sql,sizeof(sql),"select h.itemid,i.value_type,%s(h.value) from items i,history h where h.itemid=i.itemid and h.itemid in (%s) and h.clock>%d group by h.itemid,i.value_type",
			itemfunc,
			items,
			now - atoi(param));
		zbx_snprintf(sql2,sizeof(sql),"select h.itemid,i.value_type,%s(h.value) from items i,history_uint h where h.itemid=i.itemid and h.itemid in (%s) and h.clock>%d group by h.itemid,i.value_type",
			itemfunc,
			items,
			now - atoi(param));
	}
	else
	{
		zabbix_log( LOG_LEVEL_WARNING, "Unsupported item function [%s])",
			itemfunc);
		return FAIL;
	}
	zabbix_log( LOG_LEVEL_DEBUG, "SQL [%s]",sql);
	zabbix_log( LOG_LEVEL_DEBUG, "SQL2 [%s]",sql2);

	result = DBselect("%s",sql);
	while((row=DBfetch(result)))
	{
		valuetype = atoi(row[1]);
		value = row[2];
		if(FAIL == evaluate_one(&d, &num, grpfunc, value, valuetype))
		{
			zabbix_log( LOG_LEVEL_WARNING, "Unsupported group function [%s])",
				grpfunc);
			DBfree_result(result);
			return FAIL;
		}
	}
	DBfree_result(result);

	result = DBselect("%s",sql2);
	while((row=DBfetch(result)))
	{
		valuetype = atoi(row[1]);
		value = row[2];
		if(FAIL == evaluate_one(&d, &num, grpfunc, value, valuetype))
		{
			zabbix_log( LOG_LEVEL_WARNING, "Unsupported group function [%s])",
				grpfunc);
			DBfree_result(result);
			return FAIL;
		}
	}
	DBfree_result(result);

	if(num==0)
	{
		zabbix_log( LOG_LEVEL_WARNING, "No values for group[%s] key[%s])",
			hostgroup,
			itemkey);
		return FAIL;
	}

	if(strcmp(grpfunc,"grpavg") == 0)
	{
		SET_DBL_RESULT(res, d/num);
	}
	else
	{
		SET_DBL_RESULT(res, d);
	}

	zabbix_log( LOG_LEVEL_DEBUG, "End evaluate_aggregate(result:" ZBX_FS_DBL ")",
		d);
	return SUCCEED;
}

/******************************************************************************
 *                                                                            *
 * Function: get_value_aggregate                                              *
 *                                                                            *
 * Purpose: retrieve data from ZABBIX server (aggregate items)                *
 *                                                                            *
 * Parameters: item - item we are interested in                               *
 *                                                                            *
 * Return value: SUCCEED - data succesfully retrieved and stored in result    *
 *                         and result_str (as string)                         *
 *               NOTSUPPORTED - requested item is not supported               *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
int	get_value_aggregate(DB_ITEM *item, AGENT_RESULT *result)
{
	char	function_grp[MAX_STRING_LEN];
	char	key[MAX_STRING_LEN];
	char	group[MAX_STRING_LEN];
	char	itemkey[MAX_STRING_LEN];
	char	function_item[MAX_STRING_LEN];
	char	parameter[MAX_STRING_LEN];
	char	*p,*p2;

	int 	ret = SUCCEED;


	zabbix_log( LOG_LEVEL_DEBUG, "In get_value_aggregate([%s])",
		item->key);

	init_result(result);

	strscpy(key, item->key);
	if((p=strchr(key,'[')) != NULL)
	{
		*p=0;
		strscpy(function_grp,key);
		*p='[';
		p++;
	}
	else	ret = NOTSUPPORTED;

	if(ret == SUCCEED)
	{
		if((p2=strchr(p,'"')) != NULL)
		{
			p2++;
		}
		else	ret = NOTSUPPORTED;

		if((ret == SUCCEED) && (p=strchr(p2,'"')) != NULL)
		{
			*p=0;
			strscpy(group,p2);
			*p='"';
			p++;
		}
		else	ret = NOTSUPPORTED;
	}

	if(ret == SUCCEED)
	{
		if(*p != ',')	ret = NOTSUPPORTED;
	}

	if(ret == SUCCEED)
	{
		if((p2=strchr(p,'"')) != NULL)
		{
			p2++;
		}
		else	ret = NOTSUPPORTED;

		if((ret == SUCCEED) && (p=strchr(p2,'"')) != NULL)
		{
			*p=0;
			strscpy(itemkey,p2);
			*p='"';
			p++;
		}
		else	ret = NOTSUPPORTED;
	}

	if(ret == SUCCEED)
	{
		if(*p != ',')	ret = NOTSUPPORTED;
	}

	if(ret == SUCCEED)
	{
		if((p2=strchr(p,'"')) != NULL)
		{
			p2++;
		}
		else	ret = NOTSUPPORTED;

		if((ret == SUCCEED) && (p=strchr(p2,'"')) != NULL)
		{
			*p=0;
			strscpy(function_item,p2);
			*p='"';
			p++;
		}
		else	ret = NOTSUPPORTED;
	}

	if(ret == SUCCEED)
	{
		if(*p != ',')	ret = NOTSUPPORTED;
	}

	if(ret == SUCCEED)
	{
		if((p2=strchr(p,'"')) != NULL)
		{
			p2++;
		}
		else	ret = NOTSUPPORTED;

		if((ret == SUCCEED) && (p=strchr(p2,'"')) != NULL)
		{
			*p=0;
			strscpy(parameter,p2);
			*p='"';
			p++;
		}
		else	ret = NOTSUPPORTED;
	}

	zabbix_log( LOG_LEVEL_DEBUG, "Evaluating aggregate[%s] grpfunc[%s] group[%s] itemkey[%s] itemfunc [%s] parameter [%s]",
		item->key,
		function_grp,
		group,
		itemkey,
		function_item,
		parameter);

	if (ret == SUCCEED) {
	if (CONFIG_HFS_PATH) {
		if (evaluate_aggregate_hfs (result, function_grp, group, itemkey, function_item, parameter) != SUCCEED)
			ret = NOTSUPPORTED;
	}
	else
		if (evaluate_aggregate(result,function_grp, group, itemkey, function_item, parameter) != SUCCEED)
			ret = NOTSUPPORTED;
	}

	return ret;
}
