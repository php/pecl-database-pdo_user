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

#ifndef PHP_PDO_USER_INT_H
#define PHP_PDO_USER_INT_H

#include "php_pdo_user.h"
#include "pdo/php_pdo.h"
#include "pdo/php_pdo_driver.h"

typedef struct _php_pdo_user_llist php_pdo_user_llist;

ZEND_BEGIN_MODULE_GLOBALS(pdo_user)
	php_pdo_user_llist *ptr_map;
ZEND_END_MODULE_GLOBALS(pdo_user)

extern ZEND_DECLARE_MODULE_GLOBALS(pdo_user);

#ifdef ZTS
#define		PDO_USER_G(v)		TSRMG(pdo_user_globals_id, zend_pdo_user_globals *, v)
#else
#define		PDO_USER_G(v)		(pdo_user_globals.v)
#endif

typedef struct _php_pdo_user_data {
	zval *object;
	pdo_dbh_t *dbh;
	pdo_stmt_t *stmt;
} php_pdo_user_data;

int php_pdo_user_ptrmap_map(php_pdo_user_data *data TSRMLS_DC);
int php_pdo_user_ptrmap_unmap(php_pdo_user_data *data TSRMLS_DC);
void *php_pdo_user_ptrmap_locate(zval *object TSRMLS_DC);
void php_pdo_user_ptrmap_destroy(TSRMLS_D);
PHP_MINIT_FUNCTION(php_pdo_user_class);
int php_pdo_user_implements_driver(zend_class_entry *ce);
int php_pdo_user_implements_statement(zend_class_entry *ce);

extern struct pdo_stmt_methods php_pdo_user_stmt_methods;

#endif	/* PHP_PDO_USER_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
