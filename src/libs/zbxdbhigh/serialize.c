#include <string.h>

#include "common.h"
#include "comms.h"
#include "db.h"
#include "serialize.h"

static inline size_t xstrlen(const char *s)
{
	return ((s && *s) ? (strlen(s) + 1) : 1);
}

static void *xmemcpy(void *dest, const void *src, size_t n)
{
	if (!src)
		((char *)dest)[0] = '\0';
	else
		memcpy(dest, src, n);
	return (((char *) dest) + n);
}

size_t dbitem_size(DB_ITEM *item, int update_len)
{
	int i;
	char *p;
	size_t len = sizeof(DB_ITEM);

	for (i = 0; i < CHARS_LEN_MAX; i++) {
		p = ((char **)((char *)(item) + DB_ITEM_OFFSETS[i]))[0];
		if (update_len)
			item->chars_len[i] = xstrlen(p);
		len += item->chars_len[i];
	}

	return len;
}

void dbitem_serialize(DB_ITEM *item, size_t item_len)
{
	int i;
	char *p, **f;
	size_t db_item_len;

	if (item->chars == NULL) {

		if (!item_len)
			item_len = dbitem_size(item, 1);

		item->chars = (char *) zbx_malloc(item->chars, item_len);

		db_item_len = sizeof(DB_ITEM);
		memcpy(item->chars, item, db_item_len);
		p = (item->chars + db_item_len);

		for (i = 0; i < CHARS_LEN_MAX; i++) {
			f = ((char **)((char *)(item) + DB_ITEM_OFFSETS[i]));

			if (f[0])
				memcpy(p, f[0], item->chars_len[i]);
			else
				*p = '\0';
			*f = p;
			p += item->chars_len[i];
		}
	}
}

void dbitem_unserialize(char *str, DB_ITEM *item)
{
	int i;
	char **f, *p = str;
	size_t l;

	l = sizeof(DB_ITEM);
	memcpy(item, p, l);
	item->chars = str;
	p += l;

	for (i = 0; i < CHARS_LEN_MAX; i++) {
		f = ((char **)((char *)(item) + DB_ITEM_OFFSETS[i]));
		*f = p;
		p += item->chars_len[i];
	}
}
