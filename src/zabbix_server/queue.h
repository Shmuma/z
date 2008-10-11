#ifndef __QUEUE_H__
#define __QUEUE_H__


#define QUEUE_SIZE_LIMIT (1024*1024*1024)

typedef enum {
	QNK_File = 0,
	QNK_Index,
	QNK_Offset,
} queue_name_kind_t;


const char* queue_get_name (queue_name_kind_t kind, int process_id, int index);


#endif
