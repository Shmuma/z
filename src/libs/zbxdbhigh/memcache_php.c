#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>


#include "common.h"
#include "hfs.h"
#include "memcache_php.h"



typedef struct {
	char* site;
	memcached_st* conn;
} memsite_item_t;


/* array with site-connection mapping. Terminated with record with NULL site field. */
memsite_item_t* memsite;




/* routine prepares table for memcache connections */
int memcache_zbx_prepare_conn_table (const char* table)
{
	char *p, *pp;
	char* site;
	char buf[256];
	int count = 0;
	memcached_server_st *mem_servers;

	if (memsite)
		return 1;

	memsite = NULL;

	/* parse table */
	p = table;
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
			memsite[count].conn = memcached_create (NULL);
			mem_servers = memcached_servers_parse (p);
			memcached_server_push (memsite[count].conn, mem_servers);
			memcached_server_list_free(mem_servers);
			memcached_behavior_set(memsite[count].conn, MEMCACHED_BEHAVIOR_CACHE_LOOKUPS, 1);
			count++;
		}

		p = pp;
	}

	memsite = (memsite_item_t*)realloc (memsite, (count+1)*sizeof (memsite_item_t));
	memsite[count].site = NULL;
	memsite[count].conn = NULL;
	return 1;
}


memcached_st* memcache_zbx_site_lookup (const char* site)
{
	return NULL;
}



int memcache_zbx_read_last (const char* site, const char* key, void* value, int val_len, char** stderr)
{
	char *p, *pp;
	int len, flags;
	memcached_return rc;
	memcached_st* conn;

	if ((conn = memcache_zbx_site_lookup (site)) == NULL)
		return 0;

	pp = p = memcached_get (conn, key, strlen (key), &len, &flags, &rc);

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
