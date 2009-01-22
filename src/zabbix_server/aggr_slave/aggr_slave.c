#include "common.h"

#include "zlog.h"

#include "aggr_slave.h"


extern char* CONFIG_SERVER_SITE;


int proc_num;


/* Tasks for aggregate slave are simple: calculate aggregate values
   for hosts local to server's site and save it's value to HFS. */
void main_aggregate_slave_loop (int procnum)
{
	proc_num = procnum;

	DBconnect(ZBX_DB_CONNECT_NORMAL);

	while (1) {
		sleep (10);
	}
}

