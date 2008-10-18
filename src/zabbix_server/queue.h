#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "common.h"
#include "hfs.h"

#define QUEUE_SIZE_LIMIT (100*1024)

typedef enum {
	QNK_File = 0,
	QNK_Index,
	QNK_Position,
} queue_name_kind_t;


typedef struct {
	char *buf;
	hfs_time_t ts;
	char *server;
	char *key;
	char *value;
	char *error;
	char *lastlogsize;
	char *timestamp;
	char *source;
	char *severity;
} queue_entry_t;


const char* queue_get_name (queue_name_kind_t kind, int q_num, int process_id, int index);

char* queue_encode_entry (queue_entry_t* entry, char* buf, int size);
char* queue_decode_entry (queue_entry_t* entry, char* buf, int size);


#endif
