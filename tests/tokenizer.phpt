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
	var_dump(PDO_User::tokenizeSQL($s));
}
--EXPECT--
string(19) "SELECT foo FROM bar"
array(4) {
  [0]=>
  array(2) {
    ["token"]=>
    int(8)
    ["data"]=>
    string(6) "SELECT"
  }
  [1]=>
  array(2) {
    ["token"]=>
    int(32)
    ["data"]=>
    string(3) "foo"
  }
  [2]=>
  array(2) {
    ["token"]=>
    int(37)
    ["data"]=>
    string(4) "FROM"
  }
  [3]=>
  array(2) {
    ["token"]=>
    int(32)
    ["data"]=>
    string(3) "bar"
  }
}
string(43) "INSERT INTO bar (foo,baz) values(1,2),(2,3)"
array(20) {
  [0]=>
  array(2) {
    ["token"]=>
    int(9)
    ["data"]=>
    string(6) "INSERT"
  }
  [1]=>
  array(2) {
    ["token"]=>
    int(10)
    ["data"]=>
    string(4) "INTO"
  }
  [2]=>
  array(2) {
    ["token"]=>
    int(32)
    ["data"]=>
    string(3) "bar"
  }
  [3]=>
  array(2) {
    ["token"]=>
    int(31)
    ["data"]=>
    string(1) "("
  }
  [4]=>
  array(2) {
    ["token"]=>
    int(32)
    ["data"]=>
    string(3) "foo"
  }
  [5]=>
  array(2) {
    ["token"]=>
    int(11)
    ["data"]=>
    string(1) ","
  }
  [6]=>
  array(2) {
    ["token"]=>
    int(32)
    ["data"]=>
    string(3) "baz"
  }
  [7]=>
  array(2) {
    ["token"]=>
    int(28)
    ["data"]=>
    string(1) ")"
  }
  [8]=>
  array(2) {
    ["token"]=>
    int(38)
    ["data"]=>
    string(6) "values"
  }
  [9]=>
  array(2) {
    ["token"]=>
    int(31)
    ["data"]=>
    string(1) "("
  }
  [10]=>
  array(2) {
    ["token"]=>
    int(33)
    ["data"]=>
    string(1) "1"
  }
  [11]=>
  array(2) {
    ["token"]=>
    int(11)
    ["data"]=>
    string(1) ","
  }
  [12]=>
  array(2) {
    ["token"]=>
    int(33)
    ["data"]=>
    string(1) "2"
  }
  [13]=>
  array(2) {
    ["token"]=>
    int(28)
    ["data"]=>
    string(1) ")"
  }
  [14]=>
  array(2) {
    ["token"]=>
    int(11)
    ["data"]=>
    string(1) ","
  }
  [15]=>
  array(2) {
    ["token"]=>
    int(31)
    ["data"]=>
    string(1) "("
  }
  [16]=>
  array(2) {
    ["token"]=>
    int(33)
    ["data"]=>
    string(1) "2"
  }
  [17]=>
  array(2) {
    ["token"]=>
    int(11)
    ["data"]=>
    string(1) ","
  }
  [18]=>
  array(2) {
    ["token"]=>
    int(33)
    ["data"]=>
    string(1) "3"
  }
  [19]=>
  array(2) {
    ["token"]=>
    int(28)
    ["data"]=>
    string(1) ")"
  }
}
string(72) "ALTER TABLE bar insert column bling int not null default "moo" after foo"
array(13) {
  [0]=>
  array(2) {
    ["token"]=>
    int(251)
    ["data"]=>
    string(5) "ALTER"
  }
  [1]=>
  array(2) {
    ["token"]=>
    int(43)
    ["data"]=>
    string(5) "TABLE"
  }
  [2]=>
  array(2) {
    ["token"]=>
    int(32)
    ["data"]=>
    string(3) "bar"
  }
  [3]=>
  array(2) {
    ["token"]=>
    int(9)
    ["data"]=>
    string(6) "insert"
  }
  [4]=>
  array(2) {
    ["token"]=>
    int(248)
    ["data"]=>
    string(6) "column"
  }
  [5]=>
  array(2) {
    ["token"]=>
    int(32)
    ["data"]=>
    string(5) "bling"
  }
  [6]=>
  array(2) {
    ["token"]=>
    int(53)
    ["data"]=>
    string(3) "int"
  }
  [7]=>
  array(2) {
    ["token"]=>
    int(1)
    ["data"]=>
    string(3) "not"
  }
  [8]=>
  array(2) {
    ["token"]=>
    int(46)
    ["data"]=>
    string(4) "null"
  }
  [9]=>
  array(2) {
    ["token"]=>
    int(47)
    ["data"]=>
    string(7) "default"
  }
  [10]=>
  array(2) {
    ["token"]=>
    int(35)
    ["data"]=>
    string(3) "moo"
  }
  [11]=>
  array(2) {
    ["token"]=>
    int(246)
    ["data"]=>
    string(5) "after"
  }
  [12]=>
  array(2) {
    ["token"]=>
    int(32)
    ["data"]=>
    string(3) "foo"
  }
}
string(52) "SELECT foo FROM bar WHERE `select` = `\x41BC \06123`"
array(8) {
  [0]=>
  array(2) {
    ["token"]=>
    int(8)
    ["data"]=>
    string(6) "SELECT"
  }
  [1]=>
  array(2) {
    ["token"]=>
    int(32)
    ["data"]=>
    string(3) "foo"
  }
  [2]=>
  array(2) {
    ["token"]=>
    int(37)
    ["data"]=>
    string(4) "FROM"
  }
  [3]=>
  array(2) {
    ["token"]=>
    int(32)
    ["data"]=>
    string(3) "bar"
  }
  [4]=>
  array(2) {
    ["token"]=>
    int(6)
    ["data"]=>
    string(5) "WHERE"
  }
  [5]=>
  array(2) {
    ["token"]=>
    int(32)
    ["data"]=>
    string(6) "select"
  }
  [6]=>
  array(2) {
    ["token"]=>
    int(19)
    ["data"]=>
    string(1) "="
  }
  [7]=>
  array(2) {
    ["token"]=>
    int(32)
    ["data"]=>
    string(7) "ABC 123"
  }
}