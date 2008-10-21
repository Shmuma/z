#include "common.h"
#include "queue.h"
#include "hfs.h"


static char* empty_string = "";



const char* queue_get_name (queue_name_kind_t kind, int q_num, int process_id, int index)
{
	static char buf[1024];

	switch (kind) {
	case QNK_File:
		snprintf (buf, sizeof (buf), "/dev/shm/zabbix_queue/queue_%d.%d.%d", q_num, process_id, index);
		break;
	case QNK_Index:
		snprintf (buf, sizeof (buf), "/dev/shm/zabbix_queue/queue_%d_idx.%d", q_num, process_id);
		break;
	case QNK_Position:
		snprintf (buf, sizeof (buf), "/dev/shm/zabbix_queue/queue_%d_pos.%d", q_num, process_id);
		break;
	}

	return buf;
}


char* queue_encode_entry (queue_entry_t* entry, char* buf, int size)
{
	char* buf_p = buf;

	*(hfs_time_t*)buf_p = entry->ts;
	buf_p += sizeof (hfs_time_t);

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
	char* buf_p;
	char** ptrs[] = { &entry->server, &entry->key, &entry->value, &entry->error, &entry->lastlogsize, 
			  &entry->timestamp, &entry->source, &entry->severity };
	int len, i;

	entry->buf = (char*)malloc (size);
	memcpy (entry->buf, buf, size);
	buf_p = entry->buf;

	entry->ts = *(hfs_time_t*)buf_p;
	buf_p += sizeof (hfs_time_t);

	for (i = 0; i < sizeof (ptrs) / sizeof (ptrs[0]); i++) {
		len = *(int*)buf_p;
		buf_p += sizeof (len);

		if (buf_p - entry->buf > size) {
			free (entry->buf);
			return NULL;
		}
		if (len) {
			*ptrs[i] = buf_p;
			buf_p += len+1;
		}
		else
			*ptrs[i] = empty_string;
		if (buf_p - entry->buf > size) {
			free (entry->buf);
			return NULL;
		}
	}

	return buf_p;
}


int queue_encode_history_header (queue_history_entry_t* entry, const char* server, const char* key)
{
	int len = 0;
	char* res_p;

	memset (entry, 0, sizeof (queue_history_entry_t));

	len += sizeof (int);
	if (server)
		len += strlen (server) + 1;
	len += sizeof (int);
	if (key)
		len += strlen (key) + 1;
	len += sizeof (int);

	entry->buf = (char*)malloc (len);
	entry->buf_size = len;

	/* reserve space for items count */
	res_p = entry->buf;
	res_p += sizeof (int);
	
	/* server and key strings */
	res_p = buffer_str (res_p, server, len);
	if (!res_p)
		goto error;
	res_p = buffer_str (res_p, key, len-(res_p-entry->buf));
	if (!res_p)
		goto error;

	return 1;
	
 error:
	if (entry->buf)
		free (entry->buf);
	return 0;
}



int queue_encode_history_item (queue_history_entry_t* entry, hfs_time_t ts, const char* value, const char* lastlogsize, 
			       const char* timestamp, const char* source, const char* severity)
{
	int len = entry->buf_size, l;
	char* res_p, *p;

	len += sizeof (ts);
	len += str_buffer_length (value);
	len += str_buffer_length (lastlogsize);
	len += str_buffer_length (timestamp);
	len += str_buffer_length (source);
	len += str_buffer_length (severity);
	
	res_p = entry->buf = (char*)realloc (entry->buf, len);
	if (!res_p)
		goto error;
	res_p += entry->buf_size;
	l = entry->buf_size;
	entry->buf_size = len;
	len -= l;

	*((hfs_time_t*)res_p) = ts;
	res_p += sizeof (ts);
	len -= sizeof (ts);

	p = res_p;
	res_p = buffer_str (res_p, value, len);
	if (!res_p)
		goto error;
	len -= res_p - p;

	p = res_p;
	res_p = buffer_str (res_p, lastlogsize, len);
	if (!res_p)
		goto error;
	len -= res_p - p;

	p = res_p;
	res_p = buffer_str (res_p, timestamp, len);
	if (!res_p)
		goto error;
	len -= res_p - p;

	p = res_p;
	res_p = buffer_str (res_p, source, len);
	if (!res_p)
		goto error;
	len -= res_p - p;

	p = res_p;
	res_p = buffer_str (res_p, severity, len);
	if (!res_p)
		goto error;
	len -= res_p - p;

	return 1;

 error:
	if (entry->buf)
		free (entry->buf);
	return 0;
}


static char* decode_string (char* buf, int* size, char** res)
{
	int len = *(int*)buf;
	char* b = buf;

	buf += sizeof (len);
	if (len) {
		*res = buf;
		buf += len+1;
	}
	else
		*res = empty_string;

	if (buf-b > *size)
		return NULL;
	*size -= buf-b;

	return buf;
}


int queue_decode_history (queue_history_entry_t* entry, char* buf, int size)
{
	int i;

	entry->items = NULL;
	entry->buf = buf;
	entry->buf_size = size;

	entry->count = *(int*)buf;
	size -= sizeof (entry->count);
	buf  += sizeof (entry->count);

	/* server */
	if ((buf = decode_string (buf, &size, &entry->server)) == NULL)
		return 0;
	/* key */
	if ((buf = decode_string (buf, &size, &entry->key)) == NULL)
		return 0;

	entry->items = (queue_history_item_t*)malloc (sizeof (queue_history_item_t) * entry->count);

	for (i = 0; i < entry->count; i++) {
		entry->items[i].ts = *(hfs_time_t*)buf;
		buf  += sizeof (hfs_time_t);
		size -= sizeof (hfs_time_t);

		if ((buf = decode_string (buf, &size, &entry->items[i].value)) == NULL)
			goto error;
		if ((buf = decode_string (buf, &size, &entry->items[i].lastlogsize)) == NULL)
			goto error;
		if ((buf = decode_string (buf, &size, &entry->items[i].timestamp)) == NULL)
			goto error;
		if ((buf = decode_string (buf, &size, &entry->items[i].source)) == NULL)
			goto error;
		if ((buf = decode_string (buf, &size, &entry->items[i].severity)) == NULL)
			goto error;
	}

	return 1;

 error:
	if (entry->items)
		free (entry->items);
	return 0;
}

