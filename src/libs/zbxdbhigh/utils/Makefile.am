## Process this file with automake to produce Makefile.in

hfsdump_CFLAGS = -I..

hfsdump_LDADD = \
	../../zbxcomms/libzbxcomms.a \
	../../zbxdbhigh/libzbxdbhigh.a \
	../../zbxlog/libzbxlog.a \
	../../zbxconf/libzbxconf.a \
	../../zbxcommon/libzbxcommon.a \
	../../zbxcrypto/libzbxcrypto.a \
	../../zbxsys/libzbxsys.a

sbin_PROGRAMS = hfsdump hfs_trends_upd
hfsdump_SOURCES = hfsdump.c
hfs_trends_upd_SOURCES = hfs_trends_upd.c
