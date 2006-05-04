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
#include "zend_exceptions.h"
#include <stdarg.h>

/* ******************
   * Driver Methods *
   ****************** */

#define PHP_PDO_USER_DRV_CLOSER				"pdo_close"
#define PHP_PDO_USER_DRV_PREPARER			"pdo_prepare"
#define PHP_PDO_USER_DRV_DOER				"pdo_do"
#define PHP_PDO_USER_DRV_QUOTER				"pdo_quote"
#define PHP_PDO_USER_DRV_BEGIN				"pdo_begin"
#define PHP_PDO_USER_DRV_COMMIT				"pdo_commit"
#define PHP_PDO_USER_DRV_ROLLBACK			"pdo_rollback"
#define PHP_PDO_USER_DRV_SET_ATTRIBUTE		"pdo_setattribute"
#define PHP_PDO_USER_DRV_LAST_INSERT_ID		"pdo_lastinsertid"
#define PHP_PDO_USER_DRV_FETCH_ERROR_FUNC	"pdo_fetcherror"
#define PHP_PDO_USER_DRV_GET_ATTRIBUTE		"pdo_getattribute"
#define PHP_PDO_USER_DRV_CHECK_LIVENESS		"pdo_checkliveness"

static int php_pdo_user_closer(pdo_dbh_t *dbh TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)dbh->driver_data;

	if (data) {
		zval fname, retval;

		ZVAL_STRINGL(&fname, PHP_PDO_USER_DRV_CLOSER, sizeof(PHP_PDO_USER_DRV_CLOSER) - 1, 0);
		if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 0, NULL TSRMLS_CC) == SUCCESS) {
			/* Ignore retval */
			zval_dtor(&retval);
		}

		php_pdo_user_ptrmap_unmap(data TSRMLS_CC);

		zval_ptr_dtor(&(data->object));
		efree(data);

		dbh->driver_data = NULL;
	}

	return 0;
}

static int php_pdo_user_preparer(pdo_dbh_t *dbh, const char *sql, long sql_len, pdo_stmt_t *stmt, zval *driver_options TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)dbh->driver_data;
	php_pdo_user_data *stmtdata = NULL;
	zval *args[2], fname, *retval;

	stmt->supports_placeholders = PDO_PLACEHOLDER_NONE;
	ZVAL_STRINGL(&fname, PHP_PDO_USER_DRV_PREPARER, sizeof(PHP_PDO_USER_DRV_PREPARER) - 1, 0);

	MAKE_STD_ZVAL(args[0]);
	ZVAL_STRINGL(args[0], sql, sql_len, 1);
	args[1] = driver_options ? driver_options : EG(uninitialized_zval_ptr);

	ALLOC_INIT_ZVAL(retval);
	if (call_user_function(EG(function_table), &data->object, &fname, retval, 2, args TSRMLS_CC) == FAILURE) {
		zval_ptr_dtor(&args[0]);
		zval_ptr_dtor(&retval);
		return 0; /* FAILURE */
	}
	zval_ptr_dtor(&args[0]);

	if (Z_TYPE_P(retval) != IS_OBJECT ||
		!php_pdo_user_implements_statement(Z_OBJCE_P(retval))) {
		/* TODO: Throw exception */
		zval_ptr_dtor(&retval);

		return 0; /* FAILURE */
	}

	stmtdata = emalloc(sizeof(php_pdo_user_data));
	stmtdata->object = retval;
	stmtdata->dbh = dbh;
	stmtdata->stmt = stmt;
	php_pdo_user_ptrmap_map(stmtdata TSRMLS_CC);

	stmt->driver_data = stmtdata;
	stmt->methods = &php_pdo_user_stmt_methods;

	/* TODO: Allow a way to expose native prepared statements */

	return 1; /* SUCCESS */
}

static long php_pdo_user_doer(pdo_dbh_t *dbh, const char *sql, long sql_len TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)dbh->driver_data;
	zval fname, *zsql, retval;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_DRV_DOER, sizeof(PHP_PDO_USER_DRV_DOER) - 1, 0);

	MAKE_STD_ZVAL(zsql);
	ZVAL_STRINGL(zsql, (char*)sql, sql_len, 1);

	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 1, &zsql TSRMLS_CC) == SUCCESS) {
		if (Z_TYPE(retval) == IS_NULL ||
			(Z_TYPE(retval) == IS_BOOL && Z_BVAL(retval) == 0)) {
			/* Convert NULL or FALSE to an error condition */
			ZVAL_LONG(&retval, -1);
		} else {
			/* Anything else is just a number of rows affected */
	 		convert_to_long(&retval);
		}
	} else {
		ZVAL_LONG(&retval, -1);
	}
	zval_ptr_dtor(&zsql);

	if (Z_LVAL(retval) < 0) {
		Z_LVAL(retval) = -1;
	}

	return Z_LVAL(retval);
}

static int php_pdo_user_quoter(pdo_dbh_t *dbh, const char *unquoted, int unquotedlen, char **quoted, int *quotedlen, enum pdo_param_type paramtype  TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)dbh->driver_data;
 	zval fname, *zunq, retval;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_DRV_QUOTER, sizeof(PHP_PDO_USER_DRV_QUOTER) - 1, 0);

	MAKE_STD_ZVAL(zunq);
	ZVAL_STRINGL(zunq, (char*)unquoted, unquotedlen, 1);

	/* TODO: paramtype */

	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 1, &zunq TSRMLS_CC) == SUCCESS) {
 		convert_to_string(&retval);
	} else {
		ZVAL_STRINGL(&retval, "", 0, 1);
	}

	zval_ptr_dtor(&zunq);

	*quoted = Z_STRVAL(retval);
	*quotedlen = Z_STRLEN(retval);

	return 1;
}

static int php_pdo_user_begin(pdo_dbh_t *dbh TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)dbh->driver_data;
	zval fname, retval;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_DRV_BEGIN, sizeof(PHP_PDO_USER_DRV_BEGIN) - 1, 0);
	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 0, NULL TSRMLS_CC) == SUCCESS) {
		convert_to_boolean(&retval);
	} else {
		ZVAL_FALSE(&retval);
	}

	return Z_BVAL(retval);
}

static int php_pdo_user_commit(pdo_dbh_t *dbh TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)dbh->driver_data;
	zval fname, retval;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_DRV_COMMIT, sizeof(PHP_PDO_USER_DRV_COMMIT) - 1, 0);
	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 0, NULL TSRMLS_CC) == SUCCESS) {
		convert_to_boolean(&retval);
	} else {
		ZVAL_FALSE(&retval);
	}

	return Z_BVAL(retval);
}

static int php_pdo_user_rollback(pdo_dbh_t *dbh TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)dbh->driver_data;
	zval fname, retval;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_DRV_ROLLBACK, sizeof(PHP_PDO_USER_DRV_ROLLBACK) - 1, 0);
	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 0, NULL TSRMLS_CC) == SUCCESS) {
		convert_to_boolean(&retval);
	} else {
		ZVAL_FALSE(&retval);
	}

	return Z_BVAL(retval);
}

static int php_pdo_user_set_attribute(pdo_dbh_t *dbh, long attr, zval *val TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)dbh->driver_data;
 	zval fname, *zargs[2], retval;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_DRV_SET_ATTRIBUTE, sizeof(PHP_PDO_USER_DRV_SET_ATTRIBUTE) - 1, 0);

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

static char *php_pdo_user_last_insert_id(pdo_dbh_t *dbh, const char *name, unsigned int *len TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)dbh->driver_data;
 	zval fname, *zname, retval;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_DRV_LAST_INSERT_ID, sizeof(PHP_PDO_USER_DRV_LAST_INSERT_ID) - 1, 0);

	MAKE_STD_ZVAL(zname);
	ZVAL_STRING(zname, (char*)name, 1);

	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 1, &zname TSRMLS_CC) == SUCCESS) {
 		convert_to_string(&retval);
	} else {
		ZVAL_STRINGL(&retval, "", 0, 1);
	}

	*len = Z_STRLEN(retval);
	return Z_STRVAL(retval);
}

static int php_pdo_user_fetch_error_func(pdo_dbh_t *dbh, pdo_stmt_t *stmt, zval *info TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)dbh->driver_data;
 	zval fname, retval;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_DRV_FETCH_ERROR_FUNC, sizeof(PHP_PDO_USER_DRV_FETCH_ERROR_FUNC) - 1, 0);

	/* TODO: Include statement object (if it exists) */

	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 0, NULL TSRMLS_CC) == SUCCESS) {
		if (Z_TYPE(retval) == IS_ARRAY) {
			zval **tmpzval;

			if (zend_hash_index_find(Z_ARRVAL(retval), 0, (void**)&tmpzval) == SUCCESS) {
				zval *copyval;

				MAKE_STD_ZVAL(copyval);
				*copyval = **tmpzval;
				INIT_PZVAL(copyval);
				zval_copy_ctor(copyval);
				convert_to_long(copyval);
				add_next_index_zval(info, copyval);
			} else {
				add_next_index_long(info, 0);
			}

			if (zend_hash_index_find(Z_ARRVAL(retval), 0, (void**)&tmpzval) == SUCCESS) {
				zval *copyval;

				MAKE_STD_ZVAL(copyval);
				*copyval = **tmpzval;
				INIT_PZVAL(copyval);
				zval_copy_ctor(copyval);
				convert_to_string(copyval);
				add_next_index_zval(info, copyval);
			} else {
				add_next_index_string(info, "", 1);
			}
		} else {
			add_next_index_long(info, 0);
			add_next_index_string(info, "", 1);
		}
		zval_dtor(&retval);
	} else {
		add_next_index_long(info, 0);
		add_next_index_string(info, "", 1);
	}

	return 1;
}

static int php_pdo_user_get_attribute(pdo_dbh_t *dbh, long attr, zval *return_value TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)dbh->driver_data;
 	zval fname, *zattr;
	int ret = 1; /* SUCCESS */

	ZVAL_STRINGL(&fname, PHP_PDO_USER_DRV_GET_ATTRIBUTE, sizeof(PHP_PDO_USER_DRV_GET_ATTRIBUTE) - 1, 0);

	MAKE_STD_ZVAL(zattr);
	ZVAL_LONG(zattr, attr);

	if (call_user_function(EG(function_table), &(data->object), &fname, return_value, 1, &zattr TSRMLS_CC) == FAILURE) {
		ZVAL_NULL(return_value);
		ret = 0; /* FAILURE */
	}

	zval_ptr_dtor(&zattr);

	return ret;
}

static int php_pdo_user_check_liveness(pdo_dbh_t *dbh TSRMLS_DC)
{
	php_pdo_user_data *data = (php_pdo_user_data*)dbh->driver_data;
	zval fname, retval;

	ZVAL_STRINGL(&fname, PHP_PDO_USER_DRV_CHECK_LIVENESS, sizeof(PHP_PDO_USER_DRV_CHECK_LIVENESS) - 1, 0);
	if (call_user_function(EG(function_table), &(data->object), &fname, &retval, 0, NULL TSRMLS_CC) == SUCCESS) {
		convert_to_boolean(&retval);
	} else {
		ZVAL_FALSE(&retval);
	}

	return Z_BVAL(retval);
}

static struct pdo_dbh_methods php_pdo_user_drv_methods = {
	php_pdo_user_closer,
	php_pdo_user_preparer,
	php_pdo_user_doer,
	php_pdo_user_quoter,
	php_pdo_user_begin,
	php_pdo_user_commit,
	php_pdo_user_rollback,
	php_pdo_user_set_attribute,
	php_pdo_user_last_insert_id,
	php_pdo_user_fetch_error_func,
	php_pdo_user_get_attribute,
	php_pdo_user_check_liveness,
};

/* ***********
   * FACTORY *
   *********** */

static void php_pdo_factory_error(int errcode TSRMLS_DC, const char *format, ...)
{
	va_list args;
	char *message;

	va_start(args, format);
	vspprintf(&message, 0, format, args);
	zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "SQLSTATE[HY000] [%d] %s", errcode, message);
	va_end(args);
	efree(message);
}

/* {{{ pdo_user_driver_factory */
static int pdo_user_driver_factory(pdo_dbh_t *dbh, zval *driver_options TSRMLS_DC)
{
	zend_class_entry *ce;
	php_pdo_user_data *data = NULL;
	zval *object = NULL;
	zend_function *constructor;
	struct pdo_data_src_parser vars[] = {
		{ "driver", NULL, 0 }
	};

	if (driver_options && Z_TYPE_P(driver_options) == IS_ARRAY) {
		zval **tmp;
		if (zend_hash_index_find(Z_ARRVAL_P(driver_options), PDO_ATTR_PERSISTENT, (void**)&tmp) == SUCCESS &&
			zval_is_true(*tmp)) {
			php_pdo_factory_error(-5 TSRMLS_CC, "PDO_User driver may not be used persistently");
			return 0;
		}
	}

	php_pdo_parse_data_source(dbh->data_source, dbh->data_source_len, vars, 1);

	if (!vars[0].optval) {
		php_pdo_factory_error(-1 TSRMLS_CC, "No driver class specified.");
		return 0; /* FAILURE */
	}

	ce = zend_fetch_class(vars[0].optval, strlen(vars[0].optval), ZEND_FETCH_CLASS_AUTO TSRMLS_CC);

	if (!ce) {
		php_pdo_factory_error(-2 TSRMLS_CC, "Class %s not found.", vars[0].optval);
		efree(vars[0].optval);
		return 0; /* FAILURE */
	}

	if (!php_pdo_user_implements_driver(ce)) {
		php_pdo_factory_error(-3 TSRMLS_CC, "Class %s does not implement PDO_User_Driver interface.", vars[0].optval);
		efree(vars[0].optval);
		return 0; /* FAILURE */
	}

	ALLOC_INIT_ZVAL(object);
	if (object_init_ex(object, ce) == FAILURE) {
		php_pdo_factory_error(-4 TSRMLS_CC, "Failure instantiating class %s", vars[0].optval);
		efree(vars[0].optval);
		zval_ptr_dtor(&object);
		return 0; /* FAILURE */
	}

	efree(vars[0].optval);

	data = emalloc(sizeof(php_pdo_user_data));
	data->object = object;
	data->dbh = dbh;
	data->stmt = NULL;
	php_pdo_user_ptrmap_map(data TSRMLS_CC);

	dbh->driver_data = data;
	dbh->alloc_own_columns = 1;
	dbh->max_escaped_char_length = 2;
	dbh->methods = &php_pdo_user_drv_methods;

	constructor = Z_OBJ_HT_P(object)->get_constructor(object TSRMLS_CC);
	if (constructor) {
		zval fname, retval, *args[4];

		/* Lazy, but also less problematic */
		ZVAL_STRING(&fname, constructor->common.function_name, 0);

		/* $dsn */
		MAKE_STD_ZVAL(args[0]);
		ZVAL_STRINGL(args[0], dbh->data_source, dbh->data_source_len, 1);

		/* $user */
		MAKE_STD_ZVAL(args[1]);
		ZVAL_STRING(args[1], dbh->username ? dbh->username : "", 1);

		/* $pass */
		MAKE_STD_ZVAL(args[2]);
		ZVAL_STRING(args[2], dbh->password ? dbh->password : "", 1);

		/* $params */
		args[3] = driver_options ? driver_options : EG(uninitialized_zval_ptr);

		if (SUCCESS == call_user_function(EG(function_table), &object, &fname, &retval, 4, args TSRMLS_CC)) {
			zval_dtor(&retval);
		}

		zval_ptr_dtor(&args[0]);
		zval_ptr_dtor(&args[1]);
		zval_ptr_dtor(&args[2]);
	}

	return 1; /* SUCCESS */
}
/* }}} */

/* {{{ pdo_user_driver */
pdo_driver_t pdo_user_driver = {
	PDO_DRIVER_HEADER(user),
	pdo_user_driver_factory
};
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
