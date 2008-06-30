STATIC_LIBS = \
	../libzbxdbhigh.a \
	../../zbxlog/libzbxlog.a \
	../../zbxcommon/libzbxcommon.a \
	../../zbxcomms/libzbxcomms.a \
	../../zbxconf/libzbxconf.a \
	../../zbxcrypto/libzbxcrypto.a \
	../../zbxdb/libzbxdb.a \
	../../zbxdbhigh/libzbxdbhigh.a \
	../../zbxemail/libzbxemail.a \
	../../zbxlog/libzbxlog.a \
	../../zbxnix/libzbxnix.a \
	../../zbxplugin/libzbxplugin.a \
	../../zbxsms/libzbxsms.a \
	../../zbxsys/libzbxsys.a \
	-lm

all:
	$(CC) -O0 -g -I../../../../include -o hfs-test hfs-test.c $(STATIC_LIBS)
#	$(CC) -O0 -g -I../../../../include -o hfs-count hfs-count.c $(STATIC_LIBS)
	$(CC) -O0 -g -I../../../../include -o hfs-str hfs-str.c $(STATIC_LIBS)
