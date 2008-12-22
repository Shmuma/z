
#define ZBX_FIELD struct zbx_field_type
ZBX_FIELD
{
	char    *name;
	int	type;
	int	flags;
};

#define ZBX_TABLE struct zbx_table_type
ZBX_TABLE
{
	char    	*table;
	char		*recid;
	int		flags;
	ZBX_FIELD	fields[64];
};

static	ZBX_TABLE	tables[]={
	{"slideshows",	"slideshowid",	ZBX_SYNC,
		{
		{"slideshowid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"delay",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"slides",	"slideid",	ZBX_SYNC,
		{
		{"slideid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"slideshowid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"screenid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"step",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"delay",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"drules",	"druleid",	ZBX_SYNC,
		{
		{"druleid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"iprange",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"delay",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"nextcheck",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"status",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"siteid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{0}
		}
	},
	{"dchecks",	"dcheckid",	ZBX_SYNC,
		{
		{"dcheckid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"druleid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"type",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"key_",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"snmp_community",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"ports",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"dhosts",	"dhostid",	ZBX_SYNC,
		{
		{"dhostid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"druleid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"dns",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"ip",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"status",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"lastup",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"lastdown",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"dservices",	"dserviceid",	ZBX_SYNC,
		{
		{"dserviceid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"dhostid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"type",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"key_",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"value",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"port",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"status",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"lastup",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"lastdown",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"dalerts",	"alertid",	ZBX_SYNC,
		{
		{"alertid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"actionid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"triggerid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"userid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"mediatypeid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"sendto",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"subject",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"message",	ZBX_TYPE_BLOB,	ZBX_SYNC},
		{"status",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"retries",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"error",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"nextcheck",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"ids",	"nodeid,table_name,field_name",	0,
		{
		{"nodeid",	ZBX_TYPE_INT,	0},
		{"table_name",	ZBX_TYPE_CHAR,	0},
		{"field_name",	ZBX_TYPE_CHAR,	0},
		{"nextid",	ZBX_TYPE_ID,	0},
		{0}
		}
	},
	{"httptest",	"httptestid",	ZBX_SYNC,
		{
		{"httptestid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"applicationid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"lastcheck",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"nextcheck",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"curstate",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"curstep",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"lastfailedstep",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"delay",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"status",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"macros",	ZBX_TYPE_BLOB,	ZBX_SYNC},
		{"agent",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"time",	ZBX_TYPE_FLOAT,	ZBX_SYNC},
		{"error",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"httpstep",	"httpstepid",	ZBX_SYNC,
		{
		{"httpstepid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"httptestid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"no",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"url",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"timeout",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"posts",	ZBX_TYPE_BLOB,	ZBX_SYNC},
		{"required",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"status_codes",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"httpstepitem",	"httpstepitemid",	ZBX_SYNC,
		{
		{"httpstepitemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"httpstepid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"type",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"httptestitem",	"httptestitemid",	ZBX_SYNC,
		{
		{"httptestitemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"httptestid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"type",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"nodes",	"nodeid",	0,
		{
		{"nodeid",	ZBX_TYPE_INT,	0},
		{"name",	ZBX_TYPE_CHAR,	0},
		{"timezone",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"ip",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"port",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"slave_history",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"slave_trends",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"event_lastid",	ZBX_TYPE_ID,	0},
		{"history_lastid",	ZBX_TYPE_UINT,	0},
		{"history_str_lastid",	ZBX_TYPE_UINT,	0},
		{"history_uint_lastid",	ZBX_TYPE_UINT,	0},
		{"nodetype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"masterid",	ZBX_TYPE_INT,	0},
		{0}
		}
	},
	{"node_cksum",	"cksumid",	0,
		{
		{"cksumid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"nodeid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"tablename",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"fieldname",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"recordid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"cksumtype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"cksum",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"node_configlog",	"nodeid,conflogid",	0,
		{
		{"conflogid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"nodeid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"tablename",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"recordid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"operation",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"sync_master",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"sync_slave",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"history_str_sync",	"id",	0,
		{
		{"id",	ZBX_TYPE_UINT,	ZBX_SYNC},
		{"nodeid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"history_sync",	"id",	0,
		{
		{"id",	ZBX_TYPE_UINT,	ZBX_SYNC},
		{"nodeid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value",	ZBX_TYPE_FLOAT,	ZBX_SYNC},
		{0}
		}
	},
	{"history_uint_sync",	"id",	0,
		{
		{"id",	ZBX_TYPE_UINT,	ZBX_SYNC},
		{"nodeid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value",	ZBX_TYPE_UINT,	ZBX_SYNC},
		{0}
		}
	},
	{"services_times",	"timeid",	ZBX_SYNC,
		{
		{"timeid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"serviceid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"type",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"ts_from",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"ts_to",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"note",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"alerts",	"alertid",	ZBX_SYNC,
		{
		{"alertid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"actionid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"triggerid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"userid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"mediatypeid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"sendto",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"subject",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"message",	ZBX_TYPE_BLOB,	ZBX_SYNC},
		{"status",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"retries",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"error",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"nextcheck",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"events",	"eventid",	0,
		{
		{"eventid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"source",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"object",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"objectid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"acknowledged",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"history",	"",	0,
		{
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value",	ZBX_TYPE_FLOAT,	ZBX_SYNC},
		{0}
		}
	},
	{"history_uint",	"",	0,
		{
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value",	ZBX_TYPE_UINT,	ZBX_SYNC},
		{0}
		}
	},
	{"history_str",	"",	0,
		{
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"history_log",	"id",	0,
		{
		{"id",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"timestamp",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"source",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"severity",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value",	ZBX_TYPE_TEXT,	ZBX_SYNC},
		{0}
		}
	},
	{"history_text",	"id",	0,
		{
		{"id",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value",	ZBX_TYPE_TEXT,	ZBX_SYNC},
		{0}
		}
	},
	{"trends",	"itemid,clock",	0,
		{
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"num",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value_min",	ZBX_TYPE_FLOAT,	ZBX_SYNC},
		{"value_avg",	ZBX_TYPE_FLOAT,	ZBX_SYNC},
		{"value_max",	ZBX_TYPE_FLOAT,	ZBX_SYNC},
		{0}
		}
	},
	{"acknowledges",	"acknowledgeid",	ZBX_SYNC,
		{
		{"acknowledgeid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"userid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"eventid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"message",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"actions",	"actionid",	ZBX_SYNC,
		{
		{"actionid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"eventsource",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"evaltype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"status",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"operations",	"operationid",	ZBX_SYNC,
		{
		{"operationid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"actionid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"operationtype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"object",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"objectid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"shortdata",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"longdata",	ZBX_TYPE_BLOB,	ZBX_SYNC},
		{0}
		}
	},
	{"applications",	"applicationid",	ZBX_SYNC,
		{
		{"applicationid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"hostid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"templateid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{0}
		}
	},
	{"auditlog",	"auditid",	0,
		{
		{"auditid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"userid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"action",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"resourcetype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"details",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"conditions",	"conditionid",	ZBX_SYNC,
		{
		{"conditionid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"actionid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"conditiontype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"operator",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"config",	"configid",	ZBX_SYNC,
		{
		{"configid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"alert_history",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"event_history",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"refresh_unsupported",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"work_period",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"alert_usrgrpid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{0}
		}
	},
	{"functions",	"functionid",	ZBX_SYNC,
		{
		{"functionid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"triggerid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"lastvalue",	ZBX_TYPE_CHAR,	0},
		{"function",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"parameter",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"graphs",	"graphid",	ZBX_SYNC,
		{
		{"graphid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"width",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"height",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"yaxistype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"yaxismin",	ZBX_TYPE_FLOAT,	ZBX_SYNC},
		{"yaxismax",	ZBX_TYPE_FLOAT,	ZBX_SYNC},
		{"templateid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"show_work_period",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"show_triggers",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"graphtype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"description",	ZBX_TYPE_TEXT,	ZBX_SYNC},
		{0}
		}
	},
	{"graphs_items",	"gitemid",	ZBX_SYNC,
		{
		{"gitemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"graphid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"drawtype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"sortorder",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"color",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"yaxisside",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"calc_fnc",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"type",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"periods_cnt",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"groups",	"groupid",	ZBX_SYNC,
		{
		{"groupid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"help_items",	"itemtype,key_",	0,
		{
		{"itemtype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"key_",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"description",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"hosts",	"hostid",	ZBX_SYNC,
		{
		{"hostid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"host",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"dns",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"useip",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"ip",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"port",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"status",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"disable_until",	ZBX_TYPE_INT,	0},
		{"error",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"available",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"errors_from",	ZBX_TYPE_INT,	0},
		{"siteid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{0}
		}
	},
	{"sites",	"siteid",	ZBX_SYNC,
		{
		{"siteid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"description",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"db_url",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"active",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"hosts_groups",	"hostgroupid",	ZBX_SYNC,
		{
		{"hostgroupid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"hostid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"groupid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{0}
		}
	},
	{"hosts_profiles",	"hostid",	ZBX_SYNC,
		{
		{"hostid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"devicetype",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"os",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"serialno",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"tag",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"macaddress",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"hardware",	ZBX_TYPE_BLOB,	ZBX_SYNC},
		{"software",	ZBX_TYPE_BLOB,	ZBX_SYNC},
		{"contact",	ZBX_TYPE_BLOB,	ZBX_SYNC},
		{"location",	ZBX_TYPE_BLOB,	ZBX_SYNC},
		{"notes",	ZBX_TYPE_BLOB,	ZBX_SYNC},
		{0}
		}
	},
	{"hosts_templates",	"hosttemplateid",	ZBX_SYNC,
		{
		{"hosttemplateid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"hostid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"templateid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{0}
		}
	},
	{"housekeeper",	"housekeeperid",	0,
		{
		{"housekeeperid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"tablename",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"field",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"value",	ZBX_TYPE_ID,	ZBX_SYNC},
		{0}
		}
	},
	{"images",	"imageid",	ZBX_SYNC,
		{
		{"imageid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"imagetype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"image",	ZBX_TYPE_BLOB,	ZBX_SYNC},
		{0}
		}
	},
	{"items",	"itemid",	ZBX_SYNC,
		{
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"type",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"snmp_community",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"snmp_oid",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"snmp_port",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"hostid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"description",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"key_",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"delay",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"history",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"trends",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"nextcheck",	ZBX_TYPE_INT,	0},
		{"lastvalue",	ZBX_TYPE_CHAR,	0},
		{"lastclock",	ZBX_TYPE_INT,	0},
		{"prevvalue",	ZBX_TYPE_CHAR,	0},
		{"status",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value_type",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"trapper_hosts",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"units",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"multiplier",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"delta",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"prevorgvalue",	ZBX_TYPE_CHAR,	0},
		{"snmpv3_securityname",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"snmpv3_securitylevel",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"snmpv3_authpassphrase",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"snmpv3_privpassphrase",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"formula",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"error",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"lastlogsize",	ZBX_TYPE_INT,	0},
		{"logtimefmt",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"templateid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"valuemapid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"delay_flex",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"params",	ZBX_TYPE_TEXT,	ZBX_SYNC},
		{"siteid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"stderr",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"items_applications",	"itemappid",	ZBX_SYNC,
		{
		{"itemappid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"applicationid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"itemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{0}
		}
	},
	{"mappings",	"mappingid",	ZBX_SYNC,
		{
		{"mappingid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"valuemapid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"value",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"newvalue",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"media",	"mediaid",	ZBX_SYNC,
		{
		{"mediaid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"userid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"mediatypeid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"sendto",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"active",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"severity",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"period",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"media_type",	"mediatypeid",	ZBX_SYNC,
		{
		{"mediatypeid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"type",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"description",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"smtp_server",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"smtp_helo",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"smtp_email",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"exec_path",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"gsm_modem",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"username",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"passwd",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"profiles",	"profileid",	ZBX_SYNC,
		{
		{"profileid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"userid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"idx",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"value",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"valuetype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"rights",	"rightid",	ZBX_SYNC,
		{
		{"rightid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"groupid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"type",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"permission",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"id",	ZBX_TYPE_ID,	ZBX_SYNC},
		{0}
		}
	},
	{"screens",	"screenid",	ZBX_SYNC,
		{
		{"screenid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"hsize",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"vsize",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"screens_items",	"screenitemid",	ZBX_SYNC,
		{
		{"screenitemid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"screenid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"resourcetype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"resourceid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"width",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"height",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"x",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"y",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"colspan",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"rowspan",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"elements",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"valign",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"halign",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"style",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"url",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"services",	"serviceid",	ZBX_SYNC,
		{
		{"serviceid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"status",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"algorithm",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"triggerid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"showsla",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"goodsla",	ZBX_TYPE_FLOAT,	ZBX_SYNC},
		{"sortorder",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"service_alarms",	"servicealarmid",	0,
		{
		{"servicealarmid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"serviceid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"clock",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"services_links",	"linkid",	ZBX_SYNC,
		{
		{"linkid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"serviceupid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"servicedownid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"soft",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"sessions",	"sessionid",	0,
		{
		{"sessionid",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"userid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"lastaccess",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"sysmaps_links",	"linkid",	ZBX_SYNC,
		{
		{"linkid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"sysmapid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"selementid1",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"selementid2",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"triggerid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"drawtype_off",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"color_off",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"drawtype_on",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"color_on",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"sysmaps_elements",	"selementid",	ZBX_SYNC,
		{
		{"selementid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"sysmapid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"elementid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"elementtype",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"iconid_off",	ZBX_TYPE_UINT,	ZBX_SYNC},
		{"iconid_on",	ZBX_TYPE_UINT,	ZBX_SYNC},
		{"iconid_unknown",	ZBX_TYPE_UINT,	ZBX_SYNC},
		{"label",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"label_location",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"x",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"y",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"url",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"sysmaps",	"sysmapid",	ZBX_SYNC,
		{
		{"sysmapid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"width",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"height",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"backgroundid",	ZBX_TYPE_UINT,	ZBX_SYNC},
		{"label_type",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"label_location",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"triggers",	"triggerid",	ZBX_SYNC,
		{
		{"triggerid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"expression",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"description",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"url",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"status",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"value",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"priority",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"lastchange",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"dep_level",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"comments",	ZBX_TYPE_BLOB,	ZBX_SYNC},
		{"error",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"templateid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{0}
		}
	},
	{"trigger_depends",	"triggerdepid",	ZBX_SYNC,
		{
		{"triggerdepid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"triggerid_down",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"triggerid_up",	ZBX_TYPE_ID,	ZBX_SYNC},
		{0}
		}
	},
	{"users",	"userid",	ZBX_SYNC,
		{
		{"userid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"alias",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"surname",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"passwd",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"url",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"autologout",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"lang",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{"refresh",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"type",	ZBX_TYPE_INT,	ZBX_SYNC},
		{"options_bits",	ZBX_TYPE_INT,	ZBX_SYNC},
		{0}
		}
	},
	{"usrgrp",	"usrgrpid",	ZBX_SYNC,
		{
		{"usrgrpid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{"users_groups",	"id",	ZBX_SYNC,
		{
		{"id",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"usrgrpid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"userid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{0}
		}
	},
	{"valuemaps",	"valuemapid",	ZBX_SYNC,
		{
		{"valuemapid",	ZBX_TYPE_ID,	ZBX_SYNC},
		{"name",	ZBX_TYPE_CHAR,	ZBX_SYNC},
		{0}
		}
	},
	{0}
};
