#ifndef PHP_ZABBIX_H
#define PHP_ZABBIX_H

extern zend_module_entry zabbix_module_entry;
#define phpext_zabbix_ptr &zabbix_module_entry

PHP_MINIT_FUNCTION(zabbix);
PHP_MSHUTDOWN_FUNCTION(zabbix);
PHP_MINFO_FUNCTION(zabbix);
PHP_FUNCTION(recode_string);
PHP_FUNCTION(recode_file);

#endif	/* PHP_ZABBIX_H */
