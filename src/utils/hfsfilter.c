#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "hfs.h"
#include "hfs_internal.h"


int main (int argc, char** argv)
{
	hfs_meta_t* meta;
	char* path;
	int i;
	hfs_off_t ofs;
	char* data;
	int fd;
	item_value_u val, max;
	hfs_time_t ts;
	int clear, count, cl_count;
	unsigned long long inval = 0xFFFFFFFFFFFFFFFFULL;

	if (argc != 2) {
		printf ("Usage: hfsfilter history.meta\n");
		return 1;
	}

	path = argv[1];

	meta = read_metafile (path);

	if (!meta || !meta->blocks)
		return 1;

	data = strdup (path);

	i = strlen (data);
	data[i-4] = 'd';
	data[i-3] = 'a';

	fd = open (data, O_RDWR);
	cl_count = 0;

	for (i = 0; i < meta->blocks; i++) {
		ofs = meta->meta[i].ofs;
		ts = meta->meta[i].start;
		lseek (fd, ofs, SEEK_SET);
		count = 0;

		if (meta->meta[i].type == IT_DOUBLE)
			max.d = 0;
		else
			max.l = 0;

		while (ts <= meta->meta[i].end) {
			read (fd, &val, sizeof (val));

			if (is_valid_val (&val, sizeof (val))) {
				clear = 0;
				if (meta->meta[i].type == IT_DOUBLE) {
					if (count > 1000)
						if (val.d < 0 || val.d > (max.d * 1000000.0))
							clear = 1;
					if (!clear)
						if (max.d < val.d)
							max.d = val.d;
				}
				else {
					if (count > 1000)
						if (val.l > (max.l * 1000000))
							clear = 1;
					if (!clear)
						if (max.l < val.l)
							max.l = val.l;
				}

				if (clear) {
					lseek (fd, ofs, SEEK_SET);
					write (fd, &inval, sizeof (inval));
					cl_count++;
				}

				count++;
			}

			ts += meta->meta[i].delay;
			ofs += sizeof (val);
		}
	}

	close (data);
	printf ("%d items cleared\n", cl_count);

	return cl_count ? 0 : 1;
}
