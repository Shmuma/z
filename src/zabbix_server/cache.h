#ifndef __CACHE_H__
#define __CACHE_H__


typedef struct {
	int count;
	DB_TRIGGER* triggers;
} cache_triggers_t;


cache_triggers_t* cache_get_item_triggers (zbx_uint64_t itemid);
void cache_delete_cached_item_triggers (zbx_uint64_t itemid);
void cache_free_item_triggers (cache_triggers_t* triggers);


#endif
