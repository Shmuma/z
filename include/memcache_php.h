#ifndef __MEMCACHE_PHP_H__
#define __MEMCACHE_PHP_H__

#include <libmemcached/memcached.h>


int memcache_zbx_prepare_conn_table (const char* table);
memcached_st* memcache_zbx_site_lookup (const char* site);
int memcache_zbx_read_last (const char* site, const char* key, void* value, int val_len, char** stderr);


#endif
