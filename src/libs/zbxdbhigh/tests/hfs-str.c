#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "hfs.h"


int itemid = 18480;


int main(void) {
	size_t n;
	time_t now = time (NULL);
	hfs_item_str_value_t* res;

	n = HFSread_item_str("/tmp/hfs", "Default", itemid, now-600, now+600, &res);

	printf("results = %d\n", n);

	if (res)
		free (res);

	return 0;
}
