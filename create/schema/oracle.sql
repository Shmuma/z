CREATE TABLE slideshows (
	slideshowid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(255)		DEFAULT ''	,
	delay		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (slideshowid)
);
CREATE TABLE slides (
	slideid		number(20)		DEFAULT '0'	NOT NULL,
	slideshowid		number(20)		DEFAULT '0'	NOT NULL,
	screenid		number(20)		DEFAULT '0'	NOT NULL,
	step		number(10)		DEFAULT '0'	NOT NULL,
	delay		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (slideid)
);
CREATE INDEX slides_slides_1 on slides (slideshowid);

CREATE TABLE drules (
	druleid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(255)		DEFAULT ''	,
	iprange		varchar2(255)		DEFAULT ''	,
	delay		number(10)		DEFAULT '0'	NOT NULL,
	nextcheck		number(10)		DEFAULT '0'	NOT NULL,
	status		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (druleid)
);
CREATE TABLE dchecks (
	dcheckid		number(20)		DEFAULT '0'	NOT NULL,
	druleid		number(20)		DEFAULT '0'	NOT NULL,
	type		number(10)		DEFAULT '0'	NOT NULL,
	key_		varchar2(255)		DEFAULT '0'	,
	snmp_community		varchar2(255)		DEFAULT '0'	,
	ports		varchar2(255)		DEFAULT '0'	,
	PRIMARY KEY (dcheckid)
);
CREATE TABLE dhosts (
	dhostid		number(20)		DEFAULT '0'	NOT NULL,
	druleid		number(20)		DEFAULT '0'	NOT NULL,
	ip		varchar2(15)		DEFAULT ''	,
	status		number(10)		DEFAULT '0'	NOT NULL,
	lastup		number(10)		DEFAULT '0'	NOT NULL,
	lastdown		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (dhostid)
);
CREATE TABLE dservices (
	dserviceid		number(20)		DEFAULT '0'	NOT NULL,
	dhostid		number(20)		DEFAULT '0'	NOT NULL,
	type		number(10)		DEFAULT '0'	NOT NULL,
	key_		varchar2(255)		DEFAULT '0'	,
	value		varchar2(255)		DEFAULT '0'	,
	port		number(10)		DEFAULT '0'	NOT NULL,
	status		number(10)		DEFAULT '0'	NOT NULL,
	lastup		number(10)		DEFAULT '0'	NOT NULL,
	lastdown		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (dserviceid)
);
CREATE TABLE ids (
	nodeid		number(10)		DEFAULT '0'	NOT NULL,
	table_name		varchar2(64)		DEFAULT ''	,
	field_name		varchar2(64)		DEFAULT ''	,
	nextid		number(20)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (nodeid,table_name,field_name)
);
CREATE TABLE httptest (
	httptestid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(64)		DEFAULT ''	,
	applicationid		number(20)		DEFAULT '0'	NOT NULL,
	lastcheck		number(10)		DEFAULT '0'	NOT NULL,
	nextcheck		number(10)		DEFAULT '0'	NOT NULL,
	curstate		number(10)		DEFAULT '0'	NOT NULL,
	curstep		number(10)		DEFAULT '0'	NOT NULL,
	lastfailedstep		number(10)		DEFAULT '0'	NOT NULL,
	delay		number(10)		DEFAULT '60'	NOT NULL,
	status		number(10)		DEFAULT '0'	NOT NULL,
	macros		varchar2(2048)		DEFAULT ''	,
	agent		varchar2(255)		DEFAULT ''	,
	time		number(20,4)		DEFAULT '0'	NOT NULL,
	error		varchar2(255)		DEFAULT ''	,
	PRIMARY KEY (httptestid)
);
CREATE TABLE httpstep (
	httpstepid		number(20)		DEFAULT '0'	NOT NULL,
	httptestid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(64)		DEFAULT ''	,
	no		number(10)		DEFAULT '0'	NOT NULL,
	url		varchar2(128)		DEFAULT ''	,
	timeout		number(10)		DEFAULT '30'	NOT NULL,
	posts		varchar2(2048)		DEFAULT ''	,
	required		varchar2(255)		DEFAULT ''	,
	status_codes		varchar2(255)		DEFAULT ''	,
	PRIMARY KEY (httpstepid)
);
CREATE INDEX httpstep_httpstep_1 on httpstep (httptestid);

CREATE TABLE httpstepitem (
	httpstepitemid		number(20)		DEFAULT '0'	NOT NULL,
	httpstepid		number(20)		DEFAULT '0'	NOT NULL,
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	type		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (httpstepitemid)
);
CREATE UNIQUE INDEX httpstepitem_httpstepitem_1 on httpstepitem (httpstepid,itemid);

CREATE TABLE httptestitem (
	httptestitemid		number(20)		DEFAULT '0'	NOT NULL,
	httptestid		number(20)		DEFAULT '0'	NOT NULL,
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	type		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (httptestitemid)
);
CREATE UNIQUE INDEX httptestitem_httptestitem_1 on httptestitem (httptestid,itemid);

CREATE TABLE nodes (
	nodeid		number(10)		DEFAULT '0'	NOT NULL,
	name		varchar2(64)		DEFAULT '0'	,
	timezone		number(10)		DEFAULT '0'	NOT NULL,
	ip		varchar2(15)		DEFAULT ''	,
	port		number(10)		DEFAULT '10051'	NOT NULL,
	slave_history		number(10)		DEFAULT '30'	NOT NULL,
	slave_trends		number(10)		DEFAULT '365'	NOT NULL,
	event_lastid		number(20)		DEFAULT '0'	NOT NULL,
	history_lastid		number(20)		DEFAULT '0'	NOT NULL,
	history_str_lastid		number(20)		DEFAULT '0'	NOT NULL,
	history_uint_lastid		number(20)		DEFAULT '0'	NOT NULL,
	nodetype		number(10)		DEFAULT '0'	NOT NULL,
	masterid		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (nodeid)
);
CREATE TABLE node_cksum (
	cksumid		number(20)		DEFAULT '0'	NOT NULL,
	nodeid		number(20)		DEFAULT '0'	NOT NULL,
	tablename		varchar2(64)		DEFAULT ''	,
	fieldname		varchar2(64)		DEFAULT ''	,
	recordid		number(20)		DEFAULT '0'	NOT NULL,
	cksumtype		number(10)		DEFAULT '0'	NOT NULL,
	cksum		varchar2(32)		DEFAULT ''	,
	PRIMARY KEY (cksumid)
);
CREATE INDEX node_cksum_cksum_1 on node_cksum (nodeid,tablename,fieldname,recordid,cksumtype);

CREATE TABLE node_configlog (
	conflogid		number(20)		DEFAULT '0'	NOT NULL,
	nodeid		number(20)		DEFAULT '0'	NOT NULL,
	tablename		varchar2(64)		DEFAULT ''	,
	recordid		number(20)		DEFAULT '0'	NOT NULL,
	operation		number(10)		DEFAULT '0'	NOT NULL,
	sync_master		number(10)		DEFAULT '0'	NOT NULL,
	sync_slave		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (nodeid,conflogid)
);
CREATE INDEX node_configlog_configlog_1 on node_configlog (conflogid);
CREATE INDEX node_configlog_configlog_2 on node_configlog (nodeid,tablename);

CREATE TABLE history_str_sync (
	id		number(20)			,
	nodeid		number(20)		DEFAULT '0'	NOT NULL,
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	value		varchar2(255)		DEFAULT ''	,
	PRIMARY KEY (id)
);
CREATE INDEX history_str_sync_1 on history_str_sync (nodeid,id);

CREATE TABLE history_sync (
	id		number(20)			,
	nodeid		number(20)		DEFAULT '0'	NOT NULL,
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	value		number(20,4)		DEFAULT '0.0000'	NOT NULL,
	PRIMARY KEY (id)
);
CREATE INDEX history_sync_1 on history_sync (nodeid,id);

CREATE TABLE history_uint_sync (
	id		number(20)			,
	nodeid		number(20)		DEFAULT '0'	NOT NULL,
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	value		number(20)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (id)
);
CREATE INDEX history_uint_sync_1 on history_uint_sync (nodeid,id);

CREATE TABLE services_times (
	timeid		number(20)		DEFAULT '0'	NOT NULL,
	serviceid		number(20)		DEFAULT '0'	NOT NULL,
	type		number(10)		DEFAULT '0'	NOT NULL,
	ts_from		number(10)		DEFAULT '0'	NOT NULL,
	ts_to		number(10)		DEFAULT '0'	NOT NULL,
	note		varchar2(255)		DEFAULT ''	,
	PRIMARY KEY (timeid)
);
CREATE INDEX services_times_times_1 on services_times (serviceid,type,ts_from,ts_to);

CREATE TABLE alerts (
	alertid		number(20)		DEFAULT '0'	NOT NULL,
	actionid		number(20)		DEFAULT '0'	NOT NULL,
	triggerid		number(20)		DEFAULT '0'	NOT NULL,
	userid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	mediatypeid		number(20)		DEFAULT '0'	NOT NULL,
	sendto		varchar2(100)		DEFAULT ''	,
	subject		varchar2(255)		DEFAULT ''	,
	message		varchar2(2048)		DEFAULT ''	,
	status		number(10)		DEFAULT '0'	NOT NULL,
	retries		number(10)		DEFAULT '0'	NOT NULL,
	error		varchar2(128)		DEFAULT ''	,
	nextcheck		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (alertid)
);
CREATE INDEX alerts_1 on alerts (actionid);
CREATE INDEX alerts_2 on alerts (clock);
CREATE INDEX alerts_3 on alerts (triggerid);
CREATE INDEX alerts_4 on alerts (status,retries);
CREATE INDEX alerts_5 on alerts (mediatypeid);
CREATE INDEX alerts_6 on alerts (userid);

CREATE TABLE events (
	eventid		number(20)		DEFAULT '0'	NOT NULL,
	source		number(10)		DEFAULT '0'	NOT NULL,
	object		number(10)		DEFAULT '0'	NOT NULL,
	objectid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	value		number(10)		DEFAULT '0'	NOT NULL,
	acknowledged		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (eventid)
);
CREATE INDEX events_1 on events (object,objectid,clock);
CREATE INDEX events_2 on events (clock);

CREATE TABLE history (
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	value		number(20,4)		DEFAULT '0.0000'	NOT NULL
);
CREATE INDEX history_1 on history (itemid,clock);

CREATE TABLE history_uint (
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	value		number(20)		DEFAULT '0'	NOT NULL
);
CREATE INDEX history_uint_1 on history_uint (itemid,clock);

CREATE TABLE history_str (
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	value		varchar2(255)		DEFAULT ''	
);
CREATE INDEX history_str_1 on history_str (itemid,clock);

CREATE TABLE history_log (
	id		number(20)		DEFAULT '0'	NOT NULL,
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	timestamp		number(10)		DEFAULT '0'	NOT NULL,
	source		varchar2(64)		DEFAULT ''	,
	severity		number(10)		DEFAULT '0'	NOT NULL,
	value		varchar2(2048)		DEFAULT ''	,
	PRIMARY KEY (id)
);
CREATE INDEX history_log_1 on history_log (itemid,clock);

CREATE TABLE history_text (
	id		number(20)		DEFAULT '0'	NOT NULL,
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	value		clob		DEFAULT ''	NOT NULL,
	PRIMARY KEY (id)
);
CREATE INDEX history_text_1 on history_text (itemid,clock);

CREATE TABLE trends (
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	num		number(10)		DEFAULT '0'	NOT NULL,
	value_min		number(20,4)		DEFAULT '0.0000'	NOT NULL,
	value_avg		number(20,4)		DEFAULT '0.0000'	NOT NULL,
	value_max		number(20,4)		DEFAULT '0.0000'	NOT NULL,
	PRIMARY KEY (itemid,clock)
);
CREATE TABLE acknowledges (
	acknowledgeid		number(20)		DEFAULT '0'	NOT NULL,
	userid		number(20)		DEFAULT '0'	NOT NULL,
	eventid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	message		varchar2(255)		DEFAULT ''	,
	PRIMARY KEY (acknowledgeid)
);
CREATE INDEX acknowledges_1 on acknowledges (userid);
CREATE INDEX acknowledges_2 on acknowledges (eventid);
CREATE INDEX acknowledges_3 on acknowledges (clock);

CREATE TABLE actions (
	actionid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(255)		DEFAULT ''	,
	eventsource		number(10)		DEFAULT '0'	NOT NULL,
	evaltype		number(10)		DEFAULT '0'	NOT NULL,
	status		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (actionid)
);
CREATE TABLE operations (
	operationid		number(20)		DEFAULT '0'	NOT NULL,
	actionid		number(20)		DEFAULT '0'	NOT NULL,
	operationtype		number(10)		DEFAULT '0'	NOT NULL,
	object		number(10)		DEFAULT '0'	NOT NULL,
	objectid		number(20)		DEFAULT '0'	NOT NULL,
	shortdata		varchar2(255)		DEFAULT ''	,
	longdata		varchar2(2048)		DEFAULT ''	,
	PRIMARY KEY (operationid)
);
CREATE INDEX operations_1 on operations (actionid);

CREATE TABLE applications (
	applicationid		number(20)		DEFAULT '0'	NOT NULL,
	hostid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(255)		DEFAULT ''	,
	templateid		number(20)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (applicationid)
);
CREATE INDEX applications_1 on applications (templateid);
CREATE UNIQUE INDEX applications_2 on applications (hostid,name);

CREATE TABLE auditlog (
	auditid		number(20)		DEFAULT '0'	NOT NULL,
	userid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	action		number(10)		DEFAULT '0'	NOT NULL,
	resourcetype		number(10)		DEFAULT '0'	NOT NULL,
	details		varchar2(128)		DEFAULT '0'	,
	PRIMARY KEY (auditid)
);
CREATE INDEX auditlog_1 on auditlog (userid,clock);
CREATE INDEX auditlog_2 on auditlog (clock);

CREATE TABLE conditions (
	conditionid		number(20)		DEFAULT '0'	NOT NULL,
	actionid		number(20)		DEFAULT '0'	NOT NULL,
	conditiontype		number(10)		DEFAULT '0'	NOT NULL,
	operator		number(10)		DEFAULT '0'	NOT NULL,
	value		varchar2(255)		DEFAULT ''	,
	PRIMARY KEY (conditionid)
);
CREATE INDEX conditions_1 on conditions (actionid);

CREATE TABLE config (
	configid		number(20)		DEFAULT '0'	NOT NULL,
	alert_history		number(10)		DEFAULT '0'	NOT NULL,
	event_history		number(10)		DEFAULT '0'	NOT NULL,
	refresh_unsupported		number(10)		DEFAULT '0'	NOT NULL,
	work_period		varchar2(100)		DEFAULT '1-5,00:00-24:00'	,
	alert_usrgrpid		number(20)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (configid)
);
CREATE TABLE functions (
	functionid		number(20)		DEFAULT '0'	NOT NULL,
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	triggerid		number(20)		DEFAULT '0'	NOT NULL,
	lastvalue		varchar2(255)			,
	function		varchar2(12)		DEFAULT ''	,
	parameter		varchar2(255)		DEFAULT '0'	,
	PRIMARY KEY (functionid)
);
CREATE INDEX functions_1 on functions (triggerid);
CREATE INDEX functions_2 on functions (itemid,function,parameter);

CREATE TABLE graphs (
	graphid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(128)		DEFAULT ''	,
	width		number(10)		DEFAULT '0'	NOT NULL,
	height		number(10)		DEFAULT '0'	NOT NULL,
	yaxistype		number(10)		DEFAULT '0'	NOT NULL,
	yaxismin		number(20,4)		DEFAULT '0'	NOT NULL,
	yaxismax		number(20,4)		DEFAULT '0'	NOT NULL,
	description		clob		DEFAULT ''	NOT NULL,
	templateid		number(20)		DEFAULT '0'	NOT NULL,
	show_work_period		number(10)		DEFAULT '1'	NOT NULL,
	show_triggers		number(10)		DEFAULT '1'	NOT NULL,
	graphtype		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (graphid)
);
CREATE INDEX graphs_graphs_1 on graphs (name);

CREATE TABLE graphs_items (
	gitemid		number(20)		DEFAULT '0'	NOT NULL,
	graphid		number(20)		DEFAULT '0'	NOT NULL,
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	drawtype		number(10)		DEFAULT '0'	NOT NULL,
	sortorder		number(10)		DEFAULT '0'	NOT NULL,
	color		varchar2(32)		DEFAULT 'Dark Green'	,
	yaxisside		number(10)		DEFAULT '1'	NOT NULL,
	calc_fnc		number(10)		DEFAULT '2'	NOT NULL,
	type		number(10)		DEFAULT '0'	NOT NULL,
	periods_cnt		number(10)		DEFAULT '5'	NOT NULL,
	PRIMARY KEY (gitemid)
);
CREATE TABLE groups (
	groupid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(64)		DEFAULT ''	,
	PRIMARY KEY (groupid)
);
CREATE INDEX groups_1 on groups (name);

CREATE TABLE help_items (
	itemtype		number(10)		DEFAULT '0'	NOT NULL,
	key_		varchar2(255)		DEFAULT ''	,
	description		varchar2(255)		DEFAULT ''	,
	PRIMARY KEY (itemtype,key_)
);
CREATE TABLE hosts (
	hostid		number(20)		DEFAULT '0'	NOT NULL,
	host		varchar2(64)		DEFAULT ''	,
	dns		varchar2(64)		DEFAULT ''	,
	useip		number(10)		DEFAULT '1'	NOT NULL,
	ip		varchar2(15)		DEFAULT '127.0.0.1'	,
	port		number(10)		DEFAULT '10050'	NOT NULL,
	status		number(10)		DEFAULT '0'	NOT NULL,
	disable_until		number(10)		DEFAULT '0'	NOT NULL,
	error		varchar2(128)		DEFAULT ''	,
	available		number(10)		DEFAULT '0'	NOT NULL,
	errors_from		number(10)		DEFAULT '0'	NOT NULL,
	siteid		number(20)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (hostid)
);
CREATE INDEX hosts_1 on hosts (host);
CREATE INDEX hosts_2 on hosts (status);
CREATE INDEX hosts_3 on hosts (siteid);

CREATE TABLE sites (
	siteid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(64)		DEFAULT ''	,
	description		varchar2(255)		DEFAULT ''	,
	db_url		varchar2(255)		DEFAULT ''	,
	active		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (siteid)
);
CREATE INDEX sites_1 on sites (name);

CREATE TABLE hosts_groups (
	hostgroupid		number(20)		DEFAULT '0'	NOT NULL,
	hostid		number(20)		DEFAULT '0'	NOT NULL,
	groupid		number(20)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (hostgroupid)
);
CREATE INDEX hosts_groups_groups_1 on hosts_groups (hostid,groupid);

CREATE TABLE hosts_profiles (
	hostid		number(20)		DEFAULT '0'	NOT NULL,
	devicetype		varchar2(64)		DEFAULT ''	,
	name		varchar2(64)		DEFAULT ''	,
	os		varchar2(64)		DEFAULT ''	,
	serialno		varchar2(64)		DEFAULT ''	,
	tag		varchar2(64)		DEFAULT ''	,
	macaddress		varchar2(64)		DEFAULT ''	,
	hardware		varchar2(2048)		DEFAULT ''	,
	software		varchar2(2048)		DEFAULT ''	,
	contact		varchar2(2048)		DEFAULT ''	,
	location		varchar2(2048)		DEFAULT ''	,
	notes		varchar2(2048)		DEFAULT ''	,
	PRIMARY KEY (hostid)
);
CREATE TABLE hosts_templates (
	hosttemplateid		number(20)		DEFAULT '0'	NOT NULL,
	hostid		number(20)		DEFAULT '0'	NOT NULL,
	templateid		number(20)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (hosttemplateid)
);
CREATE UNIQUE INDEX hosts_templates_1 on hosts_templates (hostid,templateid);

CREATE TABLE housekeeper (
	housekeeperid		number(20)		DEFAULT '0'	NOT NULL,
	tablename		varchar2(64)		DEFAULT ''	,
	field		varchar2(64)		DEFAULT ''	,
	value		number(20)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (housekeeperid)
);
CREATE TABLE images (
	imageid		number(20)		DEFAULT '0'	NOT NULL,
	imagetype		number(10)		DEFAULT '0'	NOT NULL,
	name		varchar2(64)		DEFAULT '0'	,
	image		blob		DEFAULT ''	NOT NULL,
	PRIMARY KEY (imageid)
);
CREATE INDEX images_1 on images (imagetype,name);

CREATE TABLE items (
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	type		number(10)		DEFAULT '0'	NOT NULL,
	snmp_community		varchar2(64)		DEFAULT ''	,
	snmp_oid		varchar2(255)		DEFAULT ''	,
	snmp_port		number(10)		DEFAULT '161'	NOT NULL,
	hostid		number(20)		DEFAULT '0'	NOT NULL,
	description		varchar2(255)		DEFAULT ''	,
	key_		varchar2(255)		DEFAULT ''	,
	delay		number(10)		DEFAULT '0'	NOT NULL,
	history		number(10)		DEFAULT '90'	NOT NULL,
	trends		number(10)		DEFAULT '365'	NOT NULL,
	nextcheck		number(10)		DEFAULT '0'	NOT NULL,
	lastvalue		varchar2(255)			,
	lastclock		number(10)			NULL,
	prevvalue		varchar2(255)			,
	status		number(10)		DEFAULT '0'	NOT NULL,
	value_type		number(10)		DEFAULT '0'	NOT NULL,
	trapper_hosts		varchar2(255)		DEFAULT ''	,
	units		varchar2(10)		DEFAULT ''	,
	multiplier		number(10)		DEFAULT '0'	NOT NULL,
	delta		number(10)		DEFAULT '0'	NOT NULL,
	prevorgvalue		varchar2(255)			,
	snmpv3_securityname		varchar2(64)		DEFAULT ''	,
	snmpv3_securitylevel		number(10)		DEFAULT '0'	NOT NULL,
	snmpv3_authpassphrase		varchar2(64)		DEFAULT ''	,
	snmpv3_privpassphrase		varchar2(64)		DEFAULT ''	,
	formula		varchar2(255)		DEFAULT '1'	,
	error		varchar2(128)		DEFAULT ''	,
	lastlogsize		number(10)		DEFAULT '0'	NOT NULL,
	logtimefmt		varchar2(64)		DEFAULT ''	,
	templateid		number(20)		DEFAULT '0'	NOT NULL,
	valuemapid		number(20)		DEFAULT '0'	NOT NULL,
	delay_flex		varchar2(255)		DEFAULT ''	,
	params		varchar2(2048)		DEFAULT ''	,
	siteid		number(20)		DEFAULT '0'	NOT NULL,
	stderr		varchar2(255)		DEFAULT ''	,
	PRIMARY KEY (itemid)
);
CREATE UNIQUE INDEX items_1 on items (hostid,key_);
CREATE INDEX items_2 on items (nextcheck);
CREATE INDEX items_3 on items (status);

CREATE TABLE items_applications (
	itemappid		number(20)		DEFAULT '0'	NOT NULL,
	applicationid		number(20)		DEFAULT '0'	NOT NULL,
	itemid		number(20)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (itemappid)
);
CREATE INDEX items_applications_1 on items_applications (applicationid,itemid);
CREATE INDEX items_applications_2 on items_applications (itemid);

CREATE TABLE mappings (
	mappingid		number(20)		DEFAULT '0'	NOT NULL,
	valuemapid		number(20)		DEFAULT '0'	NOT NULL,
	value		varchar2(64)		DEFAULT ''	,
	newvalue		varchar2(64)		DEFAULT ''	,
	PRIMARY KEY (mappingid)
);
CREATE INDEX mappings_1 on mappings (valuemapid);

CREATE TABLE media (
	mediaid		number(20)		DEFAULT '0'	NOT NULL,
	userid		number(20)		DEFAULT '0'	NOT NULL,
	mediatypeid		number(20)		DEFAULT '0'	NOT NULL,
	sendto		varchar2(100)		DEFAULT ''	,
	active		number(10)		DEFAULT '0'	NOT NULL,
	severity		number(10)		DEFAULT '63'	NOT NULL,
	period		varchar2(100)		DEFAULT '1-7,00:00-23:59'	,
	PRIMARY KEY (mediaid)
);
CREATE INDEX media_1 on media (userid);
CREATE INDEX media_2 on media (mediatypeid);

CREATE TABLE media_type (
	mediatypeid		number(20)		DEFAULT '0'	NOT NULL,
	type		number(10)		DEFAULT '0'	NOT NULL,
	description		varchar2(100)		DEFAULT ''	,
	smtp_server		varchar2(255)		DEFAULT ''	,
	smtp_helo		varchar2(255)		DEFAULT ''	,
	smtp_email		varchar2(255)		DEFAULT ''	,
	exec_path		varchar2(255)		DEFAULT ''	,
	gsm_modem		varchar2(255)		DEFAULT ''	,
	username		varchar2(255)		DEFAULT ''	,
	passwd		varchar2(255)		DEFAULT ''	,
	PRIMARY KEY (mediatypeid)
);
CREATE TABLE profiles (
	profileid		number(20)		DEFAULT '0'	NOT NULL,
	userid		number(20)		DEFAULT '0'	NOT NULL,
	idx		varchar2(64)		DEFAULT ''	,
	value		varchar2(255)		DEFAULT ''	,
	valuetype		number(10)		DEFAULT 0	NOT NULL,
	PRIMARY KEY (profileid)
);
CREATE UNIQUE INDEX profiles_1 on profiles (userid,idx);

CREATE TABLE rights (
	rightid		number(20)		DEFAULT '0'	NOT NULL,
	groupid		number(20)		DEFAULT '0'	NOT NULL,
	type		number(10)		DEFAULT '0'	NOT NULL,
	permission		number(10)		DEFAULT '0'	NOT NULL,
	id		number(20)			,
	PRIMARY KEY (rightid)
);
CREATE INDEX rights_1 on rights (groupid);

CREATE TABLE screens (
	screenid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(255)		DEFAULT 'Screen'	,
	hsize		number(10)		DEFAULT '1'	NOT NULL,
	vsize		number(10)		DEFAULT '1'	NOT NULL,
	PRIMARY KEY (screenid)
);
CREATE TABLE screens_items (
	screenitemid		number(20)		DEFAULT '0'	NOT NULL,
	screenid		number(20)		DEFAULT '0'	NOT NULL,
	resourcetype		number(10)		DEFAULT '0'	NOT NULL,
	resourceid		number(20)		DEFAULT '0'	NOT NULL,
	width		number(10)		DEFAULT '320'	NOT NULL,
	height		number(10)		DEFAULT '200'	NOT NULL,
	x		number(10)		DEFAULT '0'	NOT NULL,
	y		number(10)		DEFAULT '0'	NOT NULL,
	colspan		number(10)		DEFAULT '0'	NOT NULL,
	rowspan		number(10)		DEFAULT '0'	NOT NULL,
	elements		number(10)		DEFAULT '25'	NOT NULL,
	valign		number(10)		DEFAULT '0'	NOT NULL,
	halign		number(10)		DEFAULT '0'	NOT NULL,
	style		number(10)		DEFAULT '0'	NOT NULL,
	url		varchar2(255)		DEFAULT ''	,
	PRIMARY KEY (screenitemid)
);
CREATE TABLE services (
	serviceid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(128)		DEFAULT ''	,
	status		number(10)		DEFAULT '0'	NOT NULL,
	algorithm		number(10)		DEFAULT '0'	NOT NULL,
	triggerid		number(20)			,
	showsla		number(10)		DEFAULT '0'	NOT NULL,
	goodsla		number(5,2)		DEFAULT '99.9'	NOT NULL,
	sortorder		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (serviceid)
);
CREATE TABLE service_alarms (
	servicealarmid		number(20)		DEFAULT '0'	NOT NULL,
	serviceid		number(20)		DEFAULT '0'	NOT NULL,
	clock		number(10)		DEFAULT '0'	NOT NULL,
	value		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (servicealarmid)
);
CREATE INDEX service_alarms_1 on service_alarms (serviceid,clock);
CREATE INDEX service_alarms_2 on service_alarms (clock);

CREATE TABLE services_links (
	linkid		number(20)		DEFAULT '0'	NOT NULL,
	serviceupid		number(20)		DEFAULT '0'	NOT NULL,
	servicedownid		number(20)		DEFAULT '0'	NOT NULL,
	soft		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (linkid)
);
CREATE INDEX services_links_links_1 on services_links (servicedownid);
CREATE UNIQUE INDEX services_links_links_2 on services_links (serviceupid,servicedownid);

CREATE TABLE sessions (
	sessionid		varchar2(32)		DEFAULT ''	,
	userid		number(20)		DEFAULT '0'	NOT NULL,
	lastaccess		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (sessionid)
);
CREATE TABLE sysmaps_links (
	linkid		number(20)		DEFAULT '0'	NOT NULL,
	sysmapid		number(20)		DEFAULT '0'	NOT NULL,
	selementid1		number(20)		DEFAULT '0'	NOT NULL,
	selementid2		number(20)		DEFAULT '0'	NOT NULL,
	triggerid		number(20)			,
	drawtype_off		number(10)		DEFAULT '0'	NOT NULL,
	color_off		varchar2(32)		DEFAULT 'Black'	,
	drawtype_on		number(10)		DEFAULT '0'	NOT NULL,
	color_on		varchar2(32)		DEFAULT 'Red'	,
	PRIMARY KEY (linkid)
);
CREATE TABLE sysmaps_elements (
	selementid		number(20)		DEFAULT '0'	NOT NULL,
	sysmapid		number(20)		DEFAULT '0'	NOT NULL,
	elementid		number(20)		DEFAULT '0'	NOT NULL,
	elementtype		number(10)		DEFAULT '0'	NOT NULL,
	iconid_off		number(20)		DEFAULT '0'	NOT NULL,
	iconid_on		number(20)		DEFAULT '0'	NOT NULL,
	iconid_unknown		number(20)		DEFAULT '0'	NOT NULL,
	label		varchar2(128)		DEFAULT ''	,
	label_location		number(10)			NULL,
	x		number(10)		DEFAULT '0'	NOT NULL,
	y		number(10)		DEFAULT '0'	NOT NULL,
	url		varchar2(255)		DEFAULT ''	,
	PRIMARY KEY (selementid)
);
CREATE TABLE sysmaps (
	sysmapid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(128)		DEFAULT ''	,
	width		number(10)		DEFAULT '0'	NOT NULL,
	height		number(10)		DEFAULT '0'	NOT NULL,
	backgroundid		number(20)		DEFAULT '0'	NOT NULL,
	label_type		number(10)		DEFAULT '0'	NOT NULL,
	label_location		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (sysmapid)
);
CREATE INDEX sysmaps_1 on sysmaps (name);

CREATE TABLE triggers (
	triggerid		number(20)		DEFAULT '0'	NOT NULL,
	expression		varchar2(255)		DEFAULT ''	,
	description		varchar2(255)		DEFAULT ''	,
	url		varchar2(255)		DEFAULT ''	,
	status		number(10)		DEFAULT '0'	NOT NULL,
	value		number(10)		DEFAULT '0'	NOT NULL,
	priority		number(10)		DEFAULT '0'	NOT NULL,
	lastchange		number(10)		DEFAULT '0'	NOT NULL,
	dep_level		number(10)		DEFAULT '0'	NOT NULL,
	comments		varchar2(2048)			,
	error		varchar2(128)		DEFAULT ''	,
	templateid		number(20)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (triggerid)
);
CREATE INDEX triggers_1 on triggers (status);
CREATE INDEX triggers_2 on triggers (value);

CREATE TABLE trigger_depends (
	triggerdepid		number(20)		DEFAULT '0'	NOT NULL,
	triggerid_down		number(20)		DEFAULT '0'	NOT NULL,
	triggerid_up		number(20)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (triggerdepid)
);
CREATE INDEX trigger_depends_1 on trigger_depends (triggerid_down,triggerid_up);
CREATE INDEX trigger_depends_2 on trigger_depends (triggerid_up);

CREATE TABLE users (
	userid		number(20)		DEFAULT '0'	NOT NULL,
	alias		varchar2(100)		DEFAULT ''	,
	name		varchar2(100)		DEFAULT ''	,
	surname		varchar2(100)		DEFAULT ''	,
	passwd		varchar2(32)		DEFAULT ''	,
	url		varchar2(255)		DEFAULT ''	,
	autologout		number(10)		DEFAULT '900'	NOT NULL,
	lang		varchar2(5)		DEFAULT 'en_gb'	,
	refresh		number(10)		DEFAULT '30'	NOT NULL,
	type		number(10)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (userid)
);
CREATE INDEX users_1 on users (alias);

CREATE TABLE usrgrp (
	usrgrpid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(64)		DEFAULT ''	,
	PRIMARY KEY (usrgrpid)
);
CREATE INDEX usrgrp_1 on usrgrp (name);

CREATE TABLE users_groups (
	id		number(20)		DEFAULT '0'	NOT NULL,
	usrgrpid		number(20)		DEFAULT '0'	NOT NULL,
	userid		number(20)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (id)
);
CREATE INDEX users_groups_1 on users_groups (usrgrpid,userid);

CREATE TABLE valuemaps (
	valuemapid		number(20)		DEFAULT '0'	NOT NULL,
	name		varchar2(64)		DEFAULT ''	,
	PRIMARY KEY (valuemapid)
);
CREATE INDEX valuemaps_1 on valuemaps (name);

