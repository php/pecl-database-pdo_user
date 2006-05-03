/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2006 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sara Golemon <pollita@php.net>                               |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php_pdo_user_int.h"

/* Even if the userspace describe method doesn't return a proper array
 * we still need to populate the column metadata with *something*
 * Generate a name using this prefix and a numeric (colno) suffix */
#define PHP_PDO_USER_STMT_UNKNOWN_COL_PREFIX		"column_"

#define PHP_PDO_USER_STMT_CLOSER			"pdo_close"
#define PHP_PDO_USER_STMT_EXECUTE			"pdo_execute"
#define PHP_PDO_USER_STMT_FETCH				"pdo_fetch"
#define PHP_PDO_USER_STMT_DESCRIBE			"pdo_describe"
#define PHP_PDO_USER_STMT_GET_COL			"pdo_getcol"
#define PHP_PDO_USER_STMT_PARAM_HOOK		"pdo_paramhook"
#define PHP_PDO_USER_STMT_SET_ATTR			"pdo_setattribute"
#define PHP_PDO_USER_STMT_GET_ATTR			"pdo_getattribute"
#define PHP_PDO_USER_STMT_COL_META			"pdo_colmeta"
#define PHP_PDO_USER_STMT_NEXT_ROWSET		"pdo_nextrowset"
#define PHP_PDO_USER_STMT_CURSOR_CLOSER		"pdo_closecursor"

static inline long php_pdo_user_long_from_assoc(HashTable *ht, char *key, size_t key_len)
{
	long ret = 0;
	zval **tmp;

	if (zend_hash_find(ht, key, key_len, (void**)&tmp) == SUCCESS) {
		switch (Z_TYPE_PP(tmp)) {
			case IS_NULL: ret = 0; break;
			case IS_BOOL: ret = Z_BVAL_PP(tmp) ? 1 : 0; break;
			case IS_LONG: ret = Z_LVAL_PP(tmp); break;
			case IS_DOUBLE: ret = (long)Z_DVAL_PP(tmp); break;
			default:
			{
				zval copyval = **tmp;
				zval_copy_ctor(&copyval);
				convert_to_long(&copyval);
				ret = Z_LVAL(copyval);
			}
		}
	}

	return ret;
}

static inline char *php_pdo_user_string_from_assoc(HashTable *ht, char *key, size_t key_len, int *len)
{
	zval **tmp;

	if (zend_hash_find(ht, key, key_len, (void**)&tmp) == SUCCESS) {
		if (Z_TYPE_PP(tmp) == IS_STRING) {
			*len = Z_STRLEN_PP(tmp);
			return estrndup(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
		} else {
			zval copyval = **tmp;
			zval_copy_ctor(&copyval);
			convert_to_string(&copyval);
			*len = Z_STRLEN(copyval);
			return Z_STRVAL(copyval);
		}
	}

	*len = 0;
	return estrndup("", 0);
}

static int php_pdo_user_stmt_dtor(pdo_stmt_t *stmt TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)stmt->driver_data;

	if (data) {
		zval fname, retval;

		ZVAL_STRINGL(&fname, PHP_PDO_USER_STMT_CLOSER, sizeof(PHP_PDO_USER_STMT_CLOSER) - 1, 0);
		if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 0, NULL TSRMLS_CC) == SUCCESS) {
			/* Ignore retval */
			zval_dtor(&retval);
		}

		php_pdo_user_ptrmap_unmap(data TSRMLS_CC);

		zval_ptr_dtor(&(data->object));
		efree(data);

		stmt->driver_data = NULL;
	}

	return 0;
}

static int php_pdo_user_stmt_execute(pdo_stmt_t *stmt TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)stmt->driver_data;
	zval fname, retval;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_STMT_EXECUTE, sizeof(PHP_PDO_USER_STMT_EXECUTE) - 1, 0);

	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 0, NULL TSRMLS_CC) == FAILURE) {
		return 0; /* FAILURE */
	}

	if (Z_TYPE(retval) != IS_ARRAY) {
		/* throw an exception */
		zval_dtor(&retval);
		return 0; /* FAILURE */
	}

	stmt->row_count    = php_pdo_user_long_from_assoc(Z_ARRVAL(retval), "rows", sizeof("rows"));
	stmt->column_count = php_pdo_user_long_from_assoc(Z_ARRVAL(retval), "cols", sizeof("cols"));

	zval_dtor(&retval);

	if (!stmt->column_count) {
		/* throw an exception */
		stmt->row_count = 0;
		return 0; /* FAILURE */
	}

	return 1; /* SUCCESS */
}

static int php_pdo_user_stmt_fetch(pdo_stmt_t *stmt, enum pdo_fetch_orientation ori, long offset TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)stmt->driver_data;
	zval fname, retval, *args[2];

	ZVAL_STRINGL(&fname, PHP_PDO_USER_STMT_FETCH, sizeof(PHP_PDO_USER_STMT_FETCH) - 1, 0);

	MAKE_STD_ZVAL(args[0]);
	ZVAL_LONG(args[0], ori);

	MAKE_STD_ZVAL(args[1]);
	ZVAL_LONG(args[1], offset);

	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 2, args TSRMLS_CC) == FAILURE) {
		zval_ptr_dtor(&args[0]);
		zval_ptr_dtor(&args[1]);
		return 0; /* FAILURE */
	}
	zval_ptr_dtor(&args[0]);
	zval_ptr_dtor(&args[1]);

	convert_to_boolean(&retval);

	return Z_BVAL(retval);
}

static int php_pdo_user_stmt_describe(pdo_stmt_t *stmt, int colno TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)stmt->driver_data;
	zval fname, retval, *zcolno;
	int len;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_STMT_DESCRIBE, sizeof(PHP_PDO_USER_STMT_DESCRIBE) - 1, 0);

	MAKE_STD_ZVAL(zcolno);
	ZVAL_LONG(zcolno, colno);

	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 1, &zcolno TSRMLS_CC) == FAILURE) {
		zval_ptr_dtor(&zcolno);
		return 0;
	}
	zval_ptr_dtor(&zcolno);

	if (Z_TYPE(retval) != IS_ARRAY) {
		/* throw an exception */
		zval_dtor(&retval);

		stmt->columns[colno].namelen = spprintf(&stmt->columns[colno].name, 0, PHP_PDO_USER_STMT_UNKNOWN_COL_PREFIX "%d", colno);
		stmt->columns[colno].maxlen = 0xffffffff;
		stmt->columns[colno].precision = 0;
		stmt->columns[colno].param_type = PDO_PARAM_STR;
		return 0;
	}

	stmt->columns[colno].name = php_pdo_user_string_from_assoc(Z_ARRVAL(retval), "name", sizeof("name"), &len);
	stmt->columns[colno].namelen = len;
	stmt->columns[colno].maxlen = php_pdo_user_long_from_assoc(Z_ARRVAL(retval), "maxlen", sizeof("maxlen"));
	stmt->columns[colno].precision = php_pdo_user_long_from_assoc(Z_ARRVAL(retval), "precision", sizeof("precision"));
	stmt->columns[colno].param_type = PDO_PARAM_STR;

	zval_dtor(&retval);

	return 1; /* SUCCESS */
}

static int php_pdo_user_stmt_get_col(pdo_stmt_t *stmt, int colno, char **ptr, unsigned long *len, int *caller_frees TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)stmt->driver_data;
	zval fname, retval, *zcolno;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_STMT_GET_COL, sizeof(PHP_PDO_USER_STMT_GET_COL) - 1, 0);

	MAKE_STD_ZVAL(zcolno);
	ZVAL_LONG(zcolno, colno);

	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 1, &zcolno TSRMLS_CC) == FAILURE) {
		zval_ptr_dtor(&zcolno);
		return 0;
	}
	zval_ptr_dtor(&zcolno);

	convert_to_string(&retval);
	*ptr = Z_STRVAL(retval);
	*len = Z_STRLEN(retval);
	*caller_frees = 1;

	return 1; /* SUCCESS */
}

static int php_pdo_user_stmt_param_hook(pdo_stmt_t *stmt, struct pdo_bound_param_data *param, enum pdo_param_event event_type TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)stmt->driver_data;
	zval fname, *args[5], retval;

	switch(event_type) {
		case PDO_PARAM_EVT_EXEC_PRE:
		case PDO_PARAM_EVT_EXEC_POST:
		case PDO_PARAM_EVT_FETCH_PRE:
		case PDO_PARAM_EVT_FETCH_POST:
			break;
		default:
			return 1; /* Don't handle other methods */
	}

	ZVAL_STRINGL(&fname, PHP_PDO_USER_STMT_PARAM_HOOK, sizeof(PHP_PDO_USER_STMT_PARAM_HOOK) - 1, 0);

	/* $event, $num, $name, $is_param, &$param */
	MAKE_STD_ZVAL(args[0]);
	ZVAL_LONG(args[0], event_type);

	MAKE_STD_ZVAL(args[1]);
	ZVAL_LONG(args[1], param->paramno);

	ALLOC_INIT_ZVAL(args[2]);
	if (param->name) {
		ZVAL_STRINGL(args[2], param->name, param->namelen, 1);
	}

	MAKE_STD_ZVAL(args[3]);
	ZVAL_BOOL(args[3], param->is_param);

	args[4] = param->parameter;

	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 5, args TSRMLS_CC) == FAILURE) {
		zval_ptr_dtor(&args[0]);
		zval_ptr_dtor(&args[1]);
		zval_ptr_dtor(&args[2]);
		zval_ptr_dtor(&args[3]);
		return 1; /* Soft failure */
	}
	zval_ptr_dtor(&args[0]);
	zval_ptr_dtor(&args[1]);
	zval_ptr_dtor(&args[2]);
	zval_ptr_dtor(&args[3]);
	zval_dtor(&retval); /* Ignore */

	return 1; /* SUCCESS */
}

static int php_pdo_user_stmt_set_attr(pdo_stmt_t *stmt, long attr, zval *val TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)stmt->driver_data;
	zval fname, *zargs[2], retval;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_STMT_SET_ATTR, sizeof(PHP_PDO_USER_STMT_SET_ATTR) - 1, 0);

	MAKE_STD_ZVAL(zargs[0]);
	ZVAL_LONG(zargs[0], attr);

	zargs[1] = val;

	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 2, zargs TSRMLS_CC) == SUCCESS) {
		convert_to_boolean(&retval);
    } else {
		ZVAL_FALSE(&retval);
    }

	zval_ptr_dtor(&zargs[0]);

	return Z_BVAL(retval);
}

static int php_pdo_user_stmt_get_attr(pdo_stmt_t *stmt, long attr, zval *val TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)stmt->driver_data;
	zval fname, *zattr;
	int ret = 1; /* SUCCESS */

	ZVAL_STRINGL(&fname, PHP_PDO_USER_STMT_GET_ATTR, sizeof(PHP_PDO_USER_STMT_GET_ATTR) - 1, 0);

	MAKE_STD_ZVAL(zattr);
	ZVAL_LONG(zattr, attr);

	if (call_user_function(EG(function_table), &(data->object), &fname, val, 1, &zattr TSRMLS_CC) == FAILURE) {
		ZVAL_NULL(val);
		ret = 0; /* FAILURE */
	}

	zval_ptr_dtor(&zattr);

	return ret;
}

static int php_pdo_user_stmt_col_meta(pdo_stmt_t *stmt, long colno, zval *return_value TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)stmt->driver_data;
	zval fname, *zcolno;
	int ret = 1; /* SUCCESS */

	ZVAL_STRINGL(&fname, PHP_PDO_USER_STMT_COL_META, sizeof(PHP_PDO_USER_STMT_COL_META) - 1, 0);

	MAKE_STD_ZVAL(zcolno);
	ZVAL_LONG(zcolno, colno);

	if (call_user_function(EG(function_table), &(data->object), &fname, return_value, 1, &zcolno TSRMLS_CC) == FAILURE) {
		ZVAL_NULL(return_value);
		ret = 0; /* FAILURE */
	}

	zval_ptr_dtor(&zcolno);

	return ret;
}

static int php_pdo_user_stmt_next_rowset(pdo_stmt_t *stmt TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)stmt->driver_data;
	zval fname, retval;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_STMT_NEXT_ROWSET, sizeof(PHP_PDO_USER_STMT_NEXT_ROWSET) - 1, 0);

	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 0, NULL TSRMLS_CC) == FAILURE) {
		return 0; /* FAILURE */
	}
	convert_to_boolean(&retval);

	return Z_BVAL(retval);
}

static int php_pdo_user_stmt_cursor_closer(pdo_stmt_t *stmt TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)stmt->driver_data;
	zval fname, retval;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_STMT_CURSOR_CLOSER, sizeof(PHP_PDO_USER_STMT_CURSOR_CLOSER) - 1, 0);

	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 0, NULL TSRMLS_CC) == FAILURE) {
		return 0; /* FAILURE */
	}
	convert_to_boolean(&retval);

	return Z_BVAL(retval);
}

struct pdo_stmt_methods php_pdo_user_stmt_methods = {
	php_pdo_user_stmt_dtor,
	php_pdo_user_stmt_execute,
	php_pdo_user_stmt_fetch,
	php_pdo_user_stmt_describe,
	php_pdo_user_stmt_get_col,
	php_pdo_user_stmt_param_hook,
	php_pdo_user_stmt_set_attr,
	php_pdo_user_stmt_get_attr,
	php_pdo_user_stmt_col_meta,
	php_pdo_user_stmt_next_rowset,
	php_pdo_user_stmt_cursor_closer
};

