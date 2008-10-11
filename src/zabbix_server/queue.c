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
	case QNK_Offset:
		snprintf (buf, sizeof (buf), "%s/%s/queue/queue_ofs.%d", CONFIG_HFS_PATH, CONFIG_SERVER_SITE, process_id);
		break;
	}

	return buf;
}
