#include "common.h"
#include "comms.h"
#include "db.h"
#include "log.h"
#include "hfs.h"
#include "memcache.h"
#include "memcache_php.h"

#include <string.h>
#include <errno.h>


int memcache_zbx_getitem(char *key, char *host, DB_ITEM *item)
{
	char *strkey = NULL, *strvalue = NULL;
	uint32_t flags;
	memcached_return rc;
	size_t len, item_len;

	if (!memsite)
		return 0;

	len = strlen(key) + strlen(host) + 5 + 1;

	strkey = (char *) zbx_malloc(strkey, len);
	zbx_snprintf(strkey, len, "%d|%d|%s|%s",
		    process_type, MEMCACHE_VERSION, key, host);

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_getitem()"
		    "[%s|%s]", key, host);

	strvalue = memcached_get(memsite->conn, strkey, len, &item_len, &flags, &rc);
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

	if (rc == MEMCACHED_ERRNO)
		memcache_zbx_reconnect (memsite);
	else
		zabbix_log(LOG_LEVEL_ERR, "[memcache] memcache_getitem(): ERR: %s",
			   memcached_strerror(memsite->conn, rc));

	return -1;
}

int memcache_zbx_setitem(DB_ITEM *item)
{
	char *strkey = NULL;
	memcached_return rc;
	size_t len, item_len;

	if (!memsite)
		return -1;

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

	rc = memcached_set(memsite->conn, strkey, (len-1), item->chars, item_len,
			    (time_t)(CONFIG_MEMCACHE_ITEMS_TTL * 2),
			    (uint32_t)0);
	free(strkey);

	if (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED)
		return 1;

	if (rc == MEMCACHED_ERRNO) {
		/* trying to reconnect */
		memcache_zbx_reconnect (memsite);
	}

	return -1;
}

int memcache_zbx_delitem(DB_ITEM *item)
{
	char *strkey = NULL;
	memcached_return rc;
	size_t len, item_len;

	if (!memsite)
		return -1;

	len = strlen(item->key) + strlen(item->host_name) + 5 + 1;

	strkey = (char *) zbx_malloc(strkey, len);
	zbx_snprintf(strkey, len, "%d|%d|%s|%s",
		    process_type, MEMCACHE_VERSION, item->key, item->host_name);

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_delitem()"
		    "[%d|%d|%s|%s]", process_type, MEMCACHE_VERSION, item->key, item->host_name);

	rc = memcached_delete(memsite->conn, strkey, (len-1), 0);
	free(strkey);

	if (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED)
		return 1;

	if (rc == MEMCACHED_ERRNO)
		memcache_zbx_reconnect (memsite);

	return -1;
}



int memcache_zbx_save_last (const char* key, void* value, int val_len, const char* stderr)
{
	static char buf[MAX_STRING_LEN];
	char* p;
	memcached_return rc;

	if (!memsite)
		return 0;

	memcpy (buf, value, val_len);
	p = buffer_str (buf + val_len, stderr, sizeof (buf) - val_len);
	rc = memcached_set (memsite->conn, key, strlen (key), &buf, p - buf, 0, 0);
	if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_BUFFERED) {
		zabbix_log (LOG_LEVEL_ERR, "[memcache] Error saving last value %d", rc);
	}

	if (rc == MEMCACHED_ERRNO) {
		/* trying to reconnect */
		memcache_zbx_reconnect (memsite);
	}

	return 1;
}

