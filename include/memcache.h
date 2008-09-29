#ifndef ZABBIX_MEMCACHE_H
#define ZABBIX_MEMCACHE_H

#ifdef HAVE_MEMCACHE
#include "db.h"
#include <libmemcached/memcached.h>

#define MEMCACHE_VERSION 1

extern memcached_st *mem_conn;
extern int CONFIG_MEMCACHE_ITEMS_TTL;
extern char *CONFIG_MEMCACHE_SERVER;
extern size_t DB_ITEM_OFFSETS[CHARS_LEN_MAX];

int memcache_zbx_connect(void);
int memcache_zbx_disconnect(void);
int memcache_zbx_getitem(char *key, char *host, DB_ITEM *res);
int memcache_zbx_setitem(DB_ITEM *value);
int memcache_zbx_item_remove(DB_ITEM *item);
int memcache_zbx_is_item_expire(DB_ITEM *item);
int memcache_zbx_change_chars(DB_ITEM *item, int elem, char *newstr);

#endif // HAVE_MEMCACHE

#endif // ZABBIX_MEMCACHE_H
