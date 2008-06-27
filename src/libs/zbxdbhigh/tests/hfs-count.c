#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "hfs.h"

char *progname = "test";
char title_message[] = "Title";
char usage_message[] = "Usage";
char *help_message[] = { "Help", 0 };

int itemid = 19537;

void hfs_last_functor (item_type_t type, item_value_u val, time_t timestamp, void *return_value)
{
	if (type == IT_DOUBLE)
		printf("value=%f\n", val.d);
	else
		printf("value=%d\n", val.l);
}

int main(void) {
	int i;
	size_t n;

	n = HFSread_count("/tmp/hfs", "Default", itemid, 10, NULL, hfs_last_functor);

	printf("results = %d\n", n);

	return 1;
}
