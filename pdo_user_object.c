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
#include "php_pdo_user_sql.h"

/* ******************
   * PDO_User Class *
   ****************** */

/* PDO_User::CONSTANTS */
#define PHP_PDO_USER_DRIVER_PARAM_MAX_ESCAPSED_CHAR_LENGTH		0x00000001
#define PHP_PDO_USER_DRIVER_PARAM_DATA_SOURCE					0x00000002
#define PHP_PDO_USER_DRIVER_PARAM_SQLSTATE						0x00000003

#define PHP_PDO_USER_STATEMENT_PARAM_ACTIVE_QUERY				0x00010000
#define PHP_PDO_USER_STATEMENT_PARAM_SQLSTATE					0x00010003

/* {{{ mixed PDO_User::driverParam(object dbh, integer param[, mixed value])
- aka  mixed PDO_User::statementParam(object stmt, integer param[, mixed value])
Get/Set a PDO option on a driver or statement handle
Passign value == NULL does a get-only, without changing the original value
Returns original value (or NULL on failure) */
PHP_METHOD(pdo_user, driverparam)
{
	php_pdo_user_data *data;
	zval *zobj;
	long param;
	zval *val = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ol|z", &zobj, &param, &val) == FAILURE) {
		return;
	}

	data = php_pdo_user_ptrmap_locate(zobj TSRMLS_CC);
	if (!data) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Parameter 1 must be an active PDO_User driver or statement object");
		RETURN_NULL();
	}

	switch (param) {
		case PHP_PDO_USER_DRIVER_PARAM_MAX_ESCAPSED_CHAR_LENGTH:
		{
			zval *longval = val, tmpval;

			RETVAL_LONG(data->dbh->max_escaped_char_length);

			if (longval) {
				/* Set a new value */

				if (Z_TYPE_P(longval) != IS_LONG) {
					tmpval = *longval;
					zval_copy_ctor(&tmpval);
					convert_to_long(&tmpval);
					longval = &tmpval;
				}
				if (Z_LVAL_P(longval) < 1) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "max_escaped_character_length must be a positive integer");
					RETVAL_NULL();
				} else {
					data->dbh->max_escaped_char_length = Z_LVAL_P(longval);
				}
			}

			break;
		}
		case PHP_PDO_USER_DRIVER_PARAM_DATA_SOURCE:
		{
			if (val) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot override data source");
			}
			RETVAL_STRINGL(data->dbh->data_source, data->dbh->data_source_len, 1);

			break;
		}
		case PHP_PDO_USER_DRIVER_PARAM_SQLSTATE:
		{
			RETVAL_STRING(data->dbh->error_code, 1);
			if (val) {
				zval *strval = val, tmpval;

				if (Z_TYPE_P(strval) != IS_STRING) {
					tmpval = *strval;
					zval_copy_ctor(&tmpval);
					convert_to_string(&tmpval);
					strval = &tmpval;
				}

				strcpy(data->dbh->error_code, "     ");
				memcpy(data->dbh->error_code, Z_STRVAL_P(strval), Z_STRLEN_P(strval) > 5 ? 5 : Z_STRLEN_P(strval));

				if (strval == &tmpval) {
					zval_dtor(&tmpval);
				}
			}

			break;
		}
		/* Statement params (don't forget to check for data->stmt) */
		case PHP_PDO_USER_STATEMENT_PARAM_ACTIVE_QUERY:
		{
			if (val) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot override data source");
			}
			if (!data->stmt) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot return statement params from a driver object");
				break;
			}

			if (data->stmt->active_query_string) {
				RETVAL_STRINGL(data->stmt->active_query_string, data->stmt->active_query_stringlen, 1);
			} else {
				RETVAL_NULL();
			}

			break;
		}
		case PHP_PDO_USER_STATEMENT_PARAM_SQLSTATE:
		{
			if (!data->stmt) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot return statement params from a driver object");
				break;
			}

			RETVAL_STRING(data->stmt->error_code, 1);
			if (val) {
				zval *strval = val, tmpval;

				if (Z_TYPE_P(strval) != IS_STRING) {
					tmpval = *strval;
					zval_copy_ctor(&tmpval);
					convert_to_string(&tmpval);
					strval = &tmpval;
				}

				strcpy(data->stmt->error_code, "     ");
				memcpy(data->stmt->error_code, Z_STRVAL_P(strval), Z_STRLEN_P(strval) > 5 ? 5 : Z_STRLEN_P(strval));

				if (strval == &tmpval) {
					zval_dtor(&tmpval);
				}
			}

			break;
		}
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown parameter (%ld)", param);
	}
}
/* }}} */

/* {{{ array PDO_User::parsedsn(string dsn, array params)
Parse a DSN string ( var1=val;var2=val;var3=val )
Into an associative array */
PHP_METHOD(pdo_user,parsedsn)
{
    char *dsn;
    int dsn_len, num_opts, i;
    zval *opts, **opt = NULL;
    struct pdo_data_src_parser *vars;
    HashPosition pos;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa", &dsn, &dsn_len, &opts) == FAILURE) {
        return;
    }

    array_init(return_value);

    if (!dsn_len || !(num_opts = zend_hash_num_elements(Z_ARRVAL_P(opts)))) {
        return;
    }

    vars = safe_emalloc(num_opts, sizeof(struct pdo_data_src_parser), 0);
    for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(opts), &pos), num_opts =0;
        zend_hash_get_current_data_ex(Z_ARRVAL_P(opts), (void**)&opt, &pos) == SUCCESS;
        zend_hash_move_forward_ex(Z_ARRVAL_P(opts), &pos)) {
        zval copyval = **opt;

        /* Some data gets copied here that doesn't need to be (typically all of it I'd think)
         * But it's more convenient[lazy] than tracking what does... */
        zval_copy_ctor(&copyval);
        convert_to_string(&copyval);
        vars[num_opts].optname = Z_STRVAL(copyval);
        vars[num_opts].optval = NULL;
        vars[num_opts].freeme = 0;
        num_opts++;
    }

    php_pdo_parse_data_source(dsn, dsn_len, vars, num_opts);

    for(i = 0; i < num_opts; i++) {
        if (vars[i].optval) {
            add_assoc_string(return_value, (char*)vars[i].optname, vars[i].optval, !vars[i].freeme);
        } else {
            add_assoc_null(return_value, (char*)vars[i].optname);
        }
        /* This makes that unnecessary copying hurt twice as much...
         * Must refactor this to track dups and only do them when actually needed */
        efree((void*)vars[i].optname);
    }
    efree(vars);
}
/* }}} */

/* {{{ proto array PDO_User::tokenizeSQL(string sql[, bool include_whitespace)
Break apart a SQL statement into tokens */
PHP_METHOD(pdo_user,tokenizesql)
{
	char *sql;
	int sql_len;
	zend_bool include_whitespace = 0;
	php_pdo_user_sql_tokenizer T;
	php_pdo_user_sql_token token;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b", &sql, &sql_len, &include_whitespace) == FAILURE) {
		return;
	}

	T.start = sql;
	T.end = sql + sql_len;

	array_init(return_value);
	while (PU_END != php_pdo_user_sql_get_token(&T, &token)) {
		zval *tok;

		if (token.id == PU_WHITESPACE && !include_whitespace) {
			continue;
		}
		MAKE_STD_ZVAL(tok);
		array_init(tok);
		add_assoc_long(tok, "token", token.id);
		add_assoc_stringl(tok, "data", token.token, token.token_len, !token.freeme);
		add_next_index_zval(return_value, tok);
	}
}
/* }}} */

static zend_class_entry *php_pdo_user_ce;
static zend_class_entry *php_pdo_user_driver_interface;
static zend_class_entry *php_pdo_user_statement_interface;

static zend_function_entry php_pdo_user_class_functions[] = {
	PHP_ME(pdo_user,		driverparam,						NULL,	ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_MALIAS(pdo_user,	statementparam,		driverparam,	NULL,	ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(pdo_user,		parsedsn,							NULL,	ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(pdo_user,		tokenizesql,						NULL,	ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	{ NULL, NULL, NULL }
};

static zend_function_entry php_pdo_user_driver_interface_functions[] = {
	PHP_ABSTRACT_ME(pdo_user_driver,			__construct,			NULL)
	PHP_ABSTRACT_ME(pdo_user_driver,			pdo_close,				NULL)
	PHP_ABSTRACT_ME(pdo_user_driver,			pdo_prepare,			NULL)
	PHP_ABSTRACT_ME(pdo_user_driver,			pdo_do,					NULL)
	PHP_ABSTRACT_ME(pdo_user_driver,			pdo_quote,				NULL)
	PHP_ABSTRACT_ME(pdo_user_driver,			pdo_begin,				NULL)
	PHP_ABSTRACT_ME(pdo_user_driver,			pdo_commit,				NULL)
	PHP_ABSTRACT_ME(pdo_user_driver,			pdo_rollback,			NULL)
	PHP_ABSTRACT_ME(pdo_user_driver,			pdo_setattribute,		NULL)
	PHP_ABSTRACT_ME(pdo_user_driver,			pdo_lastinsertid,		NULL)
	PHP_ABSTRACT_ME(pdo_user_driver,			pdo_fetcherror,			NULL)
	PHP_ABSTRACT_ME(pdo_user_driver,			pdo_getattribute,		NULL)
	PHP_ABSTRACT_ME(pdo_user_driver,			pdo_checkliveness,		NULL)
	{ NULL, NULL, NULL }
};

static zend_function_entry php_pdo_user_statement_interface_functions[] = {
	PHP_ABSTRACT_ME(pdo_user_statement,			pdo_close,				NULL)
	PHP_ABSTRACT_ME(pdo_user_statement,			pdo_execute,			NULL)
	PHP_ABSTRACT_ME(pdo_user_statement,			pdo_fetch,				NULL)
	PHP_ABSTRACT_ME(pdo_user_statement,			pdo_describe,			NULL)
	PHP_ABSTRACT_ME(pdo_user_statement,			pdo_getcol,				NULL)
	PHP_ABSTRACT_ME(pdo_user_statement,			pdo_paramhook,			NULL)
	PHP_ABSTRACT_ME(pdo_user_statement,			pdo_setattribute,		NULL)
	PHP_ABSTRACT_ME(pdo_user_statement,			pdo_getattribute,		NULL)
	PHP_ABSTRACT_ME(pdo_user_statement,			pdo_colmeta,			NULL)
	PHP_ABSTRACT_ME(pdo_user_statement,			pdo_nextrowset,			NULL)
	PHP_ABSTRACT_ME(pdo_user_statement,			pdo_closecursor,		NULL)
	{ NULL, NULL, NULL }
};

static inline void _php_pdo_user_declare_long_constant(zend_class_entry *ce, const char *const_name, size_t name_len, long value TSRMLS_DC)
{
#if PHP_MAJOR_VERSION > 5 || PHP_MINOR_VERSION >= 1
	zend_declare_class_constant_long(ce, (char*)const_name, name_len, value TSRMLS_CC);
#else
	zval *constant = malloc(sizeof(*constant));
	ZVAL_LONG(constant, value);
    INIT_PZVAL(constant);
    zend_hash_update(&ce->constants_table, (char*)const_name, name_len+1, &constant, sizeof(zval*), NULL);
#endif
}

#define PHP_PDO_USER_DECLARE_LONG_CONSTANT(cnst) \
	_php_pdo_user_declare_long_constant(php_pdo_user_ce, #cnst , sizeof( #cnst ) - 1, PHP_PDO_USER_##cnst TSRMLS_CC)
#define PHP_PDO_USER_DECLARE_TOKEN_CONSTANT(lbl) \
	_php_pdo_user_declare_long_constant(php_pdo_user_ce, (lbl)->label, strlen((lbl)->label), (lbl)->id TSRMLS_CC)

PHP_MINIT_FUNCTION(php_pdo_user_class)
{
	php_pdo_user_sql_token_label *labels = php_pdo_user_sql_token_labels;
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "PDO_User", php_pdo_user_class_functions);
	php_pdo_user_ce = zend_register_internal_class(&ce TSRMLS_CC);
	php_pdo_user_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS; /* Prevents instantiation */

	PHP_PDO_USER_DECLARE_LONG_CONSTANT(DRIVER_PARAM_MAX_ESCAPSED_CHAR_LENGTH);
	PHP_PDO_USER_DECLARE_LONG_CONSTANT(DRIVER_PARAM_DATA_SOURCE);
	PHP_PDO_USER_DECLARE_LONG_CONSTANT(DRIVER_PARAM_SQLSTATE);

	PHP_PDO_USER_DECLARE_LONG_CONSTANT(STATEMENT_PARAM_ACTIVE_QUERY);
	PHP_PDO_USER_DECLARE_LONG_CONSTANT(STATEMENT_PARAM_SQLSTATE);

	while (labels->label) {
		PHP_PDO_USER_DECLARE_TOKEN_CONSTANT(labels);
		labels++;
	}	

	INIT_CLASS_ENTRY(ce, "PDO_User_Driver", php_pdo_user_driver_interface_functions);
	php_pdo_user_driver_interface = zend_register_internal_interface(&ce TSRMLS_CC);

	INIT_CLASS_ENTRY(ce, "PDO_User_Statement", php_pdo_user_statement_interface_functions);
	php_pdo_user_statement_interface = zend_register_internal_interface(&ce TSRMLS_CC);

	return SUCCESS;	
}

int php_pdo_user_implements_driver(zend_class_entry *ce)
{
	int i;

	if (!ce) {
		return 0;
	}

	for(i = 0; i < ce->num_interfaces; i++) {
		if (ce->interfaces[i] == php_pdo_user_driver_interface) {
			return 1;
		}
	}

	return 0;
}

int php_pdo_user_implements_statement(zend_class_entry *ce)
{
	int i;

	if (!ce) {
		return 0;
	}

	for(i = 0; i < ce->num_interfaces; i++) {
		if (ce->interfaces[i] == php_pdo_user_statement_interface) {
			return 1;
		}
	}

	return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
