#ifndef ZABBIX_MEMCACHE_H
#define ZABBIX_MEMCACHE_H

#ifdef HAVE_MEMCACHE
#include "db.h"
#include <libmemcached/memcached.h>

#define MEMCACHE_VERSION 2

extern memcached_st *mem_conn;
extern int CONFIG_MEMCACHE_ITEMS_TTL;
extern char *CONFIG_MEMCACHE_SERVER;

int memcache_zbx_connect(const char* servers);
int memcache_zbx_disconnect(void);
int memcache_zbx_getitem(char *key, char *host, DB_ITEM *res);
int memcache_zbx_setitem(DB_ITEM *value);
int memcache_zbx_delitem(DB_ITEM *value);
int memcache_zbx_save_last (const char* key, void* value, int val_len, const char* stderr);

#endif // HAVE_MEMCACHE

#endif // ZABBIX_MEMCACHE_H
