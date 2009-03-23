#ifndef __HISTORY_INTERNAL_FS__
#define __HISTORY_INTERNAL_FS__

#define HFS_TRENDS_INTERVAL 3600

/* internal structures */
typedef struct hfs_meta_item {
    hfs_time_t start, end;
    int delay;
    item_type_t type;
    hfs_off_t ofs;
} hfs_meta_item_t;

typedef struct __attribute__ ((packed)) hfs_meta {
    int blocks;
    int last_delay;
    item_type_t last_type;
    hfs_off_t last_ofs;
    hfs_meta_item_t* meta;
} hfs_meta_t;


typedef struct {
	hfs_time_t clock;
	char* entry;
	hfs_time_t timestamp;
	char* source;
	int severity;
} hfs_log_entry_t;

#define TPL_HFS_LOG_ENTRY "S(UsUsi)"


typedef struct {
	hfs_time_t clock;
	char* prev;
	char* last;
	hfs_time_t timestamp;
	char* source;
	int severity;
	char* stderr;
} hfs_log_last_t;

#define TPL_HFS_LOG_LAST "S(UssUsis)"


typedef struct __attribute__ ((packed)) {
	hfs_time_t clock;
	hfs_off_t ofs;
} hfs_log_dir_t;



typedef enum {
	NK_ItemData,
	NK_ItemMeta,
	NK_TrendItemData,
	NK_TrendItemMeta,
	NK_HostState,
	NK_ItemValues,
	NK_ItemStatus,
	NK_ItemString,
	NK_ItemLog,
	NK_ItemLogDir,
	NK_TriggerStatus,
	NK_Alert,
	NK_HostError,
	NK_EventTrigger,
	NK_EventHost,
	NK_FunctionVals,
	NK_AggrSlaveVal,
} name_kind_t;

typedef void (*fold_fn_t) (void* db_val, void* state);


typedef struct __attribute__ ((packed)) {
	int value;
	hfs_time_t when;
} trigger_value_t;



int is_trend_type (item_type_t type);
int make_directories (const char* path);
hfs_meta_t* read_metafile (const char* metafile, const char* siteid);
hfs_meta_t* read_meta (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int trend);
int write_metafile (const char* filename, hfs_meta_t* meta, hfs_meta_item_t* extra);
void free_meta (hfs_meta_t* meta);
char* get_name (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, name_kind_t kind);
int hfs_store_value (const char* meta_path, const char* data_path, hfs_time_t clock, int delay, void* value, int len, item_type_t type);
int hfs_store_values (const char* meta_path, const char* data_path, hfs_time_t clock, int delay, void* values, int len, int count, item_type_t type);

int store_values (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t clock, int delay, void* value, int len, int count, item_type_t type);
int store_value_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t clock, const char* value, item_type_t type);
hfs_off_t find_meta_ofs (hfs_time_t time, hfs_meta_t* meta, int* block);
hfs_time_t get_data_index_from_ts (hfs_time_t ts);
void foldl_time (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t ts, void* init_res, fold_fn_t fn);
void foldl_count (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t count, void* init_res, fold_fn_t fn);
int is_valid_val (void* val, size_t len);

int obtain_lock (int fd, int write);
int release_lock (int fd, int write);

// void write_str (int fd, const char* str);
// char* read_str (int fd);

void recalculate_trend (hfs_trend_t* new, hfs_trend_t old, item_type_t type);

#endif /* __HISTORY_INTERNAL_FS__ */
