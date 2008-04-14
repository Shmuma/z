#include <stdio.h>

#include "php_config.h"
#include "php.h"
#include "php_ini.h"

#include "php_zabbix.h"

#include "hfs.h"

char *progname = "test";
char title_message[] = "Title";
char usage_message[] = "Usage";
char *help_message[] = { "Help", 0 };

static zend_function_entry php_zabbix_functions[] = {
	PHP_FE(zabbix_hfs_read, NULL)
	{NULL, NULL, NULL}
};

zend_module_entry zabbix_module_entry = {
	STANDARD_MODULE_HEADER,
	"zabbix", 
 	php_zabbix_functions, 
	NULL, 
	NULL, 
	NULL,
	NULL, 
	PHP_MINFO(zabbix), 
	NO_VERSION_YET,
	NULL,
	NULL,
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};

//#ifdef COMPILE_DL_zabbix
ZEND_GET_MODULE(zabbix)
//#endif

PHP_MINFO_FUNCTION(zabbix)
{
	php_info_print_table_start();
	php_info_print_table_row(1, "Zabbix Support", "enabled");
	php_info_print_table_end();
}

/* {{{ proto array zabbix_hfs_read(int itemid, int from_time, int to_time) */
PHP_FUNCTION(zabbix_hfs_read)
{
	size_t n;
	hfs_item_value_t *res = NULL;
	int i, itemid = 0;
	time_t from_time_sec = 0, to_time_sec = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lll", &itemid, &from_time_sec, &to_time_sec) == FAILURE)
		RETURN_FALSE;
/*
	if (from_time_sec <= 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "From_time has to be greater than 0");
		RETURN_FALSE;
	}

	if (to_time_sec <= 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "To_time has to be greater than 0");
		RETURN_FALSE;
	}
*/
	array_init(return_value);

	n = HFSread_item("/tmp/hfs", itemid, from_time_sec, to_time_sec, &res);
//	php_error_docref(NULL TSRMLS_CC, E_WARNING, "n=%d", n);

	for (i = 0; i < n; i++)
		add_assoc_long (return_value, res[i].ts, res[i].value);

	if (res)
		free(res);
}
