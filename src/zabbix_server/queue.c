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
		snprintf (buf, sizeof (buf), "/dev/shm/zabbix_queue/queue/queue_%d_idx.%d", q_num, process_id);
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
		if (buf_p - entry->buf > size)
			return NULL;
		if (len) {
			*ptrs[i] = buf_p;
			buf_p += len+1;
		}
		else
			*ptrs[i] = empty_string;
	}

	return buf_p;
}
