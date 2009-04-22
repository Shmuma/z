#ifndef __MEMCACHE_PHP_H__
#define __MEMCACHE_PHP_H__

#include <libmemcached/memcached.h>

typedef enum {
	MKT_LAST_UINT64,
	MKT_LAST_DOUBLE,
	MKT_LAST_STRING,
	MKT_LAST_LOG,
	MKT_TRIGGER,
	MKT_FUNCTION,
	MKT_ITEM_FUNCTIONS,
	MKT_ITEM_TRIGGERS,
} memcache_key_type_t;

typedef struct {
	char* site;
	char* server;
	memcached_st* conn;
} memsite_item_t;


extern memsite_item_t* memsite;


// for single-site connections (server-side)
int memcache_zbx_connect(const char* servers);
int memcache_zbx_disconnect(void);

// for multi-site connections (frontend)
int memcache_zbx_prepare_conn_table (const char* table);
memsite_item_t* memcache_zbx_site_lookup (const char* site);
void memcache_zbx_reconnect (memsite_item_t* item);
const char* memcache_get_key (memcache_key_type_t type, zbx_uint64_t itemid);

int memcache_zbx_save_val (const char* key, void* value, int val_len, int ttl);
int memcache_zbx_save_val_ext (const char* site, const char* key, void* value, int val_len, int ttl);
void* memcache_zbx_read_val (const char* site, const char* key, size_t* val_len);
void memcache_zbx_del_val (const char* site, const char* key);

#endif
