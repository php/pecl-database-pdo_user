dnl
dnl $Id$
dnl

if test "$PHP_PDO" != "no"; then

PHP_ARG_ENABLE(pdo-user, for Userdriver support for PDO,
[  --enable-pdo-user         PDO: Userdriver support.])

if test "$PHP_PDO_USER" != "no"; then
  AC_DEFINE(HAVE_PDO_USER, 1, [Whether you have Userdriver Support])

  ifdef([PHP_CHECK_PDO_INCLUDES],
  [
    PHP_CHECK_PDO_INCLUDES
  ],[
    AC_MSG_CHECKING([for PDO includes])
    if test -f $abs_srcdir/include/php/ext/pdo/php_pdo_driver.h; then
      pdo_inc_path=$abs_srcdir/ext
    elif test -f $abs_srcdir/ext/pdo/php_pdo_driver.h; then
      pdo_inc_path=$abs_srcdir/ext
    elif test -f $prefix/include/php/ext/pdo/php_pdo_driver.h; then
      pdo_inc_path=$prefix/include/php/ext
    else
      AC_MSG_ERROR([Cannot find php_pdo_driver.h.])
    fi
    AC_MSG_RESULT($pdo_inc_path)
  ])

  PHP_NEW_EXTENSION(pdo_user, pdo_user.c pdo_user_driver.c pdo_user_statement.c pdo_user_object.c \
pdo_user_sql_tokenizer.c pdo_user_sql_parser.c, $ext_shared,,-I$pdo_inc_path)

  PHP_ADD_MAKEFILE_FRAGMENT

  ifdef([PHP_ADD_EXTENSION_DEP],
  [
    PHP_ADD_EXTENSION_DEP(pdo_user, pdo)
  ])

  PHP_SUBST(PDO_USER_SHARED_LIBADD)
fi

fi
dnl vim: se ts=2 sw=2 et:
