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

#include "zlog.h"

#include "../functions.h"
#include "../expression.h"
#include "../metrics.h"
#include "poller.h"

#include "checks_agent.h"
#include "checks_aggregate.h"
#include "checks_external.h"
#include "checks_internal.h"
#include "checks_simple.h"
#include "checks_snmp.h"

#include "daemon.h"
#include "hfs.h"


extern char* CONFIG_HFS_PATH;
extern char* CONFIG_SERVER_SITE;

AGENT_RESULT    result;

int	poller_type;
int	poller_num;

static zbx_uint64_t mtr_host_updates = 0;
static metric_key_t key_host_updates;

static zbx_uint64_t mtr_get_values = 0;
static metric_key_t key_get_values;

static zbx_uint64_t mtr_items_count = 0;
static metric_key_t key_items_count;


#define POLLER_GROUP_INTERVAL 120


int	get_value(DB_ITEM *item, AGENT_RESULT *result)
{
	int res=FAIL;

	struct	sigaction phan;

	zabbix_log(LOG_LEVEL_DEBUG, "In get_value(key:%s)",
		item->key);

	phan.sa_handler = &child_signal_handler;
	sigemptyset(&phan.sa_mask);
	phan.sa_flags = 0;
	sigaction(SIGALRM, &phan, NULL);

	alarm(CONFIG_TIMEOUT);

	if(item->type == ITEM_TYPE_ZABBIX)
	{
		res=get_value_agent(item, result);
	}
	else if( (item->type == ITEM_TYPE_SNMPv1) || (item->type == ITEM_TYPE_SNMPv2c) || (item->type == ITEM_TYPE_SNMPv3))
	{
#ifdef HAVE_SNMP
		res=get_value_snmp(item, result);
#else
		zabbix_log(LOG_LEVEL_WARNING, "Support of SNMP parameters was not compiled in");
		zabbix_syslog("Support of SNMP parameters was not compiled in. Cannot process [%s:%s]",
			item->host_name,
			item->key);
		res=NOTSUPPORTED;
#endif
	}
	else if(item->type == ITEM_TYPE_SIMPLE)
	{
		res=get_value_simple(item, result);
	}
	else if(item->type == ITEM_TYPE_INTERNAL)
	{
		res=get_value_internal(item, result);
	}
	else if(item->type == ITEM_TYPE_AGGREGATE)
	{
		res=get_value_aggregate(item, result);
	}
	else if(item->type == ITEM_TYPE_EXTERNAL)
	{
		res=get_value_external(item, result);
	}
	else
	{
		zabbix_log(LOG_LEVEL_WARNING, "Not supported item type:%d",
			item->type);
		zabbix_syslog("Not supported item type:%d",
			item->type);
		res=NOTSUPPORTED;
	}
	alarm(0);

	zabbix_log(LOG_LEVEL_DEBUG, "End get_value()");
	return res;
}


static int compare_int (const void* a, const void* b)
{
	int aa = *(int*)a, bb = *(int*)b;
	return aa-bb;
}


/* obtains an array of future checks in 2 minutes in future */
static int get_nextchecks(int** res_buf)
{
	DB_RESULT	result;
	DB_ROW		row;
	int		res = 0, count = 0;
	int		ts, delay;
	int		now = time (NULL);

/* Host status	0 == MONITORED
		1 == NOT MONITORED
		2 == UNREACHABLE */
	switch (poller_type) {
	case ZBX_POLLER_TYPE_UNREACHABLE:
		result = DBselect("select inn.nextcheck, i.delay from hosts h,items i left join items_nextcheck inn on inn.itemid=i.itemid where " ZBX_SQL_MOD(h.hostid,%d) "=%d and (inn.nextcheck<=%d or inn.nextcheck is null) and i.status=%d and i.type not in (%d,%d,%d) and h.status=%d and h.disable_until<=%d and h.errors_from!=0 and h.hostid=i.hostid and i.key_ not in ('%s','%s','%s','%s') and " ZBX_COND_SITE "order by inn.nextcheck",
				  CONFIG_UNREACHABLE_POLLER_FORKS,
				  poller_num-1,
				  now+POLLER_GROUP_INTERVAL,
				  ITEM_STATUS_ACTIVE,
				  ITEM_TYPE_TRAPPER, ITEM_TYPE_ZABBIX_ACTIVE, ITEM_TYPE_HTTPTEST,
				  HOST_STATUS_MONITORED,
				  now+POLLER_GROUP_INTERVAL,
				  SERVER_STATUS_KEY, SERVER_ICMPPING_KEY, SERVER_ICMPPINGSEC_KEY,SERVER_ZABBIXLOG_KEY,
				  getSiteCondition ());
		break;
	case ZBX_POLLER_TYPE_NORMAL:
		if(CONFIG_REFRESH_UNSUPPORTED != 0)
		{
			result = DBselect("select inn.nextcheck,i.delay from hosts h,items i left join items_nextcheck inn on inn.itemid=i.itemid where h.status=%d and h.disable_until<%d and (inn.nextcheck<=%d or inn.nextcheck is null) and h.errors_from=0 and h.hostid=i.hostid and i.status in (%d,%d) and i.type not in (%d,%d,%d,%d) and " ZBX_SQL_MOD(i.itemid,%d) "=%d and i.key_ not in ('%s','%s','%s','%s') and " ZBX_COND_SITE " order by inn.nextcheck",
					  HOST_STATUS_MONITORED,
					  now+POLLER_GROUP_INTERVAL,
					  now+POLLER_GROUP_INTERVAL,
					  ITEM_STATUS_ACTIVE, ITEM_STATUS_NOTSUPPORTED,
					  ITEM_TYPE_TRAPPER, ITEM_TYPE_ZABBIX_ACTIVE, ITEM_TYPE_HTTPTEST, ITEM_TYPE_AGGREGATE,
					  CONFIG_POLLER_FORKS,
					  poller_num-1,
					  SERVER_STATUS_KEY, SERVER_ICMPPING_KEY, SERVER_ICMPPINGSEC_KEY,SERVER_ZABBIXLOG_KEY,
					  getSiteCondition ());
		}
		else
		{
			result = DBselect("select inn.nextcheck,i.delay from hosts h,items i left join items_nextcheck inn on inn.itemid=i.itemid where h.status=%d and h.disable_until<%d and (inn.nextcheck<=%d or inn.nextcheck is null) and h.errors_from=0 and h.hostid=i.hostid and i.status=%d and i.type not in (%d,%d,%d,%d) and " ZBX_SQL_MOD(i.itemid,%d) "=%d and i.key_ not in ('%s','%s','%s','%s') and " ZBX_COND_SITE " order by inn.nextcheck",
					  HOST_STATUS_MONITORED,
					  now+POLLER_GROUP_INTERVAL,
					  now+POLLER_GROUP_INTERVAL,
					  ITEM_STATUS_ACTIVE,
					  ITEM_TYPE_TRAPPER, ITEM_TYPE_ZABBIX_ACTIVE, ITEM_TYPE_HTTPTEST, ITEM_TYPE_AGGREGATE,
					  CONFIG_POLLER_FORKS,
					  poller_num-1,
					  SERVER_STATUS_KEY, SERVER_ICMPPING_KEY, SERVER_ICMPPINGSEC_KEY,SERVER_ZABBIXLOG_KEY,
					  getSiteCondition ());
		}
		break;
	case ZBX_POLLER_TYPE_AGGREGATE:
		result = DBselect("select inn.nextcheck, i.delay from hosts h,items i left join items_nextcheck inn on inn.itemid=i.itemid where h.status=%d and h.disable_until<%d and (inn.nextcheck <= %d or inn.nextcheck is null) and h.errors_from=0 and h.hostid=i.hostid and i.status=%d and i.type=%d and " ZBX_SQL_MOD(i.itemid,%d) "=%d and i.key_ not in ('%s','%s','%s','%s') and " ZBX_COND_SITE " order by inn.nextcheck",
				  HOST_STATUS_MONITORED,
				  now+POLLER_GROUP_INTERVAL,
				  now+POLLER_GROUP_INTERVAL,
				  ITEM_STATUS_ACTIVE,
				  ITEM_TYPE_AGGREGATE,
				  CONFIG_AGGR_POLLER_FORKS,
				  poller_num-1,
				  SERVER_STATUS_KEY, SERVER_ICMPPING_KEY, SERVER_ICMPPINGSEC_KEY,SERVER_ZABBIXLOG_KEY,
				  getSiteCondition ());
		break;
	default:
		res = FAIL;
		return res;
	}

	*res_buf = NULL;
	count = 0;

	while (row=DBfetch(result)) {
		ts = row[0] ? atoi (row[0]) : 0;
		delay = atoi (row[1]);

		if (!ts || ts < now)
			ts = now;

		while (ts < now+POLLER_GROUP_INTERVAL) {
			if (count == res) {
				count += 64;
				*res_buf = (int*) realloc (*res_buf, count*sizeof (int));
			}

			(*res_buf)[res++] = ts;
			ts += delay;
		}
	}

	DBfree_result(result);
	/* sort result buffer */
	qsort (*res_buf, res, sizeof (int), compare_int);

	return	res;
}

/* Update special host's item - "status" */
static void update_key_status(zbx_uint64_t hostid,int host_status)
{
/*	char		value_str[MAX_STRING_LEN];*/
	AGENT_RESULT	agent;

	DB_ITEM		item;
	DB_RESULT	result;
	DB_ROW		row;

	int		update;

	zabbix_log(LOG_LEVEL_DEBUG, "In update_key_status(" ZBX_FS_UI64 ",%d)",
		hostid,
		host_status);

	result = DBselect("select %s where h.hostid=i.hostid and h.hostid=" ZBX_FS_UI64 " and i.key_='%s'" " and " ZBX_COND_SITE,
		ZBX_SQL_ITEM_SELECT,
		hostid,
		SERVER_STATUS_KEY,
		getSiteCondition ());

	row = DBfetch(result);

	if(row)
	{
		DBget_item_from_db(&item,row);

/* Do not process new value for status, if previous status is the same */
		update = (item.lastvalue_null==1);
		update = update || ((item.value_type == ITEM_VALUE_TYPE_FLOAT) &&(cmp_double(item.lastvalue_dbl, (double)host_status) == 1));
		update = update || ((item.value_type == ITEM_VALUE_TYPE_UINT64) &&(item.lastvalue_uint64 != host_status));

		if(update)
		{
			init_result(&agent);
			SET_UI64_RESULT(&agent, host_status);
			process_new_value(0, &item,&agent, 0, NULL);
			free_result(&agent);

			update_triggers(item.itemid);
		}

		DBfree_item(&item);
	}
	else
	{
		zabbix_log( LOG_LEVEL_DEBUG, "No items to update.");
	}

	DBfree_result(result);
}

/******************************************************************************
 *                                                                            *
 * Function: get_values                                                       *
 *                                                                            *
 * Purpose: retrieve values of metrics from monitored hosts                   *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value:                                                              *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments: always SUCCEED                                                   *
 *                                                                            *
 ******************************************************************************/
int get_values(void)
{
	DB_RESULT	result;
	DB_RESULT	result2;
	DB_ROW	row;
	DB_ROW	row2;

	int		now;
	int		delay;
	int		res;
	DB_ITEM		item;
	AGENT_RESULT	agent;
	int	stop=0;

	char		*unreachable_hosts = NULL;
	char		tmp[MAX_STRING_LEN];

	zabbix_log( LOG_LEVEL_DEBUG, "In get_values()");
	metric_update (key_get_values, ++mtr_get_values);

	now = time(NULL);

	//zbx_snprintf(tmp,sizeof(tmp)-1,ZBX_FS_UI64,0);
	bzero(tmp, MAX_STRING_LEN);
	unreachable_hosts=zbx_strdcat(unreachable_hosts,tmp);

	switch (poller_type) {
	case ZBX_POLLER_TYPE_UNREACHABLE:
		result = DBselect("select h.hostid,min(i.itemid) from hosts h,items i left join items_nextcheck inn on inn.itemid=i.itemid where " ZBX_SQL_MOD(h.hostid,%d) "=%d and (inn.nextcheck<=%d or inn.nextcheck is null) and i.status=%d and i.type not in (%d,%d,%d) and h.status=%d and h.disable_until<=%d and h.errors_from!=0 and h.hostid=i.hostid and i.key_ not in ('%s','%s','%s','%s') and " ZBX_COND_SITE " group by h.hostid",
			CONFIG_UNREACHABLE_POLLER_FORKS,
			poller_num-1,
			now,
			ITEM_STATUS_ACTIVE,
			ITEM_TYPE_TRAPPER, ITEM_TYPE_ZABBIX_ACTIVE, ITEM_TYPE_HTTPTEST,
			HOST_STATUS_MONITORED,
			now,
			SERVER_STATUS_KEY, SERVER_ICMPPING_KEY, SERVER_ICMPPINGSEC_KEY,SERVER_ZABBIXLOG_KEY,
			getSiteCondition ());
		break;
	case ZBX_POLLER_TYPE_NORMAL:
		if(CONFIG_REFRESH_UNSUPPORTED != 0)
		{
			result = DBselect("select %s where i.nextcheck<=%d and i.status in (%d,%d) and i.type not in (%d,%d,%d,%d) and h.status=%d and h.disable_until<=%d and h.errors_from=0 and h.hostid=i.hostid and " ZBX_SQL_MOD(i.itemid,%d) "=%d and i.key_ not in ('%s','%s','%s','%s') and " ZBX_COND_SITE " order by i.nextcheck",
				ZBX_SQL_ITEM_SELECT,
				now,
				ITEM_STATUS_ACTIVE, ITEM_STATUS_NOTSUPPORTED,
				ITEM_TYPE_TRAPPER, ITEM_TYPE_ZABBIX_ACTIVE, ITEM_TYPE_HTTPTEST, ITEM_TYPE_AGGREGATE,
				HOST_STATUS_MONITORED,
				now,
				CONFIG_POLLER_FORKS,
				poller_num-1,
				SERVER_STATUS_KEY, SERVER_ICMPPING_KEY, SERVER_ICMPPINGSEC_KEY,SERVER_ZABBIXLOG_KEY,
				getSiteCondition ());
		}
		else
		{
			result = DBselect("select %s left join items_nextcheck inn on inn.itemid=i.itemid where (inn.nextcheck<=%d or inn.nextcheck is null) and i.status=%d and i.type not in (%d,%d,%d,%d) and h.status=%d and h.disable_until<=%d and h.errors_from=0 and h.hostid=i.hostid and " ZBX_SQL_MOD(i.itemid,%d) "=%d and i.key_ not in ('%s','%s','%s','%s') and " ZBX_COND_SITE " order by inn.nextcheck",
				ZBX_SQL_ITEM_SELECT,
				now,
				ITEM_STATUS_ACTIVE,
				ITEM_TYPE_TRAPPER, ITEM_TYPE_ZABBIX_ACTIVE, ITEM_TYPE_HTTPTEST, ITEM_TYPE_AGGREGATE,
				HOST_STATUS_MONITORED,
				now,
				CONFIG_POLLER_FORKS,
				poller_num-1,
				SERVER_STATUS_KEY, SERVER_ICMPPING_KEY, SERVER_ICMPPINGSEC_KEY,SERVER_ZABBIXLOG_KEY,
				getSiteCondition ());
		}
		break;
	case ZBX_POLLER_TYPE_AGGREGATE:
		result = DBselect("select %s left join items_nextcheck inn on inn.itemid=i.itemid where (inn.nextcheck<=%d or inn.nextcheck is null) and i.status=%d and i.type=%d and h.status=%d and h.disable_until<=%d and h.errors_from=0 and h.hostid=i.hostid and " ZBX_SQL_MOD(i.itemid,%d) "=%d and " ZBX_COND_SITE " order by inn.nextcheck",
				  ZBX_SQL_ITEM_SELECT,
				  now,
				  ITEM_STATUS_ACTIVE,
				  ITEM_TYPE_AGGREGATE,
				  HOST_STATUS_MONITORED,
				  now,
				  CONFIG_AGGR_POLLER_FORKS,
				  poller_num-1,
				  getSiteCondition ());
		break;
	}

	/* Do not stop when select is made by poller for unreachable hosts */
	while((row=DBfetch(result))&&(stop==0 || poller_type == ZBX_POLLER_TYPE_UNREACHABLE))
	{
		/* This code is just to avoid compilation warining about use of uninitialized result2 */
		result2 = result;
		/* */
		mtr_items_count++;

		/* Poller for unreachable hosts */
		if(poller_type == ZBX_POLLER_TYPE_UNREACHABLE)
		{
			result2 = DBselect("select %s where h.hostid=i.hostid and i.itemid=%s and " ZBX_COND_SITE,
				ZBX_SQL_ITEM_SELECT,
				row[1],
				getSiteCondition ());

			row2 = DBfetch(result2);

			if(!row2)
			{
				DBfree_result(result2);
				continue;
			}
			DBget_item_from_db(&item,row2);
		}
		else
		{
			DBget_item_from_db(&item,row);
			/* Skip unreachable hosts but do not break the loop. */
			if(uint64_in_list(unreachable_hosts,item.hostid) == SUCCEED)
			{
				zabbix_log( LOG_LEVEL_DEBUG, "Host " ZBX_FS_UI64 " is unreachable. Skipping [%s]",
					item.hostid,item.key);
				DBfree_item(&item);
				continue;
			}
		}

		init_result(&agent);
		res = get_value(&item, &agent);

		DBbegin();

		if(res == SUCCEED )
		{

			process_new_value(0, &item, &agent, 0, ISSET_ERR(&agent) ? agent.err : NULL);

/*			if(HOST_STATUS_UNREACHABLE == item.host_status)*/
			if(HOST_AVAILABLE_TRUE != item.host_available)
			{
				zabbix_log( LOG_LEVEL_WARNING, "Enabling host [%s]",
					item.host_name);
				zabbix_syslog("Enabling host [%s]",
					item.host_name);

				now = time(NULL);
				DBupdate_host_availability(item.hostid,HOST_AVAILABLE_TRUE,now,agent.msg);

				update_key_status(item.hostid, HOST_STATUS_MONITORED); /* 0 */
				item.host_available=HOST_AVAILABLE_TRUE;

				stop=1;
			}
			if (CONFIG_HFS_PATH) {
				metric_update (key_host_updates, ++mtr_host_updates);
				HFS_update_host_availability (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, item.hostid, HOST_AVAILABLE_TRUE, now, agent.msg);
			}
			if(item.host_errors_from!=0)
			{
				DBexecute("update hosts set errors_from=0 where hostid=" ZBX_FS_UI64,
					item.hostid);

				stop=1;
			}
		       	update_triggers(item.itemid);
		}
		else if(res == NOTSUPPORTED || res == AGENT_ERROR)
		{
			now = time(NULL);
			if(item.status == ITEM_STATUS_NOTSUPPORTED)
			{
				DBexecute("update items_nextcheck set nextcheck=%d where itemid=" ZBX_FS_UI64,
					CONFIG_REFRESH_UNSUPPORTED+now,
					item.itemid);
			}
			else
			{
				zabbix_log( LOG_LEVEL_WARNING, "Parameter [%s] is not supported by agent on host [%s] Old status [%d]",
					item.key,
					item.host_name,
					item.status);
				zabbix_syslog("Parameter [%s] is not supported by agent on host [%s]",
					item.key,
					item.host_name);
				DBupdate_item_status_to_notsupported(item.itemid, agent.msg);
				if (CONFIG_HFS_PATH)
					HFS_update_item_status (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, item.itemid,
							ITEM_STATUS_NOTSUPPORTED, "Not supported by ZABBIX agent");

	/*			if(HOST_STATUS_UNREACHABLE == item.host_status)*/
				if(HOST_AVAILABLE_TRUE != item.host_available)
				{
					zabbix_log( LOG_LEVEL_WARNING, "Enabling host [%s]",
						item.host_name);
					zabbix_syslog("Enabling host [%s]",
						item.host_name);
					DBupdate_host_availability(item.hostid,HOST_AVAILABLE_TRUE,now,agent.msg);
					update_key_status(item.hostid, HOST_STATUS_MONITORED);	/* 0 */
					item.host_available=HOST_AVAILABLE_TRUE;

					stop=1;
				}
				if (CONFIG_HFS_PATH) {
					metric_update (key_host_updates, ++mtr_host_updates);
					HFS_update_host_availability (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, item.hostid,
								      HOST_AVAILABLE_TRUE, now, agent.msg);
				}
			}
		}
		else if(res == NETWORK_ERROR)
		{
			now = time(NULL);

			/* First error */
			if(item.host_errors_from==0)
			{
				zabbix_log( LOG_LEVEL_WARNING, "Host [%s]: first network error, wait for %d seconds",
					item.host_name,
					CONFIG_UNREACHABLE_DELAY);
				zabbix_syslog("Host [%s]: first network error, wait for %d seconds",
					item.host_name,
					CONFIG_UNREACHABLE_DELAY);

				item.host_errors_from=now;
				DBexecute("update hosts set errors_from=%d,disable_until=%d where hostid=" ZBX_FS_UI64,
					now,
					now+CONFIG_UNREACHABLE_DELAY,
					item.hostid);

				delay = MIN(4*item.delay, 300);
				zabbix_log( LOG_LEVEL_WARNING, "Parameter [%s] will be checked after %d seconds on host [%s]",
					item.key,
					delay,
					item.host_name);
				DBexecute("update items_nextcheck set nextcheck=%d where itemid=" ZBX_FS_UI64,
					now + delay,
					item.itemid);
			}
			else
			{
				if(now-item.host_errors_from>CONFIG_UNREACHABLE_PERIOD)
				{
					zabbix_log( LOG_LEVEL_WARNING, "Host [%s] will be checked after %d seconds",
						item.host_name,
						CONFIG_UNAVAILABLE_DELAY);
					zabbix_syslog("Host [%s] will be checked after %d seconds",
						item.host_name,
						CONFIG_UNAVAILABLE_DELAY);

					DBupdate_host_availability(item.hostid,HOST_AVAILABLE_FALSE,now,agent.msg);
					if (CONFIG_HFS_PATH) {
						metric_update (key_host_updates, ++mtr_host_updates);
						HFS_update_host_availability (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, item.hostid,
									      HOST_AVAILABLE_FALSE, now, agent.msg);
					}
					update_key_status(item.hostid,HOST_AVAILABLE_FALSE); /* 2 */
					item.host_available=HOST_AVAILABLE_FALSE;

					DBexecute("update hosts set disable_until=%d where hostid=" ZBX_FS_UI64,
						now+CONFIG_UNAVAILABLE_DELAY,
						item.hostid);
				}
				/* Still unavailable, but won't change status to UNAVAILABLE yet */
				else
				{
					zabbix_log( LOG_LEVEL_WARNING, "Host [%s]: another network error, wait for %d seconds",
						item.host_name,
						CONFIG_UNREACHABLE_DELAY);
					zabbix_syslog("Host [%s]: another network error, wait for %d seconds",
						item.host_name,
						CONFIG_UNREACHABLE_DELAY);

					DBexecute("update hosts set disable_until=%d where hostid=" ZBX_FS_UI64,
						now+CONFIG_UNREACHABLE_DELAY,
						item.hostid);
				}
			}

			zbx_snprintf(tmp,sizeof(tmp)-1,"," ZBX_FS_UI64,item.hostid);
			unreachable_hosts=zbx_strdcat(unreachable_hosts,tmp);

/*			stop=1;*/
		}
		else
		{
			zabbix_log( LOG_LEVEL_CRIT, "Unknown response code returned.");
			assert(0==1);
		}
		/* Poller for unreachable hosts */
		if(poller_type == ZBX_POLLER_TYPE_UNREACHABLE)
		{
			/* We cannot freeit earlier because items has references to the structure */
			DBfree_result(result2);
		}
		free_result(&agent);
		DBcommit();
		DBfree_item(&item);
	}

	zbx_free(unreachable_hosts);

	metric_update (key_items_count, mtr_items_count);

	DBfree_result(result);
	zabbix_log( LOG_LEVEL_DEBUG, "End get_values()");
	return SUCCEED;
}


void update_poller_title (int type, int sleep)
{
	switch (type) {
	case ZBX_POLLER_TYPE_UNREACHABLE:
		if (sleep)
			zbx_setproctitle ("unreachable poller [sleeping for %d seconds]", sleep);
		else
			zbx_setproctitle ("unreachable poller [getting values]");
		break;
	case ZBX_POLLER_TYPE_NORMAL:
		if (sleep)
			zbx_setproctitle ("poller [sleeping for %d seconds]", sleep);
		else
			zbx_setproctitle ("poller [getting values]");
		break;
	case ZBX_POLLER_TYPE_AGGREGATE:
		if (sleep)
			zbx_setproctitle ("aggregate poller [sleeping for %d seconds]", sleep);
		else
			zbx_setproctitle ("aggregate poller [getting values]");
		break;
	default:
		zbx_setproctitle ("unknown poller doing something unknown");
	}
}


void main_poller_loop(int type, int num)
{
	int	now;
	int	nextcheck,sleeptime;
	int*	plan;
	int	plan_size, plan_pos;

	zabbix_log( LOG_LEVEL_DEBUG, "In main_poller_loop(type:%d,num:%d)",
		type,
		num);

	poller_type = type;
	poller_num = num;

	if (poller_type == ZBX_POLLER_TYPE_UNREACHABLE) {
		key_host_updates = metric_register ("poller_unr_host_updates",  num);
		key_get_values = metric_register ("poller_unr_get_values",  num);
		key_items_count = metric_register ("poller_unr_items_count",  num);
	}
	else {
		key_host_updates = metric_register ("poller_host_updates",  num);
		key_get_values = metric_register ("poller_get_values",  num);
		key_items_count = metric_register ("poller_items_count",  num);
	}

	DBconnect(ZBX_DB_CONNECT_NORMAL);

	plan_pos = plan_size = 0;
	plan = NULL;

	for(;;)
	{	
		if (plan_pos == plan_size) {
			if (plan)
				free (plan);
			plan = NULL;
			plan_size = get_nextchecks (&plan);
			plan_pos = 0;
			if (!plan_size) {
				/* if plan for interval is empty, sleep */
				update_poller_title (poller_type, POLLER_GROUP_INTERVAL);
				sleep (POLLER_GROUP_INTERVAL);
				continue;
			}
		}

		nextcheck = plan[plan_pos++];

		while (plan_pos < plan_size)
			if (nextcheck == plan[plan_pos])
				plan_pos++;
			else
				break;

		sleeptime = nextcheck - time (NULL);
		if(sleeptime < 0)
			sleeptime = 0;

		if (sleeptime > 0) {
			update_poller_title (poller_type, sleeptime);
			sleep (sleeptime);
		}

		update_poller_title (poller_type, 0);
		get_values();
	}
}
