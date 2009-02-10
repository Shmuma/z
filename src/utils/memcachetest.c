#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "common.h"
#include "memcache.h"

char *item_key = NULL;
char *item_host_name = NULL;
char *item_value = NULL;

void
__zbx_zabbix_log(int level, const char *fmt, ...) {
//	if (level == LOG_LEVEL_DEBUG && !ZABBIX_GLOBAL(debug))
//		return;

	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "zabbix: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	fflush(stderr);
	va_end(ap);
}
#ifndef zabbix_log
#define zabbix_log __zbx_zabbix_log
#endif

int main(int argc, char **argv) {
	char *strkey = NULL, *strvalue = NULL;
	uint32_t flags;
	memcached_return rc;
	size_t len, item_len = 0;

	process_type = ZBX_PROCESS_TRAPPERD;

	if (argc != 4) {
		fprintf(stderr, "Usage: memcachetest <item_host_name> <item_key> <value>\n");
		return 1;
	} else {
		item_key = argv[1];
		item_host_name = argv[2];
		item_value = argv[3];
	}

	len = strlen(item_key) + strlen(item_host_name) + 6;

	strkey = (char *) zbx_malloc(strkey, len);
	zbx_snprintf(strkey, len, "%d|%d|%s|%s",
		    process_type, MEMCACHE_VERSION, item_key, item_host_name);

	printf("memcache key: %s\n", strkey);

	if (memcache_zbx_connect(CONFIG_MEMCACHE_SERVER) == -1) {
		fprintf(stderr, "Unable to connect to memcache server\n");
		free(strkey);
		return 1;
	}

	printf("Setting item into server...");
	rc = memcached_set(mem_conn, strkey, len, item_value, strlen(item_value),
			    (time_t)(CONFIG_MEMCACHE_ITEMS_TTL * 2),
			    (uint32_t)0);

	if (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED) {
		printf("[done]\n");
	} else {
		printf("[fail]\n");
		free(strkey);
		return 1;
	}

	printf("Getting item from server...");
	strvalue = memcached_get(mem_conn, strkey, (len-1), &item_len, &flags, &rc);

	if (rc == MEMCACHED_SUCCESS) {
		if (!item_len) {
			printf("[value empty]\n");
			return 1;
		}

		printf("[value: '%s']\n", strvalue);
		free(strvalue);
		return 0;
	}
	else if (rc == MEMCACHED_NOTFOUND) {
		printf("[not found]\n");
		free(strvalue);
		return 1;
	}
	else {
		printf("error: %s\n", memcached_strerror(mem_conn, rc));
		free(strvalue);
	}

	memcache_zbx_disconnect();
	return 0;
}
