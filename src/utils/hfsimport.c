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


/* append value to bufer (resize it if needed) */
void append_value (item_value_u** buf, int* buf_size, int* count, item_value_u* val)
{
    if (*buf_size == *count) {
        *buf_size += 256;
        *buf = (item_value_u*)realloc (*buf, (*buf_size) * sizeof (item_value_u));
    }

    (*buf)[*count] = *val;
    *count += 1;
}


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
        item_value_u* values = NULL;
        int values_buf = 0;
        int values_count = 0;

        int o_type, o_delay;
        hfs_time_t o_stamp, b_stamp;

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

	if (strcmp("-", argv[2]) != 0) {
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
	}
	else {
		fp = stdin;
	}

	snprintf(metafile, MAXPATHLEN, "%s/history.meta", outdir);
	snprintf(datafile, MAXPATHLEN, "%s/history.data", outdir);

	if (access(metafile, R_OK) == 0)
		unlink(metafile);

	if (access(datafile, R_OK) == 0)
		unlink(datafile);

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
					rc = sscanf(start, "value=%lld", &value.l);
				else if (type == IT_DOUBLE)
					rc = sscanf(start, "value=%lf", &value.d);

                                /* check that we must start a new block */
                                if (values_count) {
                                    if (delay != o_delay || o_type != type || o_stamp > stamp) {
                                        /* dump old data */
                                        rc = hfs_store_values(metafile, datafile, b_stamp, o_delay,
                                                              values, sizeof(item_value_u), values_count, o_type);
                                        values_count = 0;
                                        o_type = type;
                                        o_delay = delay;
                                        b_stamp = stamp;
                                    }

                                    /* check that data value appeared before it's time */
                                    if (stamp - o_stamp < delay)
                                        continue; /* drop data item */
                                    if (stamp - o_stamp > delay) {
                                        /* append needed amount of invalid data values */
                                        item_value_u v;
                                        int i;
                                        hfs_time_t extra = (stamp - o_stamp) / delay;
                                        v.l = 0xFFFFFFFFFFFFFFFFULL;

                                        for (i = 0; i < extra-1; i++)
                                            append_value (&values, &values_buf, &values_count, &v);
                                    }
                                }
                                else {
                                    o_type = type;
                                    o_delay = delay;
                                    b_stamp = stamp;
                                }

                                o_stamp = stamp;

                                append_value (&values, &values_buf, &values_count, &value);

//				printf("TIME: %lld\n", stamp);
//				printf("TYPE: %d\n", type);
//				printf("DELAY: %d\n", delay);
//				printf("VALUE: %lld\n", value);
			}

			start = (ch != NULL) ? (ch + 1) : NULL;
		}
	}

        /* dump data (if any) */
        if (values_count)
            hfs_store_values(metafile, datafile, b_stamp, o_delay,
                             values, sizeof(item_value_u), values_count, o_type);
        if (values)
            free (values);
out:
	if (line)
		free(line);

	fclose(fp);

	return rc;
}
