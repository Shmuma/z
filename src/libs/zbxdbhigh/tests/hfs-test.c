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
int from=1209373653, to=1209377253;

int main(void) {
	int i;
	size_t n;
	hfs_item_value_t *res = NULL;
	
	n = HFSread_item("/tmp/hfs", sizex, itemid, from, to, &res);

	for (i=0; i<n; i++)
		printf("res[%d]= clock=%d max=%f min=%f\n", res[i].group, res[i].clock, res[i].max.d, res[i].min.d);

	printf("results = %d\n", n);

	if (res)
		free(res);

	res = NULL;
	return 1;
}
