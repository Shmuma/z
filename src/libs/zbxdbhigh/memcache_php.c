#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>


#include "common.h"
#include "hfs.h"
#include "memcache_php.h"



/* array with site-connection mapping. Terminated with record with NULL site field. */
memsite_item_t* memsite;




/* routine prepares table for memcache connections */
int memcache_zbx_prepare_conn_table (const char* table)
{
	char *new_table;
	char *p, *pp;
	char* site;
	char buf[256];
	int count = 0;

	if (memsite)
		return 1;

	new_table = strdup (table);
	memsite = NULL;

	/* parse table */
	p = new_table;
	while (p && *p) {
		pp = strchr (p, ',');

		if (!pp)
			pp = p + strlen (p);

		memcpy (buf, p, pp-p);
		buf[pp-p] = 0;

		p = strchr (buf, ':');
		if (p) {
			*p = 0;
			p++;
			memsite = (memsite_item_t*)realloc (memsite, (count+1)*sizeof (memsite_item_t));
			memsite[count].site = strdup (buf);
			memsite[count].server = strdup (p);
			memsite[count].conn = NULL;
			memcache_zbx_reconnect (memsite+count);
			count++;
		}

		if (*pp)
			p = pp+1;
		else
			p = NULL;
	}

	memsite = (memsite_item_t*)realloc (memsite, (count+1)*sizeof (memsite_item_t));
	memsite[count].site = NULL;
	memsite[count].conn = NULL;
	free (new_table);
	return 1;
}


int memcache_zbx_connect(const char* servers)
{
	memcached_server_st	*mem_servers;
	memcached_return	 mem_rc;
	char* srv;

	if (!servers)
		srv = "localhost";

	if (memsite)
		return 1;

	memsite = (memsite_item_t*)calloc (1, sizeof (memsite_item_t));

	memsite->conn = memcached_create(NULL);
	memsite->server = strdup (srv);
	mem_servers = memcached_servers_parse(srv);
	mem_rc = memcached_server_push(memsite->conn, mem_servers);
	memcached_server_list_free(mem_servers);

	if (mem_rc != MEMCACHED_SUCCESS) {
		memcached_free(memsite->conn);
		free (memsite);
		memsite = NULL;
		return 0;
	}

	memcached_behavior_set(memsite->conn, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
	memcached_behavior_set(memsite->conn, MEMCACHED_BEHAVIOR_CACHE_LOOKUPS, 1);
	return 1;
}


int memcache_zbx_disconnect(void)
{
	if (memsite && memsite->conn) {
		memcached_free(memsite->conn);
		free (memsite->server);
		free (memsite);
	}
	return 1;
}


memsite_item_t* memcache_zbx_site_lookup (const char* site)
{
	int i = 0;

	if (!memsite)
		return NULL;

	if (!memsite->site && memsite->server)
		return memsite;

	while (memsite[i].site) {
		if (!strcmp (site, memsite[i].site))
			return memsite+i;
		i++;
	}
	return NULL;
}


void memcache_zbx_reconnect (memsite_item_t* item)
{
	memcached_server_st *mem_servers;

	if (!item)
		return;

	if (item->conn)
		memcached_free (item->conn);

	item->conn = memcached_create (NULL);
	mem_servers = memcached_servers_parse (item->server);
	memcached_server_push (item->conn, mem_servers);
	memcached_server_list_free(mem_servers);
	memcached_behavior_set(item->conn, MEMCACHED_BEHAVIOR_CACHE_LOOKUPS, 1);
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



int memcache_zbx_read_last (const char* site, const char* key, void* value, int val_len, char** stderr)
{
	char *p, *pp;
	int len, flags;
	memcached_return rc;
	memsite_item_t* conn;

	if ((conn = memcache_zbx_site_lookup (site)) == NULL)
		return 0;

	pp = p = memcached_get (conn->conn, key, strlen (key), &len, &flags, &rc);

	if (rc == MEMCACHED_ERRNO) {
		memcache_zbx_reconnect (conn);
		return 0;
	}

	if (rc != MEMCACHED_SUCCESS)
		return 0;
	
	if (len < val_len) {
		free (p);
		return 0;
	}

	memcpy (value, p, val_len);
	pp += val_len;
	*stderr = unbuffer_str (&pp);
	free (p);
	return 1;
}


const char* memcache_get_key (memcache_key_type_t type, zbx_uint64_t itemid)
{
	static char buf[256];

	switch (type) {
	case MKT_LAST_UINT64:
		snprintf (buf, sizeof (buf), "l|i|" ZBX_FS_UI64, itemid);
		break;
	case MKT_LAST_DOUBLE:
		snprintf (buf, sizeof (buf), "l|d|" ZBX_FS_UI64, itemid);
		break;
	default:
		return NULL;
	}

	return buf;
}
