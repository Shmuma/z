#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "hfs.h"
#include "hfs_internal.h"


/* utility imports data from events into HFS structures */
/* Data is read from stdin and expected to be from the following command:
   mysql zabbix -b -N -e 'select e.eventid, e.objectid, e.clock, e.value, e.acknowledged, i.hostid from events e, functions f, items i where e.objectid = f.triggerid  and f.itemid = i.itemid'
 */

int main (int argc, char** argv)
{
	zbx_uint64_t eventid, triggerid;
	hfs_time_t clock;
	int val, ack;
	zbx_uint64_t hostid;

	if (argc != 3) {
		printf ("Usage: hfsevents hfs_path site\n");
		return 0;
	}

	while (scanf ("%lld %lld %lld %d %d %lld", &eventid, &triggerid, &clock, &val, &ack, &hostid) == 6) {
		HFS_add_event (argv[1], argv[2], eventid, triggerid, clock, val, ack, hostid);
	}

	return 0;
}
