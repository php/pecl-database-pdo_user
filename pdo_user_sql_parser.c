/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is include which follows the "include" declaration
** in the input file. */
#include <stdio.h>
#line 15 "pdo_user_sql_parser.lemon"

#include "php.h"
#include "php_pdo_user_sql.h"

static inline zval *pusp_zvalize_hnum(php_pdo_user_sql_token *T) {
	zval *ret;
	int val = 0;
	char *s = T->token, *e = T->token + T->token_len;

	MAKE_STD_ZVAL(ret);
	if (strncmp("0x", s, 2) == 0) {
		s += 2;
	}
	while (s < e) {
		/* illegal characters aren't possible, the lexer wouldn't give us any */
		val <<=4;
		if (*s >= '0' && *s <= '9') {
			val |= *s - '0';
		} else if (*s >= 'A' && *s <= 'F') {
			val |= (*s - 'A') + 10;
		} else {
			val |= (*s - 'a') + 10;
		} 
		s++;
	}
	ZVAL_LONG(ret, val);
	return ret;
}

static inline zval *pusp_zvalize_lnum(php_pdo_user_sql_token *T) {
	zval *ret;
	int val = 0;
	unsigned char *s = T->token, *e = T->token + T->token_len;

	MAKE_STD_ZVAL(ret);
	while (s < e) {
		/* illegal characters aren't possible, the lexer wouldn't give us any */
		val *= 10;
		val += *s - '0';
		s++;
	}
	ZVAL_LONG(ret, val);
	return ret;
}

static inline zval *pusp_zvalize_token(php_pdo_user_sql_token *T)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	ZVAL_STRINGL(ret, T->token, T->token_len, !T->freeme);

	return ret;
}

#define pusp_zvalize_static_string(str)	pusp_zvalize_stringl((str), sizeof(str) - 1, 1)
static inline zval *pusp_zvalize_stringl(char *str, int strlen, int dup)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	ZVAL_STRINGL(ret, str, strlen, dup);

	return ret;
}

static inline void pusp_do_push_labeled_zval(zval *ret, zval **pair)
{
	add_assoc_zval(ret, Z_STRVAL_P(pair[0]), pair[1]);
	zval_ptr_dtor(&pair[0]);
	efree(pair);
}

/* ----------- */

static inline void pusp_do_terminal_statement(zval **return_value, zval *statement, zend_bool have_semicolon)
{
	**return_value = *statement;
	efree(statement);
	add_assoc_bool(*return_value, "terminating-semicolon", have_semicolon);
}

static inline zval *pusp_do_join_expression(zval *table1, zval *jointype, zval *table2, zval *oncondition)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	array_init(ret);
	add_assoc_stringl(ret, "type", "join", sizeof("join") - 1, 1);
	add_assoc_zval(ret, "table1", table1);
	add_assoc_zval(ret, "join-type", jointype);
	add_assoc_zval(ret, "table2", table2);
	add_assoc_zval(ret, "on", oncondition);

	return ret;
}

static inline zval *pusp_do_function(zval *fname, zval *arguments)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	array_init(ret);
	add_assoc_stringl(ret, "type", "fcall", sizeof("fcall") - 1, 1);
	add_assoc_zval(ret, "fname", fname);
	add_assoc_zval(ret, "args", arguments);

	return ret;
}

static inline zval *pusp_do_add_query_modifier(zval *ret, char *lbl, zval *val)
{
	if (Z_TYPE_P(ret) == IS_NULL) { /* substitute for: ret == EG(uninitialized_zval_ptr), avoid the TSRMLS_FETCH(); */
		MAKE_STD_ZVAL(ret);
		array_init(ret);
	}
	add_assoc_zval(ret, lbl, val);

	return ret;
}

static zval *pusp_do_declare_num(char *fieldtype, zval *precision, zval *unsgn, zval *zerofill)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	array_init(ret);
	add_assoc_stringl(ret, "type", "field", sizeof("field") - 1, 1);
	add_assoc_string(ret, "fieldtype", fieldtype, 1);
	add_assoc_zval(ret, "precision", precision);
	add_assoc_zval(ret, "unsigned", unsgn);
	add_assoc_zval(ret, "zerofill", zerofill);

	return ret;
}

static zval *pusp_do_declare_type(char *type, char *attr, zval *zattr)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	array_init(ret);
	add_assoc_stringl(ret, "type", "field", sizeof("field") - 1, 1);
	add_assoc_string(ret, "fieldtype", type, 1);
	if (attr) {
		add_assoc_zval(ret, attr, zattr);
	}

	return ret;
}

/* ---------------- */

static inline zval *pusp_do_select_statement(zval *fieldlist, zval *tableexpr, zval *modifiers)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	array_init(ret);
	add_assoc_stringl(ret, "type", "statement", sizeof("statement") - 1, 1);
	add_assoc_stringl(ret, "statement", "select", sizeof("select") - 1, 1);
	add_assoc_zval(ret, "fields", fieldlist);
	add_assoc_zval(ret, "from", tableexpr);
	add_assoc_zval(ret, "modifiers", modifiers);

	return ret;
}

static inline zval *pusp_do_insert_select_statement(zval *table, zval *fields, zval *selectstmt)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	array_init(ret);
	add_assoc_stringl(ret, "type", "statement", sizeof("statement") - 1, 1);
	add_assoc_stringl(ret, "statement", "insert-select", sizeof("insert-select") - 1, 1);
	add_assoc_zval(ret, "table", table);
	add_assoc_zval(ret, "fields", fields);
	add_assoc_zval(ret, "query", selectstmt);

	return ret;
}

static inline zval *pusp_do_insert_statement(zval *table, zval *fields, zval *insertgroup)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	array_init(ret);
	add_assoc_stringl(ret, "type", "statement", sizeof("statement") - 1, 1);
	add_assoc_stringl(ret, "statement", "insert", sizeof("insert") - 1, 1);
	add_assoc_zval(ret, "table", table);
	add_assoc_zval(ret, "fields", fields);
	add_assoc_zval(ret, "data", insertgroup);

	return ret;
}

static inline zval *pusp_do_update_statement(zval *table, zval *setlist, zval *wherestmt)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	array_init(ret);
	add_assoc_stringl(ret, "type", "statement", sizeof("statement") - 1, 1);
	add_assoc_stringl(ret, "statement", "update", sizeof("update") - 1, 1);
	add_assoc_zval(ret, "table", table);
	add_assoc_zval(ret, "set", setlist);
	add_assoc_zval(ret, "where", wherestmt);

	return ret;
}

static inline zval *pusp_do_delete_statement(zval *table, zval *wherestmt)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	array_init(ret);
	add_assoc_stringl(ret, "type", "statement", sizeof("statement") - 1, 1);
	add_assoc_stringl(ret, "statement", "delete", sizeof("delete") - 1, 1);
	add_assoc_zval(ret, "table", table);
	add_assoc_zval(ret, "where", wherestmt);

	return ret;
}

static inline zval *pusp_do_rename_statement(zval *renlist)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	array_init(ret);
	add_assoc_stringl(ret, "type", "statement", sizeof("statement") - 1, 1);
	add_assoc_stringl(ret, "statement", "rename table", sizeof("rename table") - 1, 1);
	add_assoc_zval(ret, "tables", renlist);

	return ret;
}

static inline zval *pusp_do_create_statement(zval *table, zval *fields)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	array_init(ret);
	add_assoc_stringl(ret, "type", "statement", sizeof("statement") - 1, 1);
	add_assoc_stringl(ret, "statement", "create table", sizeof("create table") - 1, 1);
	add_assoc_zval(ret, "table", table);
	add_assoc_zval(ret, "fields", fields);

	return ret;
}

static inline zval *pusp_do_drop_statement(zval *table)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	array_init(ret);
	add_assoc_stringl(ret, "type", "statement", sizeof("statement") - 1, 1);
	add_assoc_stringl(ret, "statement", "drop table", sizeof("drop table") - 1, 1);
	add_assoc_zval(ret, "table", table);

	return ret;
}

#define DO_COND(R, T, A, B) { \
	MAKE_STD_ZVAL(R); \
	array_init(R); \
	add_assoc_stringl(R, "type", "condition", sizeof("condition") - 1, 1); \
	add_assoc_zval(R, "op1", A); \
	add_assoc_stringl(R, "condition", T, sizeof(T) - 1, 1); \
	add_assoc_zval(R, "op2", B); \
}

#define DO_MATHOP(R, A, O, B) { \
	MAKE_STD_ZVAL(R); \
	array_init(R); \
	add_assoc_stringl(R, "type", "math", sizeof("math") - 1, 1); \
	add_assoc_zval(R, "op1", A); \
	add_assoc_stringl(R, "operation", O, sizeof(O) - 1, 1); \
	add_assoc_zval(R, "op2", B); \
}

#line 295 "pdo_user_sql_parser.c"
/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/* 
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands. 
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash 
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    php_pdo_user_sql_parserTOKENTYPE     is the data type used for minor tokens given 
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is php_pdo_user_sql_parserTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.
**    php_pdo_user_sql_parserARG_SDECL     A static variable declaration for the %extra_argument
**    php_pdo_user_sql_parserARG_PDECL     A parameter declaration for the %extra_argument
**    php_pdo_user_sql_parserARG_STORE     Code to store %extra_argument into yypParser
**    php_pdo_user_sql_parserARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 136
#define YYACTIONTYPE unsigned short int
#define php_pdo_user_sql_parserTOKENTYPE php_pdo_user_sql_token
typedef union {
  php_pdo_user_sql_parserTOKENTYPE yy0;
  zval** yy122;
  zval* yy198;
  int yy271;
} YYMINORTYPE;
#define YYSTACKDEPTH 100
#define php_pdo_user_sql_parserARG_SDECL zval *return_value;
#define php_pdo_user_sql_parserARG_PDECL ,zval *return_value
#define php_pdo_user_sql_parserARG_FETCH zval *return_value = yypParser->return_value
#define php_pdo_user_sql_parserARG_STORE yypParser->return_value = return_value
#define YYNSTATE 293
#define YYNRULE 140
#define YYERRORSYMBOL 95
#define YYERRSYMDT yy271
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* Next are that tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   278,   50,   51,   52,   53,   54,   29,   31,   33,  293,
 /*    10 */    18,   25,  138,  195,  196,  205,  209,  213,  217,  221,
 /*    20 */   225,  227,  236,  240,  244,  248,  252,  256,  257,  258,
 /*    30 */   260,  262,  263,  264,  265,  266,  267,  268,  269,  270,
 /*    40 */   274,  285,   86,   35,  105,  111,   47,   99,   55,  103,
 /*    50 */    26,   20,   37,   38,   39,  121,  134,   82,   78,   80,
 /*    60 */    64,   70,   72,   86,   76,   41,   29,   31,   33,   28,
 /*    70 */    18,   25,   45,    4,  116,  117,  124,   45,   82,   78,
 /*    80 */    80,   64,   70,   72,    5,   76,  294,   29,   31,   33,
 /*    90 */    35,   18,   25,   49,  185,    6,  124,   95,   20,   37,
 /*   100 */    38,   39,  150,  144,  162,  165,    9,  174,  290,  123,
 /*   110 */    84,  120,   41,   29,   31,   33,   28,   18,   25,  153,
 /*   120 */   124,   93,    2,   29,   31,   33,   10,   18,   25,   18,
 /*   130 */    25,   66,   68,   74,   29,   31,   33,  161,   18,   25,
 /*   140 */   187,  189,  193,  191,  194,  136,  126,   44,  128,  130,
 /*   150 */   132,   19,   66,   68,   74,   16,   40,   29,   31,   33,
 /*   160 */   115,   18,   25,  178,   16,   46,  126,  289,  128,  130,
 /*   170 */   132,   19,   15,  113,  119,    7,   40,    8,   19,  154,
 /*   180 */   142,   23,  106,   40,  143,   13,   19,   24,   48,   14,
 /*   190 */   126,   40,  128,  130,  132,   24,  146,   22,   37,   38,
 /*   200 */    19,   19,   19,   19,  160,   40,   40,   40,   40,   63,
 /*   210 */    63,   63,   63,  148,   19,   56,   58,   60,   62,   40,
 /*   220 */    57,   59,   61,   98,  164,   19,   19,   55,   19,   96,
 /*   230 */    40,   40,  155,   40,   63,  110,   97,  115,  167,   19,
 /*   240 */   104,  434,    1,    3,   40,   12,   11,  107,   63,  160,
 /*   250 */    19,  118,  100,  173,  137,   40,   37,   38,   39,  142,
 /*   260 */   188,   19,   19,  139,   21,   40,   40,   40,   19,   41,
 /*   270 */    17,   43,  101,   40,   19,  180,  102,   42,  108,   40,
 /*   280 */    19,   19,   19,   27,  112,   40,   40,   40,   19,   30,
 /*   290 */    32,   34,  179,   40,   57,   59,   61,   36,   19,   19,
 /*   300 */   114,   19,  282,   40,   40,  135,   40,   65,   67,   19,
 /*   310 */    69,  283,   19,  122,   40,   19,   40,   40,   71,  281,
 /*   320 */    40,   73,   19,   19,   75,  125,  282,   40,   40,   19,
 /*   330 */    19,   77,   79,  127,   40,   40,  129,   19,   81,   83,
 /*   340 */   120,  140,   40,  288,  131,   19,   85,   91,   87,   89,
 /*   350 */    40,   19,   19,   19,   88,  133,   40,   40,   40,   19,
 /*   360 */    90,   92,   94,  152,   40,  141,   19,  284,  109,  149,
 /*   370 */   147,   40,   40,  280,  284,  159,  145,  151,  157,   40,
 /*   380 */   287,  156,  158,  163,   55,  166,  170,  168,  169,  171,
 /*   390 */   172,  175,  176,  177,  181,  184,  182,  183,  186,  190,
 /*   400 */   202,  201,  192,  198,  197,  200,  231,  204,  199,  206,
 /*   410 */   208,  203,  207,  210,  212,  211,  214,  216,  215,  218,
 /*   420 */   233,  219,  222,  220,  223,  226,  224,  235,  249,  228,
 /*   430 */   229,  237,  230,  251,  253,  232,  239,  234,  238,  255,
 /*   440 */   273,  242,  241,  243,  271,  245,  247,  246,  275,  277,
 /*   450 */   279,  250,  254,  259,  261,  272,  276,  286,  291,  292,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    39,  127,  128,  129,  130,  131,   25,   26,   27,    0,
 /*    10 */    29,   30,   11,   52,   53,   54,   55,   56,   57,   58,
 /*    20 */    59,   60,   61,   62,   63,   64,   65,   66,   67,   68,
 /*    30 */    69,   70,   71,   72,   73,   74,   75,   76,   77,   78,
 /*    40 */    79,   80,    1,   24,    2,    3,   45,    5,    6,    7,
 /*    50 */    31,   32,   33,   34,   35,   31,   32,   16,   17,   18,
 /*    60 */    19,   20,   21,    1,   23,   46,   25,   26,   27,   28,
 /*    70 */    29,   30,    8,    9,   93,   94,   15,    8,   16,   17,
 /*    80 */    18,   19,   20,   21,   10,   23,    0,   25,   26,   27,
 /*    90 */    24,   29,   30,  107,    1,   32,   15,   31,   32,   33,
 /*   100 */    34,   35,   38,   31,   40,   41,   37,   43,   44,   28,
 /*   110 */    12,  125,   46,   25,   26,   27,   28,   29,   30,  101,
 /*   120 */    15,   12,   36,   25,   26,   27,  100,   29,   30,   29,
 /*   130 */    30,   90,   91,   92,   25,   26,   27,  119,   29,   30,
 /*   140 */    47,   48,   49,   50,   51,   84,   85,  121,   87,   88,
 /*   150 */    89,  111,   90,   91,   92,   11,  116,   25,   26,   27,
 /*   160 */   120,   29,   30,  104,   11,  105,   85,  108,   87,   88,
 /*   170 */    89,  111,   28,  133,  134,   99,  116,   98,  111,  102,
 /*   180 */   120,   28,    4,  116,  124,   31,  111,  120,  106,  122,
 /*   190 */    85,  116,   87,   88,   89,  120,   11,  122,   33,   34,
 /*   200 */   111,  111,  111,  111,  127,  116,  116,  116,  116,  120,
 /*   210 */   120,  120,  120,   28,  111,  126,  126,  126,  126,  116,
 /*   220 */    12,   13,   14,  120,  102,  111,  111,    6,  111,  126,
 /*   230 */   116,  116,   11,  116,  120,  120,   28,  120,  103,  111,
 /*   240 */   126,   96,   97,   98,  116,  121,   11,  132,  120,  127,
 /*   250 */   111,  134,  116,  118,  126,  116,   33,   34,   35,  120,
 /*   260 */   111,  111,  111,  124,   31,  116,  116,  116,  111,   46,
 /*   270 */   120,  120,   11,  116,  111,   11,  116,  120,   11,  116,
 /*   280 */   111,  111,  111,  120,    4,  116,  116,  116,  111,  120,
 /*   290 */   120,  120,   28,  116,   12,   13,   14,  120,  111,  111,
 /*   300 */    11,  111,   11,  116,  116,  106,  116,  120,  120,  111,
 /*   310 */   120,  111,  111,  106,  116,  111,  116,  116,  120,   28,
 /*   320 */   116,  120,  111,  111,  120,   32,   11,  116,  116,  111,
 /*   330 */   111,  120,  120,   86,  116,  116,   86,  111,  120,  120,
 /*   340 */   125,   15,  116,   28,   86,  111,  120,   16,   17,   18,
 /*   350 */   116,  111,  111,  111,  120,   86,  116,  116,  116,  111,
 /*   360 */   120,  120,  120,   39,  116,   32,  111,  111,  120,   32,
 /*   370 */    32,  116,  116,  117,  111,  120,  123,   32,   32,  116,
 /*   380 */   117,  119,   19,   32,    6,   42,   32,   11,  118,   83,
 /*   390 */    32,   42,   32,   31,  108,  110,   32,  109,   46,   49,
 /*   400 */    31,   81,   49,  113,  112,   82,   31,   28,  114,  112,
 /*   410 */   114,  116,  113,  112,  114,  113,  112,  114,  113,  112,
 /*   420 */    11,  113,  112,  114,  113,  112,  114,   28,   31,  115,
 /*   430 */   113,  115,  114,   28,   31,  116,  114,  116,  113,   28,
 /*   440 */    28,  113,  115,  114,   31,  115,  114,  113,   31,   28,
 /*   450 */    31,  116,  116,  112,  112,  116,  116,   31,   42,   32,
};
#define YY_SHIFT_USE_DFLT (-40)
static const short yy_shift_ofst[] = {
 /*     0 */    64,   86,    9,  -40,   74,   63,   72,   69,  -40,  154,
 /*    10 */   235,  154,  -40,   19,  144,  -40,   19,  132,   19,  -40,
 /*    20 */   233,   19,  153,  -40,  132,   19,   19,   88,  -40,   19,
 /*    30 */   100,   19,  100,   19,  100,   19,  132,  -40,  -40,  -40,
 /*    40 */   -40,  -40,  -40,  -40,  -40,   19,    1,   24,  105,   42,
 /*    50 */   -40,  -40,  -40,  -40,  -40,   66,  282,   66,  -40,   66,
 /*    60 */   -40,   66,  -40,   62,   19,  132,   19,  132,   19,  132,
 /*    70 */    19,  132,   19,  132,   19,  132,   19,  132,   19,  132,
 /*    80 */    19,  132,   19,   98,   19,  132,  331,   19,  132,   19,
 /*    90 */   132,   19,  109,   19,  132,   66,  208,  -40,   41,  165,
 /*   100 */   261,  165,  -40,   66,  282,  178,   19,  267,   19,  132,
 /*   110 */   132,  280,   19,  289,   19,  -19,  -40,  -40,  -40,  -40,
 /*   120 */    24,   24,   81,  -40,  293,  -40,  247,  -40,  250,  -40,
 /*   130 */   258,  -40,  269,  -40,  -40,   61,   66,  282,   19,  326,
 /*   140 */   333,  -40,  132,  326,  337,  185,  338,  -40,  -40,  -40,
 /*   150 */   345,  324,  346,  221,  -40,  346,  -40,  363,   19,  132,
 /*   160 */   -40,  -40,  351,  378,  -40,  343,  354,  376,  354,  -40,
 /*   170 */   306,  358,  -40,  -40,  349,  360,  362,  364,  264,  -40,
 /*   180 */   364,  -40,  -39,  -40,   93,  352,  -40,  223,  -40,  350,
 /*   190 */   -40,  353,  -40,  -40,  -40,  -40,  369,  320,  323,  -40,
 /*   200 */   -40,  -40,  165,  379,  -40,  369,  320,  323,  -40,  369,
 /*   210 */   320,  323,  -40,  369,  320,  323,  -40,  369,  320,  323,
 /*   220 */   -40,  369,  320,  323,  -40,  369,  -40,  375,  320,  323,
 /*   230 */   -40,  165,  409,  165,  399,  -40,  375,  320,  323,  -40,
 /*   240 */   375,  320,  323,  -40,  375,  320,  323,  -40,  397,  165,
 /*   250 */   405,  -40,  403,  165,  411,  -40,  -40,  -40,  369,  -40,
 /*   260 */   369,  -40,  -40,  -40,  -40,  -40,  -40,  -40,  -40,  -40,
 /*   270 */   413,  165,  412,  -40,  417,  165,  421,  -40,  419,  223,
 /*   280 */   291,  -40,  223,  -40,  -40,  426,  223,  315,  -40,  -40,
 /*   290 */   416,  427,  -40,
};
#define YY_REDUCE_USE_DFLT (-127)
static const short yy_reduce_ofst[] = {
 /*     0 */   145, -127, -127, -127, -127, -127,   76,   79, -127,   26,
 /*    10 */  -127,  124, -127,   67, -127, -127,  150, -127,  151, -127,
 /*    20 */  -127,   75, -127, -127, -127,  157,  163, -127, -127,  169,
 /*    30 */  -127,  170, -127,  171, -127,  177, -127, -127, -127, -127,
 /*    40 */  -127, -127, -127, -127, -127,   60, -127,   82,  -14, -126,
 /*    50 */  -127, -127, -127, -127, -127,   89, -127,   90, -127,   91,
 /*    60 */  -127,   92, -127, -127,  187, -127,  188, -127,  190, -127,
 /*    70 */   198, -127,  201, -127,  204, -127,  211, -127,  212, -127,
 /*    80 */   218, -127,  219, -127,  226, -127, -127,  234, -127,  240,
 /*    90 */  -127,  241, -127,  242, -127,  103, -127, -127, -127,  136,
 /*   100 */  -127,  160, -127,  114, -127, -127,  115, -127,  248, -127,
 /*   110 */  -127, -127,   40, -127,  117, -127, -127, -127, -127, -127,
 /*   120 */   199,  207,  215, -127, -127, -127, -127, -127, -127, -127,
 /*   130 */  -127, -127, -127, -127, -127,  215,  128, -127,  139, -127,
 /*   140 */  -127, -127, -127, -127,  253, -127, -127, -127, -127, -127,
 /*   150 */  -127, -127,   18,   77, -127,  262, -127, -127,  255, -127,
 /*   160 */  -127, -127, -127,  122, -127, -127,  135, -127,  270, -127,
 /*   170 */  -127, -127, -127, -127, -127, -127, -127,   59, -127, -127,
 /*   180 */   286, -127,  288,  285, -127, -127, -127,  149, -127, -127,
 /*   190 */  -127, -127, -127, -127, -127, -127,  292,  290,  294, -127,
 /*   200 */  -127, -127,  295, -127, -127,  297,  299,  296, -127,  301,
 /*   210 */   302,  300, -127,  304,  305,  303, -127,  307,  308,  309,
 /*   220 */  -127,  310,  311,  312, -127,  313, -127,  314,  317,  318,
 /*   230 */  -127,  319, -127,  321, -127, -127,  316,  325,  322, -127,
 /*   240 */   327,  328,  329, -127,  330,  334,  332, -127, -127,  335,
 /*   250 */  -127, -127, -127,  336, -127, -127, -127, -127,  341, -127,
 /*   260 */   342, -127, -127, -127, -127, -127, -127, -127, -127, -127,
 /*   270 */  -127,  339, -127, -127, -127,  340, -127, -127, -127,  256,
 /*   280 */  -127, -127,  200, -127, -127, -127,  263, -127, -127, -127,
 /*   290 */  -127, -127, -127,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   433,  433,  433,  295,  433,  433,  366,  433,  296,  433,
 /*    10 */   297,  433,  360,  433,  433,  362,  433,  409,  433,  411,
 /*    20 */   412,  433,  433,  413,  410,  433,  433,  433,  414,  433,
 /*    30 */   417,  433,  418,  433,  419,  433,  420,  421,  422,  423,
 /*    40 */   424,  425,  416,  415,  361,  433,  433,  433,  384,  303,
 /*    50 */   379,  380,  381,  382,  383,  433,  385,  433,  392,  433,
 /*    60 */   393,  433,  394,  433,  433,  395,  433,  396,  433,  397,
 /*    70 */   433,  398,  433,  399,  433,  400,  433,  401,  433,  402,
 /*    80 */   433,  403,  433,  433,  433,  404,  433,  433,  405,  433,
 /*    90 */   406,  433,  433,  433,  407,  433,  433,  408,  433,  433,
 /*   100 */   433,  433,  386,  433,  387,  433,  433,  388,  433,  426,
 /*   110 */   427,  433,  433,  389,  433,  432,  430,  431,  428,  429,
 /*   120 */   433,  433,  433,  371,  433,  373,  433,  375,  433,  376,
 /*   130 */   433,  377,  433,  378,  374,  433,  433,  372,  433,  367,
 /*   140 */   433,  370,  369,  368,  433,  433,  433,  363,  365,  364,
 /*   150 */   433,  433,  433,  391,  298,  433,  357,  433,  433,  359,
 /*   160 */   390,  358,  433,  391,  299,  433,  433,  300,  433,  354,
 /*   170 */   433,  433,  356,  355,  433,  433,  433,  433,  433,  301,
 /*   180 */   433,  304,  433,  313,  306,  433,  307,  433,  308,  433,
 /*   190 */   309,  433,  310,  311,  312,  314,  349,  345,  347,  315,
 /*   200 */   346,  344,  433,  433,  348,  349,  345,  347,  316,  349,
 /*   210 */   345,  347,  317,  349,  345,  347,  318,  349,  345,  347,
 /*   220 */   319,  349,  345,  347,  320,  349,  321,  351,  345,  347,
 /*   230 */   322,  433,  433,  433,  433,  350,  351,  345,  347,  323,
 /*   240 */   351,  345,  347,  324,  351,  345,  347,  325,  433,  433,
 /*   250 */   433,  326,  433,  433,  433,  327,  328,  329,  349,  330,
 /*   260 */   349,  331,  332,  333,  334,  335,  336,  337,  338,  339,
 /*   270 */   433,  433,  433,  340,  433,  433,  433,  341,  433,  433,
 /*   280 */   433,  342,  433,  352,  353,  433,  433,  433,  343,  305,
 /*   290 */   433,  433,  302,
};
#define YY_SZ_ACTTAB (sizeof(yy_action)/sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammer, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  int stateno;       /* The state-number */
  int major;         /* The major token value.  This is the code
                     ** number for the token at this stack level */
  YYMINORTYPE minor; /* The user-supplied minor token value.  This
                     ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
  int yyerrcnt;                 /* Shifts left before out of the error */
  php_pdo_user_sql_parserARG_SDECL                /* A place to hold %extra_argument */
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void php_pdo_user_sql_parserTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "NOT",           "GROUP",         "ORDER",       
  "BY",            "LIMIT",         "WHERE",         "HAVING",      
  "SELECT",        "INSERT",        "INTO",          "COMMA",       
  "AND",           "OR",            "XOR",           "AS",          
  "BETWEEN",       "LIKE",          "RLIKE",         "EQUALS",      
  "LESSER",        "GREATER",       "LESS_EQUAL",    "GREATER_EQUAL",
  "DISTINCT",      "MUL",           "DIV",           "MOD",         
  "RPAREN",        "PLUS",          "MINUS",         "LPAREN",      
  "LABEL",         "LNUM",          "HNUM",          "STRING",      
  "SEMICOLON",     "VALUES",        "UPDATE",        "SET",         
  "DELETE",        "RENAME",        "TABLE",         "CREATE",      
  "DROP",          "FROM",          "NULL",          "DEFAULT",     
  "PRIMARY",       "KEY",           "UNIQUE",        "AUTO_INCREMENT",
  "BIT",           "INT",           "INTEGER",       "TINYINT",     
  "SMALLINT",      "MEDIUMINT",     "BIGINT",        "YEAR",        
  "FLOAT",         "REAL",          "DECIMAL",       "DOUBLE",      
  "CHAR",          "VARCHAR",       "DATE",          "TIME",        
  "DATETIME",      "TIMESTAMP",     "TEXT",          "TINYTEXT",    
  "MEDIUMTEXT",    "LONGTEXT",      "BLOB",          "TINYBLOB",    
  "MEDIUMBLOB",    "LONGBLOB",      "BINARY",        "VARBINARY",   
  "ENUM",          "UNSIGNED",      "ZEROFILL",      "TO",          
  "ON",            "INNER",         "JOIN",          "OUTER",       
  "LEFT",          "RIGHT",         "NOT_EQUAL",     "UNEQUAL",     
  "LESSER_EQUAL",  "ASC",           "DESC",          "error",       
  "terminal_statement",  "statement",     "selectstatement",  "optionalinsertfieldlist",
  "insertgrouplist",  "setlist",       "optionalwhereclause",  "togrouplist", 
  "fielddescriptorlist",  "fieldlist",     "tableexpr",     "optionalquerymodifiers",
  "fielddescriptor",  "fielddescriptortype",  "optionalfielddescriptormodifierlist",  "literal",     
  "optionalprecision",  "optionalunsigned",  "optionalzerofill",  "optionalfloatprecision",
  "intnum",        "literallist",   "togroup",       "setexpr",     
  "expr",          "insertgroup",   "exprlist",      "labellist",   
  "field",         "joinclause",    "cond",          "whereclause", 
  "limitclause",   "havingclause",  "groupclause",   "orderclause", 
  "grouplist",     "orderlist",     "orderelement",
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "terminal_statement ::= statement SEMICOLON",
 /*   1 */ "terminal_statement ::= statement",
 /*   2 */ "statement ::= selectstatement",
 /*   3 */ "statement ::= INSERT INTO LABEL optionalinsertfieldlist selectstatement",
 /*   4 */ "statement ::= INSERT INTO LABEL optionalinsertfieldlist VALUES insertgrouplist",
 /*   5 */ "statement ::= UPDATE LABEL SET setlist optionalwhereclause",
 /*   6 */ "statement ::= DELETE LABEL optionalwhereclause",
 /*   7 */ "statement ::= RENAME TABLE togrouplist",
 /*   8 */ "statement ::= CREATE TABLE LABEL LPAREN fielddescriptorlist RPAREN",
 /*   9 */ "statement ::= DROP TABLE LABEL",
 /*  10 */ "selectstatement ::= SELECT fieldlist FROM tableexpr optionalquerymodifiers",
 /*  11 */ "fielddescriptorlist ::= fielddescriptorlist COMMA fielddescriptor",
 /*  12 */ "fielddescriptorlist ::= fielddescriptor",
 /*  13 */ "fielddescriptor ::= LABEL fielddescriptortype optionalfielddescriptormodifierlist",
 /*  14 */ "optionalfielddescriptormodifierlist ::= optionalfielddescriptormodifierlist NOT NULL",
 /*  15 */ "optionalfielddescriptormodifierlist ::= optionalfielddescriptormodifierlist DEFAULT literal",
 /*  16 */ "optionalfielddescriptormodifierlist ::= optionalfielddescriptormodifierlist PRIMARY KEY",
 /*  17 */ "optionalfielddescriptormodifierlist ::= optionalfielddescriptormodifierlist UNIQUE KEY",
 /*  18 */ "optionalfielddescriptormodifierlist ::= optionalfielddescriptormodifierlist KEY",
 /*  19 */ "optionalfielddescriptormodifierlist ::= optionalfielddescriptormodifierlist AUTO_INCREMENT",
 /*  20 */ "optionalfielddescriptormodifierlist ::=",
 /*  21 */ "fielddescriptortype ::= BIT",
 /*  22 */ "fielddescriptortype ::= INT optionalprecision optionalunsigned optionalzerofill",
 /*  23 */ "fielddescriptortype ::= INTEGER optionalprecision optionalunsigned optionalzerofill",
 /*  24 */ "fielddescriptortype ::= TINYINT optionalprecision optionalunsigned optionalzerofill",
 /*  25 */ "fielddescriptortype ::= SMALLINT optionalprecision optionalunsigned optionalzerofill",
 /*  26 */ "fielddescriptortype ::= MEDIUMINT optionalprecision optionalunsigned optionalzerofill",
 /*  27 */ "fielddescriptortype ::= BIGINT optionalprecision optionalunsigned optionalzerofill",
 /*  28 */ "fielddescriptortype ::= YEAR optionalprecision",
 /*  29 */ "fielddescriptortype ::= FLOAT optionalfloatprecision optionalunsigned optionalzerofill",
 /*  30 */ "fielddescriptortype ::= REAL optionalfloatprecision optionalunsigned optionalzerofill",
 /*  31 */ "fielddescriptortype ::= DECIMAL optionalfloatprecision optionalunsigned optionalzerofill",
 /*  32 */ "fielddescriptortype ::= DOUBLE optionalfloatprecision optionalunsigned optionalzerofill",
 /*  33 */ "fielddescriptortype ::= CHAR LPAREN intnum RPAREN",
 /*  34 */ "fielddescriptortype ::= VARCHAR LPAREN intnum RPAREN",
 /*  35 */ "fielddescriptortype ::= DATE",
 /*  36 */ "fielddescriptortype ::= TIME",
 /*  37 */ "fielddescriptortype ::= DATETIME optionalprecision",
 /*  38 */ "fielddescriptortype ::= TIMESTAMP optionalprecision",
 /*  39 */ "fielddescriptortype ::= TEXT",
 /*  40 */ "fielddescriptortype ::= TINYTEXT",
 /*  41 */ "fielddescriptortype ::= MEDIUMTEXT",
 /*  42 */ "fielddescriptortype ::= LONGTEXT",
 /*  43 */ "fielddescriptortype ::= BLOB",
 /*  44 */ "fielddescriptortype ::= TINYBLOB",
 /*  45 */ "fielddescriptortype ::= MEDIUMBLOB",
 /*  46 */ "fielddescriptortype ::= LONGBLOB",
 /*  47 */ "fielddescriptortype ::= BINARY LPAREN intnum RPAREN",
 /*  48 */ "fielddescriptortype ::= VARBINARY LPAREN intnum RPAREN",
 /*  49 */ "fielddescriptortype ::= SET LPAREN literallist RPAREN",
 /*  50 */ "fielddescriptortype ::= ENUM LPAREN literallist RPAREN",
 /*  51 */ "optionalunsigned ::= UNSIGNED",
 /*  52 */ "optionalunsigned ::=",
 /*  53 */ "optionalzerofill ::= ZEROFILL",
 /*  54 */ "optionalzerofill ::=",
 /*  55 */ "optionalprecision ::= LPAREN intnum RPAREN",
 /*  56 */ "optionalprecision ::=",
 /*  57 */ "optionalfloatprecision ::= LPAREN intnum COMMA intnum RPAREN",
 /*  58 */ "optionalfloatprecision ::=",
 /*  59 */ "literallist ::= literallist COMMA literal",
 /*  60 */ "literallist ::= literal",
 /*  61 */ "togrouplist ::= togrouplist COMMA togroup",
 /*  62 */ "togrouplist ::= togroup",
 /*  63 */ "togroup ::= LABEL TO LABEL",
 /*  64 */ "setlist ::= setlist COMMA setexpr",
 /*  65 */ "setlist ::= setexpr",
 /*  66 */ "setexpr ::= LABEL EQUALS expr",
 /*  67 */ "insertgrouplist ::= insertgrouplist COMMA insertgroup",
 /*  68 */ "insertgrouplist ::= insertgroup",
 /*  69 */ "insertgroup ::= LPAREN exprlist RPAREN",
 /*  70 */ "labellist ::= labellist COMMA LABEL",
 /*  71 */ "labellist ::= LABEL",
 /*  72 */ "optionalinsertfieldlist ::= LPAREN labellist RPAREN",
 /*  73 */ "optionalinsertfieldlist ::=",
 /*  74 */ "fieldlist ::= fieldlist COMMA field",
 /*  75 */ "fieldlist ::= field",
 /*  76 */ "field ::= expr",
 /*  77 */ "field ::= field AS LABEL",
 /*  78 */ "tableexpr ::= LPAREN tableexpr RPAREN",
 /*  79 */ "tableexpr ::= tableexpr joinclause tableexpr ON cond",
 /*  80 */ "tableexpr ::= tableexpr AS LABEL",
 /*  81 */ "tableexpr ::= LABEL",
 /*  82 */ "joinclause ::= INNER JOIN",
 /*  83 */ "joinclause ::= OUTER JOIN",
 /*  84 */ "joinclause ::= LEFT JOIN",
 /*  85 */ "joinclause ::= RIGHT JOIN",
 /*  86 */ "optionalquerymodifiers ::= optionalquerymodifiers whereclause",
 /*  87 */ "optionalquerymodifiers ::= optionalquerymodifiers limitclause",
 /*  88 */ "optionalquerymodifiers ::= optionalquerymodifiers havingclause",
 /*  89 */ "optionalquerymodifiers ::= optionalquerymodifiers groupclause",
 /*  90 */ "optionalquerymodifiers ::= optionalquerymodifiers orderclause",
 /*  91 */ "optionalquerymodifiers ::=",
 /*  92 */ "whereclause ::= WHERE cond",
 /*  93 */ "limitclause ::= LIMIT intnum COMMA intnum",
 /*  94 */ "havingclause ::= HAVING cond",
 /*  95 */ "groupclause ::= GROUP BY grouplist",
 /*  96 */ "orderclause ::= ORDER BY orderlist",
 /*  97 */ "optionalwhereclause ::= whereclause",
 /*  98 */ "optionalwhereclause ::=",
 /*  99 */ "cond ::= cond AND cond",
 /* 100 */ "cond ::= cond OR cond",
 /* 101 */ "cond ::= cond XOR cond",
 /* 102 */ "cond ::= expr EQUALS expr",
 /* 103 */ "cond ::= expr NOT_EQUAL expr",
 /* 104 */ "cond ::= expr UNEQUAL expr",
 /* 105 */ "cond ::= expr LESSER expr",
 /* 106 */ "cond ::= expr GREATER expr",
 /* 107 */ "cond ::= expr LESSER_EQUAL expr",
 /* 108 */ "cond ::= expr GREATER_EQUAL expr",
 /* 109 */ "cond ::= expr LIKE expr",
 /* 110 */ "cond ::= expr RLIKE expr",
 /* 111 */ "cond ::= expr BETWEEN expr AND expr",
 /* 112 */ "cond ::= expr NOT LIKE expr",
 /* 113 */ "cond ::= expr NOT RLIKE expr",
 /* 114 */ "cond ::= expr NOT BETWEEN expr AND expr",
 /* 115 */ "cond ::= LPAREN cond RPAREN",
 /* 116 */ "exprlist ::= exprlist COMMA expr",
 /* 117 */ "exprlist ::= expr",
 /* 118 */ "expr ::= literal",
 /* 119 */ "expr ::= LABEL",
 /* 120 */ "expr ::= LABEL LPAREN exprlist RPAREN",
 /* 121 */ "expr ::= LPAREN expr RPAREN",
 /* 122 */ "expr ::= expr PLUS expr",
 /* 123 */ "expr ::= expr MINUS expr",
 /* 124 */ "expr ::= expr MUL expr",
 /* 125 */ "expr ::= expr DIV expr",
 /* 126 */ "expr ::= expr MOD expr",
 /* 127 */ "expr ::= DISTINCT expr",
 /* 128 */ "intnum ::= LNUM",
 /* 129 */ "intnum ::= HNUM",
 /* 130 */ "literal ::= STRING",
 /* 131 */ "literal ::= intnum",
 /* 132 */ "literal ::= NULL",
 /* 133 */ "grouplist ::= grouplist COMMA expr",
 /* 134 */ "grouplist ::= expr",
 /* 135 */ "orderlist ::= orderlist COMMA orderelement",
 /* 136 */ "orderlist ::= orderelement",
 /* 137 */ "orderelement ::= expr ASC",
 /* 138 */ "orderelement ::= expr DESC",
 /* 139 */ "orderelement ::= expr",
};
#endif /* NDEBUG */

/*
** This function returns the symbolic name associated with a token
** value.
*/
const char *php_pdo_user_sql_parserTokenName(int tokenType){
#ifndef NDEBUG
  if( tokenType>0 && tokenType<(sizeof(yyTokenName)/sizeof(yyTokenName[0])) ){
    return yyTokenName[tokenType];
  }else{
    return "Unknown";
  }
#else
  return "";
#endif
}

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to php_pdo_user_sql_parser and php_pdo_user_sql_parserFree.
*/
void *php_pdo_user_sql_parserAlloc(void *(*mallocProc)(size_t)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(YYCODETYPE yymajor, YYMINORTYPE *yypminor){
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    case 97:
    case 98:
    case 99:
    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
    case 107:
    case 108:
    case 109:
    case 110:
    case 111:
    case 112:
    case 113:
    case 114:
    case 115:
    case 116:
    case 117:
    case 120:
    case 121:
    case 122:
    case 123:
    case 124:
    case 125:
    case 126:
    case 127:
    case 128:
    case 129:
    case 130:
    case 131:
    case 132:
    case 133:
    case 134:
#line 308 "pdo_user_sql_parser.lemon"
{ zval_ptr_dtor(&(yypminor->yy198)); }
#line 968 "pdo_user_sql_parser.c"
      break;
    case 118:
    case 119:
#line 394 "pdo_user_sql_parser.lemon"
{ zval_ptr_dtor(&(yypminor->yy122)[0]); zval_ptr_dtor(&(yypminor->yy122)[1]); efree((yypminor->yy122)); }
#line 974 "pdo_user_sql_parser.c"
      break;
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor( yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/* 
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from php_pdo_user_sql_parserAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void php_pdo_user_sql_parserFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
  (*freeProc)((void*)pParser);
}

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  int iLookAhead            /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  /* if( pParser->yyidx<0 ) return YY_NO_ACTION;  */
  i = yy_shift_ofst[stateno];
  if( i==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  if( iLookAhead==YYNOCODE ){
    return YY_NO_ACTION;
  }
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
#ifdef YYFALLBACK
    int iFallback;            /* Fallback token */
    if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
           && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
           yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
      }
#endif
      return yy_find_shift_action(pParser, iFallback);
    }
#endif
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  int iLookAhead            /* The look-ahead token */
){
  int i;
  /* int stateno = pParser->yystack[pParser->yyidx].stateno; */
 
  i = yy_reduce_ofst[stateno];
  if( i==YY_REDUCE_USE_DFLT ){
    return yy_default[stateno];
  }
  if( iLookAhead==YYNOCODE ){
    return YY_NO_ACTION;
  }
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer ot the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
  if( yypParser->yyidx>=YYSTACKDEPTH ){
     php_pdo_user_sql_parserARG_FETCH;
     yypParser->yyidx--;
#ifndef NDEBUG
     if( yyTraceFILE ){
       fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
     }
#endif
     while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
     /* Here code is inserted which will execute if the parser
     ** stack every overflows */
     php_pdo_user_sql_parserARG_STORE; /* Suppress warning about unused %extra_argument var */
     return;
  }
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = yyNewState;
  yytos->major = yyMajor;
  yytos->minor = *yypMinor;
#ifndef NDEBUG
  if( yyTraceFILE && yypParser->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE," %s",yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 96, 2 },
  { 96, 1 },
  { 97, 1 },
  { 97, 5 },
  { 97, 6 },
  { 97, 5 },
  { 97, 3 },
  { 97, 3 },
  { 97, 6 },
  { 97, 3 },
  { 98, 5 },
  { 104, 3 },
  { 104, 1 },
  { 108, 3 },
  { 110, 3 },
  { 110, 3 },
  { 110, 3 },
  { 110, 3 },
  { 110, 2 },
  { 110, 2 },
  { 110, 0 },
  { 109, 1 },
  { 109, 4 },
  { 109, 4 },
  { 109, 4 },
  { 109, 4 },
  { 109, 4 },
  { 109, 4 },
  { 109, 2 },
  { 109, 4 },
  { 109, 4 },
  { 109, 4 },
  { 109, 4 },
  { 109, 4 },
  { 109, 4 },
  { 109, 1 },
  { 109, 1 },
  { 109, 2 },
  { 109, 2 },
  { 109, 1 },
  { 109, 1 },
  { 109, 1 },
  { 109, 1 },
  { 109, 1 },
  { 109, 1 },
  { 109, 1 },
  { 109, 1 },
  { 109, 4 },
  { 109, 4 },
  { 109, 4 },
  { 109, 4 },
  { 113, 1 },
  { 113, 0 },
  { 114, 1 },
  { 114, 0 },
  { 112, 3 },
  { 112, 0 },
  { 115, 5 },
  { 115, 0 },
  { 117, 3 },
  { 117, 1 },
  { 103, 3 },
  { 103, 1 },
  { 118, 3 },
  { 101, 3 },
  { 101, 1 },
  { 119, 3 },
  { 100, 3 },
  { 100, 1 },
  { 121, 3 },
  { 123, 3 },
  { 123, 1 },
  { 99, 3 },
  { 99, 0 },
  { 105, 3 },
  { 105, 1 },
  { 124, 1 },
  { 124, 3 },
  { 106, 3 },
  { 106, 5 },
  { 106, 3 },
  { 106, 1 },
  { 125, 2 },
  { 125, 2 },
  { 125, 2 },
  { 125, 2 },
  { 107, 2 },
  { 107, 2 },
  { 107, 2 },
  { 107, 2 },
  { 107, 2 },
  { 107, 0 },
  { 127, 2 },
  { 128, 4 },
  { 129, 2 },
  { 130, 3 },
  { 131, 3 },
  { 102, 1 },
  { 102, 0 },
  { 126, 3 },
  { 126, 3 },
  { 126, 3 },
  { 126, 3 },
  { 126, 3 },
  { 126, 3 },
  { 126, 3 },
  { 126, 3 },
  { 126, 3 },
  { 126, 3 },
  { 126, 3 },
  { 126, 3 },
  { 126, 5 },
  { 126, 4 },
  { 126, 4 },
  { 126, 6 },
  { 126, 3 },
  { 122, 3 },
  { 122, 1 },
  { 120, 1 },
  { 120, 1 },
  { 120, 4 },
  { 120, 3 },
  { 120, 3 },
  { 120, 3 },
  { 120, 3 },
  { 120, 3 },
  { 120, 3 },
  { 120, 2 },
  { 116, 1 },
  { 116, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 132, 3 },
  { 132, 1 },
  { 133, 3 },
  { 133, 1 },
  { 134, 2 },
  { 134, 2 },
  { 134, 1 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  php_pdo_user_sql_parserARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<sizeof(yyRuleName)/sizeof(yyRuleName[0]) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

#ifndef NDEBUG
  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  */
  memset(&yygotominor, 0, sizeof(yygotominor));
#endif

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 0:
#line 305 "pdo_user_sql_parser.lemon"
{ pusp_do_terminal_statement(&return_value, yymsp[-1].minor.yy198, 1); }
#line 1341 "pdo_user_sql_parser.c"
        break;
      case 1:
#line 306 "pdo_user_sql_parser.lemon"
{ pusp_do_terminal_statement(&return_value, yymsp[0].minor.yy198, 0); }
#line 1346 "pdo_user_sql_parser.c"
        break;
      case 2:
      case 76:
      case 92:
      case 94:
      case 95:
      case 96:
      case 97:
      case 118:
      case 131:
#line 309 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = yymsp[0].minor.yy198; }
#line 1359 "pdo_user_sql_parser.c"
        break;
      case 3:
#line 310 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_insert_select_statement(pusp_zvalize_token(&yymsp[-2].minor.yy0), yymsp[-1].minor.yy198, yymsp[0].minor.yy198); }
#line 1364 "pdo_user_sql_parser.c"
        break;
      case 4:
#line 311 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_insert_statement(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1369 "pdo_user_sql_parser.c"
        break;
      case 5:
#line 312 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_update_statement(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-1].minor.yy198, yymsp[0].minor.yy198); }
#line 1374 "pdo_user_sql_parser.c"
        break;
      case 6:
#line 313 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_delete_statement(pusp_zvalize_token(&yymsp[-1].minor.yy0), yymsp[0].minor.yy198); }
#line 1379 "pdo_user_sql_parser.c"
        break;
      case 7:
#line 314 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_rename_statement(yymsp[0].minor.yy198); }
#line 1384 "pdo_user_sql_parser.c"
        break;
      case 8:
#line 315 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_create_statement(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-1].minor.yy198); }
#line 1389 "pdo_user_sql_parser.c"
        break;
      case 9:
#line 316 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_drop_statement(pusp_zvalize_token(&yymsp[0].minor.yy0)); }
#line 1394 "pdo_user_sql_parser.c"
        break;
      case 10:
#line 319 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_select_statement(yymsp[-3].minor.yy198,yymsp[-1].minor.yy198,yymsp[0].minor.yy198); }
#line 1399 "pdo_user_sql_parser.c"
        break;
      case 11:
      case 59:
      case 67:
      case 74:
      case 116:
      case 133:
      case 135:
#line 322 "pdo_user_sql_parser.lemon"
{ add_next_index_zval(yymsp[-2].minor.yy198, yymsp[0].minor.yy198); yygotominor.yy198 = yymsp[-2].minor.yy198; }
#line 1410 "pdo_user_sql_parser.c"
        break;
      case 12:
      case 60:
      case 68:
      case 75:
      case 117:
      case 134:
      case 136:
#line 323 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); array_init(yygotominor.yy198); add_next_index_zval(yygotominor.yy198, yymsp[0].minor.yy198); }
#line 1421 "pdo_user_sql_parser.c"
        break;
      case 13:
#line 326 "pdo_user_sql_parser.lemon"
{ add_assoc_zval(yymsp[-1].minor.yy198, "name", pusp_zvalize_token(&yymsp[-2].minor.yy0)); add_assoc_zval(yymsp[-1].minor.yy198, "flags", yymsp[0].minor.yy198); yygotominor.yy198 = yymsp[-1].minor.yy198; }
#line 1426 "pdo_user_sql_parser.c"
        break;
      case 14:
#line 329 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-2].minor.yy198, "not null", 1); yygotominor.yy198 = yymsp[-2].minor.yy198; }
#line 1431 "pdo_user_sql_parser.c"
        break;
      case 15:
#line 330 "pdo_user_sql_parser.lemon"
{ add_assoc_zval(yymsp[-2].minor.yy198, "default", yymsp[0].minor.yy198); yygotominor.yy198 = yymsp[-2].minor.yy198; }
#line 1436 "pdo_user_sql_parser.c"
        break;
      case 16:
#line 331 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-2].minor.yy198, "primary key", 1); yygotominor.yy198 = yymsp[-2].minor.yy198; }
#line 1441 "pdo_user_sql_parser.c"
        break;
      case 17:
#line 332 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-2].minor.yy198, "unique key", 1); yygotominor.yy198 = yymsp[-2].minor.yy198; }
#line 1446 "pdo_user_sql_parser.c"
        break;
      case 18:
#line 333 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-1].minor.yy198, "key", 1); yygotominor.yy198 = yymsp[-1].minor.yy198; }
#line 1451 "pdo_user_sql_parser.c"
        break;
      case 19:
#line 334 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-1].minor.yy198, "auto_increment", 1); yygotominor.yy198 = yymsp[-1].minor.yy198; }
#line 1456 "pdo_user_sql_parser.c"
        break;
      case 20:
#line 335 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); array_init(yygotominor.yy198); }
#line 1461 "pdo_user_sql_parser.c"
        break;
      case 21:
#line 338 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("bit", NULL, NULL); }
#line 1466 "pdo_user_sql_parser.c"
        break;
      case 22:
#line 339 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_num("int", yymsp[-2].minor.yy198, yymsp[-1].minor.yy198, yymsp[0].minor.yy198); }
#line 1471 "pdo_user_sql_parser.c"
        break;
      case 23:
#line 340 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_num("integer", yymsp[-2].minor.yy198, yymsp[-1].minor.yy198, yymsp[0].minor.yy198); }
#line 1476 "pdo_user_sql_parser.c"
        break;
      case 24:
#line 341 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_num("tinyint", yymsp[-2].minor.yy198, yymsp[-1].minor.yy198, yymsp[0].minor.yy198); }
#line 1481 "pdo_user_sql_parser.c"
        break;
      case 25:
#line 342 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_num("smallint", yymsp[-2].minor.yy198, yymsp[-1].minor.yy198, yymsp[0].minor.yy198); }
#line 1486 "pdo_user_sql_parser.c"
        break;
      case 26:
#line 343 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_num("mediumint", yymsp[-2].minor.yy198, yymsp[-1].minor.yy198, yymsp[0].minor.yy198); }
#line 1491 "pdo_user_sql_parser.c"
        break;
      case 27:
#line 344 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_num("bigint", yymsp[-2].minor.yy198, yymsp[-1].minor.yy198, yymsp[0].minor.yy198); }
#line 1496 "pdo_user_sql_parser.c"
        break;
      case 28:
#line 345 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("year", "precision", yymsp[0].minor.yy198); }
#line 1501 "pdo_user_sql_parser.c"
        break;
      case 29:
#line 346 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_num("float", yymsp[-2].minor.yy198, yymsp[-1].minor.yy198, yymsp[0].minor.yy198); }
#line 1506 "pdo_user_sql_parser.c"
        break;
      case 30:
#line 347 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_num("real", yymsp[-2].minor.yy198, yymsp[-1].minor.yy198, yymsp[0].minor.yy198); }
#line 1511 "pdo_user_sql_parser.c"
        break;
      case 31:
#line 348 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_num("decimal", yymsp[-2].minor.yy198, yymsp[-1].minor.yy198, yymsp[0].minor.yy198); }
#line 1516 "pdo_user_sql_parser.c"
        break;
      case 32:
#line 349 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_num("double", yymsp[-2].minor.yy198, yymsp[-1].minor.yy198, yymsp[0].minor.yy198); }
#line 1521 "pdo_user_sql_parser.c"
        break;
      case 33:
#line 350 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("char", "length", yymsp[-1].minor.yy198); }
#line 1526 "pdo_user_sql_parser.c"
        break;
      case 34:
#line 351 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("varchar", "length", yymsp[-1].minor.yy198); }
#line 1531 "pdo_user_sql_parser.c"
        break;
      case 35:
#line 352 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("text", "date", NULL); }
#line 1536 "pdo_user_sql_parser.c"
        break;
      case 36:
#line 353 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("text", "time", NULL); }
#line 1541 "pdo_user_sql_parser.c"
        break;
      case 37:
#line 354 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("datetime", "precision", yymsp[0].minor.yy198); }
#line 1546 "pdo_user_sql_parser.c"
        break;
      case 38:
#line 355 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("timestamp", "precision", yymsp[0].minor.yy198); }
#line 1551 "pdo_user_sql_parser.c"
        break;
      case 39:
#line 356 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("text", "precision", NULL); }
#line 1556 "pdo_user_sql_parser.c"
        break;
      case 40:
#line 357 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "tiny", 1); yygotominor.yy198 = pusp_do_declare_type("text", "precision", p); }
#line 1561 "pdo_user_sql_parser.c"
        break;
      case 41:
#line 358 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "medium", 1); yygotominor.yy198 = pusp_do_declare_type("text", "precision", p); }
#line 1566 "pdo_user_sql_parser.c"
        break;
      case 42:
#line 359 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "long", 1); yygotominor.yy198 = pusp_do_declare_type("text", "precision", p); }
#line 1571 "pdo_user_sql_parser.c"
        break;
      case 43:
#line 360 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("blob", "precision", NULL); }
#line 1576 "pdo_user_sql_parser.c"
        break;
      case 44:
#line 361 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "tiny", 1); yygotominor.yy198 = pusp_do_declare_type("blob", "precision", p); }
#line 1581 "pdo_user_sql_parser.c"
        break;
      case 45:
#line 362 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "medium", 1); yygotominor.yy198 = pusp_do_declare_type("blob", "precision", p); }
#line 1586 "pdo_user_sql_parser.c"
        break;
      case 46:
#line 363 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "long", 1); yygotominor.yy198 = pusp_do_declare_type("blob", "precision", p); }
#line 1591 "pdo_user_sql_parser.c"
        break;
      case 47:
#line 364 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("binary", "length", yymsp[-1].minor.yy198); }
#line 1596 "pdo_user_sql_parser.c"
        break;
      case 48:
#line 365 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("varbinary", "length", yymsp[-1].minor.yy198); }
#line 1601 "pdo_user_sql_parser.c"
        break;
      case 49:
#line 366 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("set", "flags", yymsp[-1].minor.yy198); }
#line 1606 "pdo_user_sql_parser.c"
        break;
      case 50:
#line 367 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_declare_type("enum", "values", yymsp[-1].minor.yy198); }
#line 1611 "pdo_user_sql_parser.c"
        break;
      case 51:
      case 53:
#line 370 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); ZVAL_TRUE(yygotominor.yy198); }
#line 1617 "pdo_user_sql_parser.c"
        break;
      case 52:
      case 54:
#line 371 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); ZVAL_FALSE(yygotominor.yy198); }
#line 1623 "pdo_user_sql_parser.c"
        break;
      case 55:
      case 69:
      case 72:
      case 78:
      case 115:
      case 121:
#line 378 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = yymsp[-1].minor.yy198; }
#line 1633 "pdo_user_sql_parser.c"
        break;
      case 56:
      case 58:
      case 73:
      case 91:
      case 98:
      case 132:
#line 379 "pdo_user_sql_parser.lemon"
{ TSRMLS_FETCH(); yygotominor.yy198 = EG(uninitialized_zval_ptr); }
#line 1643 "pdo_user_sql_parser.c"
        break;
      case 57:
#line 382 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); array_init(yygotominor.yy198); add_assoc_zval(yygotominor.yy198, "length", yymsp[-3].minor.yy198); add_assoc_zval(yygotominor.yy198, "decimals", yymsp[-1].minor.yy198); }
#line 1648 "pdo_user_sql_parser.c"
        break;
      case 61:
#line 390 "pdo_user_sql_parser.lemon"
{ pusp_do_push_labeled_zval(yymsp[-2].minor.yy198, yymsp[0].minor.yy122); yygotominor.yy198 = yymsp[-2].minor.yy198; }
#line 1653 "pdo_user_sql_parser.c"
        break;
      case 62:
      case 65:
#line 391 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); array_init(yygotominor.yy198); pusp_do_push_labeled_zval(yygotominor.yy198, yymsp[0].minor.yy122); }
#line 1659 "pdo_user_sql_parser.c"
        break;
      case 63:
#line 395 "pdo_user_sql_parser.lemon"
{ zval **tmp = safe_emalloc(2, sizeof(zval*), 0); tmp[0] = pusp_zvalize_token(&yymsp[-2].minor.yy0); tmp[1] = pusp_zvalize_token(&yymsp[0].minor.yy0); yygotominor.yy122 = tmp; }
#line 1664 "pdo_user_sql_parser.c"
        break;
      case 64:
#line 398 "pdo_user_sql_parser.lemon"
{ pusp_do_push_labeled_zval(yymsp[-2].minor.yy198, (zval**)yymsp[0].minor.yy122); yygotominor.yy198 = yymsp[-2].minor.yy198; }
#line 1669 "pdo_user_sql_parser.c"
        break;
      case 66:
#line 403 "pdo_user_sql_parser.lemon"
{ zval **tmp = safe_emalloc(2, sizeof(zval*), 0); tmp[0] = pusp_zvalize_token(&yymsp[-2].minor.yy0); tmp[1] = yymsp[0].minor.yy198; yygotominor.yy122 = tmp; }
#line 1674 "pdo_user_sql_parser.c"
        break;
      case 70:
#line 413 "pdo_user_sql_parser.lemon"
{ add_next_index_zval(yymsp[-2].minor.yy198, pusp_zvalize_token(&yymsp[0].minor.yy0)); yygotominor.yy198 = yymsp[-2].minor.yy198; }
#line 1679 "pdo_user_sql_parser.c"
        break;
      case 71:
#line 414 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); array_init(yygotominor.yy198); add_next_index_zval(yygotominor.yy198, pusp_zvalize_token(&yymsp[0].minor.yy0)); }
#line 1684 "pdo_user_sql_parser.c"
        break;
      case 77:
#line 426 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); array_init(yygotominor.yy198); add_assoc_stringl(yygotominor.yy198, "type", "alias", sizeof("alias") - 1, 1); add_assoc_zval(yygotominor.yy198, "field", yymsp[-2].minor.yy198); add_assoc_zval(yygotominor.yy198, "as", pusp_zvalize_token(&yymsp[0].minor.yy0)); }
#line 1689 "pdo_user_sql_parser.c"
        break;
      case 79:
#line 430 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_join_expression(yymsp[-4].minor.yy198, yymsp[-3].minor.yy198, yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1694 "pdo_user_sql_parser.c"
        break;
      case 80:
#line 431 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); array_init(yygotominor.yy198); add_assoc_stringl(yygotominor.yy198, "type", "alias", sizeof("alias") - 1, 1); add_assoc_zval(yygotominor.yy198, "table", yymsp[-2].minor.yy198); add_assoc_zval(yygotominor.yy198, "as", pusp_zvalize_token(&yymsp[0].minor.yy0)); }
#line 1699 "pdo_user_sql_parser.c"
        break;
      case 81:
      case 119:
      case 130:
#line 432 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_zvalize_token(&yymsp[0].minor.yy0); }
#line 1706 "pdo_user_sql_parser.c"
        break;
      case 82:
#line 443 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_zvalize_static_string("inner"); }
#line 1711 "pdo_user_sql_parser.c"
        break;
      case 83:
#line 444 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_zvalize_static_string("outer"); }
#line 1716 "pdo_user_sql_parser.c"
        break;
      case 84:
#line 445 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_zvalize_static_string("left"); }
#line 1721 "pdo_user_sql_parser.c"
        break;
      case 85:
#line 446 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_zvalize_static_string("right"); }
#line 1726 "pdo_user_sql_parser.c"
        break;
      case 86:
#line 449 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_add_query_modifier(yymsp[-1].minor.yy198, "where", yymsp[0].minor.yy198); }
#line 1731 "pdo_user_sql_parser.c"
        break;
      case 87:
#line 450 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_add_query_modifier(yymsp[-1].minor.yy198, "limit", yymsp[0].minor.yy198); }
#line 1736 "pdo_user_sql_parser.c"
        break;
      case 88:
#line 451 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_add_query_modifier(yymsp[-1].minor.yy198, "having", yymsp[0].minor.yy198); }
#line 1741 "pdo_user_sql_parser.c"
        break;
      case 89:
#line 452 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_add_query_modifier(yymsp[-1].minor.yy198, "group-by", yymsp[0].minor.yy198); }
#line 1746 "pdo_user_sql_parser.c"
        break;
      case 90:
#line 453 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_add_query_modifier(yymsp[-1].minor.yy198, "order-by", yymsp[0].minor.yy198); }
#line 1751 "pdo_user_sql_parser.c"
        break;
      case 93:
#line 460 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); array_init(yygotominor.yy198); add_assoc_zval(yygotominor.yy198, "from", yymsp[-2].minor.yy198); add_assoc_zval(yygotominor.yy198, "to", yymsp[0].minor.yy198); }
#line 1756 "pdo_user_sql_parser.c"
        break;
      case 99:
#line 476 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "and", yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1761 "pdo_user_sql_parser.c"
        break;
      case 100:
#line 477 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "or", yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1766 "pdo_user_sql_parser.c"
        break;
      case 101:
#line 478 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "xor", yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1771 "pdo_user_sql_parser.c"
        break;
      case 102:
#line 480 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "=", yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1776 "pdo_user_sql_parser.c"
        break;
      case 103:
#line 481 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "!=", yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1781 "pdo_user_sql_parser.c"
        break;
      case 104:
#line 482 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "<>", yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1786 "pdo_user_sql_parser.c"
        break;
      case 105:
#line 483 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "<", yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1791 "pdo_user_sql_parser.c"
        break;
      case 106:
#line 484 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, ">", yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1796 "pdo_user_sql_parser.c"
        break;
      case 107:
#line 485 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "<=", yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1801 "pdo_user_sql_parser.c"
        break;
      case 108:
#line 486 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, ">=", yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1806 "pdo_user_sql_parser.c"
        break;
      case 109:
#line 487 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "like", yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1811 "pdo_user_sql_parser.c"
        break;
      case 110:
#line 488 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "rlike", yymsp[-2].minor.yy198, yymsp[0].minor.yy198); }
#line 1816 "pdo_user_sql_parser.c"
        break;
      case 111:
#line 489 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "between", yymsp[-4].minor.yy198, yymsp[-2].minor.yy198); add_assoc_zval(yygotominor.yy198, "op-c", yymsp[0].minor.yy198); }
#line 1821 "pdo_user_sql_parser.c"
        break;
      case 112:
#line 490 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "not like", yymsp[-3].minor.yy198, yymsp[0].minor.yy198); }
#line 1826 "pdo_user_sql_parser.c"
        break;
      case 113:
#line 491 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "not rlike", yymsp[-3].minor.yy198, yymsp[0].minor.yy198); }
#line 1831 "pdo_user_sql_parser.c"
        break;
      case 114:
#line 492 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy198, "not between", yymsp[-5].minor.yy198, yymsp[-2].minor.yy198);  add_assoc_zval(yygotominor.yy198, "op-c", yymsp[0].minor.yy198); }
#line 1836 "pdo_user_sql_parser.c"
        break;
      case 120:
#line 502 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_do_function(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-1].minor.yy198); }
#line 1841 "pdo_user_sql_parser.c"
        break;
      case 122:
#line 504 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy198,yymsp[-2].minor.yy198,"+",yymsp[0].minor.yy198); }
#line 1846 "pdo_user_sql_parser.c"
        break;
      case 123:
#line 505 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy198,yymsp[-2].minor.yy198,"-",yymsp[0].minor.yy198); }
#line 1851 "pdo_user_sql_parser.c"
        break;
      case 124:
#line 506 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy198,yymsp[-2].minor.yy198,"*",yymsp[0].minor.yy198); }
#line 1856 "pdo_user_sql_parser.c"
        break;
      case 125:
#line 507 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy198,yymsp[-2].minor.yy198,"/",yymsp[0].minor.yy198); }
#line 1861 "pdo_user_sql_parser.c"
        break;
      case 126:
#line 508 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy198,yymsp[-2].minor.yy198,"%",yymsp[0].minor.yy198); }
#line 1866 "pdo_user_sql_parser.c"
        break;
      case 127:
#line 509 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); array_init(yygotominor.yy198); add_assoc_stringl(yygotominor.yy198, "type", "distinct", sizeof("distinct") - 1, 1); add_assoc_zval(yygotominor.yy198, "distinct", yymsp[0].minor.yy198); }
#line 1871 "pdo_user_sql_parser.c"
        break;
      case 128:
#line 512 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_zvalize_lnum(&yymsp[0].minor.yy0); }
#line 1876 "pdo_user_sql_parser.c"
        break;
      case 129:
#line 513 "pdo_user_sql_parser.lemon"
{ yygotominor.yy198 = pusp_zvalize_hnum(&yymsp[0].minor.yy0); }
#line 1881 "pdo_user_sql_parser.c"
        break;
      case 137:
#line 529 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); add_assoc_stringl(yygotominor.yy198, "direction", "asc", sizeof("asc") - 1, 1); add_assoc_zval(yygotominor.yy198, "by", yymsp[-1].minor.yy198); }
#line 1886 "pdo_user_sql_parser.c"
        break;
      case 138:
#line 530 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); add_assoc_stringl(yygotominor.yy198, "direction", "desc", sizeof("desc") - 1, 1); add_assoc_zval(yygotominor.yy198, "by", yymsp[-1].minor.yy198); }
#line 1891 "pdo_user_sql_parser.c"
        break;
      case 139:
#line 531 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy198); add_assoc_null(yygotominor.yy198, "direction"); add_assoc_zval(yygotominor.yy198, "by", yymsp[0].minor.yy198); }
#line 1896 "pdo_user_sql_parser.c"
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = yyact;
      yymsp->major = yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else if( yyact == YYNSTATE + YYNRULE + 1 ){
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  php_pdo_user_sql_parserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  php_pdo_user_sql_parserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  php_pdo_user_sql_parserARG_FETCH;
#define TOKEN (yyminor.yy0)
#line 301 "pdo_user_sql_parser.lemon"

	RETVAL_FALSE;
#line 1957 "pdo_user_sql_parser.c"
  php_pdo_user_sql_parserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  php_pdo_user_sql_parserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  php_pdo_user_sql_parserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "php_pdo_user_sql_parserAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void php_pdo_user_sql_parser(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  php_pdo_user_sql_parserTOKENTYPE yyminor       /* The value for the token */
  php_pdo_user_sql_parserARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
    if( yymajor==0 ) return;
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  php_pdo_user_sql_parserARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,yymajor);
    if( yyact<YYNSTATE ){
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      if( yyendofinput && yypParser->yyidx>=0 ){
        yymajor = 0;
      }else{
        yymajor = YYNOCODE;
      }
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else if( yyact == YY_ERROR_ACTION ){
      int yymx;
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_shift_action(yypParser,YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }else{
      yy_accept(yypParser);
      yymajor = YYNOCODE;
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}
