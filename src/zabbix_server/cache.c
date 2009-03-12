#include "cache.h"

#include "memcache.h"
#include "memcache_php.h"


/* The most complicated part. Trying to get item's triggers list from
   memcache. If found, build structure, if not, select from DB and
   store in memcache. */
cache_triggers_t* cache_get_item_triggers (zbx_uint64_t itemid)
{
	int cache = 0;

#ifdef HAVE_MEMCACHE
#endif
	
}


void cache_delete_cached_item_triggers (zbx_uint64_t itemid)
{
	memcache_zbx_del_val (memcache_get_key (MKT_ITEM_TRIGGERS, itemid));
}


void cache_free_item_triggers (cache_triggers_t* triggers)
{
	int i;

	if (!triggers)
		return;

	for (i = 0; i < triggers->count; i++) {
		free (triggers->triggers[i].url);
		free (triggers->triggers[i].comments);
	}

	free (triggers->triggers);
	free (triggers);
}
