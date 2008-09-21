## Process this file with automake to produce Makefile.in

AM_CFLAGS = -I../libs/zbxdbhigh/

ZBX_LIBS = \
	../libs/zbxcomms/libzbxcomms.a \
	../libs/zbxdbhigh/libzbxdbhigh.a \
	../libs/zbxlog/libzbxlog.a \
	../libs/zbxconf/libzbxconf.a \
	../libs/zbxcommon/libzbxcommon.a \
	../libs/zbxcrypto/libzbxcrypto.a \
	../libs/zbxsys/libzbxsys.a

hfsdump_LDADD = $(ZBX_LIBS)
hfs_trends_upd_LDADD = $(ZBX_LIBS)
hfsimport_LDADD = $(ZBX_LIBS)
hfsfilter_LDADD = $(ZBX_LIBS)
hfsevents_LDADD = $(ZBX_LIBS)

sbin_PROGRAMS = hfsdump hfsimport hfs_trends_upd hfsfilter hfsevents
hfsdump_SOURCES = hfsdump.c
hfs_trends_upd_SOURCES = hfs_trends_upd.c
hfsimport_SOURCES = hfsimport.c
hfsfilter_SOURCES = hfsfilter.c
hfsevents_SOURCES = hfsevents.c
