## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libzbxdbhigh.a

libzbxdbhigh_a_SOURCES = \
	host.c \
	db.c \
	hfs.c

if MEMCACHE
libzbxdbhigh_a_SOURCES += memcache.c serialize.c memcache_php.c
endif

libzbxdbhigh_a_CFLAGS = -I@top_srcdir@/src/zabbix_server/
