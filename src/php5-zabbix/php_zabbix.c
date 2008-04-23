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

/* {{{ add_next_index_object
 */
static inline int add_next_index_object(zval *arg, zval *tmp TSRMLS_DC)
{
	HashTable *symtable;

	if (Z_TYPE_P(arg) == IS_OBJECT) {
		symtable = Z_OBJPROP_P(arg);
	} else {
		symtable = Z_ARRVAL_P(arg);
	}

	return zend_hash_next_index_insert(symtable, (void *) &tmp, sizeof(zval *), NULL); 
}
/* }}} */

/* {{{ proto array zabbix_hfs_read(int itemid int sizex, int from_time, int to_time) */
PHP_FUNCTION(zabbix_hfs_read)
{
	size_t n = 0;
	zval *z_obj;
	hfs_item_value_t *res = NULL;
	int i, sizex, itemid = 0;
	time_t from_time_sec = 0, to_time_sec = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llll", &sizex, &itemid, &from_time_sec, &to_time_sec) == FAILURE)
		RETURN_FALSE;

        if (array_init(return_value) == FAILURE)
		RETURN_FALSE;

	n = HFSread_item(ZABBIX_GLOBAL(hfs_base_dir), sizex, itemid, from_time_sec, to_time_sec, &res);

	for (i = 0; i < n; i++) {
		MAKE_STD_ZVAL(z_obj);
		object_init(z_obj);

		add_property_long (z_obj, "itemid",	itemid);
		add_property_long (z_obj, "count",	1);
		add_property_long (z_obj, "clock",	res[i].clock);
		add_property_long (z_obj, "i",		res[i].group);

		if (res[i].type == IT_DOUBLE) {
			add_property_double (z_obj, "avg", res[i].avg.d);
			add_property_double (z_obj, "max", res[i].max.d);
			add_property_double (z_obj, "min", res[i].min.d);
		}
		else {
			add_property_long   (z_obj, "avg", res[i].avg.l);
			add_property_long   (z_obj, "max", res[i].max.l);
			add_property_long   (z_obj, "min", res[i].min.l);
		}

		add_next_index_object(return_value, z_obj TSRMLS_CC);
	}
	if (res) free(res);
}
