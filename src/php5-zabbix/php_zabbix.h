#ifndef PHP_ZABBIX_H
#define PHP_ZABBIX_H

#ifdef ZTS
# define ReSG(v) TSRMG(zabbix_globals_id, zend_zabbix_globals *, v)
#else
# define ReSG(v) (zabbix_globals.v)
#endif

extern zend_module_entry zabbix_module_entry;
#define phpext_zabbix_ptr &zabbix_module_entry

PHP_MINIT_FUNCTION(zabbix);
PHP_MSHUTDOWN_FUNCTION(zabbix);
PHP_MINFO_FUNCTION(zabbix);
PHP_FUNCTION(zabbix_hfs_read);

#endif	/* PHP_ZABBIX_H */
