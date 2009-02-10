#ifndef __FAKE_VARS_H__
#define __FAKE_VARS_H__


extern char *progname;
extern char title_message[];
extern char usage_message[];
extern char *help_message[];

extern zbx_process_type_t process_type;

extern int CONFIG_MEMCACHE_ITEMS_TTL;
extern size_t DB_ITEM_OFFSETS[];
extern char *CONFIG_MEMCACHE_SERVER;
extern char *CONFIG_HFS_PATH;
extern char *CONFIG_SERVER_SITE;

extern char   *CONFIG_DBHOST;
extern char   *CONFIG_DBNAME;
extern char   *CONFIG_DBUSER;
extern char   *CONFIG_DBPASSWORD;
extern char   *CONFIG_DBSOCKET;
extern int    CONFIG_DBPORT;

extern char   *CONFIG_HFS_PATH;
extern char   *CONFIG_SERVER_SITE;

extern int    CONFIG_NODEID;
extern int    CONFIG_REFRESH_UNSUPPORTED;


#endif
