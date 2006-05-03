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
#include "ext/standard/info.h"

ZEND_DECLARE_MODULE_GLOBALS(pdo_user)

extern pdo_driver_t pdo_user_driver;

/* *********************************
   * PtrMap linked list management *
   ********************************* */

struct _php_pdo_user_llist {
	php_pdo_user_llist *next, *prev;
	php_pdo_user_data *data;
};

int php_pdo_user_ptrmap_map(php_pdo_user_data *data TSRMLS_DC)
{
	php_pdo_user_llist *el = emalloc(sizeof(php_pdo_user_llist));

	el->next = PDO_USER_G(ptr_map);
	el->prev = NULL;
	el->data = data;

	if (PDO_USER_G(ptr_map)) {
		PDO_USER_G(ptr_map)->prev = el;
	}

	PDO_USER_G(ptr_map) = el;

	return SUCCESS;
}

int php_pdo_user_ptrmap_unmap(php_pdo_user_data *data TSRMLS_DC)
{
	php_pdo_user_llist *el = PDO_USER_G(ptr_map);

	if (!el) {
		/* Nothing to unmap, not normal codepath...  */
		return FAILURE;
	}

	while (el) {
		if (el->data == data) {
			if (el->prev) {
				el->prev->next = el->next;
			} else {
				PDO_USER_G(ptr_map) = el->next;
			}
			if (el->next) {
				el->next->prev = el->prev;
			}
			efree(el);
			return SUCCESS;
		}
		el = el->next;
	}

	return FAILURE;
}

void *php_pdo_user_ptrmap_locate(zval *object TSRMLS_DC)
{
	php_pdo_user_llist *el = PDO_USER_G(ptr_map);

	if (!el) {
		/* Nothing to find */
		return NULL;
	}

	while (el) {
		if (el->data->object == object) {
			return el->data;
		}
		el = el->next;
	}

	return NULL;
}

void php_pdo_user_ptrmap_destroy(TSRMLS_D)
{
	php_pdo_user_llist *el = PDO_USER_G(ptr_map);

	while (el) {
		php_pdo_user_llist *next = el->next;

		efree(el);
		el = next;
	}

	PDO_USER_G(ptr_map) = NULL;
}

/* ***********************
   * Module Housekeeping *
   *********************** */

/* {{{ pdo_user_functions[] */
#if ZEND_MODULE_API_NO >= 20050922
static zend_module_dep pdo_user_deps[] = {
	ZEND_MOD_REQUIRED("pdo")
	{NULL, NULL, NULL}
};
#endif
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(pdo_user)
{
#ifdef ZTS
	ts_allocate_id(&pdo_user_globals_id, sizeof(zend_pdo_user_globals), NULL, NULL);
#endif

	if (FAILURE == PHP_MINIT(php_pdo_user_class)(INIT_FUNC_ARGS_PASSTHRU)) {
		return FAILURE;
	}

	return php_pdo_register_driver(&pdo_user_driver);
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(pdo_user)
{
	PDO_USER_G(ptr_map) = NULL;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(pdo_user)
{
	if (PDO_USER_G(ptr_map)) {
		php_pdo_user_ptrmap_destroy(TSRMLS_C);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(pdo_user)
{
	php_pdo_unregister_driver(&pdo_user_driver);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(pdo_user)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "PDO Driver for Userspace data sources", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ pdo_user_module_entry */
zend_module_entry pdo_user_module_entry = {
#if ZEND_MODULE_API_NO >= 20050922
	STANDARD_MODULE_HEADER_EX, NULL,
	pdo_user_deps,
#else
	STANDARD_MODULE_HEADER,
#endif
	PHP_PDO_USER_EXTNAME,
	NULL, /* procedural functions */
	PHP_MINIT(pdo_user),
	PHP_MSHUTDOWN(pdo_user),
	PHP_RINIT(pdo_user),
	PHP_RSHUTDOWN(pdo_user),
	PHP_MINFO(pdo_user),
	PHP_PDO_USER_EXTVER,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_PDO_USER
ZEND_GET_MODULE(pdo_user)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
