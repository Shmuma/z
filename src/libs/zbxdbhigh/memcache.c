#include "common.h"
#include "comms.h"
#include "db.h"
#include "log.h"
#include "hfs.h"
#include "memcache.h"

#include <string.h>
#include <errno.h>


memcached_st *mem_conn = NULL;
static char* last_servers = NULL;



int memcache_zbx_connect(const char* servers)
{
	memcached_server_st	*mem_servers;
	memcached_return	 mem_rc;

	if (servers)
		last_servers = strdup (servers);
	if (!last_servers)
		last_servers = "localhost";

	if (mem_conn)
		memcached_free(mem_conn);

	if ((mem_conn = memcached_create(NULL)) == NULL)
		zabbix_log(LOG_LEVEL_ERR, "memcached_create() == NULL");

	mem_servers = memcached_servers_parse(last_servers);
	mem_rc = memcached_server_push(mem_conn, mem_servers);
	memcached_server_list_free(mem_servers);

	if (mem_rc != MEMCACHED_SUCCESS) {
		zabbix_log(LOG_LEVEL_ERR, "memcached_server_push(): %s",
			   memcached_strerror(mem_conn, mem_rc));
		memcached_free(mem_conn);
		mem_conn = NULL;
		return -1;
	}

	memcached_behavior_set(mem_conn, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
	memcached_behavior_set(mem_conn, MEMCACHED_BEHAVIOR_CACHE_LOOKUPS, 1);

	zabbix_log(LOG_LEVEL_DEBUG, "Connect with memcached done");
	return 0;
}

int memcache_zbx_disconnect(void)
{
	if (mem_conn != NULL)
		memcached_free(mem_conn);
	return 0;
}

int memcache_zbx_getitem(char *key, char *host, DB_ITEM *item)
{
	char *strkey = NULL, *strvalue = NULL;
	uint32_t flags;
	memcached_return rc;
	size_t len, item_len;

	len = strlen(key) + strlen(host) + 5 + 1;

	strkey = (char *) zbx_malloc(strkey, len);
	zbx_snprintf(strkey, len, "%d|%d|%s|%s",
		    process_type, MEMCACHE_VERSION, key, host);

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_getitem()"
		    "[%s|%s]", key, host);

	strvalue = memcached_get(mem_conn, strkey, len, &item_len, &flags, &rc);
	free(strkey);

	if (rc == MEMCACHED_SUCCESS) {
		if (!item_len)
			return 0;

		dbitem_unserialize(strvalue, item);
		return 1;
	}
	else if (rc == MEMCACHED_NOTFOUND) {
		return 0;
	}

	if (rc == MEMCACHED_ERRNO) {
		/* trying to reconnect */
		if (last_servers)
			memcache_zbx_connect (last_servers);
	}
	else
		zabbix_log(LOG_LEVEL_ERR, "[memcache] memcache_getitem(): ERR: %s",
			   memcached_strerror(mem_conn, rc));

	return -1;
}

int memcache_zbx_setitem(DB_ITEM *item)
{
	char *strkey = NULL;
	memcached_return rc;
	size_t len, item_len;

	len = strlen(item->key) + strlen(item->host_name) + 5 + 1;

	strkey = (char *) zbx_malloc(strkey, len);
	zbx_snprintf(strkey, len, "%d|%d|%s|%s",
		    process_type, MEMCACHE_VERSION, item->key, item->host_name);

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_setitem()"
		    "[%d|%d|%s|%s] cache-time=%d",
		    process_type, MEMCACHE_VERSION, item->key, item->host_name,
		    item->cache_time);

	item_len = dbitem_size(item, 0);
	dbitem_serialize(item, item_len);

	rc = memcached_set(mem_conn, strkey, (len-1), item->chars, item_len,
			    (time_t)(CONFIG_MEMCACHE_ITEMS_TTL * 2),
			    (uint32_t)0);
	free(strkey);

	if (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED)
		return 1;

	if (rc == MEMCACHED_ERRNO) {
		/* trying to reconnect */
		if (last_servers)
			memcache_zbx_connect (last_servers);
	}

	return -1;
}

int memcache_zbx_delitem(DB_ITEM *item)
{
	char *strkey = NULL;
	memcached_return rc;
	size_t len, item_len;

	len = strlen(item->key) + strlen(item->host_name) + 5 + 1;

	strkey = (char *) zbx_malloc(strkey, len);
	zbx_snprintf(strkey, len, "%d|%d|%s|%s",
		    process_type, MEMCACHE_VERSION, item->key, item->host_name);

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_delitem()"
		    "[%d|%d|%s|%s]", process_type, MEMCACHE_VERSION, item->key, item->host_name);

	rc = memcached_delete(mem_conn, strkey, (len-1), 0);
	free(strkey);

	if (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED)
		return 1;

	if (rc == MEMCACHED_ERRNO) {
		/* trying to reconnect */
		if (last_servers)
			memcache_zbx_connect (last_servers);
	}

	return -1;
}



int memcache_zbx_save_last (const char* key, void* value, int val_len, const char* stderr)
{
	static char buf[MAX_STRING_LEN];
	char* p;
	memcached_return rc;

	memcpy (buf, value, val_len);
	p = buffer_str (buf + val_len, stderr, sizeof (buf) - val_len);
	rc = memcached_set (mem_conn, key, strlen (key), &buf, p - buf, 0, 0);
	if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_BUFFERED) {
		zabbix_log (LOG_LEVEL_ERR, "[memcache] Error saving last value %d (last servers = %s)", rc, last_servers);
	}

	if (rc == MEMCACHED_ERRNO) {
		/* trying to reconnect */
		if (last_servers)
			memcache_zbx_connect (last_servers);
	}

	return 1;
}

