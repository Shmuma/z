#define _GNU_SOURCE
#include <stdio.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_zabbix.h"
#include "hfs.h"

ZEND_DECLARE_MODULE_GLOBALS(zabbix)

static zend_function_entry php_zabbix_functions[] = {
	PHP_FE(zabbix_hfs_read, NULL)
	{NULL, NULL, NULL}
};

zend_module_entry zabbix_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"zabbix", 
 	php_zabbix_functions, 
	PHP_MINIT(zabbix),
	PHP_MSHUTDOWN(zabbix),
	NULL,
	NULL,
	PHP_MINFO(zabbix),
	"0.1.0",
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_ZABBIX
ZEND_GET_MODULE(zabbix)
#endif


PHP_INI_BEGIN()
STD_PHP_INI_ENTRY("zabbix.hfs_base_dir", "/tmp/hfs", PHP_INI_ALL, OnUpdateString,
                  hfs_base_dir, zend_zabbix_globals, zabbix_globals)
PHP_INI_END()

char *progname = "test";
char title_message[] = "Title";
char usage_message[] = "Usage";
char *help_message[] = { "Help", 0 };

PHP_MINIT_FUNCTION(zabbix)
{
	REGISTER_INI_ENTRIES();
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(zabbix)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

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

	array_init(return_value);

	n = HFSread_item(ZABBIX_GLOBAL(hfs_base_dir), itemid, from_time_sec, to_time_sec, &res);

	for (i = 0; i < n; i++) {
		char *key = NULL;
		asprintf(&key, "%d", res[i].ts);

		if (res[i].type == IT_UINT64) {
			add_assoc_long   (return_value, key, res[i].value);
		}
		else if (res[i].type == IT_DOUBLE) {
			add_assoc_double (return_value, key, res[i].value);
		}

		free(key);
	}
	if (res) free(res);
}
