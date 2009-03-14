CREATE TABLE slideshows (
	slideshowid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(255)		DEFAULT ''	NOT NULL,
	delay		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (slideshowid)
);
CREATE TABLE slides (
	slideid		bigint		DEFAULT '0'	NOT NULL,
	slideshowid		bigint		DEFAULT '0'	NOT NULL,
	screenid		bigint		DEFAULT '0'	NOT NULL,
	step		integer		DEFAULT '0'	NOT NULL,
	delay		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (slideid)
);
CREATE INDEX slides_slides_1 on slides (slideshowid);

CREATE TABLE drules (
	druleid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(255)		DEFAULT ''	NOT NULL,
	iprange		varchar(255)		DEFAULT ''	NOT NULL,
	delay		integer		DEFAULT '0'	NOT NULL,
	nextcheck		integer		DEFAULT '0'	NOT NULL,
	status		integer		DEFAULT '0'	NOT NULL,
	siteid		bigint		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (druleid)
);
CREATE TABLE dchecks (
	dcheckid		bigint		DEFAULT '0'	NOT NULL,
	druleid		bigint		DEFAULT '0'	NOT NULL,
	type		integer		DEFAULT '0'	NOT NULL,
	key_		varchar(255)		DEFAULT '0'	NOT NULL,
	snmp_community		varchar(255)		DEFAULT '0'	NOT NULL,
	ports		varchar(255)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (dcheckid)
);
CREATE TABLE dhosts (
	dhostid		bigint		DEFAULT '0'	NOT NULL,
	druleid		bigint		DEFAULT '0'	NOT NULL,
	dns		varchar(64)		DEFAULT ''	NOT NULL,
	ip		varchar(15)		DEFAULT ''	NOT NULL,
	status		integer		DEFAULT '0'	NOT NULL,
	lastup		integer		DEFAULT '0'	NOT NULL,
	lastdown		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (dhostid)
);
CREATE TABLE dservices (
	dserviceid		bigint		DEFAULT '0'	NOT NULL,
	dhostid		bigint		DEFAULT '0'	NOT NULL,
	type		integer		DEFAULT '0'	NOT NULL,
	key_		varchar(255)		DEFAULT '0'	NOT NULL,
	value		varchar(255)		DEFAULT '0'	NOT NULL,
	port		integer		DEFAULT '0'	NOT NULL,
	status		integer		DEFAULT '0'	NOT NULL,
	lastup		integer		DEFAULT '0'	NOT NULL,
	lastdown		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (dserviceid)
);
CREATE TABLE dalerts (
	alertid		bigint		DEFAULT '0'	NOT NULL,
	actionid		bigint		DEFAULT '0'	NOT NULL,
	triggerid		bigint		DEFAULT '0'	NOT NULL,
	userid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	mediatypeid		bigint		DEFAULT '0'	NOT NULL,
	sendto		varchar(100)		DEFAULT ''	NOT NULL,
	subject		varchar(255)		DEFAULT ''	NOT NULL,
	message		blob		DEFAULT ''	NOT NULL,
	status		integer		DEFAULT '0'	NOT NULL,
	retries		integer		DEFAULT '0'	NOT NULL,
	error		varchar(128)		DEFAULT ''	NOT NULL,
	nextcheck		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (alertid)
);
CREATE INDEX dalerts_1 on dalerts (actionid);
CREATE INDEX dalerts_2 on dalerts (clock);
CREATE INDEX dalerts_3 on dalerts (triggerid);
CREATE INDEX dalerts_4 on dalerts (status,retries);
CREATE INDEX dalerts_5 on dalerts (mediatypeid);
CREATE INDEX dalerts_6 on dalerts (userid);

CREATE TABLE ids (
	nodeid		integer		DEFAULT '0'	NOT NULL,
	table_name		varchar(64)		DEFAULT ''	NOT NULL,
	field_name		varchar(64)		DEFAULT ''	NOT NULL,
	nextid		bigint		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (nodeid,table_name,field_name)
);
CREATE TABLE httptest (
	httptestid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(64)		DEFAULT ''	NOT NULL,
	applicationid		bigint		DEFAULT '0'	NOT NULL,
	lastcheck		integer		DEFAULT '0'	NOT NULL,
	nextcheck		integer		DEFAULT '0'	NOT NULL,
	curstate		integer		DEFAULT '0'	NOT NULL,
	curstep		integer		DEFAULT '0'	NOT NULL,
	lastfailedstep		integer		DEFAULT '0'	NOT NULL,
	delay		integer		DEFAULT '60'	NOT NULL,
	status		integer		DEFAULT '0'	NOT NULL,
	macros		blob		DEFAULT ''	NOT NULL,
	agent		varchar(255)		DEFAULT ''	NOT NULL,
	time		double(16,4)		DEFAULT '0'	NOT NULL,
	error		varchar(255)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (httptestid)
);
CREATE TABLE httpstep (
	httpstepid		bigint		DEFAULT '0'	NOT NULL,
	httptestid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(64)		DEFAULT ''	NOT NULL,
	no		integer		DEFAULT '0'	NOT NULL,
	url		varchar(128)		DEFAULT ''	NOT NULL,
	timeout		integer		DEFAULT '30'	NOT NULL,
	posts		blob		DEFAULT ''	NOT NULL,
	required		varchar(255)		DEFAULT ''	NOT NULL,
	status_codes		varchar(255)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (httpstepid)
);
CREATE INDEX httpstep_httpstep_1 on httpstep (httptestid);

CREATE TABLE httpstepitem (
	httpstepitemid		bigint		DEFAULT '0'	NOT NULL,
	httpstepid		bigint		DEFAULT '0'	NOT NULL,
	itemid		bigint		DEFAULT '0'	NOT NULL,
	type		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (httpstepitemid)
);
CREATE UNIQUE INDEX httpstepitem_httpstepitem_1 on httpstepitem (httpstepid,itemid);

CREATE TABLE httptestitem (
	httptestitemid		bigint		DEFAULT '0'	NOT NULL,
	httptestid		bigint		DEFAULT '0'	NOT NULL,
	itemid		bigint		DEFAULT '0'	NOT NULL,
	type		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (httptestitemid)
);
CREATE UNIQUE INDEX httptestitem_httptestitem_1 on httptestitem (httptestid,itemid);

CREATE TABLE nodes (
	nodeid		integer		DEFAULT '0'	NOT NULL,
	name		varchar(64)		DEFAULT '0'	NOT NULL,
	timezone		integer		DEFAULT '0'	NOT NULL,
	ip		varchar(15)		DEFAULT ''	NOT NULL,
	port		integer		DEFAULT '10051'	NOT NULL,
	slave_history		integer		DEFAULT '30'	NOT NULL,
	slave_trends		integer		DEFAULT '365'	NOT NULL,
	event_lastid		bigint		DEFAULT '0'	NOT NULL,
	history_lastid		bigint		DEFAULT '0'	NOT NULL,
	history_str_lastid		bigint		DEFAULT '0'	NOT NULL,
	history_uint_lastid		bigint		DEFAULT '0'	NOT NULL,
	nodetype		integer		DEFAULT '0'	NOT NULL,
	masterid		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (nodeid)
);
CREATE TABLE node_cksum (
	cksumid		bigint		DEFAULT '0'	NOT NULL,
	nodeid		bigint		DEFAULT '0'	NOT NULL,
	tablename		varchar(64)		DEFAULT ''	NOT NULL,
	fieldname		varchar(64)		DEFAULT ''	NOT NULL,
	recordid		bigint		DEFAULT '0'	NOT NULL,
	cksumtype		integer		DEFAULT '0'	NOT NULL,
	cksum		char(32)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (cksumid)
);
CREATE INDEX node_cksum_cksum_1 on node_cksum (nodeid,tablename,fieldname,recordid,cksumtype);

CREATE TABLE node_configlog (
	conflogid		bigint		DEFAULT '0'	NOT NULL,
	nodeid		bigint		DEFAULT '0'	NOT NULL,
	tablename		varchar(64)		DEFAULT ''	NOT NULL,
	recordid		bigint		DEFAULT '0'	NOT NULL,
	operation		integer		DEFAULT '0'	NOT NULL,
	sync_master		integer		DEFAULT '0'	NOT NULL,
	sync_slave		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (nodeid,conflogid)
);
CREATE INDEX node_configlog_configlog_1 on node_configlog (conflogid);
CREATE INDEX node_configlog_configlog_2 on node_configlog (nodeid,tablename);

CREATE TABLE history_str_sync (
	id		serial			,
	nodeid		bigint		DEFAULT '0'	NOT NULL,
	itemid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	value		varchar(255)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (id)
);
CREATE INDEX history_str_sync_1 on history_str_sync (nodeid,id);

CREATE TABLE history_sync (
	id		serial			,
	nodeid		bigint		DEFAULT '0'	NOT NULL,
	itemid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	value		double(16,4)		DEFAULT '0.0000'	NOT NULL,
	PRIMARY KEY (id)
);
CREATE INDEX history_sync_1 on history_sync (nodeid,id);

CREATE TABLE history_uint_sync (
	id		serial			,
	nodeid		bigint		DEFAULT '0'	NOT NULL,
	itemid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	value		bigint		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (id)
);
CREATE INDEX history_uint_sync_1 on history_uint_sync (nodeid,id);

CREATE TABLE services_times (
	timeid		bigint		DEFAULT '0'	NOT NULL,
	serviceid		bigint		DEFAULT '0'	NOT NULL,
	type		integer		DEFAULT '0'	NOT NULL,
	ts_from		integer		DEFAULT '0'	NOT NULL,
	ts_to		integer		DEFAULT '0'	NOT NULL,
	note		varchar(255)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (timeid)
);
CREATE INDEX services_times_times_1 on services_times (serviceid,type,ts_from,ts_to);

CREATE TABLE alerts (
	alertid		bigint		DEFAULT '0'	NOT NULL,
	actionid		bigint		DEFAULT '0'	NOT NULL,
	triggerid		bigint		DEFAULT '0'	NOT NULL,
	userid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	mediatypeid		bigint		DEFAULT '0'	NOT NULL,
	sendto		varchar(100)		DEFAULT ''	NOT NULL,
	subject		varchar(255)		DEFAULT ''	NOT NULL,
	message		blob		DEFAULT ''	NOT NULL,
	status		integer		DEFAULT '0'	NOT NULL,
	retries		integer		DEFAULT '0'	NOT NULL,
	error		varchar(128)		DEFAULT ''	NOT NULL,
	nextcheck		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (alertid)
);
CREATE INDEX alerts_1 on alerts (actionid);
CREATE INDEX alerts_2 on alerts (clock);
CREATE INDEX alerts_3 on alerts (triggerid);
CREATE INDEX alerts_4 on alerts (status,retries);
CREATE INDEX alerts_5 on alerts (mediatypeid);
CREATE INDEX alerts_6 on alerts (userid);

CREATE TABLE events (
	eventid		serial			NOT NULL,
	source		integer		DEFAULT '0'	NOT NULL,
	object		integer		DEFAULT '0'	NOT NULL,
	objectid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	value		integer		DEFAULT '0'	NOT NULL,
	acknowledged		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (eventid)
);
CREATE INDEX events_1 on events (object,objectid,clock);
CREATE INDEX events_2 on events (clock);

CREATE TABLE history (
	itemid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	value		double(16,4)		DEFAULT '0.0000'	NOT NULL
);
CREATE INDEX history_1 on history (itemid,clock);

CREATE TABLE history_uint (
	itemid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	value		bigint		DEFAULT '0'	NOT NULL
);
CREATE INDEX history_uint_1 on history_uint (itemid,clock);

CREATE TABLE history_str (
	itemid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	value		varchar(255)		DEFAULT ''	NOT NULL
);
CREATE INDEX history_str_1 on history_str (itemid,clock);

CREATE TABLE history_log (
	id		bigint		DEFAULT '0'	NOT NULL,
	itemid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	timestamp		integer		DEFAULT '0'	NOT NULL,
	source		varchar(64)		DEFAULT ''	NOT NULL,
	severity		integer		DEFAULT '0'	NOT NULL,
	value		text		DEFAULT ''	NOT NULL,
	PRIMARY KEY (id)
);
CREATE INDEX history_log_1 on history_log (itemid,clock);

CREATE TABLE history_text (
	id		bigint		DEFAULT '0'	NOT NULL,
	itemid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	value		text		DEFAULT ''	NOT NULL,
	PRIMARY KEY (id)
);
CREATE INDEX history_text_1 on history_text (itemid,clock);

CREATE TABLE trends (
	itemid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	num		integer		DEFAULT '0'	NOT NULL,
	value_min		double(16,4)		DEFAULT '0.0000'	NOT NULL,
	value_avg		double(16,4)		DEFAULT '0.0000'	NOT NULL,
	value_max		double(16,4)		DEFAULT '0.0000'	NOT NULL,
	PRIMARY KEY (itemid,clock)
);
CREATE TABLE acknowledges (
	acknowledgeid		bigint		DEFAULT '0'	NOT NULL,
	userid		bigint		DEFAULT '0'	NOT NULL,
	eventid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	message		varchar(255)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (acknowledgeid)
);
CREATE INDEX acknowledges_1 on acknowledges (userid);
CREATE INDEX acknowledges_2 on acknowledges (eventid);
CREATE INDEX acknowledges_3 on acknowledges (clock);

CREATE TABLE actions (
	actionid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(255)		DEFAULT ''	NOT NULL,
	eventsource		integer		DEFAULT '0'	NOT NULL,
	evaltype		integer		DEFAULT '0'	NOT NULL,
	status		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (actionid)
);
CREATE TABLE operations (
	operationid		bigint		DEFAULT '0'	NOT NULL,
	actionid		bigint		DEFAULT '0'	NOT NULL,
	operationtype		integer		DEFAULT '0'	NOT NULL,
	object		integer		DEFAULT '0'	NOT NULL,
	objectid		bigint		DEFAULT '0'	NOT NULL,
	objectarg		varchar(255)		DEFAULT ''	NOT NULL,
	shortdata		varchar(255)		DEFAULT ''	NOT NULL,
	longdata		blob		DEFAULT ''	NOT NULL,
	PRIMARY KEY (operationid)
);
CREATE INDEX operations_1 on operations (actionid);

CREATE TABLE applications (
	applicationid		bigint		DEFAULT '0'	NOT NULL,
	hostid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(255)		DEFAULT ''	NOT NULL,
	templateid		bigint		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (applicationid)
);
CREATE INDEX applications_1 on applications (templateid);
CREATE UNIQUE INDEX applications_2 on applications (hostid,name);

CREATE TABLE auditlog (
	auditid		bigint		DEFAULT '0'	NOT NULL,
	userid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	action		integer		DEFAULT '0'	NOT NULL,
	resourcetype		integer		DEFAULT '0'	NOT NULL,
	details		varchar(1024)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (auditid)
);
CREATE INDEX auditlog_1 on auditlog (userid,clock);
CREATE INDEX auditlog_2 on auditlog (clock);

CREATE TABLE conditions (
	conditionid		bigint		DEFAULT '0'	NOT NULL,
	actionid		bigint		DEFAULT '0'	NOT NULL,
	conditiontype		integer		DEFAULT '0'	NOT NULL,
	operator		integer		DEFAULT '0'	NOT NULL,
	value		varchar(255)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (conditionid)
);
CREATE INDEX conditions_1 on conditions (actionid);

CREATE TABLE config (
	configid		bigint		DEFAULT '0'	NOT NULL,
	alert_history		integer		DEFAULT '0'	NOT NULL,
	event_history		integer		DEFAULT '0'	NOT NULL,
	refresh_unsupported		integer		DEFAULT '0'	NOT NULL,
	work_period		varchar(100)		DEFAULT '1-5,00:00-24:00'	NOT NULL,
	alert_usrgrpid		bigint		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (configid)
);
CREATE TABLE functions (
	functionid		bigint		DEFAULT '0'	NOT NULL,
	itemid		bigint		DEFAULT '0'	NOT NULL,
	triggerid		bigint		DEFAULT '0'	NOT NULL,
	lastvalue		varchar(255)			,
	function		varchar(12)		DEFAULT ''	NOT NULL,
	parameter		varchar(255)		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (functionid)
);
CREATE INDEX functions_1 on functions (triggerid);
CREATE INDEX functions_2 on functions (itemid,function,parameter);

CREATE TABLE graphs (
	graphid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(128)		DEFAULT ''	NOT NULL,
	width		integer		DEFAULT '0'	NOT NULL,
	height		integer		DEFAULT '0'	NOT NULL,
	yaxistype		integer		DEFAULT '0'	NOT NULL,
	yaxismin		double(16,4)		DEFAULT '0'	NOT NULL,
	yaxismax		double(16,4)		DEFAULT '0'	NOT NULL,
	templateid		bigint		DEFAULT '0'	NOT NULL,
	show_work_period		integer		DEFAULT '1'	NOT NULL,
	show_triggers		integer		DEFAULT '1'	NOT NULL,
	graphtype		integer		DEFAULT '0'	NOT NULL,
	description		text		DEFAULT ''	NOT NULL,
	PRIMARY KEY (graphid)
);
CREATE INDEX graphs_graphs_1 on graphs (name);

CREATE TABLE graphs_items (
	gitemid		bigint		DEFAULT '0'	NOT NULL,
	graphid		bigint		DEFAULT '0'	NOT NULL,
	itemid		bigint		DEFAULT '0'	NOT NULL,
	drawtype		integer		DEFAULT '0'	NOT NULL,
	sortorder		integer		DEFAULT '0'	NOT NULL,
	color		varchar(32)		DEFAULT 'Dark Green'	NOT NULL,
	yaxisside		integer		DEFAULT '1'	NOT NULL,
	calc_fnc		integer		DEFAULT '2'	NOT NULL,
	type		integer		DEFAULT '0'	NOT NULL,
	periods_cnt		integer		DEFAULT '5'	NOT NULL,
	PRIMARY KEY (gitemid)
);
CREATE TABLE groups (
	groupid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(64)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (groupid)
);
CREATE INDEX groups_1 on groups (name);

CREATE TABLE help_items (
	itemtype		integer		DEFAULT '0'	NOT NULL,
	key_		varchar(255)		DEFAULT ''	NOT NULL,
	description		varchar(1024)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (itemtype,key_)
);
CREATE TABLE hosts (
	hostid		bigint		DEFAULT '0'	NOT NULL,
	host		varchar(64)		DEFAULT ''	NOT NULL,
	dns		varchar(64)		DEFAULT ''	NOT NULL,
	useip		integer		DEFAULT '1'	NOT NULL,
	ip		varchar(15)		DEFAULT '127.0.0.1'	NOT NULL,
	port		integer		DEFAULT '10050'	NOT NULL,
	status		integer		DEFAULT '0'	NOT NULL,
	disable_until		integer		DEFAULT '0'	NOT NULL,
	error		varchar(128)		DEFAULT ''	NOT NULL,
	available		integer		DEFAULT '0'	NOT NULL,
	errors_from		integer		DEFAULT '0'	NOT NULL,
	siteid		bigint		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (hostid)
);
CREATE INDEX hosts_1 on hosts (host);
CREATE INDEX hosts_2 on hosts (status);
CREATE INDEX hosts_3 on hosts (siteid);

CREATE TABLE sites (
	siteid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(64)		DEFAULT ''	NOT NULL,
	description		varchar(255)		DEFAULT ''	NOT NULL,
	db_url		varchar(255)		DEFAULT ''	NOT NULL,
	active		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (siteid)
);
CREATE INDEX sites_1 on sites (name);

CREATE TABLE hosts_groups (
	hostgroupid		bigint		DEFAULT '0'	NOT NULL,
	hostid		bigint		DEFAULT '0'	NOT NULL,
	groupid		bigint		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (hostgroupid)
);
CREATE INDEX hosts_groups_groups_1 on hosts_groups (hostid,groupid);

CREATE TABLE hosts_profiles (
	hostid		bigint		DEFAULT '0'	NOT NULL,
	devicetype		varchar(64)		DEFAULT ''	NOT NULL,
	name		varchar(64)		DEFAULT ''	NOT NULL,
	os		varchar(64)		DEFAULT ''	NOT NULL,
	serialno		varchar(64)		DEFAULT ''	NOT NULL,
	tag		varchar(64)		DEFAULT ''	NOT NULL,
	macaddress		varchar(64)		DEFAULT ''	NOT NULL,
	hardware		blob		DEFAULT ''	NOT NULL,
	software		blob		DEFAULT ''	NOT NULL,
	contact		blob		DEFAULT ''	NOT NULL,
	location		blob		DEFAULT ''	NOT NULL,
	notes		blob		DEFAULT ''	NOT NULL,
	PRIMARY KEY (hostid)
);
CREATE TABLE hosts_templates (
	hosttemplateid		bigint		DEFAULT '0'	NOT NULL,
	hostid		bigint		DEFAULT '0'	NOT NULL,
	templateid		bigint		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (hosttemplateid)
);
CREATE UNIQUE INDEX hosts_templates_1 on hosts_templates (hostid,templateid);

CREATE TABLE housekeeper (
	housekeeperid		bigint		DEFAULT '0'	NOT NULL,
	tablename		varchar(64)		DEFAULT ''	NOT NULL,
	field		varchar(64)		DEFAULT ''	NOT NULL,
	value		bigint		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (housekeeperid)
);
CREATE TABLE images (
	imageid		bigint		DEFAULT '0'	NOT NULL,
	imagetype		integer		DEFAULT '0'	NOT NULL,
	name		varchar(64)		DEFAULT '0'	NOT NULL,
	image		longblob		DEFAULT ''	NOT NULL,
	PRIMARY KEY (imageid)
);
CREATE INDEX images_1 on images (imagetype,name);

CREATE TABLE items_nextcheck (
	itemid		bigint		DEFAULT '0'	NOT NULL,
	nextcheck		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (itemid)
);
CREATE INDEX items_nextcheck_1 on items_nextcheck (itemid);
CREATE INDEX items_nextcheck_2 on items_nextcheck (nextcheck);

CREATE TABLE items (
	itemid		bigint		DEFAULT '0'	NOT NULL,
	type		integer		DEFAULT '0'	NOT NULL,
	snmp_community		varchar(64)		DEFAULT ''	NOT NULL,
	snmp_oid		varchar(255)		DEFAULT ''	NOT NULL,
	snmp_port		integer		DEFAULT '161'	NOT NULL,
	hostid		bigint		DEFAULT '0'	NOT NULL,
	description		varchar(255)		DEFAULT ''	NOT NULL,
	key_		varchar(255)		DEFAULT ''	NOT NULL,
	delay		integer		DEFAULT '0'	NOT NULL,
	history		integer		DEFAULT '90'	NOT NULL,
	trends		integer		DEFAULT '365'	NOT NULL,
	nextcheck		integer		DEFAULT '0'	NOT NULL,
	lastvalue		varchar(255)			NULL,
	lastclock		integer			NULL,
	prevvalue		varchar(255)			NULL,
	status		integer		DEFAULT '0'	NOT NULL,
	value_type		integer		DEFAULT '0'	NOT NULL,
	trapper_hosts		varchar(255)		DEFAULT ''	NOT NULL,
	units		varchar(10)		DEFAULT ''	NOT NULL,
	multiplier		integer		DEFAULT '0'	NOT NULL,
	delta		integer		DEFAULT '0'	NOT NULL,
	prevorgvalue		varchar(255)			NULL,
	snmpv3_securityname		varchar(64)		DEFAULT ''	NOT NULL,
	snmpv3_securitylevel		integer		DEFAULT '0'	NOT NULL,
	snmpv3_authpassphrase		varchar(64)		DEFAULT ''	NOT NULL,
	snmpv3_privpassphrase		varchar(64)		DEFAULT ''	NOT NULL,
	formula		varchar(255)		DEFAULT '1'	NOT NULL,
	error		varchar(128)		DEFAULT ''	NOT NULL,
	lastlogsize		integer		DEFAULT '0'	NOT NULL,
	logtimefmt		varchar(64)		DEFAULT ''	NOT NULL,
	templateid		bigint		DEFAULT '0'	NOT NULL,
	valuemapid		bigint		DEFAULT '0'	NOT NULL,
	delay_flex		varchar(255)		DEFAULT ''	NOT NULL,
	params		text		DEFAULT ''	NOT NULL,
	siteid		bigint		DEFAULT '0'	NOT NULL,
	stderr		varchar(255)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (itemid)
);
CREATE UNIQUE INDEX items_1 on items (hostid,key_);
CREATE INDEX items_2 on items (nextcheck);
CREATE INDEX items_3 on items (status);
CREATE INDEX items_4 on items (type);

CREATE TABLE items_applications (
	itemappid		bigint		DEFAULT '0'	NOT NULL,
	applicationid		bigint		DEFAULT '0'	NOT NULL,
	itemid		bigint		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (itemappid)
);
CREATE INDEX items_applications_1 on items_applications (applicationid,itemid);
CREATE INDEX items_applications_2 on items_applications (itemid);

CREATE TABLE mappings (
	mappingid		bigint		DEFAULT '0'	NOT NULL,
	valuemapid		bigint		DEFAULT '0'	NOT NULL,
	value		varchar(64)		DEFAULT ''	NOT NULL,
	newvalue		varchar(64)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (mappingid)
);
CREATE INDEX mappings_1 on mappings (valuemapid);

CREATE TABLE media (
	mediaid		bigint		DEFAULT '0'	NOT NULL,
	userid		bigint		DEFAULT '0'	NOT NULL,
	mediatypeid		bigint		DEFAULT '0'	NOT NULL,
	sendto		varchar(100)		DEFAULT ''	NOT NULL,
	active		integer		DEFAULT '0'	NOT NULL,
	severity		integer		DEFAULT '63'	NOT NULL,
	period		varchar(100)		DEFAULT '1-7,00:00-23:59'	NOT NULL,
	PRIMARY KEY (mediaid)
);
CREATE INDEX media_1 on media (userid);
CREATE INDEX media_2 on media (mediatypeid);

CREATE TABLE media_type (
	mediatypeid		bigint		DEFAULT '0'	NOT NULL,
	type		integer		DEFAULT '0'	NOT NULL,
	description		varchar(100)		DEFAULT ''	NOT NULL,
	smtp_server		varchar(255)		DEFAULT ''	NOT NULL,
	smtp_helo		varchar(255)		DEFAULT ''	NOT NULL,
	smtp_email		varchar(255)		DEFAULT ''	NOT NULL,
	exec_path		varchar(255)		DEFAULT ''	NOT NULL,
	gsm_modem		varchar(255)		DEFAULT ''	NOT NULL,
	username		varchar(255)		DEFAULT ''	NOT NULL,
	passwd		varchar(255)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (mediatypeid)
);
CREATE TABLE profiles (
	profileid		bigint		DEFAULT '0'	NOT NULL,
	userid		bigint		DEFAULT '0'	NOT NULL,
	idx		varchar(64)		DEFAULT ''	NOT NULL,
	value		varchar(255)		DEFAULT ''	NOT NULL,
	valuetype		integer		DEFAULT 0	NOT NULL,
	PRIMARY KEY (profileid)
);
CREATE UNIQUE INDEX profiles_1 on profiles (userid,idx);

CREATE TABLE rights (
	rightid		bigint		DEFAULT '0'	NOT NULL,
	groupid		bigint		DEFAULT '0'	NOT NULL,
	type		integer		DEFAULT '0'	NOT NULL,
	permission		integer		DEFAULT '0'	NOT NULL,
	id		bigint			,
	PRIMARY KEY (rightid)
);
CREATE INDEX rights_1 on rights (groupid);

CREATE TABLE screens (
	screenid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(255)		DEFAULT 'Screen'	NOT NULL,
	hsize		integer		DEFAULT '1'	NOT NULL,
	vsize		integer		DEFAULT '1'	NOT NULL,
	PRIMARY KEY (screenid)
);
CREATE TABLE screens_items (
	screenitemid		bigint		DEFAULT '0'	NOT NULL,
	screenid		bigint		DEFAULT '0'	NOT NULL,
	resourcetype		integer		DEFAULT '0'	NOT NULL,
	resourceid		bigint		DEFAULT '0'	NOT NULL,
	width		integer		DEFAULT '320'	NOT NULL,
	height		integer		DEFAULT '200'	NOT NULL,
	x		integer		DEFAULT '0'	NOT NULL,
	y		integer		DEFAULT '0'	NOT NULL,
	colspan		integer		DEFAULT '0'	NOT NULL,
	rowspan		integer		DEFAULT '0'	NOT NULL,
	elements		integer		DEFAULT '25'	NOT NULL,
	valign		integer		DEFAULT '0'	NOT NULL,
	halign		integer		DEFAULT '0'	NOT NULL,
	style		integer		DEFAULT '0'	NOT NULL,
	url		varchar(255)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (screenitemid)
);
CREATE TABLE services (
	serviceid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(128)		DEFAULT ''	NOT NULL,
	status		integer		DEFAULT '0'	NOT NULL,
	algorithm		integer		DEFAULT '0'	NOT NULL,
	triggerid		bigint			,
	showsla		integer		DEFAULT '0'	NOT NULL,
	goodsla		double(5,2)		DEFAULT '99.9'	NOT NULL,
	sortorder		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (serviceid)
);
CREATE TABLE service_alarms (
	servicealarmid		bigint		DEFAULT '0'	NOT NULL,
	serviceid		bigint		DEFAULT '0'	NOT NULL,
	clock		integer		DEFAULT '0'	NOT NULL,
	value		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (servicealarmid)
);
CREATE INDEX service_alarms_1 on service_alarms (serviceid,clock);
CREATE INDEX service_alarms_2 on service_alarms (clock);

CREATE TABLE services_links (
	linkid		bigint		DEFAULT '0'	NOT NULL,
	serviceupid		bigint		DEFAULT '0'	NOT NULL,
	servicedownid		bigint		DEFAULT '0'	NOT NULL,
	soft		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (linkid)
);
CREATE INDEX services_links_links_1 on services_links (servicedownid);
CREATE UNIQUE INDEX services_links_links_2 on services_links (serviceupid,servicedownid);

CREATE TABLE sessions (
	sessionid		varchar(32)		DEFAULT ''	NOT NULL,
	userid		bigint		DEFAULT '0'	NOT NULL,
	lastaccess		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (sessionid)
);
CREATE TABLE sysmaps_links (
	linkid		bigint		DEFAULT '0'	NOT NULL,
	sysmapid		bigint		DEFAULT '0'	NOT NULL,
	selementid1		bigint		DEFAULT '0'	NOT NULL,
	selementid2		bigint		DEFAULT '0'	NOT NULL,
	triggerid		bigint			,
	drawtype_off		integer		DEFAULT '0'	NOT NULL,
	color_off		varchar(32)		DEFAULT 'Black'	NOT NULL,
	drawtype_on		integer		DEFAULT '0'	NOT NULL,
	color_on		varchar(32)		DEFAULT 'Red'	NOT NULL,
	PRIMARY KEY (linkid)
);
CREATE TABLE sysmaps_elements (
	selementid		bigint		DEFAULT '0'	NOT NULL,
	sysmapid		bigint		DEFAULT '0'	NOT NULL,
	elementid		bigint		DEFAULT '0'	NOT NULL,
	elementtype		integer		DEFAULT '0'	NOT NULL,
	iconid_off		bigint		DEFAULT '0'	NOT NULL,
	iconid_on		bigint		DEFAULT '0'	NOT NULL,
	iconid_unknown		bigint		DEFAULT '0'	NOT NULL,
	label		varchar(128)		DEFAULT ''	NOT NULL,
	label_location		integer			NULL,
	x		integer		DEFAULT '0'	NOT NULL,
	y		integer		DEFAULT '0'	NOT NULL,
	url		varchar(255)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (selementid)
);
CREATE TABLE sysmaps (
	sysmapid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(128)		DEFAULT ''	NOT NULL,
	width		integer		DEFAULT '0'	NOT NULL,
	height		integer		DEFAULT '0'	NOT NULL,
	backgroundid		bigint		DEFAULT '0'	NOT NULL,
	label_type		integer		DEFAULT '0'	NOT NULL,
	label_location		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (sysmapid)
);
CREATE INDEX sysmaps_1 on sysmaps (name);

CREATE TABLE triggers (
	triggerid		bigint		DEFAULT '0'	NOT NULL,
	expression		varchar(255)		DEFAULT ''	NOT NULL,
	description		varchar(255)		DEFAULT ''	NOT NULL,
	url		varchar(255)		DEFAULT ''	NOT NULL,
	status		integer		DEFAULT '0'	NOT NULL,
	value		integer		DEFAULT '0'	NOT NULL,
	priority		integer		DEFAULT '0'	NOT NULL,
	lastchange		integer		DEFAULT '0'	NOT NULL,
	dep_level		integer		DEFAULT '0'	NOT NULL,
	comments		blob			,
	error		varchar(128)		DEFAULT ''	NOT NULL,
	templateid		bigint		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (triggerid)
);
CREATE INDEX triggers_1 on triggers (status);
CREATE INDEX triggers_2 on triggers (value);

CREATE TABLE trigger_depends (
	triggerdepid		bigint		DEFAULT '0'	NOT NULL,
	triggerid_down		bigint		DEFAULT '0'	NOT NULL,
	triggerid_up		bigint		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (triggerdepid)
);
CREATE INDEX trigger_depends_1 on trigger_depends (triggerid_down,triggerid_up);
CREATE INDEX trigger_depends_2 on trigger_depends (triggerid_up);

CREATE TABLE users (
	userid		bigint		DEFAULT '0'	NOT NULL,
	alias		varchar(100)		DEFAULT ''	NOT NULL,
	name		varchar(100)		DEFAULT ''	NOT NULL,
	surname		varchar(100)		DEFAULT ''	NOT NULL,
	passwd		char(32)		DEFAULT ''	NOT NULL,
	url		varchar(255)		DEFAULT ''	NOT NULL,
	autologout		integer		DEFAULT '900'	NOT NULL,
	lang		varchar(5)		DEFAULT 'en_gb'	NOT NULL,
	refresh		integer		DEFAULT '30'	NOT NULL,
	type		integer		DEFAULT '0'	NOT NULL,
	options_bits		integer		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (userid)
);
CREATE INDEX users_1 on users (alias);

CREATE TABLE usrgrp (
	usrgrpid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(64)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (usrgrpid)
);
CREATE INDEX usrgrp_1 on usrgrp (name);

CREATE TABLE users_groups (
	id		bigint		DEFAULT '0'	NOT NULL,
	usrgrpid		bigint		DEFAULT '0'	NOT NULL,
	userid		bigint		DEFAULT '0'	NOT NULL,
	PRIMARY KEY (id)
);
CREATE INDEX users_groups_1 on users_groups (usrgrpid,userid);

CREATE TABLE valuemaps (
	valuemapid		bigint		DEFAULT '0'	NOT NULL,
	name		varchar(64)		DEFAULT ''	NOT NULL,
	PRIMARY KEY (valuemapid)
);
CREATE INDEX valuemaps_1 on valuemaps (name);

