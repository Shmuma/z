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

typedef struct hfs_meta {
    int blocks;
    int last_delay;
    item_type_t last_type;
    hfs_off_t last_ofs;
    hfs_meta_item_t* meta;
} hfs_meta_t;

typedef enum {
	NK_ItemData,
	NK_ItemMeta,
	NK_TrendItemData,
	NK_TrendItemMeta,
	NK_HostState,
	NK_ItemValues,
	NK_ItemStatus,
	NK_ItemStderr,
	NK_ItemString,
	NK_TriggerStatus,
	NK_Alert,
} name_kind_t;

typedef void (*fold_fn_t) (void* db_val, void* state);

int is_trend_type (item_type_t type);
int make_directories (const char* path);
hfs_meta_t* read_metafile (const char* metafile);
hfs_meta_t* read_meta (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, int trend);
void free_meta (hfs_meta_t* meta);
char* get_name (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, name_kind_t kind);
int hfs_store_value (const char* meta_path, const char* data_path, hfs_time_t clock, int delay, void* value, int len, item_type_t type);
int store_value (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t clock, int delay, void* value, int len, item_type_t type);
int store_value_str (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t clock, const char* value, item_type_t type);
hfs_off_t find_meta_ofs (hfs_time_t time, hfs_meta_t* meta, int* block);
hfs_time_t get_data_index_from_ts (hfs_time_t ts);
void foldl_time (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t ts, void* init_res, fold_fn_t fn);
void foldl_count (const char* hfs_base_dir, const char* siteid, zbx_uint64_t itemid, hfs_time_t count, void* init_res, fold_fn_t fn);
int is_valid_val (void* val, size_t len);

int obtain_lock (int fd, int write);
int release_lock (int fd, int write);

void write_str (int fd, const char* str);
char* read_str (int fd);

void recalculate_trend (hfs_trend_t* new, hfs_trend_t old, item_type_t type);

#endif /* __HISTORY_INTERNAL_FS__ */
