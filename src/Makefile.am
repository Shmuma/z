## Process this file with automake to produce Makefile.in

DIST_SUBDIRS = \
	libs \
	zabbix_agent \
	zabbix_get \
	zabbix_sender \
	zabbix_server \
	utils

if AGENT
AGENT_SUBDIRS = \
	zabbix_agent \
	zabbix_get \
	zabbix_sender
endif

if SERVER
SERVER_SUBDIRS = zabbix_server utils
endif

SUBDIRS = \
	libs \
	$(AGENT_SUBDIRS) \
	$(SERVER_SUBDIRS)
