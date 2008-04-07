PHP_ARG_WITH(zabbix,for zabbix support,
[  --with-zabbix     Include zabbix support])

if test "$PHP_ZABBIX" != "no"; then
	#PHP_ADD_INCLUDE($RECODE_DIR/$RECODE_INC)
	PHP_ADD_LIBRARY(../libs/zbxdbhigh/libzbxdbhigh.a ,,ZABBIX_SHARED_LIBADD)
	PHP_SUBST(ZABBIX_SHARED_LIBADD)
	PHP_NEW_EXTENSION(zabbix, zabbix.c, $ext_shared)
fi
