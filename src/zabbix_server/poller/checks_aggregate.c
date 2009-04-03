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
#include "../functions.h"

#include <math.h>


extern char* CONFIG_HFS_PATH;



/* ---------------------------------------- */
/* HFS gather hooks                          */
/* info_index argument is used when we calculate min or max values. In
   it we'll store index of item with resulting max/min value */
static double aggr_reduce_max (aggr_item_t* vals, int count, int* winners)
{
	int i;
	double res = 0;

	if (count > 0)
		res = vals[0].value;

	for (i = 1; i < count; i++)
		if (vals[i].value > res)
			res = vals[i].value;
	for (i = 0; i < count; i++)
		winners[i] = fabs (vals[i].value - res) < 0.001;

	return res;
}


static double aggr_reduce_min (aggr_item_t* vals, int count, int* winners)
{
	int i;
	double res = 0;

	if (count > 0)
		res = vals[0].value;

	for (i = 1; i < count; i++)
		if (vals[i].value < res)
			res = vals[i].value;
	for (i = 0; i < count; i++)
		winners[i] = fabs (vals[i].value - res) < 0.001;

	return res;
}


static double aggr_reduce_sum (aggr_item_t* vals, int count, int* winners)
{
	int i;
	double res = 0;

	for (i = 0; i < count; i++)
		res += vals[i].value;
	return res;
}


static double aggr_reduce_avg (aggr_item_t* vals, int count, int* winners)
{
	return aggr_reduce_sum (vals, count, winners) / count;
}



static aggregate_reduce_hook_t hfs_reduce_hooks[] = {
	{ "grpmax", aggr_reduce_max },
	{ "grpsum", aggr_reduce_sum },
	{ "grpavg", aggr_reduce_avg },
	{ "grpmin", aggr_reduce_min },
};


/*
 * grpfunc: grpmax, grpmin, grpsum, grpavg
 * itemfunc: last, min, max, avg, sum,count
 */
static int	evaluate_aggregate_hfs(zbx_uint64_t itemid, int delay, AGENT_RESULT *res, char *grpfunc, char* site)
{
	int i, result = NOTSUPPORTED, found = 0;
	DB_RESULT db_res;
	DB_ROW row;
	aggr_item_t* items = NULL;
	int items_count = 0, item_buf = 0;
	hfs_time_t ts, now = time (NULL);
	int valid;
	char* stderr;
	double value;
	int* winners;

	/* collect values from all sites */
	db_res = DBselect ("select s.name from sites s");

	while (row = DBfetch (db_res)) {
		if (items_count == item_buf)
			items = (aggr_item_t*)realloc (items, sizeof (aggr_item_t)*(item_buf += 10));
		if (!site || !strcmp (site, row[0])) {
			HFS_get_aggr_slave_value (CONFIG_HFS_PATH, row[0], itemid, &ts, &valid, &items[items_count].value, &stderr);
			if (valid && (now-ts) / delay < 3)
				items[items_count++].stderr = stderr;
			else
				if (stderr)
					free (stderr);
		}
	}

	DBfree_result (db_res);

	/* calculate final value */
	found = 0;
	winners = (int*)calloc (sizeof (int), items_count);
	for (i = 0; i < sizeof (hfs_reduce_hooks) / sizeof (hfs_reduce_hooks[0]) && !found; i++)
		if (strcmp (grpfunc, hfs_reduce_hooks[i].grpfunc) == 0) {
			found = 1;
			SET_DBL_RESULT (res, hfs_reduce_hooks[i].hook (items, items_count, winners));
		}

	stderr = NULL;
	for (i = 0; i < items_count; i++) {
		if (!winners[i])
			continue;
		if (items[i].stderr) {
			if (stderr) {
				char* s;
				int len = strlen (stderr) + 2 + strlen (items[i].stderr) + 1;
				s = (char*)malloc (len);
				snprintf (s, len, "%s, %s", stderr, items[i].stderr);
				free (stderr);
				stderr = s;
			}
			else
				stderr = strdup (items[i].stderr);
		}
	}

	if (stderr)
		SET_ERR_RESULT (res, stderr);

	for (i = 0; i < items_count; i++)
		if (items[i].stderr)
			free (items[i].stderr);
	free (items);
	free (winners);

	if (!found)  {
		zabbix_log( LOG_LEVEL_WARNING, "evaluate_aggregate_hfs: Unsupported grpfunc function [%s])", grpfunc);
		return FAIL;
	}

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
	char    key[MAX_STRING_LEN];
	char	function_grp[MAX_STRING_LEN];
	char	site[MAX_STRING_LEN];
	char	*p,*p2, *p3;
	int 	ret = SUCCEED;

	zabbix_log( LOG_LEVEL_DEBUG, "In get_value_aggregate([%s])",
		item->key);

	init_result(result);

	site[0] = 0;

	strscpy(key, item->key);
	if((p=strchr(key,'[')) != NULL)
	{
		*p=0;
		strscpy(function_grp,key);
		*p='[';
		p++;

		/* search for site value */
		p2 = strchr (p, ',');
		if (p2) {
			*p2 = 0;
			p3 = strchr (p, '@');
			if (p3) {
				strscpy (site, p3+1);
				/* get rid of quotes (optional) */
				p = site;
				while (*p) {
					if (*p == '"')
						*p = 0;
					else
						p++;
				}
			}
			*p2 = ',';
		}
	}
	else	ret = NOTSUPPORTED;

	zabbix_log( LOG_LEVEL_DEBUG, "Evaluating aggregate[%s] grpfunc[%s]",
		item->key,
		function_grp);

	if (ret == SUCCEED) {
		if (CONFIG_HFS_PATH) {
			if (evaluate_aggregate_hfs (item->itemid, item->delay, result, function_grp, site[0] ? site : NULL) != SUCCEED)
				ret = NOTSUPPORTED;
		}
		else
			ret = NOTSUPPORTED;
	}

	return ret;
}
