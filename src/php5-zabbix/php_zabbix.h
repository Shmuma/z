#ifndef PHP_ZABBIX_H
#define PHP_ZABBIX_H

#include "SAPI.h"
#include "zend_API.h"
#include "php.h"
#include "php_ini.h"

extern zend_module_entry zabbix_module_entry;
#define phpext_zabbix_ptr &zabbix_module_entry

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(zabbix);
PHP_MSHUTDOWN_FUNCTION(zabbix);
PHP_MINFO_FUNCTION(zabbix);
PHP_FUNCTION(zabbix_hfs_read);
PHP_FUNCTION(zabbix_hfs_last);

ZEND_BEGIN_MODULE_GLOBALS(zabbix)
char *hfs_base_dir;
ZEND_END_MODULE_GLOBALS(zabbix)

#ifdef ZTS
# define ZABBIX_GLOBAL(v) TSRMG(zabbix_globals_id, zend_zabbix_globals *, v)
#else
# define ZABBIX_GLOBAL(v) (zabbix_globals.v)
#endif

#endif	/* PHP_ZABBIX_H */