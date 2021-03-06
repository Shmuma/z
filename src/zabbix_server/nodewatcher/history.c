/* 
** ZABBIX
** Copyright (C) 2000-2006 SIA Zabbix
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

#include "cfg.h"
#include "db.h"
#include "log.h"
#include "zlog.h"

#include "nodecomms.h"
#include "nodesender.h"
#include "history.h"

/******************************************************************************
 *                                                                            *
 * Function: process_node_history_log                                         *
 *                                                                            *
 * Purpose: process new history_log data                                      *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: SUCCESS - processed succesfully                              * 
 *               FAIL - an error occured                                      *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
static int process_node_history_log(int nodeid, int master_nodeid)
{
	const char	*ids_table_name = {"history_log"};
	const char	*ids_field_name = {"sync_lastid"};
	DB_RESULT	result;
	DB_ROW		row;
	char		sql[MAX_STRING_LEN];
	int		ids_found = 0, found = 0;
	int		offset = 0;
	char		*data = NULL, *hex = NULL;
	int		allocated = 1024*1024, hex_allocated = 1024;
	zbx_uint64_t	sync_lastid = 0, id, len;

	zabbix_log( LOG_LEVEL_DEBUG, "In process_node_history_log(nodeid:%d, master_nodeid:%d)",
		nodeid,
		master_nodeid);

	data = zbx_malloc(data, allocated);
	hex = zbx_malloc(hex, hex_allocated);

	result = DBselect("select nextid from ids where nodeid=%d and table_name='%s' and field_name='%s'",
				nodeid,
				ids_table_name,
				ids_field_name);

	if((row=DBfetch(result)))
	{
		ZBX_STR2UINT64(sync_lastid,row[0])
		ids_found = 1;
	}
	DBfree_result(result);

	zbx_snprintf_alloc(&data, &allocated, &offset, 64, "History%c%d%c%d",
		ZBX_DM_DELIMITER,
		CONFIG_NODEID,
		ZBX_DM_DELIMITER,
		nodeid);

	zbx_snprintf(sql,sizeof(sql),"select id,itemid,clock,timestamp,source,severity,value,length(value) from history_log where id>"ZBX_FS_UI64" order by id",
		sync_lastid);

	result = DBselectN(sql, 10000);
	while((row=DBfetch(result)))
	{
		ZBX_STR2UINT64(id,row[0])
		found = 1;

		len = atoi(row[7]);
		zbx_binary2hex((u_char *)row[6], len, &hex, &hex_allocated);

		zbx_snprintf_alloc(&data, &allocated, &offset, len * 2 + 256, "\n%d%c%s%c%s%c%s%c%s%c%s%c%s%c%s",
				ZBX_TABLE_HISTORY_LOG, ZBX_DM_DELIMITER,
				row[1], ZBX_DM_DELIMITER,	/* itemid */
				row[2], ZBX_DM_DELIMITER,	/* clock */
				row[0], ZBX_DM_DELIMITER,	/* id */
				row[3], ZBX_DM_DELIMITER,	/* timestamp */
				row[4], ZBX_DM_DELIMITER,	/* source */
				row[5], ZBX_DM_DELIMITER,	/* severity */
				hex);				/* value */
	}
	if(found == 1)
	{
		if(send_to_node("new history_log", master_nodeid, nodeid, data) == SUCCEED)
		{
			if(ids_found == 1)
			{
				DBexecute("update ids set nextid="ZBX_FS_UI64" where nodeid=%d and table_name='%s' and field_name='%s'",
					id,
					nodeid,
					ids_table_name,
					ids_field_name);
			}
			else
			{
				DBexecute("insert into ids (nodeid,table_name,field_name,nextid) values (%d,'%s','%s',"ZBX_FS_UI64")",
					nodeid,
					ids_table_name,
					ids_field_name,
					id);
			}
		}
		else
		{
			zabbix_log( LOG_LEVEL_DEBUG, "process_node_history_log() FAIL");
		}
	}
	DBfree_result(result);

	zbx_free(data);
	zbx_free(hex);

	return SUCCEED;
}

/******************************************************************************
 *                                                                            *
 * Function: process_node_history_str                                         *
 *                                                                            *
 * Purpose: process new history_str data                                      *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: SUCCESS - processed succesfully                              * 
 *               FAIL - an error occured                                      *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
static int process_node_history_str(int nodeid, int master_nodeid)
{
	DB_RESULT	result;
	DB_ROW		row;
	char		*data = NULL;
	char		sql[MAX_STRING_LEN];
	int		found = 0;
	int		offset = 0;
	int		allocated = 1024*1024;

	zbx_uint64_t	id;

	zabbix_log( LOG_LEVEL_DEBUG, "In process_node_history_str(nodeid:%d, master_nodeid:%d",
		nodeid,
		master_nodeid);
	/* Begin work */

	data = zbx_malloc(data, allocated);
	memset(data,0,allocated);

	zbx_snprintf_alloc(&data, &allocated, &offset, 128, "History%c%d%c%d",
		ZBX_DM_DELIMITER,
		CONFIG_NODEID,
		ZBX_DM_DELIMITER,
		nodeid);

	zbx_snprintf(sql,sizeof(sql),"select id,itemid,clock,value from history_str_sync where nodeid=%d order by id",
		nodeid);

	result = DBselectN(sql, 10000);
	while((row=DBfetch(result)))
	{
		ZBX_STR2UINT64(id,row[0])
		found = 1;
		zbx_snprintf_alloc(&data, &allocated, &offset, 1024, "\n%d%c%s%c%s%c%s",
				ZBX_TABLE_HISTORY_STR,
				ZBX_DM_DELIMITER,
				row[1],
				ZBX_DM_DELIMITER,
				row[2],
				ZBX_DM_DELIMITER,
				row[3]);
	}
	if(found == 1)
	{
		/* Do not send history for current node if CONFIG_NODE_NOHISTORY is set */
		if(send_to_node("new history_str", master_nodeid, nodeid, data) == SUCCEED)
		{
			DBexecute("delete from history_str_sync where nodeid=%d and id<=" ZBX_FS_UI64,
				nodeid,
				id);
		}
		else
		{
			zabbix_log( LOG_LEVEL_DEBUG, "process_node_history_str() FAIL");
		}
	}
	DBfree_result(result);
	zbx_free(data);

	return SUCCEED;
}

/******************************************************************************
 *                                                                            *
 * Function: process_node_history_uint                                        *
 *                                                                            *
 * Purpose: process new history_uint data                                     *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: SUCCESS - processed succesfully                              * 
 *               FAIL - an error occured                                      *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
static int process_node_history_uint(int nodeid, int master_nodeid)
{
	DB_RESULT	result;
	DB_ROW		row;
	char		*data=  NULL;
	char		sql[MAX_STRING_LEN];
	int		found = 0;
	int		offset = 0;
	int		allocated = 1024*1024;

	int start, end;

	zbx_uint64_t	id;

	zabbix_log( LOG_LEVEL_DEBUG, "In process_node_history_uint(nodeid:%d, master_nodeid:%d)",
		nodeid,
		master_nodeid);
	/* Begin work */

	start = time(NULL);

	data = zbx_malloc(data, allocated);
	memset(data,0,allocated);

	zbx_snprintf_alloc(&data, &allocated, &offset, 128, "History%c%d%c%d",
		ZBX_DM_DELIMITER,
		CONFIG_NODEID,
		ZBX_DM_DELIMITER,
		nodeid);

	zbx_snprintf(sql,sizeof(sql),"select id,itemid,clock,value from history_uint_sync where nodeid=%d order by id",
		nodeid);

	result = DBselectN(sql, 10000);
	while((row=DBfetch(result)))
	{
		ZBX_STR2UINT64(id,row[0])
		found = 1;
		zbx_snprintf_alloc(&data, &allocated, &offset, 128, "\n%d%c%s%c%s%c%s",
				ZBX_TABLE_HISTORY_UINT,
				ZBX_DM_DELIMITER,
				row[1],
				ZBX_DM_DELIMITER,
				row[2],
				ZBX_DM_DELIMITER,
				row[3]);
	}
	if(found == 1)
	{
		/* Do not send history for current node if CONFIG_NODE_NOHISTORY is set */
		if(send_to_node("new history_uint", master_nodeid, nodeid, data) == SUCCEED)
		{
			DBexecute("delete from history_uint_sync where nodeid=%d and id<=" ZBX_FS_UI64,
				nodeid,
				id);
		}
		else
		{
			zabbix_log( LOG_LEVEL_DEBUG, "process_node_history_uint() FAIL");
		}
	}
	DBfree_result(result);
	zbx_free(data);

	end = time(NULL);

	zabbix_log( LOG_LEVEL_DEBUG, "Spent %d seconds in process_node_history_uint",
		end-start);

	return SUCCEED;
}

/******************************************************************************
 *                                                                            *
 * Function: process_node_history                                             *
 *                                                                            *
 * Purpose: process new history data                                          *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: SUCCESS - processed succesfully                              * 
 *               FAIL - an error occured                                      *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
static int process_node_history(int nodeid, int master_nodeid)
{
	DB_RESULT	result;
	DB_ROW		row;
	char		*data = NULL;
	char		sql[MAX_STRING_LEN];
	int		found = 0;
	int		offset = 0;
	int		allocated = 1024*1024;

	int		start, end;

	zbx_uint64_t	id;

	zabbix_log( LOG_LEVEL_DEBUG, "In process_node_history(nodeid:%d, master_nodeid:%d",
		nodeid,
		master_nodeid);
	/* Begin work */
	start = time(NULL);

	data = zbx_malloc(data, allocated);
	memset(data,0,allocated);

	zbx_snprintf_alloc(&data, &allocated, &offset, 128, "History%c%d%c%d",
		ZBX_DM_DELIMITER,
		CONFIG_NODEID,
		ZBX_DM_DELIMITER,
		nodeid);

	zbx_snprintf(sql,sizeof(sql),"select id,itemid,clock,value from history_sync where nodeid=%d order by id",
		nodeid);

	result = DBselectN(sql, 10000);
	while((row=DBfetch(result)))
	{
		ZBX_STR2UINT64(id,row[0])
		found = 1;
		zbx_snprintf_alloc(&data, &allocated, &offset, 128, "\n%d%c%s%c%s%c%s",
				ZBX_TABLE_HISTORY,
				ZBX_DM_DELIMITER,
				row[1],
				ZBX_DM_DELIMITER,
				row[2],
				ZBX_DM_DELIMITER,
				row[3]);
	}
	if(found == 1)
	{
		zabbix_log( LOG_LEVEL_DEBUG, "Sending [%s]",
			data);
		/* Do not send history for current node if CONFIG_NODE_NOHISTORY is set */
		if(send_to_node("new history", master_nodeid, nodeid, data) == SUCCEED)
		{
			DBexecute("delete from history_sync where nodeid=%d and id<=" ZBX_FS_UI64,
				nodeid,
				id);
		}
		else
		{
			zabbix_log( LOG_LEVEL_DEBUG, "process_node_history() FAIL");
		}
	}
	DBfree_result(result);
	zbx_free(data);

	end = time(NULL);

	zabbix_log( LOG_LEVEL_DEBUG, "Spent %d seconds in process_node_history",
		end-start);

	return SUCCEED;
}

/******************************************************************************
 *                                                                            *
 * Function: process_node                                                     *
 *                                                                            *
 * Purpose: process all history tables for this node                          *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: SUCCESS - processed succesfully                              * 
 *               FAIL - an error occured                                      *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
static void process_node(int nodeid, int master_nodeid)
{
	zabbix_log( LOG_LEVEL_DEBUG, "In process_node(local:%d, master_nodeid:" ZBX_FS_UI64 ")",
		nodeid,
		master_nodeid);

	process_node_history(nodeid, master_nodeid);
	process_node_history_uint(nodeid, master_nodeid);
	process_node_history_str(nodeid, master_nodeid);
	process_node_history_log(nodeid, master_nodeid);
}

/******************************************************************************
 *                                                                            *
 * Function: main_historysender                                               *
 *                                                                            *
 * Purpose: periodically sends historical data to master node                 *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value:                                                              * 
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
void main_historysender()
{
	DB_RESULT	result;
	DB_ROW		row;
	int		nodeid;
	int		master_nodeid;

	zabbix_log( LOG_LEVEL_DEBUG, "In main_historysender()");

	DBbegin();

	master_nodeid = get_master_node(CONFIG_NODEID);

	if(CONFIG_MASTER_IP)
	{
			process_node(0, 0);
	}

	DBcommit();
}
