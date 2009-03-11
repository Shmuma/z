#ifndef ZABBIX_MEMCACHE_H
#define ZABBIX_MEMCACHE_H

#ifdef HAVE_MEMCACHE
#include "db.h"
#include <libmemcached/memcached.h>

#define MEMCACHE_VERSION 2

extern int CONFIG_MEMCACHE_ITEMS_TTL;
extern char *CONFIG_MEMCACHE_SERVER;

int memcache_zbx_getitem(char *key, char *host, DB_ITEM *res);
int memcache_zbx_setitem(DB_ITEM *value);
int memcache_zbx_delitem(DB_ITEM *value);

void* memcache_zbx_get_functions (zbx_uint64_t itemid);
void  memcache_zbx_set_functions (zbx_uint64_t itemid, void* value, size_t size);

#endif // HAVE_MEMCACHE

#endif // ZABBIX_MEMCACHE_H
