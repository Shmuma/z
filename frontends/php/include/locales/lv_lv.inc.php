<?php
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
?>
<?php
	global $TRANSLATION;

	$TRANSLATION = array(

	"S_DATE_FORMAT_YMDHMS"=>		"d M H:i:s",
	"S_DATE_FORMAT_YMD"=>			"d M Y",
	"S_HTML_CHARSET"=>			"iso-8859-1",

//	actions.php
	"S_ACTIONS"=>				"Actions",
	"S_ACTION_ADDED"=>			"Action added",
	"S_CANNOT_ADD_ACTION"=>			"Cannot add action",
	"S_ACTION_UPDATED"=>			"Action updated",
	"S_CANNOT_UPDATE_ACTION"=>		"Cannot update action",
	"S_ACTION_DELETED"=>			"Action deleted",
	"S_CANNOT_DELETE_ACTION"=>		"Cannot delete action",
	"S_SCOPE"=>				"Scope",
	"S_SEND_MESSAGE_TO"=>			"Send message to",
	"S_WHEN_TRIGGER"=>			"When trigger",
	"S_DELAY"=>				"Delay",
	"S_SUBJECT"=>				"Subject",
	"S_ON"=>				"ON",
	"S_OFF"=>				"OFF",
	"S_NO_ACTIONS_DEFINED"=>		"No actions defined",
	"S_NEW_ACTION"=>			"New action",
	"S_SINGLE_USER"=>			"Single user",
	"S_USER_GROUP"=>			"User group",
	"S_GROUP"=>				"Group",
	"S_USER"=>				"User",
	"S_WHEN_TRIGGER_BECOMES"=>		"When trigger becomes",
	"S_ON_OR_OFF"=>				"ON or OFF",
	"S_DELAY_BETWEEN_MESSAGES_IN_SEC"=>	"Delay between messages (in sec)",
	"S_MESSAGE"=>				"Message",
	"S_THIS_TRIGGER_ONLY"=>			"This trigger only",
	"S_ALL_TRIGGERS_OF_THIS_HOST"=>		"All triggers of this host",
	"S_ALL_TRIGGERS"=>			"All triggers",
	"S_USE_IF_TRIGGER_SEVERITY"=>		"Use if trigger's severity equal or more than",
	"S_NOT_CLASSIFIED"=>			"Not classified",
	"S_INFORMATION"=>			"Information",
	"S_WARNING"=>				"Warning",
	"S_AVERAGE"=>				"Average",
	"S_HIGH"=>				"High",
	"S_DISASTER"=>				"Disaster",

//	alarms.php
	"S_ALARMS"=>				"Alarms",
	"S_ALARMS_SMALL"=>			"Alarms",
	"S_ALARMS_BIG"=>			"ALARMS",
	"S_SHOW_ONLY_LAST_100"=>		"Show only last 100",
	"S_SHOW_ALL"=>				"Show all",
	"S_TIME"=>				"Time",
	"S_STATUS"=>				"Status",
	"S_DURATION"=>				"Duration",
	"S_SUM"=>				"Sum",
	"S_TRUE_BIG"=>				"TRUE",
	"S_FALSE_BIG"=>				"FALSE",
	"S_DISABLED_BIG"=>			"DISABLED",
	"S_UNKNOWN_BIG"=>			"UNKNOWN",

//	alerts.php
	"S_ALERT_HISTORY_SMALL"=>		"Alert history",
	"S_ALERT_HISTORY_BIG"=>			"ALERT HISTORY",
	"S_ALERTS_BIG"=>			"ALERTS",
	"S_TYPE"=>				"Type",
	"S_RECIPIENTS"=>			"Recipient(s)",
	"S_ERROR"=>				"Error",
	"S_SENT"=>				"sent",
	"S_NOT_SENT"=>				"not sent",
	"S_NO_ALERTS"=>				"No alerts",
	"S_SHOW_NEXT_100"=>			"Show next 100",
	"S_SHOW_PREVIOUS_100"=>			"Show previous 100",

//	charts.php
	"S_CUSTOM_GRAPHS"=>			"Custom graphs",
	"S_GRAPHS_BIG"=>			"GRAPHS",
	"S_NO_GRAPHS_TO_DISPLAY"=>		"No graphs to display",
	"S_SELECT_GRAPH_TO_DISPLAY"=>		"Select graph to display",
	"S_PERIOD"=>				"Period",
	"S_1H"=>				"1h",
	"S_2H"=>				"2h",
	"S_4H"=>				"4h",
	"S_8H"=>				"8h",
	"S_12H"=>				"12h",
	"S_24H"=>				"24h",
	"S_WEEK_SMALL"=>			"week",
	"S_MONTH_SMALL"=>			"month",
	"S_YEAR_SMALL"=>			"year",
	"S_KEEP_PERIOD"=>			"Keep period",
	"S_ON_C"=>				"On",
	"S_OFF_C"=>				"Off",
	"S_MOVE"=>				"Move",
	"S_SELECT_GRAPH_DOT_DOT_DOT"=>		"Select graph...",

// Colors
	"S_BLACK"=>				"Black",
	"S_BLUE"=>				"Blue",
	"S_CYAN"=>				"Cyan",
	"S_DARK_BLUE"=>				"Dark blue",
	"S_DARK_GREEN"=>			"Dark green",
	"S_DARK_RED"=>				"Dark red",
	"S_DARK_YELLOW"=>			"Dark yellow",
	"S_GREEN"=>				"Green",
	"S_RED"=>				"Red",
	"S_WHITE"=>				"White",
	"S_YELLOW"=>				"Yellow",

//	config.php
	"S_CONFIGURATION_OF_ZABBIX"=>		"Configuration of ZABBIX",
	"S_CONFIGURATION_OF_ZABBIX_BIG"=>	"CONFIGURATION OF ZABBIX",
	"S_CONFIGURATION_UPDATED"=>		"Configuration updated",
	"S_CONFIGURATION_WAS_NOT_UPDATED"=>	"Configuration was not updated",
	"S_ADDED_NEW_MEDIA_TYPE"=>		"Added new media type",
	"S_NEW_MEDIA_TYPE_WAS_NOT_ADDED"=>	"New media type was not added",
	"S_MEDIA_TYPE_UPDATED"=>		"Media type updated",
	"S_MEDIA_TYPE_WAS_NOT_UPDATED"=>	"Media type was not updated",
	"S_MEDIA_TYPE_DELETED"=>		"Media type deleted",
	"S_MEDIA_TYPE_WAS_NOT_DELETED"=>	"Media type was not deleted",
	"S_CONFIGURATION"=>			"Configuration",
	"S_DO_NOT_KEEP_ACTIONS_OLDER_THAN"=>	"Do not keep actions older than (in days)",
	"S_DO_NOT_KEEP_EVENTS_OLDER_THAN"=>	"Do not keep events older than (in days)",
	"S_MEDIA_TYPES_BIG"=>			"MEDIA TYPES",
	"S_NO_MEDIA_TYPES_DEFINED"=>		"No media types defined",
	"S_SMTP_SERVER"=>			"SMTP server",
	"S_SMTP_HELO"=>				"SMTP helo",
	"S_SMTP_EMAIL"=>			"SMTP email",
	"S_SCRIPT_NAME"=>			"Script name",
	"S_DELETE_SELECTED_MEDIA"=>		"Delete selected media?",
	"S_DELETE_SELECTED_IMAGE"=>		"Delete selected image?",
	"S_HOUSEKEEPER"=>			"Housekeeper",
	"S_MEDIA_TYPES"=>			"Media types",
	"S_ESCALATION_RULES"=>			"Escalation rules",
	"S_ESCALATION"=>			"Escalation",
	"S_ESCALATION_RULES_BIG"=>		"ESCALATION RULES",
	"S_NO_ESCALATION_RULES_DEFINED"=>	"No escalation rules defined",
	"S_NO_ESCALATION_DETAILS"=>		"No escalation details",
	"S_ESCALATION_DETAILS_BIG"=>		"ESCALATION DETAILS",
	"S_ESCALATION_ADDED"=>			"Escalation added",
	"S_ESCALATION_WAS_NOT_ADDED"=>		"Escalation was not added",
	"S_ESCALATION_RULE_ADDED"=>		"Escalation rule added",
	"S_ESCALATION_RULE_WAS_NOT_ADDED"=>	"Escalation rule was not added",
	"S_ESCALATION_RULE_UPDATED"=>		"Escalation rule updated",
	"S_ESCALATION_RULE_WAS_NOT_UPDATED"=>	"Escalation rule was not updated",
	"S_ESCALATION_RULE_DELETED"=>		"Escalation rule deleted",
	"S_ESCALATION_RULE_WAS_NOT_DELETED"=>	"Escalation rule was not deleted",
	"S_ESCALATION_UPDATED"=>		"Escalation updated",
	"S_ESCALATION_WAS_NOT_UPDATED"=>	"Escalation was not updated",
	"S_ESCALATION_DELETED"=>		"Escalation deleted",
	"S_ESCALATION_WAS_NOT_DELETED"=>	"Escalation was not deleted",
	"S_ESCALATION_RULE"=>			"Escalation rule",
	"S_DO"=>				"Do",
	"S_DEFAULT"=>				"Default",
	"S_IS_DEFAULT"=>			"Is default",
	"S_LEVEL"=>				"Level",
	"S_DELAY_BEFORE_ACTION"=>		"Delay before action",
	"S_IMAGES"=>				"Images",
	"S_IMAGE"=>				"Image",
	"S_IMAGES_BIG"=>			"IMAGES",
	"S_NO_IMAGES_DEFINED"=>			"No images defined",
	"S_BACKGROUND"=>			"Background",
	"S_UPLOAD"=>				"Upload",
	"S_IMAGE_ADDED"=>			"Image added",
	"S_CANNOT_ADD_IMAGE"=>			"Cannot add image",
	"S_IMAGE_DELETED"=>			"Image deleted",
	"S_CANNOT_DELETE_IMAGE"=>		"Cannot delete image",
	"S_IMAGE_UPDATED"=>			"Image updated",
	"S_CANNOT_UPDATE_IMAGE"=>		"Cannot update image",
	"S_UPDATE_SELECTED_IMAGE"=>		"Update selected image?",
	"S_AUTODISCOVERY"=>			"Autodiscovery",

//	Latest values
	"S_LATEST_VALUES"=>			"Latest values",
	"S_NO_PERMISSIONS"=>			"No permissions !",
	"S_LATEST_DATA"=>			"LATEST DATA",
	"S_ALL_SMALL"=>				"all",
	"S_DESCRIPTION_LARGE"=>			"DESCRIPTION",
	"S_DESCRIPTION_SMALL"=>			"Description",
	"S_GRAPH"=>				"Graph",
	"S_TREND"=>				"Trend",
	"S_COMPARE"=>				"Compare",

//	Footer
	"S_COPYRIGHT_BY"=>			"Copyright 2001-2006 by ",
	"S_CONNECTED_AS"=>			"Connected as",
	"S_SIA_ZABBIX"=>			"SIA Zabbix",

//	graph.php
	"S_CONFIGURATION_OF_GRAPH"=>		"Configuration of graph",
	"S_CONFIGURATION_OF_GRAPH_BIG"=>	"CONFIGURATION OF GRAPH",
	"S_ITEM_ADDED"=>			"Item added",
	"S_ITEM_UPDATED"=>			"Item updated",
	"S_SORT_ORDER_UPDATED"=>		"Sort order updated",
	"S_CANNOT_UPDATE_SORT_ORDER"=>		"Cannot update sort order",
	"S_DISPLAYED_PARAMETERS_BIG"=>		"DISPLAYED PARAMETERS",
	"S_SORT_ORDER"=>			"Sort order",
	"S_PARAMETER"=>				"Parameter",
	"S_COLOR"=>				"Color",
	"S_UP"=>				"Up",
	"S_DOWN"=>				"Down",
	"S_NEW_ITEM_FOR_THE_GRAPH"=>		"New item for the graph",
	"S_SORT_ORDER_1_100"=>			"Sort order (0->100)",

//	graphs.php
	"S_CONFIGURATION_OF_GRAPHS"=>		"Configuration of graphs",
	"S_CONFIGURATION_OF_GRAPHS_BIG"=>	"CONFIGURATION OF GRAPHS",
	"S_GRAPH_ADDED"=>			"Graph added",
	"S_GRAPH_UPDATED"=>			"Graph updated",
	"S_CANNOT_UPDATE_GRAPH"=>		"Cannot update graph",
	"S_GRAPH_DELETED"=>			"Graph deleted",
	"S_CANNOT_DELETE_GRAPH"=>		"Cannot delete graph",
	"S_CANNOT_ADD_GRAPH"=>			"Cannot add graph",
	"S_ID"=>				"Id",
	"S_NO_GRAPHS_DEFINED"=>			"No graphs defined",
	"S_DELETE_GRAPH_Q"=>			"Delete graph?",
	"S_YAXIS_TYPE"=>			"Y axis type",
	"S_YAXIS_MIN_VALUE"=>			"Y axis MIN value",
	"S_YAXIS_MAX_VALUE"=>			"Y axis MAX value",
	"S_CALCULATED"=>			"Calculated",
	"S_FIXED"=>				"Fixed",

//	history.php
	"S_LAST_HOUR_GRAPH"=>			"Last hour graph",
	"S_LAST_HOUR_GRAPH_DIFF"=>		"Last hour graph (diff)",
	"S_VALUES_OF_LAST_HOUR"=>		"Values of last hour",
	"S_VALUES_OF_SPECIFIED_PERIOD"=>	"Values of specified period",
	"S_VALUES_IN_PLAIN_TEXT_FORMAT"=>	"Values in plain text format",
	"S_TIMESTAMP"=>				"Timestamp",

//	hosts.php
	"S_HOSTS"=>				"Hosts",
	"S_ITEMS"=>				"Items",
	"S_TRIGGERS"=>				"Triggers",
	"S_GRAPHS"=>				"Graphs",
	"S_HOST_ADDED"=>			"Host added",
	"S_CANNOT_ADD_HOST"=>			"Cannot add host",
	"S_ITEMS_ADDED"=>			"Items added",
	"S_CANNOT_ADD_ITEMS"=>			"Cannot add items",
	"S_HOST_UPDATED"=>			"Host updated",
	"S_CANNOT_UPDATE_HOST"=>		"Cannot update host",
	"S_HOST_STATUS_UPDATED"=>		"Host status updated",
	"S_CANNOT_UPDATE_HOST_STATUS"=>		"Cannot update host status",
	"S_HOST_DELETED"=>			"Host deleted",
	"S_CANNOT_DELETE_HOST"=>		"Cannot delete host",
	"S_TEMPLATE_LINKAGE_ADDED"=>		"Template linkage added",
	"S_CANNOT_ADD_TEMPLATE_LINKAGE"=>	"Cannot add template linkage",
	"S_TEMPLATE_LINKAGE_UPDATED"=>		"Template linkage updated",
	"S_CANNOT_UPDATE_TEMPLATE_LINKAGE"=>	"Cannot update template linkage",
	"S_TEMPLATE_LINKAGE_DELETED"=>		"Template linkage deleted",
	"S_CANNOT_DELETE_TEMPLATE_LINKAGE"=>	"Cannot delete template linkage",
	"S_CONFIGURATION_OF_HOSTS_AND_HOST_GROUPS"=>"CONFIGURATION OF HOSTS AND HOST GROUPS",
	"S_HOST_GROUPS_BIG"=>			"HOST GROUPS",
	"S_NO_HOST_GROUPS_DEFINED"=>		"No host groups defined",
	"S_NO_LINKAGES_DEFINED"=>		"No linkages defined",
	"S_NO_HOSTS_DEFINED"=>			"No hosts defined",
	"S_HOSTS_BIG"=>				"HOSTS",
	"S_HOST"=>				"Host",
	"S_IP"=>				"IP",
	"S_PORT"=>				"Port",
	"S_MONITORED"=>				"Monitored",
	"S_NOT_MONITORED"=>			"Not monitored",
	"S_UNREACHABLE"=>			"Unreachable",
	"S_TEMPLATE"=>				"Template",
	"S_DELETED"=>				"Deleted",
	"S_UNKNOWN"=>				"Unknown",
	"S_GROUPS"=>				"Groups",
	"S_NEW_GROUP"=>				"New group",
	"S_USE_IP_ADDRESS"=>			"Use IP address",
	"S_IP_ADDRESS"=>			"IP address",
//	"S_USE_THE_HOST_AS_A_TEMPLATE"=>		"Use the host as a template",
	"S_USE_TEMPLATES_OF_THIS_HOST"=>	"Use templates of this host",
	"S_DELETE_SELECTED_HOST_Q"=>		"Delete selected host?",
	"S_GROUP_NAME"=>			"Group name",
	"S_HOST_GROUP"=>			"Host group",
	"S_HOST_GROUPS"=>			"Host groups",
	"S_UPDATE"=>				"Update",
	"S_AVAILABILITY"=>			"Availability",
	"S_AVAILABLE"=>				"Available",
	"S_NOT_AVAILABLE"=>			"Not available",

//	items.php
	"S_CONFIGURATION_OF_ITEMS"=>		"Configuration of items",
	"S_CONFIGURATION_OF_ITEMS_BIG"=>	"CONFIGURATION OF ITEMS",
	"S_CANNOT_UPDATE_ITEM"=>		"Cannot update item",
	"S_STATUS_UPDATED"=>			"Status updated",
	"S_CANNOT_UPDATE_STATUS"=>		"Cannot update status",
	"S_CANNOT_ADD_ITEM"=>			"Cannot add item",
	"S_ITEM_DELETED"=>			"Item deleted",
	"S_CANNOT_DELETE_ITEM"=>		"Cannot delete item",
	"S_ITEMS_DELETED"=>			"Items deleted",
	"S_CANNOT_DELETE_ITEMS"=>		"Cannot delete items",
	"S_ITEMS_ACTIVATED"=>			"Items activated",
	"S_CANNOT_ACTIVATE_ITEMS"=>		"Cannot activate items",
	"S_ITEMS_DISABLED"=>			"Items disabled",
	"S_SERVERNAME"=>			"Server Name",
	"S_KEY"=>				"Key",
	"S_DESCRIPTION"=>			"Description",
	"S_UPDATE_INTERVAL"=>			"Update interval",
	"S_HISTORY"=>				"History",
	"S_TRENDS"=>				"Trends",
	"S_SHORT_NAME"=>			"Short name",
	"S_ZABBIX_AGENT"=>			"ZABBIX agent",
	"S_ZABBIX_AGENT_ACTIVE"=>		"ZABBIX agent (active)",
	"S_SNMPV1_AGENT"=>			"SNMPv1 agent",
	"S_ZABBIX_TRAPPER"=>			"ZABBIX trapper",
	"S_SIMPLE_CHECK"=>			"Simple check",
	"S_SNMPV2_AGENT"=>			"SNMPv2 agent",
	"S_SNMPV3_AGENT"=>			"SNMPv3 agent",
	"S_ZABBIX_INTERNAL"=>			"ZABBIX internal",
	"S_ZABBIX_UNKNOWN"=>			"Unknown",
	"S_ACTIVE"=>				"Active",
	"S_NOT_ACTIVE"=>			"Not active",
	"S_NOT_SUPPORTED"=>			"Not supported",
	"S_ACTIVATE_SELECTED_ITEMS_Q"=>		"Activate selected items?",
	"S_DISABLE_SELECTED_ITEMS_Q"=>		"Disable selected items?",
	"S_DELETE_SELECTED_ITEMS_Q"=>		"Delete selected items?",
	"S_EMAIL"=>				"Email",
	"S_SCRIPT"=>				"Script",
	"S_UNITS"=>				"Units",
	"S_MULTIPLIER"=>			"Multiplier",
	"S_UPDATE_INTERVAL_IN_SEC"=>		"Update interval (in sec)",
	"S_KEEP_HISTORY_IN_DAYS"=>		"Keep history (in days)",
	"S_KEEP_TRENDS_IN_DAYS"=>		"Keep trends (in days)",
	"S_TYPE_OF_INFORMATION"=>		"Type of information",
	"S_STORE_VALUE"=>			"Store value",
	"S_NUMERIC_FLOAT"=>			"Numeric (float)",
	"S_NUMERIC_UINT64"=>			"Numeric (integer 64 bit)",
	"S_CHARACTER"=>				"Character",
	"S_LOG"=>				"Log",
	"S_AS_IS"=>				"As is",
	"S_DELTA_SPEED_PER_SECOND"=>		"Delta (speed per second)",
	"S_DELTA_SIMPLE_CHANGE"=>		"Delta (simple change)",
	"S_ITEM"=>				"Item",
	"S_SNMP_COMMUNITY"=>			"SNMP community",
	"S_SNMP_OID"=>				"SNMP OID",
	"S_SNMP_PORT"=>				"SNMP port",
	"S_ALLOWED_HOSTS"=>			"Allowed hosts",
	"S_SNMPV3_SECURITY_NAME"=>		"SNMPv3 security name",
	"S_SNMPV3_SECURITY_LEVEL"=>		"SNMPv3 security level",
	"S_SNMPV3_AUTH_PASSPHRASE"=>		"SNMPv3 auth passphrase",
	"S_SNMPV3_PRIV_PASSPHRASE"=>		"SNMPv3 priv passphrase",
	"S_CUSTOM_MULTIPLIER"=>			"Custom multiplier",
	"S_DO_NOT_USE"=>			"Do not use",
	"S_USE_MULTIPLIER"=>			"Use multiplier",
	"S_SELECT_HOST_DOT_DOT_DOT"=>		"Select host...",

//	latestalarms.php
	"S_LATEST_EVENTS"=>			"Latest events",
	"S_HISTORY_OF_EVENTS_BIG"=>		"HISTORY OF EVENTS",

//	latest.php
	"S_LAST_CHECK"=>			"Last check",
	"S_LAST_CHECK_BIG"=>			"LAST CHECK",
	"S_LAST_VALUE"=>			"Last value",

//	sysmap.php
	"S_LABEL"=>				"Label",
	"S_X"=>					"X",
	"S_Y"=>					"Y",
	"S_ICON"=>				"Icon",
	"S_HOST_1"=>				"Host 1",
	"S_HOST_2"=>				"Host 2",
	"S_LINK_STATUS_INDICATOR"=>		"Link status indicator",

//	map.php
	"S_OK_BIG"=>				"OK",
	"S_PROBLEMS_SMALL"=>			"problems",
	"S_ZABBIX_URL"=>			"http://www.zabbix.com",

//	maps.php
	"S_NETWORK_MAPS"=>			"Network maps",
	"S_NETWORK_MAPS_BIG"=>			"NETWORK MAPS",
	"S_NO_MAPS_TO_DISPLAY"=>		"No maps to display",
	"S_SELECT_MAP_TO_DISPLAY"=>		"Select map to display",
	"S_SELECT_MAP_DOT_DOT_DOT"=>		"Select map...",
	"S_BACKGROUND_IMAGE"=>			"Background image",
	"S_ICON_LABEL_TYPE"=>			"Icon label type",
	"S_HOST_LABEL"=>			"Host label",
	"S_HOST_NAME"=>				"Host name",
	"S_STATUS_ONLY"=>			"Status only",
	"S_NOTHING"=>				"Nothing",

//	media.php
	"S_MEDIA"=>				"Media",
	"S_MEDIA_BIG"=>				"MEDIA",
	"S_MEDIA_ACTIVATED"=>			"Media activated",
	"S_CANNOT_ACTIVATE_MEDIA"=>		"Cannot activate media",
	"S_MEDIA_DISABLED"=>			"Media disabled",
	"S_CANNOT_DISABLE_MEDIA"=>		"Cannot disable media",
	"S_MEDIA_ADDED"=>			"Media added",
	"S_CANNOT_ADD_MEDIA"=>			"Cannot add media",
	"S_MEDIA_UPDATED"=>			"Media updated",
	"S_CANNOT_UPDATE_MEDIA"=>		"Cannot update media",
	"S_MEDIA_DELETED"=>			"Media deleted",
	"S_CANNOT_DELETE_MEDIA"=>		"Cannot delete media",
	"S_SEND_TO"=>				"Send to",
	"S_WHEN_ACTIVE"=>			"When active",
	"S_NO_MEDIA_DEFINED"=>			"No media defined",
	"S_NEW_MEDIA"=>				"New media",
	"S_USE_IF_SEVERITY"=>			"Use if severity",
	"S_DELETE_SELECTED_MEDIA_Q"=>		"Delete selected media?",

//	Menu
	"S_MENU_LATEST_VALUES"=>		"LATEST VALUES",
	"S_MENU_TRIGGERS"=>			"TRIGGERS",
	"S_MENU_QUEUE"=>			"QUEUE",
	"S_MENU_ALARMS"=>			"ALARMS",
	"S_MENU_ALERTS"=>			"ALERTS",
	"S_MENU_NETWORK_MAPS"=>			"NETWORK MAPS",
	"S_MENU_GRAPHS"=>			"GRAPHS",
	"S_MENU_SCREENS"=>			"SCREENS",
	"S_MENU_IT_SERVICES"=>			"IT SERVICES",
	"S_MENU_HOME"=>				"HOME",
	"S_MENU_ABOUT"=>			"ABOUT",
	"S_MENU_STATUS_OF_ZABBIX"=>		"STATUS OF ZABBIX",
	"S_MENU_AVAILABILITY_REPORT"=>		"AVAILABILITY REPORT",
	"S_MENU_CONFIG"=>			"CONFIG",
	"S_MENU_USERS"=>			"USERS",
	"S_MENU_HOSTS"=>			"HOSTS",
	"S_MENU_ITEMS"=>			"ITEMS",
	"S_MENU_AUDIT"=>			"AUDIT",

//	overview.php
	"S_SELECT_GROUP_DOT_DOT_DOT"=>		"Select group ...",
	"S_OVERVIEW"=>				"Overview",
	"S_OVERVIEW_BIG"=>			"OVERVIEW",
	"S_EXCL"=>				"!",
	"S_DATA"=>				"Data",

//	queue.php
	"S_QUEUE_BIG"=>				"QUEUE",
	"S_QUEUE_OF_ITEMS_TO_BE_UPDATED_BIG"=>	"QUEUE OF ITEMS TO BE UPDATED",
	"S_NEXT_CHECK"=>			"Next check",
	"S_THE_QUEUE_IS_EMPTY"=>		"The queue is empty",
	"S_TOTAL"=>				"Total",
	"S_COUNT"=>				"Count",
	"S_5_SECONDS"=>				"5 seconds",
	"S_10_SECONDS"=>			"10 seconds",
	"S_30_SECONDS"=>			"30 seconds",
	"S_1_MINUTE"=>				"1 minute",
	"S_5_MINUTES"=>				"5 minutes",
	"S_MORE_THAN_5_MINUTES"=>		"More than 5 minutes",

//	report1.php
	"S_STATUS_OF_ZABBIX"=>			"Status of ZABBIX",
	"S_STATUS_OF_ZABBIX_BIG"=>		"STATUS OF ZABBIX",
	"S_VALUE"=>				"Value",
	"S_ZABBIX_SERVER_IS_RUNNING"=>		"ZABBIX server is running",
	"S_NUMBER_OF_VALUES_STORED"=>		"Number of values stored",
	"S_NUMBER_OF_TRENDS_STORED"=>		"Number of trends stored",
	"S_NUMBER_OF_ALARMS"=>			"Number of alarms",
	"S_NUMBER_OF_ALERTS"=>			"Number of alerts",
	"S_NUMBER_OF_TRIGGERS_ENABLED_DISABLED"=>"Number of triggers (enabled/disabled)",
	"S_NUMBER_OF_ITEMS_ACTIVE_TRAPPER"=>	"Number of items (active/trapper/not active/not supported)",
	"S_NUMBER_OF_USERS"=>			"Number of users",
	"S_NUMBER_OF_HOSTS_MONITORED"=>		"Number of hosts (monitored/not monitored/templates/deleted)",
	"S_YES"=>				"Yes",
	"S_NO"=>				"No",

//	report2.php
	"S_AVAILABILITY_REPORT"=>		"Availability report",
	"S_AVAILABILITY_REPORT_BIG"=>		"AVAILABILITY REPORT",
	"S_SHOW"=>				"Show",
	"S_TRUE"=>				"True",
	"S_FALSE"=>				"False",

//	report3.php
	"S_IT_SERVICES_AVAILABILITY_REPORT_BIG"=>	"IT SERVICES AVAILABILITY REPORT",
	"S_FROM"=>				"From",
	"S_TILL"=>				"Till",
	"S_OK"=>				"Ok",
	"S_PROBLEMS"=>				"Problems",
	"S_PERCENTAGE"=>			"Percentage",
	"S_SLA"=>				"SLA",
	"S_DAY"=>				"Day",
	"S_MONTH"=>				"Month",
	"S_YEAR"=>				"Year",
	"S_DAILY"=>				"Daily",
	"S_WEEKLY"=>				"Weekly",
	"S_MONTHLY"=>				"Monthly",
	"S_YEARLY"=>				"Yearly",

//	screenconf.php
	"S_SCREENS"=>				"Screens",
	"S_SCREEN"=>				"Screen",
	"S_CONFIGURATION_OF_SCREENS_BIG"=>	"CONFIGURATION OF SCREENS",
	"S_SCREEN_ADDED"=>			"Screen added",
	"S_CANNOT_ADD_SCREEN"=>			"Cannot add screen",
	"S_SCREEN_UPDATED"=>			"Screen updated",
	"S_CANNOT_UPDATE_SCREEN"=>		"Cannot update screen",
	"S_SCREEN_DELETED"=>			"Screen deleted",
	"S_CANNOT_DELETE_SCREEN"=>		"Cannot deleted screen",
	"S_COLUMNS"=>				"Columns",
	"S_ROWS"=>				"Rows",
	"S_NO_SCREENS_DEFINED"=>		"No screens defined",
	"S_DELETE_SCREEN_Q"=>			"Delete screen?",
	"S_CONFIGURATION_OF_SCREEN_BIG"=>	"CONFIGURATION OF SCREEN",
	"S_SCREEN_CELL_CONFIGURATION"=>		"Screen cell configuration",
	"S_RESOURCE"=>				"Resource",
	"S_SIMPLE_GRAPH"=>			"Simple graph",
	"S_GRAPH_NAME"=>			"Graph name",
	"S_WIDTH"=>				"Width",
	"S_HEIGHT"=>				"Height",
	"S_EMPTY"=>				"Empty",

//	screenedit.php
	"S_MAP"=>				"Map",
	"S_PLAIN_TEXT"=>			"Plain text",
	"S_COLUMN_SPAN"=>			"Column span",
	"S_ROW_SPAN"=>				"Row span",

//	screens.php
	"S_CUSTOM_SCREENS"=>			"Custom screens",
	"S_SCREENS_BIG"=>			"SCREENS",
	"S_NO_SCREENS_TO_DISPLAY"=>		"No screens to display",
	"S_SELECT_SCREEN_TO_DISPLAY"=>		"Select screen to display",
	"S_SELECT_SCREEN_DOT_DOT_DOT"=>		"Select screen ...",

//	services.php
	"S_IT_SERVICES"=>			"IT services",
	"S_SERVICE_UPDATED"=>			"Service updated",
	"S_CANNOT_UPDATE_SERVICE"=>		"Cannot update service",
	"S_SERVICE_ADDED"=>			"Service added",
	"S_CANNOT_ADD_SERVICE"=>		"Cannot add service",
	"S_LINK_ADDED"=>			"Link added",
	"S_CANNOT_ADD_LINK"=>			"Cannot add link",
	"S_SERVICE_DELETED"=>			"Service deleted",
	"S_CANNOT_DELETE_SERVICE"=>		"Cannot delete service",
	"S_LINK_DELETED"=>			"Link deleted",
	"S_CANNOT_DELETE_LINK"=>		"Cannot delete link",
	"S_STATUS_CALCULATION"=>		"Status calculation",
	"S_STATUS_CALCULATION_ALGORITHM"=>	"Status calculation algorithm",
	"S_NONE"=>				"None",
	"S_MAX_OF_CHILDS"=>			"MAX of childs",
	"S_MIN_OF_CHILDS"=>			"MIN of childs",
	"S_SERVICE_1"=>				"Service 1",
	"S_SERVICE_2"=>				"Service 2",
	"S_SOFT_HARD_LINK"=>			"Soft/hard link",
	"S_SOFT"=>				"Soft",
	"S_HARD"=>				"Hard",
	"S_DO_NOT_CALCULATE"=>			"Do not calculate",
	"S_MAX_BIG"=>				"MAX",
	"S_MIN_BIG"=>				"MIN",
	"S_SHOW_SLA"=>				"Show SLA",
	"S_ACCEPTABLE_SLA_IN_PERCENT"=>		"Acceptabe SLA (in %)",
	"S_LINK_TO_TRIGGER_Q"=>			"Link to trigger?",
	"S_SORT_ORDER_0_999"=>			"Sort order (0->999)",
	"S_DELETE_SERVICE_Q"=>			"S_DELETE_SERVICE_Q",
	"S_LINK_TO"=>				"Link to",
	"S_SOFT_LINK_Q"=>			"Soft link?",
	"S_ADD_SERVER_DETAILS"=>		"Add server details",
	"S_TRIGGER"=>				"Trigger",
	"S_SERVER"=>				"Server",
	"S_DELETE"=>				"Delete",

//	srv_status.php
	"S_IT_SERVICES_BIG"=>			"IT SERVICES",
	"S_SERVICE"=>				"Service",
	"S_REASON"=>				"Reason",
	"S_SLA_LAST_7_DAYS"=>			"SLA (last 7 days)",
	"S_PLANNED_CURRENT_SLA"=>		"Planned/current SLA",
	"S_TRIGGER_BIG"=>			"TRIGGER",

//	triggers.php
	"S_CONFIGURATION_OF_TRIGGERS"=>		"Configuration of triggers",
	"S_CONFIGURATION_OF_TRIGGERS_BIG"=>	"CONFIGURATION OF TRIGGERS",
	"S_DEPENDENCY_ADDED"=>			"Dependency added",
	"S_CANNOT_ADD_DEPENDENCY"=>		"Cannot add dependency",
	"S_TRIGGERS_UPDATED"=>			"Triggers updated",
	"S_CANNOT_UPDATE_TRIGGERS"=>		"Cannot update triggers",
	"S_TRIGGERS_DISABLED"=>			"Triggers disabled",
	"S_CANNOT_DISABLE_TRIGGERS"=>		"Cannot disable triggers",
	"S_TRIGGERS_DELETED"=>			"Triggers deleted",
	"S_CANNOT_DELETE_TRIGGERS"=>		"Cannot delete triggers",
	"S_TRIGGER_DELETED"=>			"Trigger deleted",
	"S_CANNOT_DELETE_TRIGGER"=>		"Cannot delete trigger",
	"S_INVALID_TRIGGER_EXPRESSION"=>	"Invalid trigger expression",
	"S_TRIGGER_ADDED"=>			"Trigger added",
	"S_CANNOT_ADD_TRIGGER"=>		"Cannot add trigger",
	"S_SEVERITY"=>				"Severity",
	"S_EXPRESSION"=>			"Expression",
	"S_DISABLED"=>				"Disabled",
	"S_ENABLED"=>				"Enabled",
	"S_ENABLE_SELECTED_TRIGGERS_Q"=>	"Enable selected triggers?",
	"S_DISABLE_SELECTED_TRIGGERS_Q"=>	"Disable selected triggers?",
	"S_CHANGE"=>				"Change",
	"S_TRIGGER_UPDATED"=>			"Trigger updated",
	"S_CANNOT_UPDATE_TRIGGER"=>		"Cannot update trigger",
	"S_DEPENDS_ON"=>			"Depends on",

//	tr_comments.php
	"S_TRIGGER_COMMENTS"=>			"Trigger comments",
	"S_TRIGGER_COMMENTS_BIG"=>		"TRIGGER COMMENTS",
	"S_COMMENT_UPDATED"=>			"Comment updated",
	"S_CANNOT_UPDATE_COMMENT"=>		"Cannot update comment",
	"S_ADD"=>				"Add",

//	tr_status.php
	"S_STATUS_OF_TRIGGERS"=>		"Status of triggers",
	"S_STATUS_OF_TRIGGERS_BIG"=>		"STATUS OF TRIGGERS",
	"S_SHOW_ONLY_TRUE"=>			"Show only true",
	"S_HIDE_ACTIONS"=>			"Hide actions",
	"S_SHOW_ACTIONS"=>			"Show actions",
	"S_SHOW_ALL_TRIGGERS"=>			"Show all triggers",
	"S_HIDE_DETAILS"=>			"Hide details",
	"S_SHOW_DETAILS"=>			"Show details",
	"S_SELECT"=>				"Select",
	"S_HIDE_SELECT"=>			"Hide select",
	"S_TRIGGERS_BIG"=>			"TRIGGERS",
	"S_DESCRIPTION_BIG"=>			"DESCRIPTION",
	"S_SEVERITY_BIG"=>			"SEVERITY",
	"S_LAST_CHANGE_BIG"=>			"LAST CHANGE",
	"S_LAST_CHANGE"=>			"Last change",
	"S_COMMENTS"=>				"Comments",

//	users.php
	"S_USERS"=>				"Users",
	"S_USER_ADDED"=>			"User added",
	"S_CANNOT_ADD_USER"=>			"Cannot add user",
	"S_CANNOT_ADD_USER_BOTH_PASSWORDS_MUST"=>"Cannot add user. Both passwords must be equal.",
	"S_USER_DELETED"=>			"User deleted",
	"S_CANNOT_DELETE_USER"=>		"Cannot delete user",
	"S_PERMISSION_DELETED"=>		"Permission deleted",
	"S_CANNOT_DELETE_PERMISSION"=>		"Cannot delete permission",
	"S_PERMISSION_ADDED"=>			"Permission added",
	"S_CANNOT_ADD_PERMISSION"=>		"Cannot add permission",
	"S_USER_UPDATED"=>			"User updated",
	"S_CANNOT_UPDATE_USER"=>		"Cannot update user",
	"S_CANNOT_UPDATE_USER_BOTH_PASSWORDS"=>	"Cannot update user. Both passwords must be equal.",
	"S_GROUP_ADDED"=>			"Group added",
	"S_CANNOT_ADD_GROUP"=>			"Cannot add group",
	"S_GROUP_UPDATED"=>			"Group updated",
	"S_CANNOT_UPDATE_GROUP"=>		"Cannot update group",
	"S_GROUP_DELETED"=>			"Group deleted",
	"S_CANNOT_DELETE_GROUP"=>		"Cannot delete group",
	"S_CONFIGURATION_OF_USERS_AND_USER_GROUPS"=>"CONFIGURATION OF USERS AND USER GROUPS",
	"S_USER_GROUPS_BIG"=>			"USER GROUPS",
	"S_USERS_BIG"=>				"USERS",
	"S_USER_GROUPS"=>			"User groups",
	"S_MEMBERS"=>				"Members",
	"S_TEMPLATES"=>				"Templates",
	"S_HOSTS_TEMPLATES_LINKAGE"=>		"Hosts/templates linkage",
	"S_CONFIGURATION_OF_TEMPLATES_LINKAGE"=>"CONFIGURATION OF TEMPLATES LINKAGE",
	"S_LINKED_TEMPLATES_BIG"=>		"LINKED TEMPLATES",
	"S_NO_USER_GROUPS_DEFINED"=>		"No user groups defined",
	"S_ALIAS"=>				"Alias",
	"S_NAME"=>				"Name",
	"S_SURNAME"=>				"Surname",
	"S_IS_ONLINE_Q"=>			"Is online?",
	"S_NO_USERS_DEFINED"=>			"No users defined",
	"S_PERMISSION"=>			"Permission",
	"S_RIGHT"=>				"Right",
	"S_RESOURCE_NAME"=>			"Resource name",
	"S_READ_ONLY"=>				"Read only",
	"S_READ_WRITE"=>			"Read-write",
	"S_HIDE"=>				"Hide",
	"S_PASSWORD"=>				"Password",
	"S_PASSWORD_ONCE_AGAIN"=>		"Password (once again)",
	"S_URL_AFTER_LOGIN"=>			"URL (after login)",
	"S_AUTO_LOGOUT_IN_SEC"=>		"Auto-logout (in sec=>0 - disable)",
	"S_SCREEN_REFRESH"=>                    "Refresh (in seconds)",

//	audit.php
	"S_AUDIT_LOG"=>				"Audit log",
	"S_AUDIT_LOG_BIG"=>			"AUDIT LOG",
	"S_ACTION"=>				"Action",
	"S_DETAILS"=>				"Details",
	"S_UNKNOWN_ACTION"=>			"Unknown action",
	"S_ADDED"=>				"Added",
	"S_UPDATED"=>				"Updated",
	"S_LOGGED_IN"=>				"Logged in",
	"S_LOGGED_OUT"=>			"Logged out",
	"S_MEDIA_TYPE"=>			"Media type",
	"S_GRAPH_ELEMENT"=>			"Graph element",

//	profile.php
	"S_USER_PROFILE_BIG"=>			"USER PROFILE",
	"S_USER_PROFILE"=>			"User profile",
	"S_LANGUAGE"=>				"Language",
	"S_ENGLISH_GB"=>			"English (GB)",
	"S_FRENCH_FR"=>				"French (FR)",
	"S_GERMAN_DE"=>				"German (DE)",
	"S_LATVIAN_LV"=>			"Latvian (LV)",
	"S_RUSSIAN_RU"=>			"Russian (RU)",

//	index.php
	"S_ZABBIX_BIG"=>			"ZABBIX",

//	bulkloader.php
	"S_MENU_BULKLOADER"=>			"Bulkloader",
	"S_BULKLOADER_MAIN"=>			"Bulkloader: Main Page",
	"S_BULKLOADER_HOSTS"=>			"Bulkloader: Hosts",
	"S_BULKLOADER_ITEMS"=>			"Bulkloader: Items",
	"S_BULKLOADER_USERS"=>			"Bulkloader: Users",
	"S_BULKLOADER_TRIGGERS"=>		"Bulkloader: Triggers",
	"S_BULKLOADER_ACTIONS"=>		"Bulkloader: Actions",
	"S_BULKLOADER_ITSERVICES"=>		"Bulkloader: IT Services",

	"S_BULKLOADER_IMPORT_HOSTS"=>		"Import Hosts",
	"S_BULKLOADER_IMPORT_ITEMS"=>		"Import Items",
	"S_BULKLOADER_IMPORT_USERS"=>		"Import Users",
	"S_BULKLOADER_IMPORT_TRIGGERS"=>	"Import Triggers",
	"S_BULKLOADER_IMPORT_ACTIONS"=>		"Import Actions",
	"S_BULKLOADER_IMPORT_ITSERVICES"=>	"Import IT Services",

//	Menu

	"S_HELP"=>				"Help",
	"S_PROFILE"=>				"Profile",
	);
?>
