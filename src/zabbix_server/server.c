/* 
** ZABBIX
** Copyright (C) 2000-2005 SIA Zabbix
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**/

/*#define ZABBIX_TEST*/

#include "common.h"

#include "cfg.h"
#include "pid.h"
#include "db.h"
#include "log.h"
#include "zlog.h"
#include "zbxgetopt.h"
#include "metrics.h"

#include "functions.h"
#include "expression.h"
#include "sysinfo.h"

#include "daemon.h"

#include "alerter/alerter.h"
#include "discoverer/discoverer.h"
#include "httppoller/httppoller.h"
#include "housekeeper/housekeeper.h"
#include "pinger/pinger.h"
#include "poller/poller.h"
#include "poller/checks_snmp.h"
#include "timer/timer.h"
#include "trapper/trapper.h"
#include "nodewatcher/nodewatcher.h"
#include "watchdog/watchdog.h"
#include "utils/nodechange.h"
#include "feeder/feeder.h"
#include "aggr_slave/aggr_slave.h"

#define       LISTENQ 1024

#ifdef ZABBIX_TEST
#include <time.h>
#endif

size_t DB_ITEM_OFFSETS[CHARS_LEN_MAX] = {
/*  0 */	offsetof(DB_ITEM, description),
/*  1 */	offsetof(DB_ITEM, key),
/*  2 */	offsetof(DB_ITEM, host_name),
/*  3 */	offsetof(DB_ITEM, host_ip),
/*  4 */	offsetof(DB_ITEM, host_dns),
/*  5 */	offsetof(DB_ITEM, shortname),
/*  6 */	offsetof(DB_ITEM, snmp_community),
/*  7 */	offsetof(DB_ITEM, snmp_oid),
/*  8 */	offsetof(DB_ITEM, trapper_hosts),
/*  9 */	offsetof(DB_ITEM, units),
/* 10 */	offsetof(DB_ITEM, snmpv3_securityname),
/* 11 */	offsetof(DB_ITEM, snmpv3_authpassphrase),
/* 12 */	offsetof(DB_ITEM, snmpv3_privpassphrase),
/* 13 */	offsetof(DB_ITEM, formula),
/* 14 */	offsetof(DB_ITEM, logtimefmt),
/* 15 */	offsetof(DB_ITEM, delay_flex),
/* 16 */	offsetof(DB_ITEM, eventlog_source)
};

zbx_process_type_t process_type = -1;

char *progname = NULL;
char title_message[] = "ZABBIX Server (daemon)";
char usage_message[] = "[-hV] [-c <file>] [-n <nodeid>]";

#ifndef HAVE_GETOPT_LONG
char *help_message[] = {
        "Options:",
        "  -c <file>       Specify configuration file",
        "  -h              give this help",
        "  -n <nodeid>     convert database data to new nodeid",
        "  -V              display version number",
        0 /* end of text */
};
#else
char *help_message[] = {
        "Options:",
        "  -c --config <file>       Specify configuration file",
        "  -h --help                give this help",
        "  -n --new-nodeid <nodeid> convert database data to new nodeid",
        "  -V --version             display version number",
        0 /* end of text */
};
#endif

/* COMMAND LINE OPTIONS */

/* long options */

static struct zbx_option longopts[] =
{
	{"config",	1,	0,	'c'},
	{"help",	0,	0,	'h'},
	{"new-nodeid",	1,	0,	'n'},
	{"version",	0,	0,	'V'},

#if defined (_WINDOWS)

	{"install",	0,	0,	'i'},
	{"uninstall",	0,	0,	'd'},

	{"start",	0,	0,	's'},
	{"stop",	0,	0,	'x'},

#endif /* _WINDOWS */

	{0,0,0,0}
};

/* short options */

static char	shortopts[] = 
	"c:n:hV"
#if defined (_WINDOWS)
	"idsx"
#endif /* _WINDOWS */
	;

/* end of COMMAND LINE OPTIONS*/

pid_t	*threads=NULL;

int	CONFIG_ALERTER_FORKS		= 1;
int	CONFIG_DISCOVERER_FORKS		= 1;
int	CONFIG_HOUSEKEEPER_FORKS	= 1;
int	CONFIG_PINGER_FORKS		= 1;
int	CONFIG_POLLER_FORKS		= 5;
int	CONFIG_HTTPPOLLER_FORKS		= 5;
int	CONFIG_TIMER_FORKS		= 1;
int	CONFIG_TRAPPERD_FORKS		= 5;
int	CONFIG_FEEDER_FORKS		= 5;
int	CONFIG_UNREACHABLE_POLLER_FORKS	= 1;
int	CONFIG_AGGR_POLLER_FORKS	= 5;
int	CONFIG_AGGR_SLAVE_FORKS		= 5;

/* Time in minutes main trigger must be in OFF state to send
   notifications about depended triggers */
int	CONFIG_DEPENDENCY_THRESHOLD	= 10;

char	*CONFIG_SERVER_MODE		= NULL;

int	CONFIG_LISTEN_PORT		= 10051;
char	*CONFIG_LISTEN_IP		= NULL;
char	*CONFIG_POLLER_IP               = NULL;
int	CONFIG_TRAPPER_TIMEOUT		= TRAPPER_TIMEOUT;
/**/
/*int	CONFIG_NOTIMEWAIT		=0;*/
int	CONFIG_HOUSEKEEPING_FREQUENCY	= 1;
int	CONFIG_SENDER_FREQUENCY		= 30;
int	CONFIG_PINGER_FREQUENCY		= 60;
/*int	CONFIG_DISABLE_PINGER		= 0;*/
int	CONFIG_DISABLE_HOUSEKEEPING	= 0;
int	CONFIG_UNREACHABLE_PERIOD	= 45;
int	CONFIG_UNREACHABLE_DELAY	= 15;
int	CONFIG_UNAVAILABLE_DELAY	= 60;
int	CONFIG_LOG_LEVEL		= LOG_LEVEL_WARNING;
char	*CONFIG_ALERT_SCRIPTS_PATH	= NULL;
char	*CONFIG_EXTERNALSCRIPTS		= NULL;
char	*CONFIG_FPING_LOCATION		= NULL;
char	*CONFIG_DBHOST			= NULL;
char	*CONFIG_DBNAME			= NULL;
char	*CONFIG_DBUSER			= NULL;
char	*CONFIG_DBPASSWORD		= NULL;
char	*CONFIG_DBSOCKET		= NULL;
int	CONFIG_DBPORT			= 0;
int	CONFIG_ENABLE_REMOTE_COMMANDS	= 0;

int	CONFIG_NODEID			= 0;
int	CONFIG_MASTER_NODEID		= 0;
int	CONFIG_NODE_NOEVENTS		= 0;
int	CONFIG_NODE_NOHISTORY		= 0;

char	*CONFIG_SERVER_SITE		= NULL;

char	*CONFIG_MASTER_IP		= NULL;
int	CONFIG_MASTER_PORT		= 10051;

/* Global variable to control if we should write warnings to log[] */
int	CONFIG_ENABLE_LOG		= 1;

/* From table config */
int	CONFIG_REFRESH_UNSUPPORTED	= 0;

/* Zabbix server sturtup time */
int     CONFIG_SERVER_STARTUP_TIME      = 0;

/* Path to store History on disk (directory). If not specified, old DB engine used. */
char	*CONFIG_HFS_PATH		= NULL;

#ifdef HAVE_MEMCACHE
char *CONFIG_MEMCACHE_SERVER		= NULL;
int CONFIG_MEMCACHE_ITEMS_TTL		= 30;
int CONFIG_MEMCACHE_META_TTL		= 60;
#endif

/******************************************************************************
 *                                                                            *
 * Function: init_config                                                      *
 *                                                                            *
 * Purpose: parse config file and update configuration parameters             *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value:                                                              *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments: will terminate process if parsing fails                          *
 *                                                                            *
 ******************************************************************************/
void	init_config(void)
{
	static struct cfg_line cfg[]=
	{
/*		 PARAMETER	,VAR	,FUNC,	TYPE(0i,1s),MANDATORY,MIN,MAX	*/
		{"StartDiscoverers",&CONFIG_DISCOVERER_FORKS,0,TYPE_INT,PARM_OPT,0,255},
		{"StartHTTPPollers",&CONFIG_HTTPPOLLER_FORKS,0,TYPE_INT,PARM_OPT,0,255},
		{"StartPingers",&CONFIG_PINGER_FORKS,0,TYPE_INT,PARM_OPT,0,255},
		{"StartPollers",&CONFIG_POLLER_FORKS,0,TYPE_INT,PARM_OPT,0,255},
		{"StartPollersUnreachable",&CONFIG_UNREACHABLE_POLLER_FORKS,0,TYPE_INT,PARM_OPT,0,255},
		{"StartPollersAggregate",&CONFIG_AGGR_POLLER_FORKS,0,TYPE_INT,PARM_OPT,0,255},
		{"StartAggregateSlave",&CONFIG_AGGR_SLAVE_FORKS,0,TYPE_INT,PARM_OPT,0,255},
		{"StartTrappers",&CONFIG_TRAPPERD_FORKS,0,TYPE_INT,PARM_OPT,0,255},
		{"StartFeeders",&CONFIG_FEEDER_FORKS,0,TYPE_INT,PARM_OPT,0,255},
		{"HousekeepingFrequency",&CONFIG_HOUSEKEEPING_FREQUENCY,0,TYPE_INT,PARM_OPT,1,24},
		{"SenderFrequency",&CONFIG_SENDER_FREQUENCY,0,TYPE_INT,PARM_OPT,5,3600},
		{"PingerFrequency",&CONFIG_PINGER_FREQUENCY,0,TYPE_INT,PARM_OPT,1,3600},
		{"FpingLocation",&CONFIG_FPING_LOCATION,0,TYPE_STRING,PARM_OPT,0,0},
		{"Timeout",&CONFIG_TIMEOUT,0,TYPE_INT,PARM_OPT,1,30},
		{"TrapperTimeout",&CONFIG_TRAPPER_TIMEOUT,0,TYPE_INT,PARM_OPT,1,30},
		{"UnreachablePeriod",&CONFIG_UNREACHABLE_PERIOD,0,TYPE_INT,PARM_OPT,1,3600},
		{"UnreachableDelay",&CONFIG_UNREACHABLE_DELAY,0,TYPE_INT,PARM_OPT,1,3600},
		{"UnavailableDelay",&CONFIG_UNAVAILABLE_DELAY,0,TYPE_INT,PARM_OPT,1,3600},
		{"DependencyThreshold",&CONFIG_DEPENDENCY_THRESHOLD,0,TYPE_INT,PARM_OPT,1,600},
		{"ListenIP",&CONFIG_LISTEN_IP,0,TYPE_STRING,PARM_OPT,0,0},
		{"ListenPort",&CONFIG_LISTEN_PORT,0,TYPE_INT,PARM_OPT,1024,32768},
		{"PollerIP",&CONFIG_POLLER_IP,0,TYPE_STRING,PARM_OPT,0,0},
/*		{"NoTimeWait",&CONFIG_NOTIMEWAIT,0,TYPE_INT,PARM_OPT,0,1},*/
/*		{"DisablePinger",&CONFIG_DISABLE_PINGER,0,TYPE_INT,PARM_OPT,0,1},*/
		{"DisableHousekeeping",&CONFIG_DISABLE_HOUSEKEEPING,0,TYPE_INT,PARM_OPT,0,1},
		{"DebugLevel",&CONFIG_LOG_LEVEL,0,TYPE_INT,PARM_OPT,0,4},
		{"PidFile",&APP_PID_FILE,0,TYPE_STRING,PARM_OPT,0,0},
		{"LogFile",&CONFIG_LOG_FILE,0,TYPE_STRING,PARM_OPT,0,0},
		{"LogFileSize",&CONFIG_LOG_FILE_SIZE,0,TYPE_INT,PARM_OPT,0,1024},
		{"AlertScriptsPath",&CONFIG_ALERT_SCRIPTS_PATH,0,TYPE_STRING,PARM_OPT,0,0},
		{"ExternalScripts",&CONFIG_EXTERNALSCRIPTS,0,TYPE_STRING,PARM_OPT,0,0},
		{"DBHost",&CONFIG_DBHOST,0,TYPE_STRING,PARM_OPT,0,0},
		{"DBName",&CONFIG_DBNAME,0,TYPE_STRING,PARM_MAND,0,0},
		{"DBUser",&CONFIG_DBUSER,0,TYPE_STRING,PARM_OPT,0,0},
		{"DBPassword",&CONFIG_DBPASSWORD,0,TYPE_STRING,PARM_OPT,0,0},
		{"DBSocket",&CONFIG_DBSOCKET,0,TYPE_STRING,PARM_OPT,0,0},
		{"DBPort",&CONFIG_DBPORT,0,TYPE_INT,PARM_OPT,1024,65535},
		{"NodeNoEvents",&CONFIG_NODE_NOEVENTS,0,TYPE_INT,PARM_OPT,0,1},
		{"NodeNoHistory",&CONFIG_NODE_NOHISTORY,0,TYPE_INT,PARM_OPT,0,1},
		{"ServerSite",&CONFIG_SERVER_SITE,0,TYPE_STRING,PARM_OPT,0,0},
		{"ServerMode",&CONFIG_SERVER_MODE,0,TYPE_STRING,PARM_OPT,0,0},
		{"ServerMasterIp",&CONFIG_MASTER_IP,0,TYPE_STRING,PARM_OPT,0,0},
		{"ServerMasterPort",&CONFIG_MASTER_PORT,0,TYPE_INT,PARM_OPT,0,0},
		{"ServerHistoryFSPath",&CONFIG_HFS_PATH,0,TYPE_STRING,PARM_OPT,0,0},
#ifdef HAVE_MEMCACHE
		{"MemcacheServers",&CONFIG_MEMCACHE_SERVER,0,TYPE_STRING,PARM_OPT,0,0},
		{"MemcacheItemsTTL",&CONFIG_MEMCACHE_ITEMS_TTL,0,TYPE_INT,PARM_OPT,0,0},
		{"MemcacheMetaTTL",&CONFIG_MEMCACHE_META_TTL,0,TYPE_INT,PARM_OPT,0,0},
#endif
		{0}
	};

	CONFIG_SERVER_STARTUP_TIME = time(NULL);


	parse_cfg_file(CONFIG_FILE,cfg);

	if(CONFIG_DBNAME == NULL)
	{
		zabbix_log( LOG_LEVEL_CRIT, "DBName not in config file");
		exit(1);
	}
	if(APP_PID_FILE == NULL)
	{
		APP_PID_FILE=strdup("/tmp/zabbix_server.pid");
	}
	if(CONFIG_ALERT_SCRIPTS_PATH == NULL)
	{
		CONFIG_ALERT_SCRIPTS_PATH=strdup("/home/zabbix/bin");
	}
	if(CONFIG_FPING_LOCATION == NULL)
	{
		CONFIG_FPING_LOCATION=strdup("/usr/sbin/fping");
	}
	if(CONFIG_EXTERNALSCRIPTS == NULL)
	{
		CONFIG_EXTERNALSCRIPTS=strdup("/etc/zabbix/externalscripts");
	}
#ifndef	HAVE_LIBCURL
	CONFIG_HTTPPOLLER_FORKS = 0;
#endif
}


/******************************************************************************
 *                                                                            *
 * Function: main                                                             *
 *                                                                            *
 * Purpose: executes server processes                                         *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value:                                                              *
 *                                                                            *
 * Author: Eugene Grigorjev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
static void test_child_signal_handler(int sig)
{
	exit(1);
}

int main(int argc, char **argv)
{
	zbx_task_t	task  = ZBX_TASK_START;
	char    ch      = '\0';

	int	nodeid;

	progname = argv[0];

	/* Parse the command-line. */
	while ((ch = (char)zbx_getopt_long(argc, argv, shortopts, longopts,NULL)) != (char)EOF)
	switch (ch) {
		case 'c':
			CONFIG_FILE = strdup(zbx_optarg);
			break;
		case 'h':
			help();
			exit(-1);
			break;
		case 'n':
			nodeid=0;
			if(zbx_optarg)	nodeid = atoi(zbx_optarg);
			task = ZBX_TASK_CHANGE_NODEID;
			break;
		case 'V':
			version();
			exit(-1);
			break;
		default:
			usage();
			exit(-1);
			break;
        }

	if(CONFIG_FILE == NULL)
	{
		CONFIG_FILE=strdup("/etc/zabbix/zabbix_server.conf");
	}

	/* Required for simple checks */
	init_metrics();

	init_config();

#if 0
	{
		if (0) {
			zbx_sock_t	listen_sock;

			zbx_tcp_listen(&listen_sock, CONFIG_LISTEN_IP, (unsigned short)CONFIG_LISTEN_PORT);
			memcache_zbx_connect(CONFIG_MEMCACHE_SERVER);
			process_type = ZBX_PROCESS_FEEDER;
			child_feeder_main(0, &listen_sock);
		}
		else {
			memcache_zbx_connect(CONFIG_MEMCACHE_SERVER);
			process_type = ZBX_PROCESS_AGGREGATE_SLAVE;
/* 			child_trapper_main (0); */
//			child_hist_trapper_main (0);
			main_aggregate_slave_loop (1);
		}
	}
#else
	return daemon_start(CONFIG_ALLOW_ROOT, "zabbix");
#endif
}

int MAIN_ZABBIX_ENTRY(void)
{
#ifdef HAVE_MEMCACHE
	memcached_server_st	*mem_servers;
	memcached_return	 mem_rc;
#endif

        DB_RESULT       result;
        DB_ROW          row;

	int	i;
	pid_t	pid;

	zbx_sock_t	listen_sock;

	int		server_num = 0;

	if(CONFIG_LOG_FILE == NULL)
	{
		zabbix_open_log(LOG_TYPE_SYSLOG,CONFIG_LOG_LEVEL,NULL);
	}
	else
	{
		zabbix_open_log(LOG_TYPE_FILE,CONFIG_LOG_LEVEL,CONFIG_LOG_FILE);
	}

	metrics_init ();

/*	zabbix_log( LOG_LEVEL_WARNING, "INFO [%s]", ZBX_SQL_MOD(a,%d)); */
	zabbix_log( LOG_LEVEL_WARNING, "Starting zabbix_server. ZABBIX %s.", ZABBIX_VERSION);

	zabbix_log( LOG_LEVEL_WARNING, "**** Enabled features ****");
#ifdef	HAVE_SNMP
	zabbix_log( LOG_LEVEL_WARNING, "SNMP monitoring:       YES");
#else
	zabbix_log( LOG_LEVEL_WARNING, "SNMP monitoring:        NO");
#endif
#ifdef	HAVE_LIBCURL
	zabbix_log( LOG_LEVEL_WARNING, "WEB monitoring:        YES");
#else
	zabbix_log( LOG_LEVEL_WARNING, "WEB monitoring:         NO");
#endif
#ifdef	HAVE_JABBER
	zabbix_log( LOG_LEVEL_WARNING, "Jabber notifications:  YES");
#else
	zabbix_log( LOG_LEVEL_WARNING, "Jabber notifications:   NO");
#endif
#ifdef	HAVE_IPV6
	zabbix_log( LOG_LEVEL_WARNING, "IPv6 support:          YES");
#else
	zabbix_log( LOG_LEVEL_WARNING, "IPv6 support:           NO");
#endif

	zabbix_log( LOG_LEVEL_WARNING, "**************************");

	DBconnect(ZBX_DB_CONNECT_EXIT);

	result = DBselect("select refresh_unsupported from config");

	if (NULL != (row = DBfetch(result)) && DBis_null(row[0]) != SUCCEED)
		CONFIG_REFRESH_UNSUPPORTED = atoi(row[0]);
	DBfree_result(result);

	result = DBselect("select masterid from nodes where nodeid=%d",
		CONFIG_NODEID);
	row = DBfetch(result);

	if( (row != NULL) && DBis_null(row[0]) != SUCCEED)
	{
		CONFIG_MASTER_NODEID = atoi(row[0]);
	}
	DBfree_result(result);

/* Need to set trigger status to UNKNOWN since last run */
/* DBconnect() already made in init_config() */
/*	DBconnect();*/
//	DBupdate_triggers_status_after_restart();
	DBclose();

/* To make sure that we can connect to the database before forking new processes */
	DBconnect(ZBX_DB_CONNECT_EXIT);
	DBclose();

/*#define CALC_TREND*/

#ifdef CALC_TREND
	trend();
	return 0;
#endif

	threads = calloc(1+CONFIG_POLLER_FORKS+CONFIG_TRAPPERD_FORKS*2+CONFIG_FEEDER_FORKS+CONFIG_PINGER_FORKS+CONFIG_ALERTER_FORKS
		+CONFIG_HOUSEKEEPER_FORKS+CONFIG_TIMER_FORKS+CONFIG_UNREACHABLE_POLLER_FORKS+CONFIG_AGGR_POLLER_FORKS+CONFIG_AGGR_SLAVE_FORKS
		+CONFIG_HTTPPOLLER_FORKS+CONFIG_DISCOVERER_FORKS,
		sizeof(pid_t));

	if(CONFIG_FEEDER_FORKS > 0)
	{
		if( FAIL == zbx_tcp_listen(&listen_sock, CONFIG_LISTEN_IP, (unsigned short)CONFIG_LISTEN_PORT) )
		{
			zabbix_log(LOG_LEVEL_CRIT, "Listener failed with error: %s.", zbx_tcp_strerror());
			exit(1);
		}
	}

	for(	i=1;
		i<=CONFIG_POLLER_FORKS+CONFIG_TRAPPERD_FORKS*2+CONFIG_FEEDER_FORKS+CONFIG_PINGER_FORKS+CONFIG_ALERTER_FORKS+CONFIG_HOUSEKEEPER_FORKS+CONFIG_TIMER_FORKS+CONFIG_UNREACHABLE_POLLER_FORKS+CONFIG_HTTPPOLLER_FORKS+CONFIG_AGGR_POLLER_FORKS+CONFIG_AGGR_SLAVE_FORKS+CONFIG_DISCOVERER_FORKS; 
		i++)
	{
		if((pid = zbx_fork()) == 0)
		{
			server_num = i;
			break; 
		}
		else
		{
			threads[i]=pid;
		}
	}

	/* Main process */
	if(server_num == 0)
	{
		init_main_process();
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Watchdog]",
			server_num);
		main_watchdog_loop();
	}
#ifdef HAVE_MEMCACHE
	else
		memcache_zbx_connect(CONFIG_MEMCACHE_SERVER);
#endif

	if(server_num <= CONFIG_POLLER_FORKS)
	{
#ifdef HAVE_SNMP
		init_snmp("zabbix_server");
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Poller. SNMP:ON]",
			server_num);
#else
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Poller. SNMP:OFF]",
			server_num);
#endif
		process_type = ZBX_PROCESS_POLLER;
		main_poller_loop(ZBX_POLLER_TYPE_NORMAL, server_num);
	}
	else if(server_num <= CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2)
	{
		process_type = ZBX_PROCESS_TRAPPERD;
		if ((server_num - CONFIG_POLLER_FORKS) % 2)
			child_hist_trapper_main((server_num - CONFIG_POLLER_FORKS + 1) >> 1);
		else
			child_trapper_main((server_num - CONFIG_POLLER_FORKS + 1) >> 1);
	}
	else if(server_num <= CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2 + CONFIG_FEEDER_FORKS)
	{
		process_type = ZBX_PROCESS_FEEDER;
		child_feeder_main(server_num - (CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2), &listen_sock);
	}
	else if(server_num <= CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2 + CONFIG_FEEDER_FORKS + CONFIG_PINGER_FORKS)
	{
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [ICMP pinger]",
			server_num);
		process_type = ZBX_PROCESS_PINGER;
		main_pinger_loop(server_num-(CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2 + CONFIG_FEEDER_FORKS));
	}
	else if(server_num <= CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2 + CONFIG_FEEDER_FORKS + CONFIG_PINGER_FORKS + CONFIG_ALERTER_FORKS)
	{
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Alerter]",
			server_num);
		process_type = ZBX_PROCESS_ALERTER;
		main_alerter_loop();
	}
	else if(server_num <= CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2 + CONFIG_FEEDER_FORKS + CONFIG_PINGER_FORKS + CONFIG_ALERTER_FORKS
			+CONFIG_HOUSEKEEPER_FORKS)
	{
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Housekeeper]",
			server_num);
		process_type = ZBX_PROCESS_HOUSEKEEPER;
		main_housekeeper_loop();
	}
	else if(server_num <= CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2 + CONFIG_FEEDER_FORKS + CONFIG_PINGER_FORKS + CONFIG_ALERTER_FORKS
			+CONFIG_HOUSEKEEPER_FORKS + CONFIG_TIMER_FORKS)
	{
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Timer]",
			server_num);
		process_type = ZBX_PROCESS_TIMER;
		main_timer_loop();
	}
	else if(server_num <= CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2 + CONFIG_FEEDER_FORKS + CONFIG_PINGER_FORKS + CONFIG_ALERTER_FORKS
			+CONFIG_HOUSEKEEPER_FORKS + CONFIG_TIMER_FORKS + CONFIG_UNREACHABLE_POLLER_FORKS)
	{
#ifdef HAVE_SNMP
		init_snmp("zabbix_server");
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Poller for unreachable hosts. SNMP:ON]",
			server_num);
#else
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Poller for unreachable hosts. SNMP:OFF]",
			server_num);
#endif
		process_type = ZBX_PROCESS_UNREACHABLE_POLLER;
		main_poller_loop(ZBX_POLLER_TYPE_UNREACHABLE,
				server_num - (CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2 + CONFIG_FEEDER_FORKS + CONFIG_PINGER_FORKS + CONFIG_ALERTER_FORKS+CONFIG_HOUSEKEEPER_FORKS + CONFIG_TIMER_FORKS));
	}
	else if(server_num <= CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2 + CONFIG_FEEDER_FORKS + CONFIG_PINGER_FORKS + CONFIG_ALERTER_FORKS
			+ CONFIG_HOUSEKEEPER_FORKS + CONFIG_TIMER_FORKS + CONFIG_UNREACHABLE_POLLER_FORKS
			+ CONFIG_HTTPPOLLER_FORKS)
	{
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [HTTP Poller]",
				server_num);
		process_type = ZBX_PROCESS_HTTPPOLLER;
		main_httppoller_loop(server_num - CONFIG_POLLER_FORKS - CONFIG_TRAPPERD_FORKS*2 - CONFIG_FEEDER_FORKS - CONFIG_PINGER_FORKS
				- CONFIG_ALERTER_FORKS - CONFIG_HOUSEKEEPER_FORKS - CONFIG_TIMER_FORKS
				- CONFIG_UNREACHABLE_POLLER_FORKS);
	}
	else if(server_num <= CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2 + CONFIG_FEEDER_FORKS + CONFIG_PINGER_FORKS + CONFIG_ALERTER_FORKS
			+ CONFIG_HOUSEKEEPER_FORKS + CONFIG_TIMER_FORKS + CONFIG_UNREACHABLE_POLLER_FORKS
			+ CONFIG_HTTPPOLLER_FORKS + CONFIG_DISCOVERER_FORKS)
	{
#ifdef HAVE_SNMP
		init_snmp("zabbix_server");
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Discoverer. SNMP:ON]",
				server_num);
#else
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Discoverer. SNMP:OFF]",
				server_num);
#endif
		process_type = ZBX_PROCESS_DISCOVERER;
		main_discoverer_loop(server_num - CONFIG_POLLER_FORKS - CONFIG_TRAPPERD_FORKS*2 - CONFIG_FEEDER_FORKS -CONFIG_PINGER_FORKS
				- CONFIG_ALERTER_FORKS - CONFIG_HOUSEKEEPER_FORKS - CONFIG_TIMER_FORKS
				- CONFIG_UNREACHABLE_POLLER_FORKS - CONFIG_HTTPPOLLER_FORKS);
	}
	else if(server_num <= CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2 + CONFIG_FEEDER_FORKS + CONFIG_PINGER_FORKS + CONFIG_ALERTER_FORKS
			+ CONFIG_HOUSEKEEPER_FORKS + CONFIG_TIMER_FORKS + CONFIG_UNREACHABLE_POLLER_FORKS
			+ CONFIG_HTTPPOLLER_FORKS + CONFIG_DISCOVERER_FORKS + CONFIG_AGGR_POLLER_FORKS)
	{
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Aggregate Poller]",
			server_num);
		process_type = ZBX_PROCESS_POLLER;
		main_poller_loop(ZBX_POLLER_TYPE_AGGREGATE, server_num - CONFIG_POLLER_FORKS - CONFIG_TRAPPERD_FORKS*2 
				 - CONFIG_FEEDER_FORKS -CONFIG_PINGER_FORKS
				 - CONFIG_ALERTER_FORKS - CONFIG_HOUSEKEEPER_FORKS - CONFIG_TIMER_FORKS
				 - CONFIG_UNREACHABLE_POLLER_FORKS - CONFIG_HTTPPOLLER_FORKS - CONFIG_DISCOVERER_FORKS);
	}
	else if(server_num <= CONFIG_POLLER_FORKS + CONFIG_TRAPPERD_FORKS*2 + CONFIG_FEEDER_FORKS + CONFIG_PINGER_FORKS + CONFIG_ALERTER_FORKS
			+ CONFIG_HOUSEKEEPER_FORKS + CONFIG_TIMER_FORKS + CONFIG_UNREACHABLE_POLLER_FORKS
			+ CONFIG_HTTPPOLLER_FORKS + CONFIG_DISCOVERER_FORKS + CONFIG_AGGR_POLLER_FORKS + CONFIG_AGGR_SLAVE_FORKS)
	{
		zabbix_log( LOG_LEVEL_WARNING, "server #%d started [Aggregate Slave]", server_num);
		process_type = ZBX_PROCESS_AGGREGATE_SLAVE;
		main_aggregate_slave_loop(server_num - CONFIG_POLLER_FORKS - CONFIG_TRAPPERD_FORKS*2 
				 - CONFIG_FEEDER_FORKS -CONFIG_PINGER_FORKS
				 - CONFIG_ALERTER_FORKS - CONFIG_HOUSEKEEPER_FORKS - CONFIG_TIMER_FORKS
				 - CONFIG_UNREACHABLE_POLLER_FORKS - CONFIG_HTTPPOLLER_FORKS - CONFIG_DISCOVERER_FORKS - CONFIG_AGGR_POLLER_FORKS);
	}
#ifdef HAVE_MEMCACHE
	memcache_zbx_disconnect();
#endif
	return SUCCEED;
}

void	zbx_on_exit()
{
#if !defined(_WINDOWS)
	
	int i = 0;

	if(threads != NULL)
	{
		for(i = 1; i <= CONFIG_POLLER_FORKS+CONFIG_AGGR_POLLER_FORKS+CONFIG_AGGR_SLAVE_FORKS+CONFIG_TRAPPERD_FORKS*2 + CONFIG_FEEDER_FORKS+CONFIG_PINGER_FORKS+CONFIG_ALERTER_FORKS+CONFIG_HOUSEKEEPER_FORKS+CONFIG_TIMER_FORKS+CONFIG_UNREACHABLE_POLLER_FORKS+CONFIG_HTTPPOLLER_FORKS+CONFIG_DISCOVERER_FORKS; i++)
		{
			if(threads[i]) {
				kill(threads[i],SIGTERM);
				threads[i] = (ZBX_THREAD_HANDLE)NULL;
			}
		}
	}
	
#endif /* not _WINDOWS */

#ifdef USE_PID_FILE

	daemon_stop();

#endif /* USE_PID_FILE */

	free_metrics();

	zbx_sleep(2); /* wait for all threads closing */
	
	zabbix_log(LOG_LEVEL_INFORMATION, "ZABBIX Server stopped");
	zabbix_close_log();
	
#ifdef  HAVE_SQLITE3
	php_sem_remove(&sqlite_access);
#endif /* HAVE_SQLITE3 */

	exit(SUCCEED);
}

