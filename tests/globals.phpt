--TEST--
Globals Driver
--SKIPIF--
<?php
	if (!extension_loaded('pdo_user')) {
		die('pdo_user not loaded');
	}
?>
--FILE--
<?php

/* DON'T USE THIS IN THE WILD
 * It's just a basic driver implementation meant to provide a test framework,
 * It makes assumptions about the caller that would be unsafe in uncontrolled environments */

class PDO_User_Globals_Statement implements PDO_User_Statement {
	private $cursor = NULL;
	private $varname = NULL;

	function pdo_close() { }

	function pdo_execute() {
		$query = PDO_User::statementParam($this, PDO_User::STATEMENT_PARAM_ACTIVE_QUERY);
		if (preg_match('/^SELECT FROM "(.*?)"$/i', $query, $matches)) {
			$this->fetched = false;
			if (isset($GLOBALS[$matches[1]])) {
				$this->varname = $matches[1];
				$this->cursor = $GLOBALS[$matches[1]];
				if (is_array($this->cursor)) {
					reset($this->cursor);
					return array('rows' => count($this->cursor), 'cols' => 1);
				}
				return array('rows' => 1, 'cols' => 1);
			} else {
				return false;
			}
		}
		return false;
	}

	function pdo_fetch($ori, $fetch) {
		switch ($ori) {
			case PDO::FETCH_ORI_NEXT:
				if ($this->fetched) {
					if (is_array($this->cursor)) {
						next($this->cursor);
						return true;
					} else {
						$this->cursor = NULL;
						return false;
					}
				} else {
					$this->fetched = true;
					return true;
				}
				break;
			case PDO::FETCH_ORI_PRIOR:
				if ($this->fetched) {
					if (is_array($this->cursor)) {
						prev($this->cursor);
						return true;
					} else {
						$this->cursor = NULL;
						return false;
					}
				} else {
					$this->fetched = true;
					return true;
				}
				break;
			case PDO::FETCH_ORI_FIRST:
				if ($this->fetched) {
					if (is_array($this->cursor)) {
						reset($this->cursor);
						return true;
					} else {
						$this->cursor = NULL;
						return false;
					}
				} else {
					$this->fetched = true;
					return true;
				}
				break;
			case PDO::FETCH_ORI_LAST:
				if ($this->fetched) {
					if (is_array($this->cursor)) {
						end($this->cursor);
						return true;
					} else {
						$this->cursor = NULL;
						return false;
					}
				} else {
					$this->fetched = true;
					return true;
				}
				break;
			case PDO::FETCH_ORI_ABS:
				if ($this->fetched) {
					if (is_array($this->cursor)) {
						reset($this->cursor);
						while ($ofs--) next($this->cursor);
						return true;
					} else {
						$this->cursor = NULL;
						return false;
					}
				} else {
					$this->fetched = true;
					return true;
				}
				break;
			case PDO::FETCH_ORI_REL:
				if ($this->fetched) {
					if (is_array($this->cursor)) {
						if ($ofs > 0) {
							while ($ofs--) prev($this->cursor);
						} elseif ($ofs < 0) {
							while ($ofs++) next($this->cursor);
						}
						return true;
					} else {
						$this->cursor = NULL;
						return false;
					}
				} else {
					$this->fetched = true;
					return true;
				}
				break;
		}
		return false;
	}

	function pdo_describe($col) {
		return array('name'=>$this->varname,'maxlen'=>0x7FFFFFFF, 'precision'=>0);
	}

	function pdo_getCol($col) {
		if (is_array($this->cursor)) {
			return current($this->cursor);
		} else {
			return $this->cursor;
		}
	}

	function pdo_paramHook($type, $colno, $paramname, $is_param, &$param) { }
	function pdo_getAttribute($attr) { return NULL; }
	function pdo_setAttribute($attr, $val) { return false; }
	function pdo_colMeta($col) {
		return array('table'=>'global_symbol_table', 'type'=>gettype($this->cursor));
	}
	function pdo_nextRowset() {
		return $this->fetch(PDO::FETCH_ORI_NEXT, 1);
	}
	function pdo_closeCursor() {
		$this->cursor = NULL;
	}
}

class PDO_User_Globals_Driver implements PDO_User_Driver {
	private $errorCode = 0;
	private $errorDesc = '';
	private $lastInsert = NULL;

	function insert($key, $value) {
		if (isset($GLOBALS[$key])) {
			return 0;
		}
		$GLOBALS[$key] = $value;
		return 1;
	}

	function update($key, $value) {
		if (!isset($GLOBALS[$key])) {
			return 0;
		}
		$GLOBALS[$key] = $value;
		return 1;
	}

	function delete($key) {
		if (!isset($GLOBALS[$key])) {
			return 0;
		}
		unset($GLOBALS[$key]);
		return 1;
	}

	/* PDO Driver Implementation */

	function __construct($dsn, $user, $pass, $options) { }
	function pdo_close() { }

	function pdo_prepare($sql, $options) {
		return new PDO_User_Globals_Statement($sql, $options);
	}
		
	function pdo_do($sql) {
		if (preg_match('/^INSERT "(.*?)" as "(.*?)"$/i', $sql, $matches)) {
			return $this->insert($matches[1], $matches[2]);
		} elseif (preg_match('/^UPDATE "(.*?)" to "(.*?)"$/i', $sql, $matches)) {
			return $this->update($matches[1], $matches[2]);
		} elseif (preg_match('/^DELETE "(.*?)"$/i', $sql, $matches)) {
			return $this->delete($matches[1]);
		}
		$this->errorCode = 1000;
		$this->errorDesc = 'Invalid query: ' . $sql;
	 	return false;
	}

	function pdo_quote($str) {
		return '"' . addslashes($str) . '"';
	}

	function pdo_begin() { return false; }
	function pdo_commit() { return false; }
	function pdo_rollback() { return false; }
	function pdo_setAttribute($attr, $val) { return false; }
	function pdo_getAttribute($attr) { return NULL; }
	function pdo_lastInsertID($seq) { return $this->lastInsert; }

	function pdo_fetchError() {
		return array($this->errorCode, $this->errorDesc);
	}

	function pdo_checkLiveness() { return true; }
}

$preset = 'Some Value';

$drv = new PDO('user:driver=PDO_User_Globals_Driver');
var_dump($drv->exec('INSERT "newvar" as "newvalue"'), $newvar);
var_dump($drv->exec('UPDATE "preset" to "Other Value"'), $preset);
var_dump($drv->exec('DELETE "preset"'), @$preset);
var_dump($drv->exec('DELETE "preset"'));
var_dump($drv->exec('Invalid Query'), $drv->errorInfo());

echo "-*-\n";
var_dump($stmt = $drv->prepare('SELECT FROM "newvar"'));
var_dump($stmt->execute());
var_dump($stmt->columnCount());
var_dump($stmt->rowCount());
var_dump($stmt->getColumnMeta(0));
var_dump($stmt->fetchColumn(0));
unset($stmt);

echo "-*-\n";
$foo = 'bar';
$baz = NULL;
var_dump($stmt = $drv->prepare('SELECT FROM ?'));
var_dump($stmt->bindValue(1, 'foo'));
var_dump($stmt->bindColumn(1, $baz));
var_dump($stmt->execute());
var_dump($stmt->fetch(PDO::FETCH_BOUND), $baz);
unset($stmt);

echo "-*-\n";
var_dump($stmt = $drv->prepare('INVALID QUERY'));
var_dump($stmt->execute());

--EXPECTF--
int(1)
string(8) "newvalue"
int(1)
string(11) "Other Value"
int(1)
NULL
int(0)
bool(false)
array(3) {
  [0]=>
  string(5) "00000"
  [1]=>
  int(1000)
  [2]=>
  string(4) "1000"
}
-*-
object(PDOStatement)#%d (1) {
  ["queryString"]=>
  string(20) "SELECT FROM "newvar""
}
bool(true)
int(1)
int(1)
array(6) {
  ["table"]=>
  string(19) "global_symbol_table"
  ["type"]=>
  string(6) "string"
  ["name"]=>
  string(6) "newvar"
  ["len"]=>
  int(2147483647)
  ["precision"]=>
  int(0)
  ["pdo_type"]=>
  int(2)
}
string(8) "newvalue"
-*-
object(PDOStatement)#%d (1) {
  ["queryString"]=>
  string(13) "SELECT FROM ?"
}
bool(true)
bool(true)
bool(true)
bool(true)
string(3) "bar"
-*-
object(PDOStatement)#%d (1) {
  ["queryString"]=>
  string(13) "INVALID QUERY"
}
bool(false)

