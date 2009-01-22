#include "common.h"
#include "cfg.h"
#include "db.h"

#include "zlog.h"

#include "aggr_slave.h"


extern char* CONFIG_SERVER_SITE;


int proc_num;


/* build plan with all needed information for 2 minutes of activity */
static void build_checks_plan (int period)
{
	DB_RESULT result;
	DB_ROW row;

	result = DBselect ("select i.itemid, i.delay, i.key_ from items i, hosts h where " ZBX_SQL_MOD(i.itemid,%d) 
			   "=%d and i.status=%d and i.type=%d and h.hostid=i.hostid and h.status=%d",
			   CONFIG_AGGR_SLAVE_FORKS,
			   proc_num,
			   ITEM_STATUS_ACTIVE,
			   ITEM_TYPE_AGGREGATE,
			   HOST_STATUS_MONITORED);

	while (row = DBfetch (result)) {

	}

	DBfree_result (result);
	return;
}


/* Tasks for aggregate slave are simple: calculate aggregate values
   for hosts local to server's site and save it's value to HFS. */
void main_aggregate_slave_loop (int procnum)
{
	proc_num = procnum;

	DBconnect(ZBX_DB_CONNECT_NORMAL);

	while (1) {
		/* build checks plan for 2 minutes */
		build_checks_plan ();
		sleep (10);
	}
}

