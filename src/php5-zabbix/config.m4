PHP_ARG_WITH(zabbix, Zabbix support,
[  --with-zabbix        Zabbix support])

if test "$PHP_ZABBIX" != "no"; then
  PHP_ADD_INCLUDE(../../include)
  PHP_ADD_INCLUDE(../../src/libs/zbxdbhigh)

  #shared_objects_zabbix='../libs/zbxdbhigh/libzbxdbhigh.a ../libs/zbxlog/libzbxlog.a ../libs/zbxcommon/libzbxcommon.a ../libs/zbxsys/libzbxsys.a ../libs/zbxconf/libzbxconf.a ../libs/zbxcrypto/libzbxcrypto.a'
  #PHP_SUBST(shared_objects_zabbix)
  
  PHP_NEW_EXTENSION(zabbix, php_zabbix.c hfs.c memcache_php.c, $ext_shared,,-DZABBIX_PHP_MODULE=1)
  ZABBIX_SHARED_LIBADD=-lmemcached
  PHP_SUBST(ZABBIX_SHARED_LIBADD)
fi
