#include "db.h"
#include "common.h"
#include "cache.h"
#include "tpl.h"

#include "memcache.h"
#include "memcache_php.h"

extern int CONFIG_MEMCACHE_ITEMS_TTL;


/* The most complicated part. Trying to get item's triggers list from
   memcache. If found, build structure, if not, select from DB and
   store in memcache. */
cache_triggers_t* cache_get_item_triggers (zbx_uint64_t itemid)
{
	const char* key;
	char* format = "A(S(Ussssiii))";
	char* buf;
	size_t size;
	cache_triggers_t* res;
	int buf_count, i, err;
	DB_RESULT result;
	DB_ROW row;
	tpl_node* tpl;
	DB_TRIGGER place;

	res = (cache_triggers_t*)malloc (sizeof (cache_triggers_t));
	buf_count = res->count = 0;
	res->triggers = NULL;

#ifdef HAVE_MEMCACHE
	key = memcache_get_key (MKT_ITEM_TRIGGERS, itemid);

	/* retrive cached buffer */
	buf = memcache_zbx_read_val (NULL, key, &size);

	if (buf) {
		/* if found, deserialize it  */
		tpl = tpl_map (format, &place);
		tpl_load (tpl, TPL_MEM, buf, size);

		res->count = tpl_Alen (tpl, 1);
		if (res->count)
			res->triggers = (DB_TRIGGER*)malloc (res->count * sizeof (DB_TRIGGER));

		err = 0;
		for (i = 0; i < res->count; i++) {
			if (tpl_unpack (tpl, 1) < 0) {
				err = 1;
				break;
			}

			memcpy (res->triggers+i, &place, sizeof (DB_TRIGGER));
		}

		free (buf);
		if (!err)
			return res;

		/* unpack error, obtain values from DB */
		free (res->triggers);
		res->triggers = NULL;
		res->count = 0;
	}
#endif

	/* obtain value from DB and build structure */
	result = DBselect("select t.triggerid,t.expression,t.description,t.url,t.comments,t.status,t.value,t.priority from triggers t,functions f,items i where i.status<>%d and i.itemid=f.itemid and t.status=%d and f.triggerid=t.triggerid and f.itemid=" ZBX_FS_UI64,
		ITEM_STATUS_NOTSUPPORTED,
		TRIGGER_STATUS_ENABLED,
		itemid);

	while (row = DBfetch (result)) {
		if (buf_count == res->count) {
			res->triggers = (DB_TRIGGER*)realloc (res->triggers, sizeof (DB_TRIGGER) * (buf_count += 2));
			if (!res->triggers) {
				res->count = 0;
				break;
			}
		}

		ZBX_STR2UINT64 (res->triggers[res->count].triggerid, row[0]);
		res->triggers[res->count].expression	= strdup (row[1]);
		res->triggers[res->count].description	= strdup (row[2]);
		res->triggers[res->count].url		= strdup (row[3]);
		res->triggers[res->count].comments	= strdup (row[4]);
		res->triggers[res->count].status	= atoi (row[5]);
		res->triggers[res->count].value		= atoi (row[6]);
		res->triggers[res->count].priority	= atoi (row[7]);
		res->count++;
	}

	DBfree_result (result);
	
#ifdef HAVE_MEMCACHE
	/* serialize value */
	tpl = tpl_map (format, &place);

	for (i = 0; i < res->count; i++) {
		memcpy (&place, res->triggers+i, sizeof (DB_TRIGGER));
		tpl_pack (tpl, 1);
	}

	/* save in memcache */
	buf = NULL;
	tpl_dump (tpl, TPL_MEM, &buf, &size);
	tpl_free (tpl);
	if (buf) {
		memcache_zbx_save_val (key, buf, size, CONFIG_MEMCACHE_ITEMS_TTL);
		free (buf);
	}
#endif

	return res;
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
		free (triggers->triggers[i].expression);
		free (triggers->triggers[i].description);
		free (triggers->triggers[i].url);
		free (triggers->triggers[i].comments);
	}

	free (triggers->triggers);
	free (triggers);
}
