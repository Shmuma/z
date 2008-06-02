#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "hfs.h"

char *progname = "test";
char title_message[] = "Title";
char usage_message[] = "Usage";
char *help_message[] = { "Help", 0 };

int itemid = 19481;
int sizex = 908;
int graph_from=1210861248, graph_to=1210864848, from=1210861248, to=1210864848;

int main(void) {
	int i;
	size_t n;
	hfs_item_value_t *res = NULL;
//	itemid = 19480;
//	sizex = 908;
//	from = 1210240437;
//	to = 1210244037;

	itemid=19537;
	graph_from=1212148001;
	graph_to=1212151601;
	from=1212148001;
	to=1212151601;

	n = HFSread_item("/tmp/hfs", sizex, itemid, graph_from, graph_to, from, to, &res);

	for (i=0; i<n; i++)
		printf("res[%d] group=%d clock=%d max=%lld min=%lld\n", i, res[i].group, res[i].clock, res[i].max.l, res[i].min.l);
//		printf("res[%d] group=%d clock=%d max=%f min=%f\n", i, res[i].group, res[i].clock, res[i].max.d, res[i].min.d);

	printf("results = %d\n", n);
	printf ("Cur timestamp: %d\n", to);

	if (res)
		free(res);

	res = NULL;
	return 1;
}
