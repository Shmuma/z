#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "hfs.h"
#include "hfs_internal.h"

char *progname = "test";
char title_message[] = "Title";
char usage_message[] = "Usage";
char *help_message[] = { "Help", 0 };


int main (int argc, char** argv)
{
	hfs_meta_t* meta;

	/* read given trends file */
	if (argc != 2) {
		printf ("Usage: hfs_trends_upd trends.meta\n");
		return 1;
	}

	meta = read_metafile (argv[1]);

	if (!meta) {
		printf ("Cannot read trend's metafile\n");
		return 1;
	}

	return 0;
}
