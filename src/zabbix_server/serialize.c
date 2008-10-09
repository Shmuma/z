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

int dbitem_change_chars(DB_ITEM *item, int elem, char *newstr)
{
	int i;
	size_t db_item_len = sizeof(DB_ITEM);

	char *ptr = (item->chars + db_item_len);
	char *new_ptr, *new_chars = NULL;
	char **p;

	size_t new_offset, old_offset;
	size_t all_len = db_item_len;
	size_t new_len = xstrlen(newstr);

	if (item->chars_len[elem] != new_len) {
		for (i=0; i<CHARS_LEN_MAX; i++)
			all_len += item->chars_len[i];

		new_chars = (char *) zbx_malloc(new_chars, (sizeof(char) * (all_len - item->chars_len[elem] + new_len)));

		new_ptr = (new_chars + db_item_len);
		new_offset = 0;
		old_offset = 0;

		for (i=0; i<CHARS_LEN_MAX; i++) {
			p = ((char **)((char *)(item) + DB_ITEM_OFFSETS[i]));

			if (i < elem) {
				*p = (new_ptr + old_offset);
			}
			else if (i == elem) {
				memcpy(new_ptr, ptr, old_offset);
				xmemcpy((new_ptr + old_offset), newstr, new_len);
				*p = (new_ptr + old_offset);
				new_offset += old_offset + new_len;
			}
			else { // (i > elem)
				memcpy((new_ptr + new_offset), (ptr + old_offset), item->chars_len[i]);
				*p = (new_ptr + new_offset);
				new_offset += item->chars_len[i];
			}

			old_offset += item->chars_len[i];
		}
		item->chars_len[elem] = new_len;

		free(item->chars);
		item->chars = new_chars;

		memcpy(item->chars, item, db_item_len);
	}
	else {
		for (i=0; i<elem; i++)
			ptr += item->chars_len[i];

		memcpy(ptr, newstr, new_len);
	}

	return 0;
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
			if (i >= DYN_DB_ITEM_ELEM && f[0])
				free(*f);
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
