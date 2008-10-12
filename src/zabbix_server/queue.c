#include "common.h"
#include "queue.h"
#include "hfs.h"


extern char* CONFIG_SERVER_SITE;
extern char* CONFIG_HFS_PATH;



const char* queue_get_name (queue_name_kind_t kind, int process_id, int index)
{
	static char buf[1024];

	switch (kind) {
	case QNK_File:
		snprintf (buf, sizeof (buf), "%s/%s/queue/queue.%d.%d", CONFIG_HFS_PATH, CONFIG_SERVER_SITE, process_id, index);
		break;
	case QNK_Index:
		snprintf (buf, sizeof (buf), "%s/%s/queue/queue_idx.%d", CONFIG_HFS_PATH, CONFIG_SERVER_SITE, process_id);
		break;
	case QNK_Position:
		snprintf (buf, sizeof (buf), "%s/%s/queue/queue_pos.%d", CONFIG_HFS_PATH, CONFIG_SERVER_SITE, process_id);
		break;
	}

	return buf;
}


char* queue_encode_entry (queue_entry_t* entry, char* buf, int size)
{
	char* buf_p = buf;
	if ((buf_p = buffer_str (buf_p, entry->server, size)) == NULL)
		return NULL;
	if ((buf_p = buffer_str (buf_p, entry->key, size-(buf_p-buf))) == NULL)
		return NULL;
	if ((buf_p = buffer_str (buf_p, entry->value, size-(buf_p-buf))) == NULL)
		return NULL;
	if ((buf_p = buffer_str (buf_p, entry->error, size-(buf_p-buf))) == NULL)
		return NULL;
	if ((buf_p = buffer_str (buf_p, entry->lastlogsize, size-(buf_p-buf))) == NULL)
		return NULL;
	if ((buf_p = buffer_str (buf_p, entry->timestamp, size-(buf_p-buf))) == NULL)
		return NULL;
	if ((buf_p = buffer_str (buf_p, entry->source, size-(buf_p-buf))) == NULL)
		return NULL;
	if ((buf_p = buffer_str (buf_p, entry->severity, size-(buf_p-buf))) == NULL)
		return NULL;
	return buf_p;
}



char* queue_decode_entry (queue_entry_t* entry, char* buf, int size)
{
	char* buf_p = buf;
	char** ptrs[] = { &entry->server, &entry->key, &entry->value, &entry->error, &entry->lastlogsize, 
			  &entry->timestamp, &entry->source, &entry->severity };
	int len, i;

	for (i = 0; i < sizeof (ptrs) / sizeof (ptrs[0]); i++) {
		len = *(int*)buf_p;
		buf_p += sizeof (len);
		if (buf_p - buf > size)
			return NULL;
		if (len) {
			*ptrs[i] = buf_p;
			buf_p += len+1;
		}
		else
			*ptrs[i] = NULL;
	}

	return buf_p;
}
