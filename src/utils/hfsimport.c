#include "config.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hfs.h"
#include "hfs_internal.h"

char *progname = "test";
char title_message[] = "Title";
char usage_message[] = "Usage";
char *help_message[] = { "Help", 0 };

int main(int argc, char **argv)
{
	FILE * fp;
	struct stat sb;
	size_t len;
	ssize_t read;
	char *line = NULL;
	char *outdir = NULL;
	char *dumpfile = NULL;
	const char datafile[MAXPATHLEN];
	const char metafile[MAXPATHLEN];
	int rc = EXIT_SUCCESS;

	if (argc != 3) {
		printf("Usage %s <outdir> <hfs-dump-file>\n", argv[0]);
		return EXIT_SUCCESS;
	}

	if (stat(argv[1], &sb) == -1) {
		perror("stat");
		return EXIT_FAILURE;
	}

	if (!S_ISDIR(sb.st_mode)) {
		fprintf(stderr, "%s: first argument should be a directory.\n", argv[1]);
		return EXIT_FAILURE;
	}

	outdir = argv[1];

	if (stat(argv[2], &sb) == -1) {
		perror("stat");
		return EXIT_FAILURE;
	}

	if (!S_ISREG(sb.st_mode)) {
		fprintf(stderr, "%s: first argument should be a regular file.\n", argv[2]);
		return EXIT_FAILURE;
	}

	dumpfile = argv[2];

	if ((fp = fopen(dumpfile, "r")) == NULL) {
		perror("fopen");
		return EXIT_FAILURE;
	}

	snprintf(metafile, MAXPATHLEN, "%s/history.meta", outdir);
	snprintf(datafile, MAXPATHLEN, "%s/history.data", outdir);

	if (access(metafile, R_OK) == 0)
		unlink(metafile);

	if (access(datafile, R_OK) == 0)
		unlink(datafile);

	printf("Metafile: %s\nDatafile: %s\n", metafile, datafile);

	while ((read = getline(&line, &len, fp)) != -1) {
		char *ch, *start;
		hfs_time_t stamp = -1;
		int rc, type = -1, delay = -1;
		item_value_u value;

		start = line;
		line[read-1] = '\0';

		// Expected: time=1217851317 delay=30 type=0 value=1
		while (start != NULL) {
			if ((ch = strchr(start, '\t')) != NULL)
				*ch = '\0';

			if (strncmp("time=", start, 5) == 0)
				sscanf(start, "time=%lld", &stamp);
			else if (strncmp("delay=", start, 6) == 0)
				sscanf(start, "delay=%d", &delay);
			else if (strncmp("type=", start, 5) == 0)
				sscanf(start, "type=%d", &type);
			else if (strncmp("value=", start, 6) == 0) {
				if (type == -1 || delay == -1 || stamp == -1) {
					fprintf(stderr, "Wrong value order!\n");
					goto out;
				}

				if (type == IT_UINT64)
					sscanf(start, "value=%lld", &value.l);
				else if (type == IT_DOUBLE)
					sscanf(start, "value=%f", &value.d);

				rc = hfs_store_value(metafile, datafile, stamp, delay,
						     &value, sizeof(item_value_u), type);

				if (rc != 0)
					fprintf(stderr, "hfs_store_value() == %d\n", rc);

//				printf("TIME: %lld\n", stamp);
//				printf("TYPE: %d\n", type);
//				printf("DELAY: %d\n", delay);
//				printf("VALUE: %lld\n", value);
			}

			start = (ch != NULL) ? (ch + 1) : NULL;
		}
	}
out:
	if (line)
		free(line);

	fclose(fp);

	return rc;
}
