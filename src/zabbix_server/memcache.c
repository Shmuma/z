#include "common.h"
#include "comms.h"
#include "db.h"
#include "log.h"
#include "hfs.h"
#include "memcache.h"

#include <string.h>
#include <errno.h>

inline size_t xstrlen(const char *s)
{
	return ((s && *s) ? (strlen(s) + 1) : 1);
}

inline void *xmemcpy(void *dest, const void *src, size_t n)
{
	memcpy(dest, (src ? src : "\0"), n);
	return (((char *) dest) + n);
}

int memcache_zbx_connect(void)
{
	memcached_server_st	*mem_servers;
	memcached_return	 mem_rc;

	if (mem_conn)
		memcached_free(mem_conn);

	if ((mem_conn = memcached_create(NULL)) == NULL)
		zabbix_log(LOG_LEVEL_ERR, "memcached_create() == NULL");

	mem_servers = memcached_servers_parse(CONFIG_MEMCACHE_SERVER ?
					      CONFIG_MEMCACHE_SERVER :
					      "localhost");
	mem_rc = memcached_server_push(mem_conn, mem_servers);
	memcached_server_list_free(mem_servers);

	if (mem_rc != MEMCACHED_SUCCESS) {
		zabbix_log(LOG_LEVEL_ERR, "memcached_server_push(): %s",
			   memcached_strerror(mem_conn, mem_rc));
		memcached_free(mem_conn);
		mem_conn = NULL;
		return -1;
	}
	zabbix_log(LOG_LEVEL_DEBUG, "Connect with memcached done");
	return 0;
}

int memcache_zbx_disconnect(void)
{
	if (mem_conn != NULL)
		memcached_free(mem_conn);
	return 0;
}

int memcache_zbx_change_chars(DB_ITEM *item, int elem, char *newstr)
{
	int i;
	size_t db_item_len = sizeof(DB_ITEM);

	char *ptr = (item->chars + db_item_len);
	char *new_ptr, *new_chars = NULL;
	char **p;

	size_t new_offset, old_offset;
	size_t all_len = db_item_len;
	size_t new_len = xstrlen(newstr);

	if (item->chars_len[elem] != new_len) {
		for (i=0; i<CHARS_LEN_MAX; i++)
			all_len += item->chars_len[i];

		new_chars = (char *) zbx_malloc(new_chars, (sizeof(char) * (all_len - item->chars_len[elem] + new_len)));

		new_ptr = (new_chars + db_item_len);
		new_offset = 0;
		old_offset = 0;

		for (i=0; i<CHARS_LEN_MAX; i++) {
			p = ((char **)((char *)(item) + DB_ITEM_OFFSETS[i]));

			if (i < elem) {
				*p = (new_ptr + old_offset);
			}
			else if (i == elem) {
				memcpy(new_ptr, ptr, old_offset);
				xmemcpy((new_ptr + old_offset), newstr, new_len);
				*p = (new_ptr + old_offset);
				new_offset += old_offset + new_len;
			}
			else { // (i > elem)
				memcpy((new_ptr + new_offset), (ptr + old_offset), item->chars_len[i]);
				*p = (new_ptr + new_offset);
				new_offset += item->chars_len[i];
			}

			old_offset += item->chars_len[i];
		}
		item->chars_len[elem] = new_len;

		free(item->chars);
		item->chars = new_chars;

		memcpy(item->chars, item, db_item_len);
	}
	else {
		for (i=0; i<elem; i++)
			ptr += item->chars_len[i];

		memcpy(ptr, newstr, new_len);
	}

	return 0;
}

size_t get_itemsize(DB_ITEM *item)
{
	int i;
	char *p;
	size_t len = sizeof(DB_ITEM);

	for (i = 0; i < CHARS_LEN_MAX; i++) {
		p = ((char **)((char *)(item) + DB_ITEM_OFFSETS[i]))[0];
		item->chars_len[i] = xstrlen(p);
		len += item->chars_len[i];
	}

	return len;
}

void memcache_zbx_serialize_item(DB_ITEM *item, size_t item_len)
{
	int i;
	char *p, **f;
	size_t db_item_len;

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_zbx_serialize_item()"
		    "[%s|%s]", item->key, item->host_name);

	if (item->chars == NULL) {
		item->chars = (char *) zbx_malloc(item->chars, item_len);

		db_item_len = sizeof(DB_ITEM);
		memcpy(item->chars, item, db_item_len);
		p = (item->chars + db_item_len);

		for (i = 0; i < CHARS_LEN_MAX; i++) {
			f = ((char **)((char *)(item) + DB_ITEM_OFFSETS[i]));

			memcpy(p, (f[0] ? f[0] : "\0"), item->chars_len[i]);
			if (i >= DYN_DB_ITEM_ELEM && f[0])
				free(*f);
			*f = p;
			p += item->chars_len[i];
		}
	}
//	memcpy(item->chars, item, db_item_len);
}

void memcache_zbx_unserialize_item(char *str, DB_ITEM *item)
{
	int i;
	char **f, *p = str;
	size_t l;

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_zbx_unserialize_item()");

	l = sizeof(DB_ITEM);
	memcpy(item, p, l);
	item->chars = str;
	p += l;

	for (i = 0; i < CHARS_LEN_MAX; i++) {
		f = ((char **)((char *)(item) + DB_ITEM_OFFSETS[i]));
		*f = (*p != '\0') ? p : NULL;
		p += item->chars_len[i];
	}
}

int memcache_zbx_getitem(char *key, char *host, DB_ITEM *item)
{
	char *strkey = NULL, *strvalue = NULL;
	uint32_t flags;
	memcached_return rc;
	size_t len, item_len;

	len = strlen(key) + strlen(host) + 5;

	strkey = (char *) zbx_malloc(strkey, len);
	zbx_snprintf(strkey, len, "%c%c|%s|%s",
		    (char)process_type, (char)MEMCACHE_VERSION,
		    key, host);

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_getitem()"
		    "[%s|%s]", key, host);

	strvalue = memcached_get(mem_conn, strkey, len, &item_len, &flags, &rc);
	free(strkey);

	if (rc == MEMCACHED_SUCCESS) {
		if (!item_len)
			return 0;

		memcache_zbx_unserialize_item(strvalue, item);
//		free(strvalue);
		return 1;
	}
	else if (rc == MEMCACHED_NOTFOUND) {
		return 0;
	}

	zabbix_log(LOG_LEVEL_ERR, "[memcache] memcache_getitem(): ERR: %s",
		memcached_strerror(mem_conn, rc));

	return -1;
}

int memcache_zbx_setitem(DB_ITEM *item)
{
	char *strkey = NULL;
	memcached_return rc;
	size_t len, item_len;

	len = strlen(item->key) + strlen(item->host_name) + 5;

	strkey = (char *) zbx_malloc(strkey, len);
	zbx_snprintf(strkey, len, "%c%c|%s|%s",
		    (char)process_type, (char)MEMCACHE_VERSION,
		    item->key, item->host_name);

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_setitem()"
		    "[%s]", strkey);

	item_len = get_itemsize(item);
	memcache_zbx_serialize_item(item, item_len);

	rc = memcached_set(mem_conn, strkey, (len-1), item->chars, item_len,
			    (time_t)(CONFIG_MEMCACHE_ITEMS_TTL * 2),
			    (uint32_t)0);
	free(strkey);

	if (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED)
		return 1;

	return -1;
}

int memcache_zbx_item_remove(DB_ITEM *item)
{
	char *strkey = NULL;
	memcached_return rc;
	size_t len;

	len = strlen(item->key) + strlen(item->host_name) + 5;

	strkey = (char *) zbx_malloc(strkey, len);
	zbx_snprintf(strkey, len, "%c%c|%s|%s",
		    (char)process_type, (char)MEMCACHE_VERSION,
		    item->key, item->host_name);

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_remove()"
		    "[%s]", strkey);

	rc = memcached_delete(mem_conn, strkey, len, (time_t)0);
	free(strkey);

	if (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED)
		return 1;

	return -1;
}

int memcache_zbx_is_item_expire(DB_ITEM *item)
{
	hfs_time_t cur_time;

	if (!item->cache_time)
		return 1;

	cur_time = time(NULL);
	return ((cur_time - item->cache_time) >= CONFIG_MEMCACHE_ITEMS_TTL);
}
