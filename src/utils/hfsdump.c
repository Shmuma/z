#include "config.h"

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

char *get_datafile(const char *metafile)
{
	char *res = strdup(metafile);
	char *s = strrchr(res, '.');
	snprintf((s+1),5,"data");
	return res;
}

void show_value(item_type_t type, item_value_u val)
{
	switch (type) {
		case IT_UINT64:
			printf("%lld", val.l);
			break;

		case IT_DOUBLE:
		case IT_TRENDS:
			printf("%f", val.d);
			break;
		default:
			break;
	}
}

int dump_by_meta(const char *metafile)
{
	void 		*val;
	size_t		val_len;
	item_value_u 	val_history;
	hfs_trend_t  	val_trends;

	int fd, i;
	char *datafile = NULL;
	hfs_meta_t *meta = NULL;
	hfs_meta_item_t *ip = NULL;
	hfs_time_t ts;
	hfs_off_t ofs;

	if ((meta = read_metafile(metafile)) == NULL)
		return -1; // Somethig real bad happend :(

	if (meta->blocks == 0) {
		fprintf(stderr, "%s: No data!\n", metafile);
		free_meta(meta);
		return -1;
	}

	fprintf(stderr, "Metafile: %s\n", metafile);
	if (is_trend_type(meta->last_type)) {
		val = &val_trends;
		val_len = sizeof(val_trends);
	}
	else {
		val = &val_history;
		val_len = sizeof(val_history);
	}

	datafile = get_datafile(metafile);     
	fprintf(stderr, "Datafile: %s\n", datafile);

	if ((fd = open (datafile, O_RDONLY)) == -1) {
		fprintf(stderr, "%s: file open failed: %s", datafile, strerror(errno));
		free_meta(meta);
		free(datafile);
		return -1;
	}
	free(datafile);

	for (i = 0; i < meta->blocks; i++) {
		ip = meta->meta + i;
		ts = ip->start;

		if ((ofs = find_meta_ofs (ts, meta)) == -1) {
			fprintf(stderr, "%s: %d: unable to get offset in file",
				datafile, (int)ts);
			free_meta(meta);
			return -1;
		}

		if (lseek (fd, ofs, SEEK_SET) == -1) {
			fprintf(stderr, "%s: unable to change file offset: %s",
				datafile, strerror(errno));
			free_meta(meta);
			return -1;
		}

		while (read (fd, val, val_len) > 0) {

			if (!is_valid_val(val, val_len)) {
				ts += ip->delay;
				continue;
			}

			printf("time=%d\tdelay=%d\ttype=%d\t",
				(int)ts, ip->delay, ip->type);

			if (is_trend_type(ip->type)) {
				printf("count=%d\tmax=", val_trends.count);
				show_value(ip->type, val_trends.max);
				printf("\tmin=");
				show_value(ip->type, val_trends.min);
				printf("\tavg=");
				show_value(ip->type, val_trends.avg);
			}
			else {
				printf("value=");
				show_value(ip->type, val_history);
			}
			printf("\n");
			ts += ip->delay;

			if (ts > ip->end)
				break;
		}
	}

	free_meta(meta);
	return 0;
}

int main(int argc, char **argv)
{
	int i;

	if (argc == 1) {
		printf("Usage %s <meta-file> [<meta-file1>...]\n", argv[0]);
		return EXIT_SUCCESS;
	}

	for (i = 1; i < argc; i++) {
		if (dump_by_meta(argv[i]) == -1)
			fprintf(stderr,"%s: Bad meta!\n", argv[i]);
	}

	return EXIT_SUCCESS;
}
