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

int parse_hfs_path(char *path,
                   char **basedir, char **site, zbx_uint64_t *item)
{
	int i = 0;
	char *s;
	struct stat sb;

	if (stat(path, &sb) == -1) {
		perror("stat");
		return (EXIT_FAILURE);
        }

	if (!S_ISREG(sb.st_mode)) {
		fprintf(stderr, "%s: Not regular file\n", path);
		return (EXIT_FAILURE);
	}

	/*         i=3     i=2   i=1   i=0  
	   /tmp/hfs/Default/items/18534/1216.meta
	   ^------- ^------       ^----
	    basedir  site          item
	*/
	while ((s = strrchr(path, '/')) != NULL) {
		switch (i) {
			case 1: *item = atoi(s+1);
				break;
			case 3: *site = strdup(s+1);
				break;
			case 4: *basedir = strdup(path);
				break;
		}
		*s = '\0';
		i++;
	}

	return EXIT_SUCCESS;
}

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
		case IT_TRENDS_UINT64:
			printf("%lld", val.l);
			break;

		case IT_DOUBLE:
		case IT_TRENDS_DOUBLE:
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
	zbx_uint64_t ts;
	zbx_uint64_t ofs;

	if ((meta = read_metafile(metafile)) == NULL)
		return -1; // Somethig real bad happend :(

	if (meta->blocks == 0) {
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
			ts += ip->delay;

			if (!is_valid_val(val, val_len))
				continue;

			printf("time=%d\ttype=%d\t", (int)ts, ip->type);

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
			
			if (ts == ip->end)
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
		printf("Usage %s <meta-file>\n", argv[0]);
		return EXIT_SUCCESS;
	}

	for (i = 1; i < argc; i++) {
		if (dump_by_meta(argv[i]) == -1)
			fprintf(stderr,"%s: Bad meta!\n", argv[i]);
	}

	return EXIT_SUCCESS;
}
