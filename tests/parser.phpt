--TEST--
SQL Parser
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
		'SELECT *,bar.*,db.bar.* FROM bar WHERE `select` = `\x41BC \06123`',
	);
foreach($sql as $s) {
	var_dump($s);
	print_r(PDO_User::parseSQL($s));
}
--EXPECT--
string(19) "SELECT foo FROM bar"
Array
(
    [type] => statement
    [statement] => select
    [fields] => Array
        (
            [0] => foo
        )

    [from] => bar
    [modifiers] => 
    [terminating-semicolon] => 
)
string(43) "INSERT INTO bar (foo,baz) values(1,2),(2,3)"
Array
(
    [type] => statement
    [statement] => insert
    [table] => bar
    [fields] => Array
        (
            [0] => foo
            [1] => baz
        )

    [data] => Array
        (
            [0] => Array
                (
                    [0] => 1
                    [1] => 2
                )

            [1] => Array
                (
                    [0] => 2
                    [1] => 3
                )

        )

    [terminating-semicolon] => 
)
string(65) "SELECT *,bar.*,db.bar.* FROM bar WHERE `select` = `\x41BC \06123`"
Array
(
    [type] => statement
    [statement] => select
    [fields] => Array
        (
            [0] => *
            [1] => Array
                (
                    [table] => bar
                    [field] => *
                )

            [2] => Array
                (
                    [database] => db
                    [table] => bar
                    [field] => *
                )

        )

    [from] => bar
    [modifiers] => Array
        (
            [where] => Array
                (
                    [type] => condition
                    [op1] => select
                    [condition] => =
                    [op2] => ABC 123
                )

        )

    [terminating-semicolon] => 
)
