#ifndef ZABBIX_MEMCACHE_H
#define ZABBIX_MEMCACHE_H

#ifdef HAVE_MEMCACHE
#include <libmemcached/memcached.h>

extern memcached_st *mem_conn;
extern int CONFIG_MEMCACHE_ITEMS_TTL;

int memcache_zbx_getitem(char *key, char *host, DB_ITEM *res);
int memcache_zbx_setitem(DB_ITEM *value);

#endif // HAVE_MEMCACHE

#endif // ZABBIX_MEMCACHE_H
