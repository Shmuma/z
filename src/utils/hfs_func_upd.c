#include "config.h"
#include "cfg.h"
#include "db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hfs.h"
#include "hfs_internal.h"

char *progname = "test";
char title_message[] = "Title";
char usage_message[] = "Usage";
char *help_message[] = { "Help", 0 };


char	*CONFIG_DBHOST			= NULL;
char	*CONFIG_DBNAME			= NULL;
char	*CONFIG_DBUSER			= NULL;
char	*CONFIG_DBPASSWORD		= NULL;
char	*CONFIG_DBSOCKET		= NULL;
int	CONFIG_DBPORT			= 0;

char	*CONFIG_HFS_PATH		= NULL;
char	*CONFIG_SERVER_SITE		= NULL;

int	CONFIG_NODEID			= 0;
int	CONFIG_REFRESH_UNSUPPORTED      = 0;

#ifdef HAVE_MEMCACHE
char *CONFIG_MEMCACHE_SERVER		= NULL;
int CONFIG_MEMCACHE_ITEMS_TTL		= 30;
#endif

zbx_process_type_t process_type = -1;


/* Ugly, ugly hacks. */
void __zbx_zabbix_syslog(const char *fmt, ...)
{
}

int  process_event(DB_EVENT *event)
{
	return 0;
}

void dbitem_serialize(DB_ITEM *item, size_t item_len)
{
}


int main (int argc, char** argv)
{
	DB_RESULT	result;
	DB_ROW		row;
	zbx_uint64_t	id;
	char*		value;
	hfs_function_value_t fun_val;
	int		index = 0;

	static struct cfg_line cfg[] = {
		{"DBHost",&CONFIG_DBHOST,0,TYPE_STRING,PARM_OPT,0,0},
		{"DBName",&CONFIG_DBNAME,0,TYPE_STRING,PARM_MAND,0,0},
		{"DBUser",&CONFIG_DBUSER,0,TYPE_STRING,PARM_OPT,0,0},
		{"DBPassword",&CONFIG_DBPASSWORD,0,TYPE_STRING,PARM_OPT,0,0},
		{"DBSocket",&CONFIG_DBSOCKET,0,TYPE_STRING,PARM_OPT,0,0},
		{"DBPort",&CONFIG_DBPORT,0,TYPE_INT,PARM_OPT,1024,65535},
		{"ServerHistoryFSPath",&CONFIG_HFS_PATH,0,TYPE_STRING,PARM_OPT,0,0},
		{"ServerSite",&CONFIG_SERVER_SITE,0,TYPE_STRING,PARM_OPT,0,0},	
		{0}
	};

	/* read config file */
	parse_cfg_file ("/etc/zabbix/zabbix_server.conf", cfg);

	/* connect to database */
	DBconnect (ZBX_DB_CONNECT_EXIT);
	/* select all lastvalues from functions */
	result = DBselect ("select functionid, lastvalue from functions where lastvalue != ''");

	/* update them in HFS */
	while (row = DBfetch (result)) {
		ZBX_STR2UINT64 (id, row[0]);
		value = row[1];

		if (HFS_convert_function_str2val (value, &fun_val))
			if (HFS_save_function_value (CONFIG_HFS_PATH, CONFIG_SERVER_SITE, id, &fun_val))
				printf ("Converted item %d of %lld. ID=%lld, val='%s', type=%d\n", index++, result->row_count, id, value, fun_val.type);
			else
				printf ("Item %d save failed. ID=%lld, val=%s\n", index++, id, value);
		else
			printf ("Item %d convert failed. ID=%lld, val=%s\n", index++, id, value);
	}

	DBclose ();
}

