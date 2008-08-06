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

char *progname = "test";
char title_message[] = "Title";
char usage_message[] = "Usage";
char *help_message[] = { "Help", 0 };


typedef struct {
    hfs_time_t ts;
    item_type_t type;
    item_value_u val;
} hfs_data_item_t;


typedef struct {
    hfs_time_t ts;
    hfs_trend_t value;
} hfs_trend_item_t;


hfs_data_item_t* read_data (hfs_meta_t* meta, hfs_time_t from, const char* path, int* count);
hfs_trend_item_t* process_trends (hfs_data_item_t* data, int count, int* tr_count);

void store_trends (const char* item_dir, hfs_trend_item_t* trends, int count);


int main (int argc, char** argv)
{
	hfs_meta_t* meta;
	hfs_meta_t* trends_meta;
	const char* item_dir;
	hfs_time_t start_ts;
	char path[1024];
	hfs_data_item_t* values;
	int count, tr_count;
	hfs_trend_item_t* trends;

	/* read given trends file */
	if (argc != 2) {
		printf ("Usage: hfs_trends_upd item_dir\n");
		return 1;
	}

	item_dir = argv[1];

	zbx_snprintf (path, sizeof (path), "%s/trends.meta", item_dir);
	trends_meta = read_metafile (path);

	if (trends_meta && trends_meta->blocks) {
		start_ts = trends_meta->meta[trends_meta->blocks-1].end;
		free_meta (trends_meta);
	}
	else
		start_ts = 0;

	/* round trend by 1 hour */
	start_ts -= start_ts % 3600;

	zbx_snprintf (path, sizeof (path), "%s/history.meta", item_dir);
	meta = read_metafile (path);

        if (meta) {
		if (meta->blocks) {
			/* have meta, check trends */
			if (start_ts < meta->meta[meta->blocks-1].end) {
				/* fetch data from this meta and calculate trends */
				zbx_snprintf (path, sizeof (path), "%s/history.data", item_dir);
				values = read_data (meta, start_ts ? start_ts : meta->meta[0].start, path, &count);
				printf ("New data found, %d values\n", count);

				if (values) {
					/* calculate trends */
					trends = process_trends (values, count, &tr_count);
					printf ("Calculated %d trend values\n", tr_count);

					/* save trends */
					store_trends (item_dir, trends, tr_count);

					free (trends);
					free (values);
				}
			}
		}

		free_meta (meta);
        }

	return 0;
}



hfs_data_item_t* read_data (hfs_meta_t* meta, hfs_time_t from, const char* path, int* count)
{
    hfs_data_item_t* res = NULL;
    int size, buf_size;
    int fd;
    hfs_off_t ofs;
    int block, cur_block;

    size = buf_size = 0;
    *count = 0;
    ofs = find_meta_ofs (from, meta);

    /* find meta block */
    block = 0;
    cur_block = -1;
    while (block < meta->blocks) {
        if (meta->meta[block].end >= from) {
            cur_block = block;
            break;
        }

        block++;
    }

    if (cur_block < 0)
        return NULL;

    fd = open (path, O_RDONLY);

    if (fd < 0)
        return NULL;

    lseek (fd, ofs, SEEK_SET);

    while (cur_block < meta->blocks) {
        while (from <= meta->meta[cur_block].end) {
            if (size == buf_size) {
                buf_size += 256;
                res = (hfs_data_item_t*)realloc (res, buf_size * sizeof (hfs_data_item_t));
            }

            if (read (fd, &res[size].val, sizeof (item_value_u)) == sizeof (item_value_u))  {
                res[size].ts = from;
                res[size].type = meta->meta[cur_block].type;
                if (is_valid_val (&res[size].val, sizeof (item_value_u)))
                    size++;
            }
            from += meta->meta[cur_block].delay;
        }

        cur_block++;
        if (cur_block < meta->blocks)
            from = meta->meta[cur_block].start;
    }

    *count = size;
    close (fd);

    return res;
}


hfs_trend_item_t* process_trends (hfs_data_item_t* data, int count, int* tr_count)
{
    hfs_trend_item_t* res = NULL;
    int size, buf_size, i;
    hfs_time_t ts = data[0].ts;
    hfs_time_t hr = ts / 3600;
    hfs_trend_t trend;

    size = buf_size = trend.count = 0;

    for (i = 0; i < count; i++) {
        if (data[i].ts / 3600 != hr) {
            /* new hour, new trend value */
            if (size == buf_size) {
                buf_size += 256;
                res = (hfs_trend_item_t*)realloc (res, buf_size * sizeof (hfs_trend_item_t));
            }

            res[size].ts = ts;
            res[size].value = trend;
            trend.count = 0;
            ts = data[i].ts;
            hr = ts / 3600;
            size++;
        }

        if (!trend.count) {
            if (data[i].type == IT_UINT64)
                trend.min.d = trend.max.d = trend.avg.d = data[i].val.l;
            else
                trend.min.d = trend.max.d = trend.avg.d = data[i].val.d;
            trend.count = 1;
        }
        else {
            double val;

            /* perform averaging */
            if (data[i].type == IT_UINT64)
                val = data[i].val.l;
            else
                val = data[i].val.d;

            if (val < trend.min.d)
                trend.min.d = val;
            if (val > trend.max.d)
                trend.max.d = val;
            trend.avg.d = ((trend.avg.d * trend.count) + val) / (trend.count + 1);
            trend.count++;
        }
    }

    if (trend.count) {
        /* save last value */
        if (size == buf_size) {
            buf_size += 256;
            res = (hfs_trend_item_t*)realloc (res, buf_size * sizeof (hfs_trend_item_t));
        }

        res[size].ts = ts;
        res[size].value = trend;
        size++;
    }

    *tr_count = size;

    return res;
}


void store_trends (const char* item_dir, hfs_trend_item_t* trends, int count)
{
    int i;
    char p_data[1024];
    char p_meta[1024];

    zbx_snprintf (p_meta, sizeof (p_meta), "%s/trends.meta", item_dir);
    zbx_snprintf (p_data, sizeof (p_data), "%s/trends.data", item_dir);

    for (i = 0; i < count; i++) {
        /* skip empty trends values */
        if (trends[i].value.count)
            hfs_store_value (p_meta, p_data, trends[i].ts, 3600, &trends[i].value, sizeof (hfs_trend_t), IT_TRENDS);
    }
}
