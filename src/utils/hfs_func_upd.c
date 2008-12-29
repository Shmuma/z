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
	static struct cfg_line cfg[] = {
		{"DBHost",&CONFIG_DBHOST,0,TYPE_STRING,PARM_OPT,0,0},
		{"DBName",&CONFIG_DBNAME,0,TYPE_STRING,PARM_MAND,0,0},
		{"DBUser",&CONFIG_DBUSER,0,TYPE_STRING,PARM_OPT,0,0},
		{"DBPassword",&CONFIG_DBPASSWORD,0,TYPE_STRING,PARM_OPT,0,0},
		{"DBSocket",&CONFIG_DBSOCKET,0,TYPE_STRING,PARM_OPT,0,0},
		{"DBPort",&CONFIG_DBPORT,0,TYPE_INT,PARM_OPT,1024,65535},
		{"ServerSite",&CONFIG_SERVER_SITE,0,TYPE_STRING,PARM_OPT,0,0},	
		{0}
	};

	/* read config file */
	parse_cfg_file ("/etc/zabbix/zabbix_server.conf", cfg);

	/* connect to database */
	DBconnect (ZBX_DB_CONNECT_EXIT);
	/* select all lastvalues from functions */
	/* update them in HFS */
	DBclose ();
}

