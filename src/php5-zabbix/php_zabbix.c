#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"

#ifdef ZTS
# define ReSG(v) TSRMG(zabbix_globals_id, zend_zabbix_globals *, v)
#else
# define ReSG(v) (zabbix_globals.v)
#endif

#include <stdio.h>
