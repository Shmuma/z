## Process this file with automake to produce Makefile.in

hfsdump_CFLAGS = -I../libs/zbxdbhigh/

ZBX_LIBS = \
	../../zbxcomms/libzbxcomms.a \
	../../zbxdbhigh/libzbxdbhigh.a \
	../../zbxlog/libzbxlog.a \
	../../zbxconf/libzbxconf.a \
	../../zbxcommon/libzbxcommon.a \
	../../zbxcrypto/libzbxcrypto.a \
	../../zbxsys/libzbxsys.a

hfsdump_LDADD = $(ZBX_LIBS)
hfs_trends_upd_LDADD = $(ZBX_LIBS)

sbin_PROGRAMS = hfsdump hfs_trends_upd
hfsdump_SOURCES = hfsdump.c
hfs_trends_upd_SOURCES = hfs_trends_upd.c
