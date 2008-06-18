#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_zabbix.h"
#include "hfs.h"

#define LOG_LEVEL_DEBUG 4
#define __zbx_zbx_snprintf snprinf

ZEND_DECLARE_MODULE_GLOBALS(zabbix)

static zend_function_entry php_zabbix_functions[] = {
	PHP_FE(zabbix_hfs_read, NULL)
	PHP_FE(zabbix_hfs_last, NULL)
	PHP_FE(zabbix_hfs_host_availability, NULL)
	PHP_FE(zabbix_hfs_item_status, NULL)
	PHP_FE(zabbix_hfs_item_stderr, NULL)
	PHP_FE(zabbix_hfs_item_values, NULL)
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
STD_PHP_INI_BOOLEAN("zabbix.debug",      "0",        PHP_INI_ALL, OnUpdateBool,
		  debug,        zend_zabbix_globals, zabbix_globals)
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

void
__zbx_zabbix_log(int level, const char *fmt, ...) {
	if (level == LOG_LEVEL_DEBUG && !ZABBIX_GLOBAL(debug))
		return;

	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "zabbix: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	fflush(stderr);
	va_end(ap);
}
#ifndef zabbix_log
#define zabbix_log __zbx_zabbix_log
#endif

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

/* {{{ proto array zabbix_hfs_read(char *site, int itemid, int sizex, int graph_from_time, int graph_to_time, int from_time, int to_time) */
PHP_FUNCTION(zabbix_hfs_read)
{
	size_t n = 0;
	zval *z_obj;
	char *site = NULL;
	int site_len = 0;
	hfs_item_value_t *res = NULL;
	int i;
	time_t graph_from = 0, graph_to = 0;
	time_t from = 0, to = 0;
	size_t sizex = 0;
	long long itemid = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sllllll", &site, &site_len, &sizex, &itemid, &graph_from, &graph_to, &from, &to) == FAILURE)
		RETURN_FALSE;

        if (array_init(return_value) == FAILURE)
		RETURN_FALSE;

	n = HFSread_item(ZABBIX_GLOBAL(hfs_base_dir), site, sizex, itemid, graph_from, graph_to, from, to, &res);

	for (i = 0; i < n; i++) {
		char *buf = NULL;
		zval *max, *min, *avg;
		MAKE_STD_ZVAL(max);
		MAKE_STD_ZVAL(min);
		MAKE_STD_ZVAL(avg);

		MAKE_STD_ZVAL(z_obj);
		object_init(z_obj);

		add_property_long (z_obj, "itemid",	itemid);
		add_property_long (z_obj, "count",	1);
		add_property_long (z_obj, "clock",	res[i].clock);
		add_property_long (z_obj, "i",		res[i].group);

		if (res[i].type == IT_DOUBLE) {
			ZVAL_DOUBLE(avg, res[i].avg.d);
			ZVAL_DOUBLE(max, res[i].max.d);
			ZVAL_DOUBLE(min, res[i].min.d);
		}
		else {
			asprintf(&buf, "%lld", res[i].avg.l);
			ZVAL_STRING(avg, buf, 1);
			free(buf);

			asprintf(&buf, "%lld", res[i].max.l);
			ZVAL_STRING(max, buf, 1);
			free(buf);

			asprintf(&buf, "%lld", res[i].min.l);
			ZVAL_STRING(min, buf, 1);
			free(buf);
		}

		add_property_zval(z_obj, "avg", avg);
		add_property_zval(z_obj, "max", max);
		add_property_zval(z_obj, "min", min);

		add_next_index_object(return_value, z_obj TSRMLS_CC);
	}
	if (res) free(res);
}
/* }}} */

struct item {
	item_type_t	type;
	time_t		clock;
	item_value_u	value;
};

struct items_array {
	size_t count;
	struct item *items;
};

read_count_fn_t
hfs_last_functor (item_type_t type, item_value_u val, time_t timestamp, void *ptr)
{
	struct items_array *res = ptr;
	struct item *elem = (res->items + res->count);

	elem->type  = type;
	elem->clock = timestamp;
	elem->value = val;
	res->count++;
}


/* {{{ proto array zabbix_hfs_last(char *site, int itemid, int count) */
PHP_FUNCTION(zabbix_hfs_last)
{
	int i, count;
	long long itemid = 0;
	char *site = NULL;
	int site_len = 0;
	struct items_array res;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sll", &site, &site_len, &itemid, &count) == FAILURE)
		RETURN_FALSE;

        if (array_init(return_value) == FAILURE)
		RETURN_FALSE;

	res.count = 0;

	if ((res.items = malloc(sizeof(struct item) * count)) == NULL)
		RETURN_FALSE;

	if (HFSread_count(ZABBIX_GLOBAL(hfs_base_dir), site, itemid, count, &res, hfs_last_functor) != 0)
		RETURN_FALSE;

	for (i = 0; i < res.count; i++) {
		char *buf = NULL;
		struct item *elem = (res.items + i);
		zval *z_obj, *value;

		MAKE_STD_ZVAL(value);
		MAKE_STD_ZVAL(z_obj);
		object_init(z_obj);

		if (elem->type == IT_DOUBLE) {
			ZVAL_DOUBLE(value, elem->value.d);
		}
		else {
			asprintf(&buf, "%lld", elem->value.l);
			ZVAL_STRING(value, buf, 1);
			free(buf);
		}

		add_property_long(z_obj, "clock", elem->clock);
		add_property_zval(z_obj, "value", value);
		add_next_index_object(return_value, z_obj TSRMLS_CC);
	}

	free(res.items);
}
/* }}} */


/* {{{ proto object zabbix_hfs_host_availability(char *site, int hostid) */
PHP_FUNCTION(zabbix_hfs_host_availability)
{
	long long hostid = 0;
	char *site = NULL;
	int site_len = 0, available, clock;
	char* error = NULL;
	zval* zerror;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &site, &site_len, &hostid) == FAILURE)
		RETURN_FALSE;

	if (!HFS_get_host_availability (ZABBIX_GLOBAL(hfs_base_dir), site, hostid, &available, &clock, &error))
		RETURN_FALSE;

        if (object_init(return_value) == FAILURE)
		RETURN_FALSE;

	MAKE_STD_ZVAL (zerror);

	if (error) {
		ZVAL_STRING (zerror, error, 1);
	}
	else {
		ZVAL_EMPTY_STRING (zerror);
	}

	add_property_long (return_value, "available", available);
	add_property_long (return_value, "clock", clock);
	add_property_zval (return_value, "error", zerror);

	if (error)
		free (error);
}
/* }}} */


/* {{{ proto object zabbix_hfs_item_status(char *site, int itemid) */
PHP_FUNCTION(zabbix_hfs_item_status)
{
	long long itemid = 0;
	char *site = NULL;
	int site_len = 0, status;
	char* error = NULL;
	zval* zerror;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &site, &site_len, &itemid) == FAILURE)
		RETURN_FALSE;

	if (!HFS_get_item_status (ZABBIX_GLOBAL(hfs_base_dir), site, itemid, &status, &error))
		RETURN_FALSE;

        if (object_init(return_value) == FAILURE)
		RETURN_FALSE;

	MAKE_STD_ZVAL (zerror);

	if (error) {
		ZVAL_STRING (zerror, error, 1);
	}
	else {
		ZVAL_EMPTY_STRING (zerror);
	}

	add_property_long (return_value, "status", status);
	add_property_zval (return_value, "error", zerror);

	if (error)
		free (error);
}
/* }}} */


/* {{{ proto object zabbix_hfs_item_stderr(char *site, int itemid) */
PHP_FUNCTION(zabbix_hfs_item_stderr)
{
	long long itemid = 0;
	char *site = NULL;
	int site_len = 0;
	char* error = NULL;
	zval* zerror;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &site, &site_len, &itemid) == FAILURE)
		RETURN_FALSE;

	if (!HFS_get_item_stderr (ZABBIX_GLOBAL(hfs_base_dir), site, itemid, &error))
		RETURN_FALSE;

        if (object_init(return_value) == FAILURE)
		RETURN_FALSE;

	MAKE_STD_ZVAL (zerror);

	if (error) {
		ZVAL_STRING (zerror, error, 1);
	}
	else {
		ZVAL_EMPTY_STRING (zerror);
	}

	add_property_zval (return_value, "stderr", zerror);

	if (error)
		free (error);
}
/* }}} */



/* returns array with fields 'lastclock', 'prevvalue' and 'lastvalue' */
/* {{{ proto array zabbix_hfs_item_values(char *site, int itemid, int type) */
PHP_FUNCTION(zabbix_hfs_item_values)
{
	long long itemid = 0;
	char *site = NULL;
	int site_len = 0, type;
	int lastclock, nextcheck;
	char* buf = NULL;

	double d_prev, d_last, d_prevorg;
	unsigned long long i_prev, i_last, i_prevorg;
	char *s_prev, *s_last, *s_prevorg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sll", &site, &site_len, &itemid, &type) == FAILURE)
		RETURN_FALSE;

        if (array_init(return_value) == FAILURE)
		RETURN_FALSE;

	switch (type) {
	case ITEM_VALUE_TYPE_FLOAT:
		if (!HFS_get_item_values_dbl (ZABBIX_GLOBAL(hfs_base_dir), site, itemid, &lastclock, &nextcheck, &d_prev, &d_last, &d_prevorg))
			RETURN_FALSE;
		add_assoc_double (return_value, "prevvalue", d_prev);
		add_assoc_double (return_value, "lastvalue", d_last);
		break;

	case ITEM_VALUE_TYPE_STR:
		if (!HFS_get_item_values_str (ZABBIX_GLOBAL(hfs_base_dir), site, itemid, &lastclock, &nextcheck, &s_prev, &s_last, &s_prevorg))
			RETURN_FALSE;
		if (s_prev)
			add_assoc_str (return_value, "prevvalue", s_prev);
		if (s_last)
			add_assoc_str (return_value, "lastvalue", s_last);
		if (s_prevorg)
			free (s_prevorg);
		break;

	case ITEM_VALUE_TYPE_UINT64:
		if (!HFS_get_item_values_int (ZABBIX_GLOBAL(hfs_base_dir), site, itemid, &lastclock, &nextcheck, &i_prev, &i_last, &i_prevorg))
			RETURN_FALSE;

		asprintf(&buf, "%lld", i_prev);
		add_assoc_str (return_value, "prevvalue", buf);

		asprintf(&buf, "%lld", i_last);
		add_assoc_str (return_value, "lastvalue", buf);
		break;

	default:
		RETURN_FALSE;
	}

	add_assoc_long (return_value, "lastclock", lastclock);
}
/* }}} */

