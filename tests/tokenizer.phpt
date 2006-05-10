--TEST--
SQL Tokenizer
--SKIPIF--
<?php
	if (!extension_loaded('pdo_user')) {
		die('pdo_user not loaded');
	}
?>
--FILE--
<?php

$sql = array(
		'SELECT foo FROM bar',
		'INSERT INTO bar (foo,baz) values(1,2),(2,3)',
		'ALTER TABLE bar insert column bling int not null default "moo" after foo',
		'SELECT foo FROM bar WHERE `select` = `\x41BC \06123`',
	);
foreach($sql as $s) {
	var_dump($s);
	$t = PDO_User::tokenizeSQL($s);
	foreach($t as $token) {
		printf("%- 20s %- 20s %d\n", PDO_User::tokenName($token['token']), $token['data'], strlen($token['data']));
	}
}
--EXPECT--
string(19) "SELECT foo FROM bar"
PU_SELECT            SELECT               6
PU_LABEL             foo                  3
PU_FROM              FROM                 4
PU_LABEL             bar                  3
string(43) "INSERT INTO bar (foo,baz) values(1,2),(2,3)"
PU_INSERT            INSERT               6
PU_INTO              INTO                 4
PU_LABEL             bar                  3
PU_LPAREN            (                    1
PU_LABEL             foo                  3
PU_COMMA             ,                    1
PU_LABEL             baz                  3
PU_RPAREN            )                    1
PU_VALUES            values               6
PU_LPAREN            (                    1
PU_LNUM              1                    1
PU_COMMA             ,                    1
PU_LNUM              2                    1
PU_RPAREN            )                    1
PU_COMMA             ,                    1
PU_LPAREN            (                    1
PU_LNUM              2                    1
PU_COMMA             ,                    1
PU_LNUM              3                    1
PU_RPAREN            )                    1
string(72) "ALTER TABLE bar insert column bling int not null default "moo" after foo"
PU_ALTER             ALTER                5
PU_TABLE             TABLE                5
PU_LABEL             bar                  3
PU_INSERT            insert               6
PU_COLUMN            column               6
PU_LABEL             bling                5
PU_INT               int                  3
PU_NOT               not                  3
PU_NULL              null                 4
PU_DEFAULT           default              7
PU_STRING            moo                  3
PU_AFTER             after                5
PU_LABEL             foo                  3
string(52) "SELECT foo FROM bar WHERE `select` = `\x41BC \06123`"
PU_SELECT            SELECT               6
PU_LABEL             foo                  3
PU_FROM              FROM                 4
PU_LABEL             bar                  3
PU_WHERE             WHERE                5
PU_LABEL             select               6
PU_EQUALS            =                    1
PU_LABEL             ABC 123              7
