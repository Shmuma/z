#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_zabbix.h"
#include "hfs.h"

#define LOG_LEVEL_DEBUG 4
#define __zbx_zbx_snprintf snprintf

ZEND_DECLARE_MODULE_GLOBALS(zabbix)

static zend_function_entry php_zabbix_functions[] = {
	PHP_FE(zabbix_hfs_read_history, NULL)
	PHP_FE(zabbix_hfs_read, NULL)
	PHP_FE(zabbix_hfs_read_trends, NULL)
	PHP_FE(zabbix_hfs_last, NULL)
	PHP_FE(zabbix_hfs_read_str, NULL)
	PHP_FE(zabbix_hfs_last_str, NULL)
	PHP_FE(zabbix_hfs_hosts_availability, NULL)
	PHP_FE(zabbix_hfs_update_item_status, NULL)
	PHP_FE(zabbix_hfs_item_status, NULL)
	PHP_FE(zabbix_hfs_item_values, NULL)
	PHP_FE(zabbix_hfs_triggers_values, NULL)
	PHP_FE(zabbix_hfs_trigger_value, NULL)
	PHP_FE(zabbix_hfs_trigger_events, NULL)
	PHP_FE(zabbix_hfs_host_events, NULL)
	PHP_FE(zabbix_hfs_clear_item_history, NULL)
	PHP_FE(zabbix_hfs_get_alerts, NULL)
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



struct item {
	item_type_t	type;
	time_t		clock;
	item_value_u	value;
};

struct items_array {
	size_t count;
	struct item *items;
};



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

/* {{{ proto array zabbix_hfs_read_history(char *site, int itemid, int sizex, int graph_from_time, int graph_to_time, int from_time, int to_time) */
PHP_FUNCTION(zabbix_hfs_read_history)
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

	n = HFSread_item(ZABBIX_GLOBAL(hfs_base_dir), site,
			    0,		itemid,
			    sizex,
			    graph_from, graph_to,
			    from,	to,
			    &res);

	for (i = 0; i < n; i++) {
		char *buf = NULL;

		MAKE_STD_ZVAL(z_obj);
		object_init(z_obj);

		add_property_long (z_obj, "itemid",	itemid);
		add_property_long (z_obj, "count",	res[i].count);
		add_property_long (z_obj, "clock",	res[i].clock);
		add_property_long (z_obj, "i",		res[i].group);

		add_property_double(z_obj, "avg", res[i].value.avg.d);
		add_property_double(z_obj, "max", res[i].value.max.d);
		add_property_double(z_obj, "min", res[i].value.min.d);

		add_next_index_object(return_value, z_obj TSRMLS_CC);
	}
	if (res) free(res);
}
/* }}} */

/* {{{ proto array zabbix_hfs_read_trends(char *site, int itemid, int sizex, int graph_from_time, int graph_to_time, int from_time, int to_time) */
PHP_FUNCTION(zabbix_hfs_read_trends)
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

	n = HFSread_item(ZABBIX_GLOBAL(hfs_base_dir), site,
			    1,		itemid,
			    sizex,
			    graph_from, graph_to,
			    from,	to,
			    &res);

	for (i = 0; i < n; i++) {
		char *buf = NULL;

		MAKE_STD_ZVAL(z_obj);
		object_init(z_obj);

		add_property_long (z_obj, "itemid",	itemid);
		add_property_long (z_obj, "count",	res[i].count);
		add_property_long (z_obj, "clock",	res[i].clock);
		add_property_long (z_obj, "i",		res[i].group);

		add_property_double(z_obj, "avg", res[i].value.avg.d);
		add_property_double(z_obj, "max", res[i].value.max.d);
		add_property_double(z_obj, "min", res[i].value.min.d);

		add_next_index_object(return_value, z_obj TSRMLS_CC);
	}
	if (res) free(res);
}
/* }}} */



/* read_count_fn_t */
void
hfs_interval_functor (item_type_t type, item_value_u val, hfs_time_t timestamp, void *ptr)
{
	struct items_array *res = ptr;

	res->items = (struct item*)realloc (res->items, sizeof (struct item) * (res->count+1));

	struct item *elem = (res->items + res->count);

	elem->type  = type;
	elem->clock = timestamp;
	elem->value = val;
	res->count++;
}


/* obtains all values of specified interval */
/* {{{ proto array zabbix_hfs_read(char *site, int itemid, int from_time, int to_time) */
PHP_FUNCTION(zabbix_hfs_read)
{
	size_t n = 0;
	zval *z_obj;
	char *site = NULL;
	int site_len = 0;
	struct items_array res;
	int i;
	time_t from = 0, to = 0;
	long long itemid = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slll", &site, &site_len, &itemid, &from, &to) == FAILURE)
		RETURN_FALSE;

        if (array_init(return_value) == FAILURE)
		RETURN_FALSE;

	res.count = 0;
	res.items = NULL;
	if (HFSread_interval(ZABBIX_GLOBAL(hfs_base_dir), site, itemid, from, to, &res, hfs_interval_functor) <= 0)
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

	if (res.items)
		free(res.items);
}
/* }}} */


/* {{{ proto array zabbix_hfs_read_str(char *site, int itemid, int from_time, int to_time) */
PHP_FUNCTION(zabbix_hfs_read_str)
{
	size_t n = 0;
	zval *z_obj;
	char *site = NULL;
	int site_len = 0;
	hfs_item_str_value_t *res = NULL;
	int i;
	time_t from = 0, to = 0;
	long long itemid = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slll", &site, &site_len, &itemid, &from, &to) == FAILURE)
		RETURN_FALSE;

        if (array_init(return_value) == FAILURE)
		RETURN_FALSE;

	n = HFSread_item_str(ZABBIX_GLOBAL(hfs_base_dir), site, itemid, from, to, &res);

	for (i = 0; i < n; i++) {
		char *buf = NULL;
		zval* val;

		MAKE_STD_ZVAL(z_obj);
		MAKE_STD_ZVAL(val);

		object_init(z_obj);

		add_property_long (z_obj, "itemid",	itemid);
		add_property_long (z_obj, "clock",	res[i].clock);

		if (res[i].value) {
			ZVAL_STRING (val, res[i].value, 1);
		}
		else {
			ZVAL_EMPTY_STRING (val);
		}

		add_property_zval (z_obj, "value", val);
		add_next_index_object(return_value, z_obj TSRMLS_CC);
		if (res[i].value)
			free (res[i].value);
	}

	if (res)
		free(res);
}
/* }}} */



/* read_count_fn_t */
void
hfs_last_functor (item_type_t type, item_value_u val, hfs_time_t timestamp, void *ptr)
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

	if (HFSread_count(ZABBIX_GLOBAL(hfs_base_dir), site, itemid, count, &res, hfs_last_functor) < 0)
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


/* {{{ proto array zabbix_hfs_last_str(char *site, int itemid, int count) */
PHP_FUNCTION(zabbix_hfs_last_str)
{
	size_t n = 0;
	zval *z_obj;
	char *site = NULL;
	int site_len = 0;
	hfs_item_str_value_t *res = NULL;
	int i;
	long long count = 0;
	long long itemid = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sll", &site, &site_len, &itemid, &count) == FAILURE)
		RETURN_FALSE;

        if (array_init(return_value) == FAILURE)
		RETURN_FALSE;

	n = HFSread_count_str(ZABBIX_GLOBAL(hfs_base_dir), site, itemid, count, &res);

	for (i = 0; i < n; i++) {
		char *buf = NULL;
		zval* val;

		MAKE_STD_ZVAL(z_obj);
		MAKE_STD_ZVAL(val);

		object_init(z_obj);

		add_property_long (z_obj, "itemid",	itemid);
		add_property_long (z_obj, "clock",	res[i].clock);

		if (res[i].value) {
			ZVAL_STRING (val, res[i].value, 1);
		}
		else {
			ZVAL_EMPTY_STRING (val);
		}

		add_property_zval (z_obj, "value", val);
		add_next_index_object(return_value, z_obj TSRMLS_CC);
		if (res[i].value)
			free (res[i].value);
	}

	if (res)
		free(res);
}
/* }}} */



/* {{{ proto array zabbix_hfs_hosts_availability(char *site) */
PHP_FUNCTION(zabbix_hfs_hosts_availability)
{
	char *site = NULL;
	int site_len = 0, count, i;
	hfs_time_t clock;
	char* error = NULL;
	hfs_host_status_t* statuses;
	char buf[100];

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &site, &site_len) == FAILURE)
		RETURN_FALSE;

	count = HFS_get_hosts_statuses (ZABBIX_GLOBAL(hfs_base_dir), site, &statuses);

        if (array_init(return_value) == FAILURE)
		RETURN_FALSE;

	for (i = 0; i < count; i++) {
		zval *z_obj;

		MAKE_STD_ZVAL(z_obj);
		object_init(z_obj);

		zbx_snprintf (buf, sizeof (buf), "%lld", statuses[i].hostid);

		add_property_long (z_obj, "last", statuses[i].clock);
		add_property_long (z_obj, "available", statuses[i].available);
		add_assoc_zval (return_value, buf, z_obj);
	}

	if (count)
		free (statuses);
}
/* }}} */


/* {{{ proto null zabbix_hfs_update_item_status(char *site, int itemid, int status) */
PHP_FUNCTION(zabbix_hfs_update_item_status)
{
	long long itemid = 0, status;
	char *site = NULL;
	int site_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sll", &site, &site_len, &itemid, &status) == FAILURE)
		RETURN_FALSE;

        HFS_update_item_status (ZABBIX_GLOBAL(hfs_base_dir), site, itemid, status, NULL);
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



/* returns array with fields 'lastclock', 'prevvalue', 'lastvalue' and 'stderr' */
/* {{{ proto array zabbix_hfs_item_values(char *site, int itemid, int type) */
PHP_FUNCTION(zabbix_hfs_item_values)
{
	long long itemid = 0;
	char *site = NULL;
	int site_len = 0, type;
	hfs_time_t lastclock, nextcheck;
	char* buf = NULL;

	double d_prev, d_last, d_prevorg;
	unsigned long long i_prev, i_last, i_prevorg;
	char *s_prev, *s_last, *s_prevorg, *s_stderr;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sll", &site, &site_len, &itemid, &type) == FAILURE)
		RETURN_FALSE;

        if (array_init(return_value) == FAILURE)
		RETURN_FALSE;

	switch (type) {
	case ITEM_VALUE_TYPE_FLOAT:
		if (!HFS_get_item_values_dbl (ZABBIX_GLOBAL(hfs_base_dir), site, itemid, &lastclock, &nextcheck, &d_prev, &d_last, &d_prevorg, &s_stderr))
			RETURN_FALSE;
		add_assoc_double (return_value, "prevvalue", d_prev);
		add_assoc_double (return_value, "lastvalue", d_last);
		break;

	case ITEM_VALUE_TYPE_TEXT:
	case ITEM_VALUE_TYPE_STR:
		if (!HFS_get_item_values_str (ZABBIX_GLOBAL(hfs_base_dir), site, itemid, &lastclock, &nextcheck, &s_prev, &s_last, &s_prevorg, &s_stderr))
			RETURN_FALSE;
		if (s_prev) {
			add_assoc_string (return_value, "prevvalue", s_prev, 1);
			free (s_prev);
		}
                else
			add_assoc_string (return_value, "prevvalue", "", 1);
		if (s_last) {
			add_assoc_string (return_value, "lastvalue", s_last, 1);
			free (s_last);
		}
                else
			add_assoc_string (return_value, "lastvalue", "", 1);
		if (s_prevorg)
			free (s_prevorg);
		break;

	case ITEM_VALUE_TYPE_UINT64:
		if (!HFS_get_item_values_int (ZABBIX_GLOBAL(hfs_base_dir), site, itemid, &lastclock, &nextcheck, &i_prev, &i_last, &i_prevorg, &s_stderr))
			RETURN_FALSE;

		asprintf(&buf, "%lld", i_prev);
		add_assoc_string (return_value, "prevvalue", buf, 1);
		free (buf);

		asprintf(&buf, "%lld", i_last);
		add_assoc_string (return_value, "lastvalue", buf, 1);
		free (buf);
		break;

	default:
		RETURN_FALSE;
	}

	add_assoc_long (return_value, "lastclock", lastclock);
	if (s_stderr) {
		add_assoc_string (return_value, "stderr", s_stderr, 1);
		free (s_stderr);
	}
	else
		add_assoc_string (return_value, "stderr", "", 1);
}
/* }}} */


/* {{{ proto array zabbix_hfs_triggers_values(char *site) */
PHP_FUNCTION(zabbix_hfs_triggers_values)
{
	char *site = NULL;
	int site_len = 0, count, i;
        hfs_trigger_value_t* values;
        char buf[100];

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &site, &site_len) == FAILURE)
		RETURN_FALSE;

	count = HFS_get_triggers_values (ZABBIX_GLOBAL(hfs_base_dir), site, &values);

        if (array_init(return_value) == FAILURE)
		RETURN_FALSE;

        for (i = 0; i < count; i++) {
            zval *z_obj;

            MAKE_STD_ZVAL (z_obj);
            object_init (z_obj);

            zbx_snprintf (buf, sizeof (buf), "%lld", values[i].triggerid);

            add_property_long (z_obj, "when", values[i].when);
            add_property_long (z_obj, "value", values[i].value);
            add_assoc_zval (return_value, buf, z_obj);
        }

        if (count)
            free (values);
}
/* }}} */


/* {{{ proto object zabbix_hfs_trigger_value(char *site, int triggerid) */
PHP_FUNCTION(zabbix_hfs_trigger_value)
{
	long long triggerid = 0;
	char *site = NULL;
	int site_len = 0, value;
	hfs_time_t when;
        hfs_trigger_value_t val;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &site, &site_len, &triggerid) == FAILURE)
		RETURN_FALSE;

	if (!HFS_get_trigger_value (ZABBIX_GLOBAL(hfs_base_dir), site, triggerid, &val))
		RETURN_FALSE;

        if (object_init(return_value) == FAILURE)
		RETURN_FALSE;

	add_property_long (return_value, "value", val.value);
	add_property_long (return_value, "when", val.when);
}
/* }}} */


/* {{{ proto array zabbix_hfs_trigger_events(char *site, int triggerid, int count) */
PHP_FUNCTION(zabbix_hfs_trigger_events)
{
	int i, count;
	long long triggerid = 0;
	char *site = NULL;
	int site_len = 0, res_count;
	hfs_event_value_t* res;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sll", &site, &site_len, &triggerid, &count) == FAILURE)
		RETURN_FALSE;

        if (array_init(return_value) == FAILURE)
		RETURN_FALSE;

	res_count = HFS_get_trigger_events (ZABBIX_GLOBAL(hfs_base_dir), site, triggerid, count, &res);

	if (res_count) {
		for (i = 0; i < res_count; i++) {
			zval *z_obj;

			MAKE_STD_ZVAL(z_obj);
			object_init(z_obj);

			add_property_long(z_obj, "clock", res[i].clock);
			add_property_long(z_obj, "val", res[i].val);
			add_property_long(z_obj, "ack", res[i].ack);
			add_next_index_object(return_value, z_obj TSRMLS_CC);
		}
		free (res);
	}
}
/* }}} */



/* {{{ proto array zabbix_hfs_host_events(char *site, int hostid, int skip, int count) */
PHP_FUNCTION(zabbix_hfs_host_events)
{
	int i, count, skip;
	long long hostid = 0;
	char *site = NULL;
	int site_len = 0, res_count;
	hfs_event_value_t* res;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slll", &site, &site_len, &hostid, &skip, &count) == FAILURE)
		RETURN_FALSE;

        if (array_init(return_value) == FAILURE)
		RETURN_FALSE;

	res_count = HFS_get_host_events (ZABBIX_GLOBAL(hfs_base_dir), site, hostid, skip, count, &res);

	if (res_count) {
		for (i = 0; i < res_count; i++) {
			zval *z_obj;

			MAKE_STD_ZVAL(z_obj);
			array_init (z_obj);

			add_assoc_long(z_obj, "triggerid", res[i].triggerid);
			add_assoc_long(z_obj, "clock", res[i].clock);
			add_assoc_long(z_obj, "value", res[i].val);
			add_assoc_long(z_obj, "ack", res[i].ack);
			add_next_index_object(return_value, z_obj TSRMLS_CC);
		}
		free (res);
	}
}
/* }}} */


/* {{{ proto bool zabbix_hfs_clear_item_history(char *site, int itemid) */
PHP_FUNCTION(zabbix_hfs_clear_item_history)
{
	long long itemid = 0;
	char *site = NULL;
	int site_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &site, &site_len, &itemid) == FAILURE)
		RETURN_FALSE;

	HFS_clear_item_history (ZABBIX_GLOBAL(hfs_base_dir), site, itemid);
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto array zabbix_hfs_get_alerts(char *site, int skip, int count) */
PHP_FUNCTION(zabbix_hfs_get_alerts)
{
	char *site = NULL;
	int site_len = 0;
	long long skip;
	int count, res_count, i;
	hfs_alert_value_t* alerts = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sll", &site, &site_len, &skip, &count) == FAILURE)
		RETURN_FALSE;

        if (array_init(return_value) == FAILURE)
		RETURN_FALSE;

	res_count = HFS_get_alerts (ZABBIX_GLOBAL(hfs_base_dir), site, skip, count, &alerts);

	for (i = 0; i < res_count; i++) {
		zval *z_obj;
		zval *z_str;

		MAKE_STD_ZVAL(z_obj);
		object_init(z_obj);

		add_property_long(z_obj, "clock", alerts[i].clock);
		add_property_long(z_obj, "actionid", alerts[i].actionid);
		add_property_long(z_obj, "userid", alerts[i].userid);
		add_property_long(z_obj, "triggerid", alerts[i].triggerid);
		add_property_long(z_obj, "mediatypeid", alerts[i].mediatypeid);
		add_property_long(z_obj, "status", alerts[i].status);
		add_property_long(z_obj, "retries", alerts[i].retries);

		MAKE_STD_ZVAL (z_str);
		if (alerts[i].sendto) {
			ZVAL_STRING (z_str, alerts[i].sendto, 1);
		}
		else {
			ZVAL_EMPTY_STRING (z_str);
		}
		add_property_zval (z_obj, "sendto", z_str);

		MAKE_STD_ZVAL (z_str);
		if (alerts[i].subject) {
			ZVAL_STRING (z_str, alerts[i].subject, 1);
		}
		else {
			ZVAL_EMPTY_STRING (z_str);
		}
		add_property_zval (z_obj, "subject", z_str);

		MAKE_STD_ZVAL (z_str);
		if (alerts[i].message) {
			ZVAL_STRING (z_str, alerts[i].message, 1);
		}
		else {
			ZVAL_EMPTY_STRING (z_str);
		}
		add_property_zval (z_obj, "message", z_str);

		add_next_index_object(return_value, z_obj TSRMLS_CC);

		if (alerts[i].sendto)
			free (alerts[i].sendto);
		if (alerts[i].subject)
			free (alerts[i].subject);
		if (alerts[i].message)
			free (alerts[i].message);
	}

	if (res_count && alerts)
		free (alerts);
}
/* }}} */

