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

inline char *xstrdup(const char *src)
{
	return (((src) && ((*src) != '\0')) ? strdup(src) : NULL );
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

size_t get_itemsize(DB_ITEM *item)
{
	size_t len = 0;
	len += ITEM_KEY_LEN_MAX;
	len += xstrlen(item->delay_flex);
	len += xstrlen(item->description);
	len += xstrlen(item->eventlog_source);
	len += xstrlen(item->formula);
	len += xstrlen(item->host_dns);
	len += xstrlen(item->host_ip);
	len += xstrlen(item->host_name);
	len += xstrlen(item->lastvalue_str);
	len += xstrlen(item->logtimefmt);
	len += xstrlen(item->prevorgvalue_str);
	len += xstrlen(item->prevvalue_str);
	len += xstrlen(item->shortname);
	len += xstrlen(item->siteid);
	len += xstrlen(item->snmp_community);
	len += xstrlen(item->snmp_oid);
	len += xstrlen(item->snmpv3_authpassphrase);
	len += xstrlen(item->snmpv3_privpassphrase);
	len += xstrlen(item->snmpv3_securityname);
	len += xstrlen(item->trapper_hosts);
	len += xstrlen(item->units);
	len += sizeof(item->lastvalue_dbl) +
		sizeof(item->prevorgvalue_dbl) +
		sizeof(item->prevvalue_dbl) +
		sizeof(item->delay) +
		sizeof(item->delta) +
		sizeof(item->eventlog_severity) +
		sizeof(item->history) +
		sizeof(item->host_available) +
		sizeof(item->host_errors_from) +
		sizeof(item->host_status) +
		sizeof(item->lastclock) +
		sizeof(item->lastlogsize) +
		sizeof(item->lastvalue_null) +
		sizeof(item->multiplier) +
		sizeof(item->port) +
		sizeof(item->prevorgvalue_null) +
		sizeof(item->prevvalue_null) +
		sizeof(item->snmp_port) +
		sizeof(item->snmpv3_securitylevel) +
		sizeof(item->timestamp) +
		sizeof(item->trends) +
		sizeof(item->useip) +
		sizeof(item->lastcheck) +
		sizeof(item->nextcheck) +
		sizeof(item->status) +
		sizeof(item->type) +
		sizeof(item->value_type) +
		sizeof(item->hostid) +
		sizeof(item->itemid) +
		sizeof(item->lastvalue_uint64) +
		sizeof(item->prevorgvalue_uint64) +
		sizeof(item->prevvalue_uint64) +
		sizeof(item->valuemapid) +
		sizeof(item->cache_time);
	return len;
}

char *memcache_zbx_serialize_item(DB_ITEM *item, size_t item_len)
{
	char *ptr = NULL, *p;

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_zbx_serialize_item()"
		    "[%s|%s]", item->key, item->host_name);

	ptr = (char *) zbx_malloc(ptr, item_len);
	p = ptr;
	p = xmemcpy(p, &item->cache_time,		sizeof(item->cache_time));
	p = xmemcpy(p, &item->itemid,			sizeof(item->itemid));
	p = xmemcpy(p, &item->hostid,			sizeof(item->hostid));
	p = xmemcpy(p, &item->type,			sizeof(item->type));
	p = xmemcpy(p, &item->status,			sizeof(item->status));
	p = xmemcpy(p, item->siteid,			xstrlen(item->siteid));
	p = xmemcpy(p, item->description,		xstrlen(item->description));
	p = xmemcpy(p, item->key,			ITEM_KEY_LEN_MAX);
	p = xmemcpy(p, item->host_name,			xstrlen(item->host_name));
	p = xmemcpy(p, item->host_ip,			xstrlen(item->host_ip));
	p = xmemcpy(p, item->host_dns,			xstrlen(item->host_dns));
	p = xmemcpy(p, &item->host_status,		sizeof(item->host_status));
	p = xmemcpy(p, &item->host_available,		sizeof(item->host_available));
	p = xmemcpy(p, &item->host_errors_from,		sizeof(item->host_errors_from));
	p = xmemcpy(p, &item->useip,			sizeof(item->useip));
	p = xmemcpy(p, item->shortname,			xstrlen(item->shortname));
	p = xmemcpy(p, item->snmp_community,		xstrlen(item->snmp_community));
	p = xmemcpy(p, item->snmp_oid,			xstrlen(item->snmp_oid));
	p = xmemcpy(p, &item->snmp_port,		sizeof(item->snmp_port));
	p = xmemcpy(p, item->trapper_hosts,		xstrlen(item->trapper_hosts));
	p = xmemcpy(p, &item->port,			sizeof(item->port));
	p = xmemcpy(p, &item->delay,			sizeof(item->delay));
	p = xmemcpy(p, &item->history,			sizeof(item->history));
	p = xmemcpy(p, &item->trends,			sizeof(item->trends));
	p = xmemcpy(p, &item->lastclock,		sizeof(item->lastclock));

	p = xmemcpy(p, item->prevorgvalue_str,		xstrlen(item->prevorgvalue_str));
	p = xmemcpy(p, &item->prevorgvalue_dbl,		sizeof(item->prevorgvalue_dbl));
	p = xmemcpy(p, &item->prevorgvalue_uint64,	sizeof(item->prevorgvalue_uint64));
	p = xmemcpy(p, &item->prevorgvalue_null,	sizeof(item->prevorgvalue_null));

	p = xmemcpy(p, item->lastvalue_str,		xstrlen(item->lastvalue_str));
	p = xmemcpy(p, &item->lastvalue_dbl,		sizeof(item->lastvalue_dbl));
	p = xmemcpy(p, &item->lastvalue_uint64,		sizeof(item->lastvalue_uint64));
	p = xmemcpy(p, &item->lastvalue_null,		sizeof(item->lastvalue_null));

	p = xmemcpy(p, item->prevvalue_str,		xstrlen(item->prevvalue_str));
	p = xmemcpy(p, &item->prevvalue_dbl,		sizeof(item->prevvalue_dbl));
	p = xmemcpy(p, &item->prevvalue_uint64,		sizeof(item->prevvalue_uint64));
	p = xmemcpy(p, &item->prevvalue_null,		sizeof(item->prevvalue_null));

	p = xmemcpy(p, &item->lastcheck,		sizeof(item->lastcheck));
	p = xmemcpy(p, &item->nextcheck,		sizeof(item->nextcheck));
	p = xmemcpy(p, &item->value_type,		sizeof(item->value_type));
	p = xmemcpy(p, &item->delta,			sizeof(item->delta));
	p = xmemcpy(p, &item->multiplier,		sizeof(item->multiplier));
	p = xmemcpy(p, item->units,			xstrlen(item->units));

	p = xmemcpy(p, item->snmpv3_securityname,	xstrlen(item->snmpv3_securityname));
	p = xmemcpy(p, &item->snmpv3_securitylevel,	sizeof(item->snmpv3_securitylevel));
	p = xmemcpy(p, item->snmpv3_authpassphrase,	xstrlen(item->snmpv3_authpassphrase));
	p = xmemcpy(p, item->snmpv3_privpassphrase,	xstrlen(item->snmpv3_privpassphrase));

	p = xmemcpy(p, item->formula,			xstrlen(item->formula));
	p = xmemcpy(p, &item->lastlogsize,		sizeof(item->lastlogsize));
	p = xmemcpy(p, &item->timestamp,		sizeof(item->timestamp));
	p = xmemcpy(p, &item->eventlog_severity,	sizeof(item->eventlog_severity));
	p = xmemcpy(p, item->eventlog_source,		xstrlen(item->eventlog_source));
	p = xmemcpy(p, item->logtimefmt,		xstrlen(item->logtimefmt));
	p = xmemcpy(p, &item->valuemapid,		sizeof(item->valuemapid));
	p = xmemcpy(p, item->delay_flex,		xstrlen(item->delay_flex));
	return ptr;
}

void memcache_zbx_unserialize_item(DB_ITEM *item, char *itemstr)
{
	char *p = itemstr;
	size_t l;

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_zbx_unserialize_item()");

	item->from_memcache = 1;

	l = sizeof(item->cache_time);
	memcpy(&item->cache_time, p, l);		p += l;

	l = sizeof(item->itemid);
	memcpy(&item->itemid, p, l);			p += l;

	l = sizeof(item->hostid);
	memcpy(&item->hostid, p, l);			p += l;

	l = sizeof(item->type);
	memcpy(&item->type, p, l);			p += l;

	l = sizeof(item->status);
	memcpy(&item->status, p, l);			p += l;

	item->siteid = xstrdup(p);			p += xstrlen(item->siteid);
	item->description = xstrdup(p);			p += xstrlen(item->description);

	xmemcpy(item->key, p, ITEM_KEY_LEN_MAX);	p += ITEM_KEY_LEN_MAX;

	item->host_name = xstrdup(p);			p += xstrlen(item->host_name);
	item->host_ip = xstrdup(p);			p += xstrlen(item->host_ip);
	item->host_dns = xstrdup(p);			p += xstrlen(item->host_dns);

	l = sizeof(item->host_status);
	xmemcpy(&item->host_status, p, l);		p += l;

	l = sizeof(item->host_available);
	xmemcpy(&item->host_available, p, l);		p += l;

	l = sizeof(item->host_errors_from);
	xmemcpy(&item->host_errors_from, p, l);		p += l;

	l = sizeof(item->useip);
	xmemcpy(&item->useip, p, l);			p += l;

	item->shortname = xstrdup(p);			p += xstrlen(item->shortname);
	item->snmp_community = xstrdup(p);		p += xstrlen(item->snmp_community);
	item->snmp_oid = xstrdup(p);			p += xstrlen(item->snmp_oid);

	l = sizeof(item->snmp_port);
	xmemcpy(&item->snmp_port, p, l);		p += l;

	item->trapper_hosts = xstrdup(p);		p += xstrlen(item->trapper_hosts);

	l = sizeof(item->port);
	xmemcpy(&item->port, p, l);			p += l;

	l = sizeof(item->delay);
	xmemcpy(&item->delay, p, l);			p += l;

	l = sizeof(item->history);
	xmemcpy(&item->history, p, l);			p += l;

	l = sizeof(item->trends);
	xmemcpy(&item->trends, p, l);			p += l;

	l = sizeof(item->lastclock);
	xmemcpy(&item->lastclock, p, l);		p += l;

	item->prevorgvalue_str = xstrdup(p);		p += xstrlen(item->prevorgvalue_str);


	l = sizeof(item->prevorgvalue_dbl);
	xmemcpy(&item->prevorgvalue_dbl, p, l);		p += l;

	l = sizeof(item->prevorgvalue_uint64);
	xmemcpy(&item->prevorgvalue_uint64, p, l); 	p += l;

	l = sizeof(item->prevorgvalue_null);
	xmemcpy(&item->prevorgvalue_null, p, l);	p += l;

	item->lastvalue_str = xstrdup(p);		p += xstrlen(item->lastvalue_str);

	l = sizeof(item->lastvalue_dbl);
	xmemcpy(&item->lastvalue_dbl, p, l);		p += l;

	l = sizeof(item->lastvalue_uint64);
	xmemcpy(&item->lastvalue_uint64, p, l);		p += l;

	l = sizeof(item->lastvalue_null);
	xmemcpy(&item->lastvalue_null, p, l);		p += l;

	item->prevvalue_str = xstrdup(p);		p += xstrlen(item->prevvalue_str);

	l = sizeof(item->prevvalue_dbl);
	xmemcpy(&item->prevvalue_dbl, p, l);		p += l;

	l = sizeof(item->prevvalue_uint64);
	xmemcpy(&item->prevvalue_uint64, p, l);		p += l;

	l = sizeof(item->prevvalue_null);
	xmemcpy(&item->prevvalue_null, p, l);		p += l;

	l = sizeof(item->lastcheck);
	xmemcpy(&item->lastcheck, p, l);		p += l;

	l = sizeof(item->nextcheck);
	xmemcpy(&item->nextcheck, p, l);		p += l;

	l = sizeof(item->value_type);
	xmemcpy(&item->value_type, p, l);		p += l;

	l = sizeof(item->delta);
	xmemcpy(&item->delta, p, l);			p += l;

	l = sizeof(item->multiplier);
	xmemcpy(&item->multiplier, p, l);		p += l;

	item->units = xstrdup(p);			p += xstrlen(item->units);
	item->snmpv3_securityname = xstrdup(p);		p += xstrlen(item->snmpv3_securityname);

	l = sizeof(item->snmpv3_securitylevel);
	xmemcpy(&item->snmpv3_securitylevel, p, l);	p += l;

	item->snmpv3_authpassphrase = xstrdup(p);	p += xstrlen(item->snmpv3_authpassphrase);
	item->snmpv3_privpassphrase = xstrdup(p);	p += xstrlen(item->snmpv3_privpassphrase);
	item->formula = xstrdup(p);			p += xstrlen(item->formula);

	l = sizeof(item->lastlogsize);
	xmemcpy(&item->lastlogsize, p, l);		p += l;

	l = sizeof(item->timestamp);
	xmemcpy(&item->timestamp, p, l);		p += l;

	l = sizeof(item->eventlog_severity);
	xmemcpy(&item->eventlog_severity, p, l);	p += l;

	item->eventlog_source = xstrdup(p);		p += xstrlen(item->eventlog_source);
	item->logtimefmt = xstrdup(p);			p += xstrlen(item->logtimefmt);

	l = sizeof(item->valuemapid);
	xmemcpy(&item->valuemapid, p, l);		p += l;

	item->delay_flex = xstrdup(p);			p += xstrlen(item->delay_flex);
}

int memcache_zbx_getitem(char *key, char *host, DB_ITEM *item)
{
	char *strkey = NULL, *stritem = NULL;
	uint32_t flags;
	memcached_return rc;
	size_t len, item_len;

	len = strlen(key) + strlen(host) + 2;

	strkey = (char *) zbx_malloc(strkey, len);
	zbx_snprintf(strkey, len, "%s|%s", key, host);

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_getitem()"
		    "[%s]", strkey);

	stritem = memcached_get(mem_conn, strkey, len, &item_len, &flags, &rc);
	free(strkey);

	if (rc == MEMCACHED_SUCCESS) {
		memcache_zbx_unserialize_item(item, stritem);
		free(stritem);
		return 1;
	}
	else if (rc == MEMCACHED_NOTFOUND) {
		if (stritem)
			free(stritem);
		return 0;
	}

	zabbix_log(LOG_LEVEL_ERR, "[memcache] memcache_getitem(): ERR: %s",
		memcached_strerror(mem_conn, rc));

	return -1;
}

int memcache_zbx_setitem(DB_ITEM *item)
{
	char *strkey = NULL, *stritem = NULL;
	memcached_return rc;
	size_t len, item_len;

	len = strlen(item->key) + strlen(item->host_name) + 2;

	strkey = (char *) zbx_malloc(strkey, len);
	zbx_snprintf(strkey, len, "%s|%s", item->key, item->host_name);

	zabbix_log(LOG_LEVEL_DEBUG, "[memcache] memcache_setitem()"
		    "[%s]", strkey);

	item_len = get_itemsize(item);
	stritem = memcache_zbx_serialize_item(item, item_len);

	rc = memcached_set(mem_conn, strkey, (len-1), stritem, item_len,
			    (time_t)(CONFIG_MEMCACHE_ITEMS_TTL * 2),
			    (uint32_t)0);
	free(strkey);
	free(stritem);

	if (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED)
		return 1;

	return -1;
}

int memcache_zbx_item_remove(DB_ITEM *item)
{
	char *strkey = NULL;
	memcached_return rc;
	size_t len;

	len = strlen(item->key) + strlen(item->host_name) + 2;

	strkey = (char *) zbx_malloc(strkey, len);
	zbx_snprintf(strkey, len, "%s|%s", item->key, item->host_name);

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
