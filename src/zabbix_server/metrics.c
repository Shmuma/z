#include "common.h"
#include "metrics.h"


typedef struct {
	char* path;
	int fd;
} metric_t;


static metric_t *metrics = NULL;
static int metrics_count = 0;

static const char* base_path = "/dev/shm/zabbix_server/";


/*
   Initializes metrics structures. Must be called at top process of server. 
   Performs creation of new directory /dev/shm/zabbix_server/ or cleans it if it exists.
*/
void metrics_init ()
{
	DIR* dir;
	strict dirent d;

	/* checks for directory exists */
	dir = opendir (base_path);
	
	if (!dir)
		mkdir (base_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
	else {
		while (readdir (dir, &d, 1))
			if (strncmp (d.d_name, ".", 2) && strncmp (d.d_name, "..", 3))
				unlink (d.d_name);
		closedir (dir);
	}

	metrics = NULL;
	metrics_count = 0;
}


/*
   Appends new metric to management. If id is negative, no suffix
   is used. If id is a positive number, resulting file will have
*/
metric_key_t metric_register (const char* name, int id)
{
	static char buf[256];
	metric_key_t key = -1;

	if (id < 0)
		snprintf (buf, sizeof (buf), "%s/%s", base_path, name);
	else
		snprintf (buf, sizeof (buf), "%s/%s.%d", base_path, name, id);

	key = metrics_count;
	metrics_count++;
	metrics = (metric_t*)realloc (metrics, sizeof (metric_t) * metrics_count);
	metrics[key].path = strdup (buf);
	metrics[key].fd = open (buf, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (metrics[key].fd == -1) {
		free (metrics[key].path);
		metrics_count--;
		metrics = (metric_t*)realloc (metrics, sizeof (metric_t) * metrics_count);
		return -1;
	}

	return key;
}


/*
   Writes new value to metric's file
*/
int metric_update (metric_key_t key, zbx_uint64_t val)
{
	static char buf[256];
	int len;

	ftruncate (metrics[key].fd, 0);
	lseek (metrics[key].fd, 0, SEEK_SET);
	snprintf (buf, sizeof (buf), "%lld", val);
	len = strlen (buf);
	write (metrics[key].fd, buf, len);

	return 0;
}

