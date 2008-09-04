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

#include "comms.h"
#include "db.h"
#include "log.h"
#include "zlog.h"
#include "hfs.h"

#include "evalfunc.h"
#include "functions.h"
#include "expression.h"

extern char* CONFIG_SERVER_SITE;
extern char* CONFIG_HFS_PATH;

/******************************************************************************
 *                                                                            *
 * Function: update_functions                                                 *
 *                                                                            *
 * Purpose: re-calculate and updates values of functions related to the item  *
 *                                                                            *
 * Parameters: item - item to update functions for                            *
 *                                                                            *
 * Return value:                                                              *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
void	update_functions(DB_ITEM *item)
{
	DB_FUNCTION	function;
	DB_RESULT	result;
	DB_ROW		row;
	char		value[MAX_STRING_LEN];
	char		value_esc[MAX_STRING_LEN];
	char		*lastvalue;
	int		ret=SUCCEED;

	zabbix_log( LOG_LEVEL_DEBUG, "In update_functions(" ZBX_FS_UI64 ")",
		item->itemid);

/* Oracle does'n support this */
/*	zbx_snprintf(sql,sizeof(sql),"select function,parameter,itemid,lastvalue from functions where itemid=%d group by function,parameter,itemid order by function,parameter,itemid",item->itemid);*/
	result = DBselect("select distinct function,parameter,itemid,lastvalue from functions where itemid=" ZBX_FS_UI64,
		item->itemid);

	while((row=DBfetch(result)))
	{
		function.function=row[0];
		function.parameter=row[1];
		ZBX_STR2UINT64(function.itemid,row[2]);
/*		function.itemid=atoi(row[2]); */
		lastvalue=row[3];

		zabbix_log( LOG_LEVEL_DEBUG, "ItemId:" ZBX_FS_UI64 " Evaluating %s(%s)",
			function.itemid,
			function.function,
			function.parameter);

		ret = evaluate_function(value,item,function.function,function.parameter);
		if( FAIL == ret)
		{
			zabbix_log( LOG_LEVEL_DEBUG, "Evaluation failed for function:%s",
				function.function);
			continue;
		}
		if (ret == SUCCEED)
		{
			/* Update only if lastvalue differs from new one */
			if( (lastvalue == NULL) || (strcmp(lastvalue,value) != 0))
			{
				DBescape_string(value,value_esc,MAX_STRING_LEN);
				DBexecute("update functions set lastvalue='%s' where itemid=" ZBX_FS_UI64 " and function='%s' and parameter='%s'",
					value_esc,
					function.itemid,
					function.function,
					function.parameter );
			}
			else
			{
				zabbix_log( LOG_LEVEL_DEBUG, "Do not update functions, same value");
			}
		}
	}

	DBfree_result(result);

	zabbix_log( LOG_LEVEL_DEBUG, "End update_functions()");
}

/******************************************************************************
 *                                                                            *
 * Function: update_triggers                                                  *
 *                                                                            *
 * Purpose: re-calculate and updates values of triggers related to the item   *
 *                                                                            *
 * Parameters: itemid - item to update trigger values for                     *
 *                                                                            *
 * Return value:                                                              *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
void	update_triggers(zbx_uint64_t itemid)
{
	char	*exp;
	char	error[MAX_STRING_LEN];
	int	exp_value;
	DB_TRIGGER	trigger;
	DB_RESULT	result;
	DB_ROW		row;

	zabbix_log( LOG_LEVEL_DEBUG, "In update_triggers [itemid:" ZBX_FS_UI64 "]",
		itemid);

	result = DBselect("select distinct t.triggerid,t.expression,t.description,t.url,t.comments,t.status,t.value,t.priority from triggers t,functions f,items i where i.status<>%d and i.itemid=f.itemid and t.status=%d and f.triggerid=t.triggerid and f.itemid=" ZBX_FS_UI64,
		ITEM_STATUS_NOTSUPPORTED,
		TRIGGER_STATUS_ENABLED,
		itemid);

	while((row=DBfetch(result)))
	{
		ZBX_STR2UINT64(trigger.triggerid,row[0]);
		strscpy(trigger.expression,row[1]);
		strscpy(trigger.description,row[2]);
		trigger.url		= row[3];
		trigger.comments	= row[4];
		trigger.status		= atoi(row[5]);
		trigger.value		= atoi(row[6]);
		trigger.priority	= atoi(row[7]);

		exp = strdup(trigger.expression);
		if( evaluate_expression(&exp_value, &exp, trigger.value, error, sizeof(error)) != 0 )
		{
			zabbix_log( LOG_LEVEL_WARNING, "Expression [%s] cannot be evaluated [%s]",
				trigger.expression,
				error);
			zabbix_syslog("Expression [%s] cannot be evaluated [%s]",
				trigger.expression,
				error);
/*			DBupdate_trigger_value(&trigger, exp_value, time(NULL), error);*//* We shouldn't update triggervalue if expressions failed */
		}
		else
		{
			DBupdate_trigger_value(&trigger, exp_value, time(NULL), NULL);
			if (CONFIG_HFS_PATH && trigger.value != exp_value)
				HFS_update_trigger_value (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, trigger.triggerid, exp_value, (hfs_time_t) time(NULL));
		}
		zbx_free(exp);
	}
	DBfree_result(result);
	zabbix_log( LOG_LEVEL_DEBUG, "End update_triggers [" ZBX_FS_UI64 "]",
		itemid);
}

void	calc_timestamp(char *line,int *timestamp, char *format)
{
	int hh=0,mm=0,ss=0,yyyy=0,dd=0,MM=0;
	int hhc=0,mmc=0,ssc=0,yyyyc=0,ddc=0,MMc=0;
	int i,num;
	struct  tm      tm;
	time_t t;

	zabbix_log( LOG_LEVEL_DEBUG, "In calc_timestamp()");

	hh=mm=ss=yyyy=dd=MM=0;

	for(i=0;(format[i]!=0)&&(line[i]!=0);i++)
	{
		if(isdigit(line[i])==0)	continue;
		num=(int)line[i]-48;

		switch ((char) format[i]) {
			case 'h':
				hh=10*hh+num;
				hhc++;
				break;
			case 'm':
				mm=10*mm+num;
				mmc++;
				break;
			case 's':
				ss=10*ss+num;
				ssc++;
				break;
			case 'y':
				yyyy=10*yyyy+num;
				yyyyc++;
				break;
			case 'd':
				dd=10*dd+num;
				ddc++;
				break;
			case 'M':
				MM=10*MM+num;
				MMc++;
				break;
		}
	}

	zabbix_log( LOG_LEVEL_DEBUG, "hh [%d] mm [%d] ss [%d] yyyy [%d] dd [%d] MM [%d]",
		hh,
		mm,
		ss,
		yyyy,
		dd,
		MM);

	/* Seconds can be ignored. No ssc here. */
	if(hhc!=0&&mmc!=0&&yyyyc!=0&&ddc!=0&&MMc!=0)
	{
		tm.tm_sec=ss;
		tm.tm_min=mm;
		tm.tm_hour=hh;
		tm.tm_mday=dd;
		tm.tm_mon=MM-1;
		tm.tm_year=yyyy-1900;

		t=mktime(&tm);
		if(t>0)
		{
			*timestamp=t;
		}
	}

	zabbix_log( LOG_LEVEL_DEBUG, "End timestamp [%d]",
		*timestamp);
}

/******************************************************************************
 *                                                                            *
 * Function: process_data                                                     *
 *                                                                            *
 * Purpose: process new item value                                            *
 *                                                                            *
 * Parameters: sockfd - descriptor of agent-server socket connection          *
 *             server - server name                                           *
 *             key - item's key                                               *
 *             value - new value of server:key                                *
 *             lastlogsize - if key=log[*], last size of log file             *
 *             when - timestamp in unix format. If NULL, latest value         *
 *                                                                            *
 * Return value: SUCCEED - new value processed sucesfully                     *
 *               FAIL - otherwise                                             *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments: for trapper server process                                       *
 *                                                                            *
 ******************************************************************************/
int	process_data(zbx_sock_t *sock,char *server,char *key,char *value, char* error, char *lastlogsize, char *timestamp,
		char *source, char *severity, char* when)
{
	AGENT_RESULT	agent;

	DB_RESULT       result;
	DB_ROW	row;
	DB_ITEM	item;
	time_t ts = 0;

#ifdef HAVE_MEMCACHE
	int	in_cache = 0;
#endif
	char	server_esc[MAX_STRING_LEN];
	char	key_esc[MAX_STRING_LEN];

	if (when)
		ts = atoi (when);

	zabbix_log( LOG_LEVEL_DEBUG, "In process_data([%s],[%s],[%s],[%s],[%s],[%s])",
		server,
		key,
		value,
		lastlogsize,
		when,
		error);

	init_result(&agent);

#ifdef HAVE_MEMCACHE
	if (process_type == ZBX_PROCESS_TRAPPERD)
		in_cache = memcache_zbx_getitem(key, server, &item);

	if (in_cache != 1) {
		zabbix_log( LOG_LEVEL_DEBUG, "In process_data: [NOT IN MEMCACHE] '%s %s'", key, server);

		DBescape_string(server, server_esc, MAX_STRING_LEN);
		DBescape_string(key, key_esc, MAX_STRING_LEN);

#endif
		result = DBselect("select %s where h.status=%d and h.hostid=i.hostid and h.host='%s' and i.key_='%s' and i.status in (%d,%d) and i.type in (%d,%d) and" ZBX_COND_NODEID " and " ZBX_COND_SITE,
			ZBX_SQL_ITEM_SELECT,
			HOST_STATUS_MONITORED,
			server_esc,
			key_esc,
			ITEM_STATUS_ACTIVE, ITEM_STATUS_NOTSUPPORTED,
			ITEM_TYPE_TRAPPER,
			ITEM_TYPE_ZABBIX_ACTIVE,
			LOCAL_NODE("h.hostid"),
			getSiteCondition ());

		row=DBfetch(result);

		if(!row)
		{
			DBfree_result(result);
			return FAIL;
		}

		DBget_item_from_db(&item,row);

#ifdef HAVE_MEMCACHE
	}
	else {
		zabbix_log( LOG_LEVEL_DEBUG, "In process_data: [IN MEMCACHE] '%s %s'", key, server);
	}
#endif

	if( (item.type==ITEM_TYPE_ZABBIX_ACTIVE) && (zbx_tcp_check_security(sock,item.trapper_hosts,1) == FAIL))
	{
		DBfree_result(result);
		DBfree_item(&item);
		return  FAIL;
	}

	zabbix_log( LOG_LEVEL_DEBUG, "Processing [%s]",
		value);

	/* update stderr only for latest data, not for history */
	if (!ts) {
		if (CONFIG_HFS_PATH) {
			HFS_update_item_stderr (CONFIG_HFS_PATH, item.siteid, item.itemid, error);
/* 			HFS_update_host_availability (CONFIG_HFS_PATH, item.siteid, item.hostid, HOST_AVAILABLE_TRUE, time (NULL), NULL); */
		}
		else
			DBupdate_item_stderr (item.itemid, error);
        }

	if(strcmp(value,"ZBX_NOTSUPPORTED") ==0)
	{
			zabbix_log( LOG_LEVEL_WARNING, "Active parameter [%s] is not supported by agent on host [%s]",
				item.key,
				item.host_name);
			zabbix_syslog("Active parameter [%s] is not supported by agent on host [%s]",
				item.key,
				item.host_name);

			DBupdate_item_status_to_notsupported(item.itemid, "Not supported by ZABBIX agent");
			if (CONFIG_HFS_PATH)
				HFS_update_item_status (CONFIG_HFS_PATH, item.siteid, item.itemid,
							ITEM_STATUS_NOTSUPPORTED, "Not supported by ZABBIX agent");
	}
	else
	{
		if(	(strncmp(item.key,"log[",4)==0) ||
			(strncmp(item.key,"eventlog[",9)==0)
		)
		{
			item.lastlogsize=atoi(lastlogsize);
			item.timestamp=atoi(timestamp);

			calc_timestamp(value,&item.timestamp,item.logtimefmt);

			item.eventlog_severity=atoi(severity);
			item.eventlog_source=source;
			zabbix_log(LOG_LEVEL_DEBUG, "Value [%s] Lastlogsize [%s] Timestamp [%s]",
				value,
				lastlogsize,
				timestamp);
		}

		if(set_result_type(&agent, item.value_type, value) == SUCCEED)
		{
			process_new_value(&item,&agent, ts);

			/* if we inserting historical value, don't update triggers */
			if (!ts)
				update_triggers(item.itemid);
		}
		else
		{
			zabbix_log( LOG_LEVEL_WARNING, "Type of received value [%s] is not suitable for [%s@%s]",
				value,
				item.key,
				item.host_name);
			zabbix_syslog("Type of received value [%s] is not suitable for [%s@%s]",
				value,
				item.key,
				item.host_name);
		}
 	}

#ifdef HAVE_MEMCACHE
	if (!in_cache)
#endif
		DBfree_result(result);

	free_result(&agent);

	DBfree_item(&item);

	return SUCCEED;
}



static int process_item_delta (DB_ITEM *item, item_value_u* new_val, int now, item_value_u* res_val)
{
	/* Should we store delta or original value? */
	if(item->delta == ITEM_STORE_AS_IS) {
		if(item->value_type==ITEM_VALUE_TYPE_UINT64)
			res_val->l = new_val->l;
		else if(item->value_type==ITEM_VALUE_TYPE_FLOAT)
			res_val->d = new_val->d;
		else
			return 0;
		return 1;
	}
	/* Delta as speed of change */
	else if(item->delta == ITEM_STORE_SPEED_PER_SECOND) {
		/* Save delta */
		if( ITEM_VALUE_TYPE_FLOAT == item->value_type ) {
			if(item->prevorgvalue_null == 0 && (item->prevorgvalue_dbl <= new_val->d) && (now != item->lastclock))
				res_val->d = (new_val->d - item->prevorgvalue_dbl)/(now-item->lastclock);
			else
				return 0;
		}
		else if( ITEM_VALUE_TYPE_UINT64 == item->value_type ) {
			if((item->prevorgvalue_null == 0) && (item->prevorgvalue_uint64 <= new_val->l) && (now != item->lastclock))
				res_val->l = (new_val->l - item->prevorgvalue_uint64)/(now-item->lastclock);
			else
				return 0;
		}
		return 1;
	}
	/* Real delta: simple difference between values */
	else if(item->delta == ITEM_STORE_SIMPLE_CHANGE) {
		/* Save delta */
		if( ITEM_VALUE_TYPE_FLOAT == item->value_type ) {
			if((item->prevorgvalue_null == 0) && (item->prevorgvalue_dbl <= new_val->d) )
				res_val->d = new_val->d - item->prevorgvalue_dbl;
			else
				return 0;
		}
		else if(item->value_type==ITEM_VALUE_TYPE_UINT64) {
			if((item->prevorgvalue_null == 0) && (item->prevorgvalue_uint64 <= new_val->l) )
				res_val->l = new_val->l - item->prevorgvalue_uint64;
			else 
				return 0;
		}
		else
			return 0;
		return 1;
	}
	else
		return 0;
}



/******************************************************************************
 *                                                                            *
 * Function: add_history                                                      *
 *                                                                            *
 * Purpose: add new value to history                                          *
 *                                                                            *
 * Parameters: item - item data                                               *
 *             value - new value of the item                                  *
 *             now   - new value of the item                                  *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
static int	add_history(DB_ITEM *item, AGENT_RESULT *value, int now)
{
	int ret = SUCCEED;

	zabbix_log( LOG_LEVEL_DEBUG, "In add_history(key:%s,value_type:%X,type:%X)",
		item->key,
		item->value_type,
		value->type);

	if (value->type & AR_UINT64)
		zabbix_log( LOG_LEVEL_DEBUG, "In add_history(itemid:"ZBX_FS_UI64",UINT64:"ZBX_FS_UI64")",
			item->itemid,
			value->ui64);
	if (value->type & AR_STRING)
		zabbix_log( LOG_LEVEL_DEBUG, "In add_history(itemid:"ZBX_FS_UI64",STRING:%s)",
			item->itemid,
			value->str);
	if (value->type & AR_DOUBLE)
		zabbix_log( LOG_LEVEL_DEBUG, "In add_history(itemid:"ZBX_FS_UI64",DOUBLE:"ZBX_FS_DBL")",
			item->itemid,
			value->dbl);
	if (value->type & AR_TEXT)
		zabbix_log( LOG_LEVEL_DEBUG, "In add_history(itemid:"ZBX_FS_UI64",TEXT:[%s])",
			item->itemid,
			value->text);

	if(item->history>0)
	{
		if( (item->value_type==ITEM_VALUE_TYPE_FLOAT) || (item->value_type==ITEM_VALUE_TYPE_UINT64))
		{
			item_value_u new_val, res_val;
			int ok = 1;

			if(item->value_type==ITEM_VALUE_TYPE_UINT64) {
				if (GET_UI64_RESULT (value))
					new_val.l = value->ui64;
				else
					ok = 0;
			}
			else {
				if (GET_DBL_RESULT (value))
					new_val.d = value->dbl;
				else
					ok = 0;
			}
			if (!ok || !process_item_delta (item, &new_val, now, &res_val)) {
				zabbix_log(LOG_LEVEL_ERR, "Value not stored for itemid [%d]. Unknown delta [%d]",
					item->itemid,
					item->delta);
				zabbix_syslog("Value not stored for itemid [%d]. Unknown delta [%d]",
					item->itemid,
					item->delta);
				ret = FAIL;
			}
			else {
				if (CONFIG_HFS_PATH)
					if(item->value_type==ITEM_VALUE_TYPE_UINT64)
						HFSadd_history_uint (CONFIG_HFS_PATH, item->siteid, item->itemid, item->delay, res_val.l, now);
					else
						HFSadd_history (CONFIG_HFS_PATH, item->siteid, item->itemid, item->delay, res_val.d, now);
				else
					if(item->value_type==ITEM_VALUE_TYPE_UINT64)
						DBadd_history_uint(item->itemid, res_val.l, now);
					else
						DBadd_history(item->itemid, res_val.d, now);
			}

		}
		else if(item->value_type==ITEM_VALUE_TYPE_STR)
		{
			if(GET_STR_RESULT(value)) {
				if (CONFIG_HFS_PATH)
					HFSadd_history_str (CONFIG_HFS_PATH, item->siteid, item->itemid, now, value->str);
				else
					DBadd_history_str(item->itemid,value->str,now);
			}
		}
		else if(item->value_type==ITEM_VALUE_TYPE_LOG)
		{
			if(GET_STR_RESULT(value))
				DBadd_history_log(0, item->itemid,value->str,now,item->timestamp,item->eventlog_source,item->eventlog_severity);
#ifndef HAVE_MEMCACHE
			DBexecute("update items set lastlogsize=%d where itemid=" ZBX_FS_UI64,
				item->lastlogsize,
				item->itemid);
#else
//			memcache_zbx_setitem(item);
#endif
		}
		else if(item->value_type==ITEM_VALUE_TYPE_TEXT)
		{
			if(GET_TEXT_RESULT(value)) {
				if (CONFIG_HFS_PATH)
					HFSadd_history_str (CONFIG_HFS_PATH, item->siteid, item->itemid, now, value->text);
                                else
                                    DBadd_history_text(item->itemid,value->text,now);
			}
		}
		else
		{
			zabbix_log(LOG_LEVEL_ERR, "Unknown value type [%d] for itemid [" ZBX_FS_UI64 "]",
				item->value_type,
				item->itemid);
		}
	}

	zabbix_log( LOG_LEVEL_DEBUG, "End of add_history");

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Function: update_item                                                      *
 *                                                                            *
 * Purpose: update item info after new value is received                      *
 *                                                                            *
 * Parameters: item - item data                                               *
 *             value - new value of the item                                  *
 *             now   - current timestamp                                      *
 *                                                                            *
 * Author: Alexei Vladishev, Eugene Grigorjev                                 *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
static void	update_item(DB_ITEM *item, AGENT_RESULT *value, time_t now)
{
	char	value_esc[MAX_STRING_LEN];
	int	nextcheck;

	zabbix_log( LOG_LEVEL_DEBUG, "In update_item()");

	value_esc[0]	= '\0';

	nextcheck = calculate_item_nextcheck(item->itemid, item->type, item->delay, item->delay_flex, now);
	if(item->delta == ITEM_STORE_AS_IS)
	{
		if(GET_STR_RESULT(value))
		{
			DBescape_string(value->str, value_esc, sizeof(value_esc));
		}

#ifdef HAVE_MEMCACHE
		if (process_type != ZBX_PROCESS_TRAPPERD)
#endif
		DBexecute("update items set nextcheck=%d,prevvalue=lastvalue,lastvalue='%s',lastclock=%d where itemid=" ZBX_FS_UI64,
			nextcheck,
			value_esc,
			(int)now,
			item->itemid);

		if (CONFIG_HFS_PATH) {
		    switch (item->value_type) {
		    case ITEM_VALUE_TYPE_STR:
			HFS_update_item_values_str (CONFIG_HFS_PATH, item->siteid, item->itemid, (hfs_time_t)now, (hfs_time_t)nextcheck,
						    item->lastvalue_null ? NULL : item->lastvalue_str, value->str, NULL);
			break;
		    case ITEM_VALUE_TYPE_FLOAT:
			HFS_update_item_values_dbl (CONFIG_HFS_PATH, item->siteid, item->itemid, (hfs_time_t)now, (hfs_time_t)nextcheck,
						    item->lastvalue_null ? 0.0 : item->lastvalue_dbl, value->dbl, 0.0);
			break;
		    case ITEM_VALUE_TYPE_UINT64:
			HFS_update_item_values_int (CONFIG_HFS_PATH, item->siteid, item->itemid, (hfs_time_t)now, (hfs_time_t)nextcheck,
						    item->lastvalue_null ? 0 : item->lastvalue_uint64, value->ui64, 0);
			break;
		    }
		}
	}
	/* Logic for delta as speed of change */
	else if(item->delta == ITEM_STORE_SPEED_PER_SECOND)
	{
		if(item->value_type == ITEM_VALUE_TYPE_FLOAT)
		{
			if(GET_DBL_RESULT(value))
			{
				if((item->prevorgvalue_null == 0) && (item->prevorgvalue_dbl <= value->dbl) )
				{
					/* In order to continue normal processing, we assume difference 1 second
					   Otherwise function update_functions and update_triggers won't work correctly*/
					if(now != item->lastclock)
					{
#ifdef HAVE_MEMCACHE
						if (process_type != ZBX_PROCESS_TRAPPERD)
#endif
						DBexecute("update items set nextcheck=%d,prevvalue=lastvalue,prevorgvalue='" ZBX_FS_DBL "',"
							"lastvalue='" ZBX_FS_DBL "',lastclock=%d where itemid=" ZBX_FS_UI64,
							nextcheck,
							value->dbl,
							(value->dbl - item->prevorgvalue_dbl)/(now-item->lastclock),
							(int)now,
							item->itemid);

						if (CONFIG_HFS_PATH)
							HFS_update_item_values_dbl (CONFIG_HFS_PATH, item->siteid, item->itemid, (int)now,
										    nextcheck, item->lastvalue_dbl,
										    (value->dbl - item->prevorgvalue_dbl)/(now-item->lastclock),
										    value->dbl);

						SET_DBL_RESULT(value, (double)(value->dbl - item->prevorgvalue_dbl)/(now-item->lastclock));
					}
					else
					{
#ifdef HAVE_MEMCACHE
						if (process_type != ZBX_PROCESS_TRAPPERD)
#endif
						DBexecute("update items set nextcheck=%d,prevvalue=lastvalue,prevorgvalue='" ZBX_FS_DBL "',"
							"lastvalue='" ZBX_FS_DBL "',lastclock=%d where itemid=" ZBX_FS_UI64,
							nextcheck,
							value->dbl,
							value->dbl - item->prevorgvalue_dbl,
							(int)now,
							item->itemid);

						if (CONFIG_HFS_PATH)
							HFS_update_item_values_dbl (CONFIG_HFS_PATH, item->siteid, item->itemid, (int)now, nextcheck,
										    item->lastvalue_dbl, value->dbl - item->prevorgvalue_dbl, value->dbl);

						SET_DBL_RESULT(value, (double)(value->dbl - item->prevorgvalue_dbl));
					}
				}
				else
				{
#ifdef HAVE_MEMCACHE
					if (process_type != ZBX_PROCESS_TRAPPERD)
#endif
					DBexecute("update items set nextcheck=%d,prevorgvalue='" ZBX_FS_DBL "',lastclock=%d where itemid=" ZBX_FS_UI64,
						nextcheck,
						value->dbl,
						(int)now,
						item->itemid);

					if (CONFIG_HFS_PATH)
						HFS_update_item_values_dbl (CONFIG_HFS_PATH, item->siteid, item->itemid, (int)now, nextcheck,
									    item->lastvalue_dbl, value->dbl, value->dbl);
				}
			}
		}
		else if(item->value_type == ITEM_VALUE_TYPE_UINT64)
		{
			if(GET_UI64_RESULT(value))
			{
				if((item->prevorgvalue_null == 0) && (item->prevorgvalue_uint64 <= value->ui64) )
				{
					if(now != item->lastclock)
					{
#ifdef HAVE_MEMCACHE
						if (process_type != ZBX_PROCESS_TRAPPERD)
#endif
						DBexecute("update items set nextcheck=%d,prevvalue=lastvalue,prevorgvalue='" ZBX_FS_UI64 "',"
							"lastvalue='" ZBX_FS_UI64 "',lastclock=%d where itemid=" ZBX_FS_UI64,
							nextcheck,
							value->ui64,
							((zbx_uint64_t)(value->ui64 - item->prevorgvalue_uint64))/(now-item->lastclock),
							(int)now,
							item->itemid);

						if (CONFIG_HFS_PATH)
							HFS_update_item_values_int (CONFIG_HFS_PATH, item->siteid, item->itemid, (int)now,
										    nextcheck, item->lastvalue_uint64,
										    (zbx_uint64_t)(value->ui64 - item->prevorgvalue_uint64)/(now-item->lastclock),
										    value->ui64);

						SET_UI64_RESULT(value, (zbx_uint64_t)(value->ui64 - item->prevorgvalue_uint64)/(now-item->lastclock));
					}
					else
					{
#ifdef HAVE_MEMCACHE
						if (process_type != ZBX_PROCESS_TRAPPERD)
#endif
						DBexecute("update items set nextcheck=%d,prevvalue=lastvalue,prevorgvalue='" ZBX_FS_UI64 "',"
							"lastvalue='" ZBX_FS_DBL "',lastclock=%d where itemid=" ZBX_FS_UI64,
							calculate_item_nextcheck(item->itemid, item->type, item->delay,item->delay_flex,now),
							value->ui64,
							(double)(value->ui64 - item->prevorgvalue_uint64),
							(int)now,
							item->itemid);

						if (CONFIG_HFS_PATH)
							HFS_update_item_values_int (CONFIG_HFS_PATH, item->siteid, item->itemid, (int)now, nextcheck,
										    item->lastvalue_uint64,
										    (zbx_uint64_t)(value->ui64 - item->prevorgvalue_uint64),
										    value->ui64);

						SET_UI64_RESULT(value, (zbx_uint64_t)(value->ui64 - item->prevorgvalue_uint64));
					}
				}
				else
				{
#ifdef HAVE_MEMCACHE
					if (process_type != ZBX_PROCESS_TRAPPERD)
#endif
					DBexecute("update items set nextcheck=%d,prevorgvalue='" ZBX_FS_UI64 "',lastclock=%d where itemid=" ZBX_FS_UI64,
						calculate_item_nextcheck(item->itemid, item->type, item->delay,item->delay_flex,now),
						value->ui64,
						(int)now,
						item->itemid);

					if (CONFIG_HFS_PATH)
						HFS_update_item_values_int (CONFIG_HFS_PATH, item->siteid, item->itemid, (int)now, nextcheck,
									    item->lastvalue_uint64, value->ui64, value->ui64);
				}
			}
		}
	}
	/* Real delta: simple difference between values */
	else if(item->delta == ITEM_STORE_SIMPLE_CHANGE)
	{
		if(item->value_type == ITEM_VALUE_TYPE_FLOAT)
		{
			if(GET_DBL_RESULT(value))
			{
				if((item->prevorgvalue_null == 0) && (item->prevorgvalue_dbl <= value->dbl))
				{
#ifdef HAVE_MEMCACHE
				    if (process_type != ZBX_PROCESS_TRAPPERD)
#endif
				    DBexecute("update items set nextcheck=%d,prevvalue=lastvalue,prevorgvalue='" ZBX_FS_DBL "',"
						"lastvalue='" ZBX_FS_DBL "',lastclock=%d where itemid=" ZBX_FS_UI64,
						calculate_item_nextcheck(item->itemid, item->type, item->delay,item->delay_flex,now),
						value->dbl,
						(value->dbl - item->prevorgvalue_dbl),
						(int)now,
						item->itemid);

				    if (CONFIG_HFS_PATH)
					    HFS_update_item_values_dbl (CONFIG_HFS_PATH, item->siteid, item->itemid, (int)now, nextcheck,
									item->lastvalue_dbl, value->dbl - item->prevorgvalue_dbl, value->dbl);
				    SET_DBL_RESULT(value, (double)(value->dbl - item->prevorgvalue_dbl));
				}
				else
				{
#ifdef HAVE_MEMCACHE
				    if (process_type != ZBX_PROCESS_TRAPPERD)
#endif
				    DBexecute("update items set nextcheck=%d,prevorgvalue='" ZBX_FS_DBL "',lastclock=%d where itemid=" ZBX_FS_UI64,
						calculate_item_nextcheck(item->itemid, item->type, item->delay,item->delay_flex, now),
						value->dbl,
						(int)now,
						item->itemid);

				    if (CONFIG_HFS_PATH)
					    HFS_update_item_values_dbl (CONFIG_HFS_PATH, item->siteid, item->itemid, (int)now, nextcheck,
									item->lastvalue_dbl, value->dbl, value->dbl);
				}
			}
		}
		else if(item->value_type == ITEM_VALUE_TYPE_UINT64)
		{
			if(GET_UI64_RESULT(value))
			{
				if((item->prevorgvalue_null == 0) && (item->prevorgvalue_uint64 <= value->ui64))
				{
#ifdef HAVE_MEMCACHE
				    if (process_type != ZBX_PROCESS_TRAPPERD)
#endif
				    DBexecute("update items set nextcheck=%d,prevvalue=lastvalue,prevorgvalue='" ZBX_FS_UI64 "',"
						"lastvalue='" ZBX_FS_UI64 "',lastclock=%d where itemid=" ZBX_FS_UI64,
						calculate_item_nextcheck(item->itemid, item->type, item->delay,item->delay_flex,now),
						value->ui64,
						(value->ui64 - item->prevorgvalue_uint64),
						(int)now,
						item->itemid);

				    if (CONFIG_HFS_PATH)
					    HFS_update_item_values_int (CONFIG_HFS_PATH, item->siteid, item->itemid, (int)now, nextcheck,
									item->lastvalue_uint64,
									(zbx_uint64_t)(value->ui64 - item->prevorgvalue_uint64),
									value->ui64);
				    SET_UI64_RESULT(value, (zbx_uint64_t)(value->ui64 - item->prevorgvalue_uint64));
				}
				else
				{
#ifdef HAVE_MEMCACHE
					if (process_type != ZBX_PROCESS_TRAPPERD)
#endif
    					DBexecute("update items set nextcheck=%d,prevorgvalue='" ZBX_FS_UI64 "',lastclock=%d where itemid=" ZBX_FS_UI64,
						calculate_item_nextcheck(item->itemid, item->type, item->delay,item->delay_flex, now),
						value->ui64,
						(int)now,
						item->itemid);

					if (CONFIG_HFS_PATH)
						HFS_update_item_values_int (CONFIG_HFS_PATH, item->siteid, item->itemid, (int)now, nextcheck,
									    item->lastvalue_uint64,
									    value->ui64, value->ui64);
				}
			}
		}
	}

	if (item->prevvalue_str)
		free(item->prevvalue_str);
	item->prevvalue_str 		= (item->lastvalue_str) ? strdup(item->lastvalue_str) : NULL;
	item->prevvalue_dbl		= item->lastvalue_dbl;
	item->prevvalue_uint64		= item->lastvalue_uint64;
	item->prevvalue_null		= item->lastvalue_null;

	if (item->lastvalue_str)
		free(item->lastvalue_str);
	item->lastvalue_str 		= (value->str) ? strdup(value->str) : NULL;
	item->lastvalue_uint64		= value->ui64;
	item->lastvalue_dbl		= value->dbl;
	item->lastvalue_null		= 0;

	if (item->prevorgvalue_str)
		free(item->prevorgvalue_str);
	item->prevorgvalue_str		= (value->str) ? strdup(value->str) : NULL;
	item->prevorgvalue_uint64	= value->ui64;
	item->prevorgvalue_dbl		= value->dbl;
	item->prevorgvalue_null		= 0;

/* Update item status if required */
	if(item->status == ITEM_STATUS_NOTSUPPORTED)
	{
		zabbix_log( LOG_LEVEL_WARNING, "Parameter [%s] became supported by agent on host [%s]",
			item->key,
			item->host_name);
		zabbix_syslog("Parameter [%s] became supported by agent on host [%s]",
			item->key,
			item->host_name);
		item->status = ITEM_STATUS_ACTIVE;

#ifdef HAVE_MEMCACHE
		if (process_type != ZBX_PROCESS_TRAPPERD)
#endif
		DBexecute("update items set status=%d,error='' where itemid=" ZBX_FS_UI64,
			ITEM_STATUS_ACTIVE,
			item->itemid);

		if (CONFIG_HFS_PATH)
			HFS_update_item_status (CONFIG_HFS_PATH, item->siteid, item->itemid, ITEM_STATUS_ACTIVE, NULL);
	}

	/* Required for nodata() */
	item->lastclock = now;
#ifdef HAVE_MEMCACHE
	if (process_type == ZBX_PROCESS_TRAPPERD) {
		item->nextcheck = nextcheck;
		memcache_zbx_setitem(item);
	}
#endif

	zabbix_log( LOG_LEVEL_DEBUG, "End update_item()");
}

/******************************************************************************
 *                                                                            *
 * Function: process_new_value                                                *
 *                                                                            *
 * Purpose: process new item value                                            *
 *                                                                            *
 * Parameters: item - item data                                               *
 *             value - new value of the item                                  *
 *             timestamp - timestamp of item to insert (used by history)      *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments: for trapper poller process                                       *
 *                                                                            *
 ******************************************************************************/
void	process_new_value(DB_ITEM *item, AGENT_RESULT *value, time_t timestamp)
{
	time_t 	now;

	zabbix_log( LOG_LEVEL_DEBUG, "In process_new_value(%s@%lu)",
		item->key, timestamp);

	if (!timestamp)
		now = time(NULL);
	else
		now = timestamp;

	if( ITEM_MULTIPLIER_USE == item->multiplier )
	{
		if( ITEM_VALUE_TYPE_FLOAT == item->value_type )
		{
			if(GET_DBL_RESULT(value))
			{
				UNSET_RESULT_EXCLUDING(value, AR_DOUBLE);
				SET_DBL_RESULT(value, value->dbl * strtod(item->formula, NULL));
			}
		}
		else if( ITEM_VALUE_TYPE_UINT64 == item->value_type )
		{
			if(GET_UI64_RESULT(value))
			{
				UNSET_RESULT_EXCLUDING(value, AR_UINT64);
				if(is_uint(item->formula) == SUCCEED)
				{
					SET_UI64_RESULT(value, value->ui64 * zbx_atoui64((item->formula)));
				}
				else
				{
					SET_UI64_RESULT(value, (zbx_uint64_t)((double)value->ui64 * strtod(item->formula, NULL)));
				}
			}
		}
	}

	add_history(item, value, now);
	if (!timestamp) {
		update_item(item, value, now);
		update_functions( item );
	}
}

/*
   Routine retuns condition equation for ZBX_COND_SITE macro.

   If CONFIG_SERVER_SITE is not null, it returns "s.name = 'CONFIG_SERVER_SITE'",
   otherwise, it return true value "1=1"

   Author: Max Lapan <max.lapan@gmail.com>
 */
const char* getSiteCondition ()
{
	static char exprBuffer[256] = { 0 };

	if (CONFIG_SERVER_SITE)
	{
		if (!exprBuffer[0])
		{
			if (strlen (CONFIG_SERVER_SITE) > sizeof (exprBuffer)-12)
			{
				zabbix_log (LOG_LEVEL_ERR, "getSiteCondition: Config file value in ServerSite is too large");
				return "1=1";
			}
			else
				zbx_snprintf (exprBuffer, sizeof (exprBuffer), "s.name = '%s'", CONFIG_SERVER_SITE);
		}
		return exprBuffer;
	}
	else
		return "1=1";
}


/* history addition token */
typedef struct {
	/* server and key for which we store data */
	char* server;
	char* key;

	/* state */
	hfs_time_t start_ts;
	hfs_time_t prev_ts;
	item_type_t type;
	DB_ITEM item;
	DB_RESULT result;

	/* buffer */
	item_value_u* buf;
	int buf_size;
	int buf_count;
} history_state_t;


/* performs append of history value to internally-allocated
   buffer. Calls to this routine must be sorted by timestamp. This
   routine calls flush_history when key or server changed. You must
   also call flush_history when all items are read and added.
   */
void	append_history (char* server, char* key, char* value, char* clock, void** token)
{
	history_state_t* state = (history_state_t*)*token;
	hfs_time_t ts;
	item_value_u val, val_res;

	/* check that server and key are still the same */
	if (state) {
		if (strcmp (server, state->server) || strcmp (key, state->key)) {
			flush_history (token);
			state = NULL;
		}
	}

	if (!state) {
		DB_ROW	row;
		char	server_esc[MAX_STRING_LEN];
		char	key_esc[MAX_STRING_LEN];

		/* obtain item's attributes */
		DBescape_string(server, server_esc, MAX_STRING_LEN);
		DBescape_string(key, key_esc, MAX_STRING_LEN);

		/* first run of append_history, initialize structure */
		state = (history_state_t*)malloc (sizeof (history_state_t));

		state->result = DBselect("select %s where h.status=%d and h.hostid=i.hostid and h.host='%s' and i.key_='%s' and i.status in (%d,%d) and i.type in (%d,%d) and" ZBX_COND_NODEID " and " ZBX_COND_SITE,
				  ZBX_SQL_ITEM_SELECT,
				  HOST_STATUS_MONITORED,
				  server_esc,
				  key_esc,
				  ITEM_STATUS_ACTIVE, ITEM_STATUS_NOTSUPPORTED,
				  ITEM_TYPE_TRAPPER,
				  ITEM_TYPE_ZABBIX_ACTIVE,
				  LOCAL_NODE("h.hostid"),
				  getSiteCondition ());

		row=DBfetch(state->result);
		if(!row) {
			DBfree_result(state->result);
			return;
		}


		DBget_item_from_db(&state->item, row);

		state->server = strdup (server);
		state->key = strdup (key);

		sscanf (clock, "%lld", &state->start_ts);
		state->prev_ts = state->start_ts;

		if (state->item.value_type == ITEM_VALUE_TYPE_FLOAT)
			state->type = IT_DOUBLE;
		else
			state->type = IT_UINT64;

		state->buf = NULL;
		state->buf_size = 0;
		state->buf_count = 0;

		*token = state;
	}

	sscanf (clock, "%lld", &ts);

	/* zero time occured sometimes, ignore it */
	if (!ts)
		return;

	if (!state->start_ts)
		state->start_ts = state->prev_ts = ts;

	/* append value to buffer */
	if (state->buf_size == state->buf_count) {
		state->buf_size += 256;
		state->buf = (item_value_u*)realloc (state->buf, sizeof (item_value_u) * state->buf_size);
	}

	/* compare timestamp values and perform padding if needed */
	if ((ts - state->prev_ts) / state->item.delay > 1) {
		int count = (ts - state->prev_ts) / state->item.delay - 1;

		while (count--) {
			state->buf[state->buf_count++].l = 0xFFFFFFFFFFFFFFFFULL;
			if (state->buf_size == state->buf_count) {
				state->buf_size += 256;
				state->buf = (item_value_u*)realloc (state->buf, sizeof (item_value_u) * state->buf_size);
			}
		}
		state->item.prevorgvalue_null = 1;
	}

	if (state->type == IT_DOUBLE) {
		sscanf (value, "%lf", &val.d);
		if (process_item_delta (&state->item, &val, ts, &val_res)) {
			state->buf[state->buf_count++].d = val_res.d;
			state->item.prevorgvalue_dbl = val.d;
			state->item.prevorgvalue_null = 0;
		}
		else {
			state->buf[state->buf_count++].l = 0xFFFFFFFFFFFFFFFFULL;
			state->item.prevorgvalue_null = 1;
		}
	}
	else {
		sscanf (value, "%llu", &val.l);
		if (process_item_delta (&state->item, &val, ts, &val_res)) {
			state->buf[state->buf_count++].l = val_res.l;
			state->item.prevorgvalue_uint64 = val.l;
			state->item.prevorgvalue_null = 0;
		}
		else {
			state->buf[state->buf_count++].l = 0xFFFFFFFFFFFFFFFFULL;
			state->item.prevorgvalue_null = 1;
		}
	}

	state->item.lastclock = ts;
	state->prev_ts = ts;
}


void	flush_history (void** token)
{
	history_state_t* state = (history_state_t*)*token;

	if (!state)
		return;

	/* add portion of data */
	if (state->buf_count) {
		if (state->type == IT_DOUBLE)
			HFSadd_history_vals (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, state->item.itemid, 
					     state->item.delay, state->buf, state->buf_count, state->start_ts);
		else
			HFSadd_history_vals_uint (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, state->item.itemid, 
					     state->item.delay, state->buf, state->buf_count, state->start_ts);
	}

	/* free */
	if (state->buf)
		free (state->buf);
	if (state->key)
		free (state->key);
	if (state->server)
		free (state->server);
	DBfree_result(state->result);
	DBfree_item(&state->item);
	free (state);
	*token = NULL;
}
