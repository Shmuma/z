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
	else
		srv = servers;

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


/* save value to memcache. Returns 0 if save failed, 1 otherise */
int memcache_zbx_save_val (const char* key, void* value, int val_len)
{
	memcached_return rc;

	if (!memsite)
		return 0;

	rc = memcached_set (memsite->conn, key, strlen (key), value, val_len, 0, 0);
	if (rc == MEMCACHED_ERRNO) {
		/* trying to reconnect */
		memcache_zbx_reconnect (memsite);
	}

	if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_BUFFERED)
		return 0;

	return 1;
}



/* fetch value from memcache. Return NULL if fetch failed. Return value must bee freed */
void* memcache_zbx_read_val (const char* site, const char* key, size_t* val_len)
{
	char *p;
	int len, flags;
	memcached_return rc;
	memsite_item_t* conn;

	if ((conn = memcache_zbx_site_lookup (site)) == NULL)
		return NULL;

	p = memcached_get (conn->conn, key, strlen (key), val_len, &flags, &rc);

	if (rc == MEMCACHED_ERRNO) {
		memcache_zbx_reconnect (conn);
		return NULL;
	}

	if (rc != MEMCACHED_SUCCESS)
		return NULL;

	return p;
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
	case MKT_LAST_STRING:
		snprintf (buf, sizeof (buf), "l|s|" ZBX_FS_UI64, itemid);
		break;
	default:
		return NULL;
	}

	return buf;
}
