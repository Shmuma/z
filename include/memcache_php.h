#ifndef __MEMCACHE_PHP_H__
#define __MEMCACHE_PHP_H__

#include <libmemcached/memcached.h>

typedef enum {
	MKT_LAST_UINT64,
	MKT_LAST_DOUBLE
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

int memcache_zbx_save_last (const char* key, void* value, int val_len, const char* stderr);
int memcache_zbx_read_last (const char* site, const char* key, void* value, int val_len, char** stderr);

#endif
