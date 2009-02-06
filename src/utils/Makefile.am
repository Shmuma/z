## Process this file with automake to produce Makefile.in

AM_CFLAGS = -I../libs/zbxdbhigh/

ZBX_LIBS = \
	../libs/zbxcomms/libzbxcomms.a \
	../libs/zbxdbhigh/libzbxdbhigh.a \
	../libs/zbxlog/libzbxlog.a \
	../libs/zbxconf/libzbxconf.a \
	../libs/zbxcommon/libzbxcommon.a \
	../libs/zbxcrypto/libzbxcrypto.a \
	../libs/zbxsys/libzbxsys.a -lrt -lmemcached

hfsdump_LDADD = $(ZBX_LIBS)
hfs_trends_upd_LDADD = $(ZBX_LIBS)
hfsimport_LDADD = $(ZBX_LIBS)
hfsfilter_LDADD = $(ZBX_LIBS)
hfsevents_LDADD = $(ZBX_LIBS)
memcachetest_LDADD = $(ZBX_LIBS) -lmemcached
hfs_func_upd_LDADD = $(ZBX_LIBS) -L../libs/zbxlog -lzbxlog -L../libs/zbxsys -lzbxsys -L../libs/zbxdb -lzbxdb -L../libs/zbxdbhigh -lzbxdbhigh $(DB_LDFLAGS) $(DB_LIBS)
sbin_PROGRAMS = hfsdump hfsimport hfs_trends_upd hfsfilter hfsevents memcachetest hfs_func_upd

hfsdump_SOURCES = hfsdump.c fake-vars.c
hfs_trends_upd_SOURCES = hfs_trends_upd.c fake-vars.c
hfsimport_SOURCES = hfsimport.c fake-vars.c
hfsfilter_SOURCES = hfsfilter.c fake-vars.c
hfsevents_SOURCES = hfsevents.c fake-vars.c
hfs_func_upd_SOURCES = hfs_func_upd.c fake-vars.c
memcachetest_SOURCES = memcachetest.c fake-vars.c
