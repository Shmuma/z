
#include "common.h"
#include "db.h"


char *progname = "test";
char title_message[] = "Title";
char usage_message[] = "Usage";
char *help_message[] = { "Help", 0 };

zbx_process_type_t process_type = -1;

int CONFIG_MEMCACHE_ITEMS_TTL;
size_t DB_ITEM_OFFSETS[CHARS_LEN_MAX];
char *CONFIG_MEMCACHE_SERVER;
char *CONFIG_HFS_PATH;
char *CONFIG_SERVER_SITE;


char   *CONFIG_DBHOST                  = NULL;
char   *CONFIG_DBNAME                  = NULL;
char   *CONFIG_DBUSER                  = NULL;
char   *CONFIG_DBPASSWORD              = NULL;
char   *CONFIG_DBSOCKET                = NULL;
int    CONFIG_DBPORT                   = 0;

char   *CONFIG_HFS_PATH                = NULL;
char   *CONFIG_SERVER_SITE             = NULL;

int    CONFIG_NODEID                   = 0;
int    CONFIG_REFRESH_UNSUPPORTED      = 0;


/* Ugly, ugly hacks. */
void __zbx_zabbix_syslog(const char *fmt, ...)
{
}

int  process_event(DB_EVENT *event)
{
       return 0;
}
