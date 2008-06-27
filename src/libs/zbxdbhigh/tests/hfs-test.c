#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "hfs.h"

char *progname = "test";
char title_message[] = "Title";
char usage_message[] = "Usage";
char *help_message[] = { "Help", 0 };

int itemid, sizex, graph_from, graph_to, from, to;

int main(void) {
	int i;
	size_t n;
	hfs_item_value_t *res = NULL;

	sizex = 908;
	itemid=18527;
	graph_from=1214564101;
	  graph_to=1214567701;
	      from=1214564101;
	        to=1214567701;

	n = HFSread_item("/tmp/hfs", "Default", 0, itemid, sizex, graph_from, graph_to, from, to, &res);

	for (i=0; i<n; i++)
		printf("res[%d] group=%d clock=%d max=%lld min=%lld\n", i, res[i].group, res[i].clock, res[i].value.max.l, res[i].value.min.l);
//		printf("res[%d] group=%d clock=%d max=%f min=%f\n", i, res[i].group, res[i].clock, res[i].value.max.d, res[i].value.min.d);

	printf("results = %d\n", n);
	printf ("Cur timestamp: %d\n", to);

	if (res)
		free(res);

	res = NULL;
	return 1;
}
