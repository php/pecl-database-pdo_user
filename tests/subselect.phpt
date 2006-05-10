--TEST--
Parse Subselect statement
--SKIPIF--
<?php
    if (!extension_loaded('pdo_user')) {
        die('pdo_user not loaded');
    }
?>
--FILE--
<?php
var_dump(PDO_User::parseSQL('SELECT foo FROM (SELECT bar as foo FROM bling) as t'));
--EXPECT--
array(6) {
  ["type"]=>
  string(9) "statement"
  ["statement"]=>
  string(6) "select"
  ["fields"]=>
  array(1) {
    [0]=>
    string(3) "foo"
  }
  ["from"]=>
  array(3) {
    ["type"]=>
    string(5) "alias"
    ["table"]=>
    array(5) {
      ["type"]=>
      string(9) "statement"
      ["statement"]=>
      string(6) "select"
      ["fields"]=>
      array(1) {
        [0]=>
        array(3) {
          ["type"]=>
          string(5) "alias"
          ["field"]=>
          string(3) "bar"
          ["as"]=>
          string(3) "foo"
        }
      }
      ["from"]=>
      string(5) "bling"
      ["modifiers"]=>
      NULL
    }
    ["as"]=>
    string(1) "t"
  }
  ["modifiers"]=>
  NULL
  ["terminating-semicolon"]=>
  bool(false)
}
