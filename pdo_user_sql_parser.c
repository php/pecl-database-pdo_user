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

static inline zval *pusp_zvalize_dnum(php_pdo_user_sql_token *T) {
	zval *ret;
	double val = 0, div = 0;
	int sign = 1;
	unsigned char *s = T->token, *e = T->token + T->token_len;

	MAKE_STD_ZVAL(ret);
	if (*s == '-') {
		sign = -1;
		s++;
	}
	while (s < e) {
		/* illegal characters aren't possible, the lexer wouldn't give us any */
		if (*s == '.') {
			div = 1;
			s++;
			continue;
		}
		val *= 10;
		val += *s - '0';
		div *= 10;
		s++;
	}
	ZVAL_DOUBLE(ret, sign * val / div);
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
	if (Z_TYPE_PP(return_value) == IS_ARRAY) {
		/* Toss out 2nd and subsequent statements */
		zval_ptr_dtor(&statement);
		return;
	}
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

static zval *pusp_do_field(php_pdo_user_sql_token *database, php_pdo_user_sql_token *table, php_pdo_user_sql_token *field)
{
	zval *ret;

	if (!database && !table) {
		return pusp_zvalize_token(field);
	}
	MAKE_STD_ZVAL(ret);
	array_init(ret);
	if (database) {
		add_assoc_zval(ret, "database", pusp_zvalize_token(database));
	}
	add_assoc_zval(ret, "table", pusp_zvalize_token(table));
	add_assoc_zval(ret, "field", pusp_zvalize_token(field));

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

#line 345 "pdo_user_sql_parser.c"
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
#define YYNOCODE 140
#define YYACTIONTYPE unsigned short int
#define php_pdo_user_sql_parserTOKENTYPE php_pdo_user_sql_token
typedef union {
  php_pdo_user_sql_parserTOKENTYPE yy0;
  zval* yy92;
  zval** yy204;
  int yy279;
} YYMINORTYPE;
#define YYSTACKDEPTH 100
#define php_pdo_user_sql_parserARG_SDECL zval *return_value;
#define php_pdo_user_sql_parserARG_PDECL ,zval *return_value
#define php_pdo_user_sql_parserARG_FETCH zval *return_value = yypParser->return_value
#define php_pdo_user_sql_parserARG_STORE yypParser->return_value = return_value
#define YYNSTATE 304
#define YYNRULE 147
#define YYERRORSYMBOL 97
#define YYERRSYMDT yy279
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
 /*     0 */   289,  452,    1,    3,   33,   35,   37,  304,   18,   29,
 /*    10 */    63,   65,   67,  206,  207,  216,  220,  224,  228,  232,
 /*    20 */   236,  238,  247,  251,  255,  259,  263,  267,  268,  269,
 /*    30 */   271,  273,  274,  275,  276,  277,  278,  279,  280,  281,
 /*    40 */   285,  296,   92,   39,  111,  117,    5,  105,   61,  109,
 /*    50 */    30,   20,   41,   42,   44,   10,    6,   88,   84,   86,
 /*    60 */    70,   76,   78,   92,   82,   47,   33,   35,   37,   32,
 /*    70 */    18,   29,   51,    4,  122,  123,   50,   51,   88,   84,
 /*    80 */    86,   70,   76,   78,  155,   82,  305,   33,   35,   37,
 /*    90 */    39,   18,   29,   63,   65,   67,  178,  101,   20,   41,
 /*   100 */    42,   44,  161,    7,  173,  176,    9,  185,  301,  103,
 /*   110 */    90,  184,   47,   43,   33,   35,   37,   32,   18,   29,
 /*   120 */   130,   54,    2,   33,   35,   37,  130,   18,   29,   52,
 /*   130 */   196,  130,   72,   74,   80,   19,   11,  140,   19,  129,
 /*   140 */    45,   19,  142,   45,  153,  128,   45,  121,  154,   25,
 /*   150 */    28,   16,   14,   72,   74,   80,    8,   99,   13,   46,
 /*   160 */    43,  119,   46,  125,   19,   46,  142,   12,   15,   45,
 /*   170 */    33,   35,   37,   17,   18,   29,  198,  200,  204,  202,
 /*   180 */   205,   33,   35,   37,  149,   18,   29,   19,   46,  147,
 /*   190 */    22,  132,   45,  134,  136,  138,   28,  132,   26,  134,
 /*   200 */   136,  138,  132,   21,  134,  136,  138,   23,   97,   93,
 /*   210 */    95,   46,   56,   57,   58,   59,   60,   51,   53,   19,
 /*   220 */    61,   19,   19,   24,   45,  166,   45,   45,   69,  107,
 /*   230 */    69,   69,  165,   19,   62,  106,   64,   66,   45,  175,
 /*   240 */   127,  143,   69,   46,   19,   46,   46,   19,   68,   45,
 /*   250 */   127,  143,   45,  104,  108,   19,   69,   46,  171,  102,
 /*   260 */    45,   19,  110,  112,  116,  171,   45,   19,   46,  146,
 /*   270 */   121,   46,   45,   18,   29,   19,   69,  113,   19,   46,
 /*   280 */    45,  157,  148,   45,  153,   46,  124,   49,  150,   55,
 /*   290 */   142,   46,   19,   41,   42,   44,  164,   45,  159,   46,
 /*   300 */    19,   48,   46,   19,   16,   45,   47,  126,   45,   31,
 /*   310 */    19,  114,   34,   19,  172,   45,   46,  118,   45,   36,
 /*   320 */    19,   27,   38,  120,   46,   45,   19,   46,  189,   40,
 /*   330 */   126,   45,  300,   19,   46,   71,   19,   46,   45,  133,
 /*   340 */   131,   45,   73,  141,   46,   75,  144,   19,  199,  151,
 /*   350 */    46,  135,   45,   45,   43,  145,   77,   46,   19,   19,
 /*   360 */    46,   19,  191,   45,   45,  152,   45,   79,   81,   19,
 /*   370 */    83,   46,   46,  137,   45,   19,   41,   42,   85,  190,
 /*   380 */    45,   19,   46,   46,   87,   46,   45,  293,  139,   19,
 /*   390 */    89,   19,   19,   46,   45,  293,   45,   45,   91,   46,
 /*   400 */    94,   96,   19,  160,  292,   46,  156,   45,  163,   19,
 /*   410 */   158,   98,  299,   46,   45,   46,   46,  295,  100,   19,
 /*   420 */    19,  162,   45,  291,   45,   45,   46,  294,  115,  170,
 /*   430 */   295,  168,   45,   46,  169,   45,  298,  167,  174,   61,
 /*   440 */   177,   46,  181,   46,   46,  179,  180,  182,  183,  186,
 /*   450 */   187,   46,  188,  194,   46,  193,  192,  195,  197,  213,
 /*   460 */   201,  208,  203,  212,  209,  211,  210,  215,  217,  219,
 /*   470 */   214,  218,  221,  223,  222,  225,  227,  226,  230,  229,
 /*   480 */   242,  233,  231,  234,  244,  246,  235,  237,  302,  239,
 /*   490 */   240,  260,  243,  241,  262,  249,  264,  245,  248,  266,
 /*   500 */   282,  250,  253,  252,  254,  284,  257,  256,  258,  270,
 /*   510 */   272,  261,  265,  286,  288,  283,  290,  288,  287,  297,
 /*   520 */   288,  303,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    39,   98,   99,  100,   25,   26,   27,    0,   29,   30,
 /*    10 */    12,   13,   14,   52,   53,   54,   55,   56,   57,   58,
 /*    20 */    59,   60,   61,   62,   63,   64,   65,   66,   67,   68,
 /*    30 */    69,   70,   71,   72,   73,   74,   75,   76,   77,   78,
 /*    40 */    79,   80,    1,   24,    2,    3,   10,    5,    6,    7,
 /*    50 */    31,   32,   33,   34,   35,  102,   32,   16,   17,   18,
 /*    60 */    19,   20,   21,    1,   23,   46,   25,   26,   27,   28,
 /*    70 */    29,   30,    8,    9,   95,   96,  123,    8,   16,   17,
 /*    80 */    18,   19,   20,   21,   31,   23,    0,   25,   26,   27,
 /*    90 */    24,   29,   30,   12,   13,   14,  105,   31,   32,   33,
 /*   100 */    34,   35,   38,  101,   40,   41,   37,   43,   44,   28,
 /*   110 */    12,  120,   46,   94,   25,   26,   27,   28,   29,   30,
 /*   120 */    15,  108,   36,   25,   26,   27,   15,   29,   30,  107,
 /*   130 */     1,   15,   91,   92,   93,  113,   11,  100,  113,   28,
 /*   140 */   118,  113,  129,  118,  122,  108,  118,  122,  126,   31,
 /*   150 */   122,   11,  124,   91,   92,   93,  100,   12,   31,  137,
 /*   160 */    94,  136,  137,  138,  113,  137,  129,  123,   28,  118,
 /*   170 */    25,   26,   27,  122,   29,   30,   47,   48,   49,   50,
 /*   180 */    51,   25,   26,   27,   11,   29,   30,  113,  137,   84,
 /*   190 */    32,   86,  118,   88,   89,   90,  122,   86,  124,   88,
 /*   200 */    89,   90,   86,   85,   88,   89,   90,   85,   16,   17,
 /*   210 */    18,  137,  130,  131,  132,  133,  134,    8,   45,  113,
 /*   220 */     6,  113,  113,   32,  118,   11,  118,  118,  122,   11,
 /*   230 */   122,  122,  104,  113,  128,  118,  128,  128,  118,  104,
 /*   240 */    31,   32,  122,  137,  113,  137,  137,  113,  128,  118,
 /*   250 */    31,   32,  118,  122,  118,  113,  122,  137,  130,  128,
 /*   260 */   118,  113,  128,    4,  122,  130,  118,  113,  137,  108,
 /*   270 */   122,  137,  118,   29,   30,  113,  122,  135,  113,  137,
 /*   280 */   118,   11,  128,  118,  122,  137,  138,  122,  126,  109,
 /*   290 */   129,  137,  113,   33,   34,   35,  103,  118,   28,  137,
 /*   300 */   113,  122,  137,  113,   11,  118,   46,  127,  118,  122,
 /*   310 */   113,   11,  122,  113,  121,  118,  137,    4,  118,  122,
 /*   320 */   113,   28,  122,   11,  137,  118,  113,  137,  106,  122,
 /*   330 */   127,  118,  110,  113,  137,  122,  113,  137,  118,   87,
 /*   340 */    32,  118,  122,   28,  137,  122,   85,  113,  113,   15,
 /*   350 */   137,   87,  118,  118,   94,   32,  122,  137,  113,  113,
 /*   360 */   137,  113,   11,  118,  118,   32,  118,  122,  122,  113,
 /*   370 */   122,  137,  137,   87,  118,  113,   33,   34,  122,   28,
 /*   380 */   118,  113,  137,  137,  122,  137,  118,   11,   87,  113,
 /*   390 */   122,  113,  113,  137,  118,   11,  118,  118,  122,  137,
 /*   400 */   122,  122,  113,   32,   28,  137,  125,  118,   39,  113,
 /*   410 */    32,  122,   28,  137,  118,  137,  137,  113,  122,  113,
 /*   420 */   113,   32,  118,  119,  118,  118,  137,  113,  122,  122,
 /*   430 */   113,   32,  118,  137,   19,  118,  119,  121,   32,    6,
 /*   440 */    42,  137,   32,  137,  137,   11,  120,   83,   32,   42,
 /*   450 */    32,  137,   31,  111,  137,   32,  110,  112,   46,   31,
 /*   460 */    49,  114,   49,   81,  115,   82,  116,   28,  114,  116,
 /*   470 */   118,  115,  114,  116,  115,  114,  116,  115,  115,  114,
 /*   480 */    31,  114,  116,  115,   11,   28,  116,  114,   42,  117,
 /*   490 */   115,   31,  118,  116,   28,  115,   31,  118,  117,   28,
 /*   500 */    31,  116,  115,  117,  116,   28,  115,  117,  116,  114,
 /*   510 */   114,  118,  118,   31,   28,  118,   31,  139,  118,   31,
 /*   520 */   139,   32,
};
#define YY_SHIFT_USE_DFLT (-40)
static const short yy_shift_ofst[] = {
 /*     0 */    64,   86,    7,  -40,   36,   24,   53,   69,  -40,  127,
 /*    10 */   125,  127,  -40,   19,  140,  -40,   19,  156,   19,  -40,
 /*    20 */   118,  158,  122,  191,  -40,   19,  293,  -40,  156,   19,
 /*    30 */    19,   89,  -40,   19,  244,   19,  244,   19,  244,   19,
 /*    40 */   156,  -40,  -40,  -40,  -40,  -40,  -40,  -40,  -40,  -40,
 /*    50 */   -40,   19,  173,  219,  116,   42,  -40,  -40,  -40,  -40,
 /*    60 */   -40,   66,   -2,   66,  -40,   66,  -40,   66,  -40,   62,
 /*    70 */    19,  156,   19,  156,   19,  156,   19,  156,   19,  156,
 /*    80 */    19,  156,   19,  156,   19,  156,   19,  156,   19,   98,
 /*    90 */    19,  156,  192,   19,  156,   19,  156,   19,  145,   19,
 /*   100 */   156,   66,   81,  -40,   41,  343,  218,  343,  -40,   66,
 /*   110 */    -2,  259,   19,  300,   19,  156,  156,  313,   19,  312,
 /*   120 */    19,  -21,  -40,  -40,  -40,  -40,  219,  209,  111,  -40,
 /*   130 */   308,  -40,  252,  -40,  264,  -40,  286,  -40,  301,  -40,
 /*   140 */   315,  -40,  -40,  261,  323,  -40,  105,   66,   -2,   19,
 /*   150 */   334,  333,  -40,  156,  334,  371,  270,  378,  -40,  -40,
 /*   160 */   -40,  389,  369,  399,  214,  -40,  399,  -40,  415,   19,
 /*   170 */   156,  -40,  -40,  406,  433,  -40,  398,  410,  434,  410,
 /*   180 */   -40,  364,  416,  -40,  -40,  407,  418,  421,  423,  351,
 /*   190 */   -40,  423,  -40,  -39,  -40,  129,  412,  -40,  260,  -40,
 /*   200 */   411,  -40,  413,  -40,  -40,  -40,  -40,  428,  382,  383,
 /*   210 */   -40,  -40,  -40,  343,  439,  -40,  428,  382,  383,  -40,
 /*   220 */   428,  382,  383,  -40,  428,  382,  383,  -40,  428,  382,
 /*   230 */   383,  -40,  428,  382,  383,  -40,  428,  -40,  449,  382,
 /*   240 */   383,  -40,  343,  473,  343,  457,  -40,  449,  382,  383,
 /*   250 */   -40,  449,  382,  383,  -40,  449,  382,  383,  -40,  460,
 /*   260 */   343,  466,  -40,  465,  343,  471,  -40,  -40,  -40,  428,
 /*   270 */   -40,  428,  -40,  -40,  -40,  -40,  -40,  -40,  -40,  -40,
 /*   280 */   -40,  469,  343,  477,  -40,  482,  343,  486,  -40,  485,
 /*   290 */   260,  376,  -40,  260,  -40,  -40,  488,  260,  384,  -40,
 /*   300 */   -40,  446,  489,  -40,
};
#define YY_REDUCE_USE_DFLT (-98)
static const short yy_reduce_ofst[] = {
 /*     0 */   -97,  -98,  -98,  -98,  -98,  -98,    2,   56,  -98,  -47,
 /*    10 */   -98,   44,  -98,   28,  -98,  -98,   51,  -98,  165,  -98,
 /*    20 */   -98,  -98,  -98,  -98,  -98,   74,  -98,  -98,  -98,  179,
 /*    30 */   187,  -98,  -98,  190,  -98,  197,  -98,  200,  -98,  207,
 /*    40 */   -98,  -98,  -98,  -98,  -98,  -98,  -98,  -98,  -98,  -98,
 /*    50 */   -98,   22,  -98,   13,  180,   82,  -98,  -98,  -98,  -98,
 /*    60 */   -98,  106,  -98,  108,  -98,  109,  -98,  120,  -98,  -98,
 /*    70 */   213,  -98,  220,  -98,  223,  -98,  234,  -98,  245,  -98,
 /*    80 */   246,  -98,  248,  -98,  256,  -98,  262,  -98,  268,  -98,
 /*    90 */   276,  -98,  -98,  278,  -98,  279,  -98,  289,  -98,  296,
 /*   100 */   -98,  131,  -98,  -98,  -98,  117,  -98,  136,  -98,  134,
 /*   110 */   -98,  -98,  142,  -98,  306,  -98,  -98,  -98,   25,  -98,
 /*   120 */   148,  -98,  -98,  -98,  -98,  -98,  161,   37,  203,  -98,
 /*   130 */   -98,  -98,  -98,  -98,  -98,  -98,  -98,  -98,  -98,  -98,
 /*   140 */   -98,  -98,  -98,  -98,  -98,  -98,  203,  154,  -98,  162,
 /*   150 */   -98,  -98,  -98,  -98,  -98,  281,  -98,  -98,  -98,  -98,
 /*   160 */   -98,  -98,  -98,  193,  128,  -98,  316,  -98,  -98,  307,
 /*   170 */   -98,  -98,  -98,  -98,  135,  -98,  -98,   -9,  -98,  326,
 /*   180 */   -98,  -98,  -98,  -98,  -98,  -98,  -98,  -98,  222,  -98,
 /*   190 */   -98,  346,  -98,  342,  345,  -98,  -98,  -98,  235,  -98,
 /*   200 */   -98,  -98,  -98,  -98,  -98,  -98,  -98,  347,  349,  350,
 /*   210 */   -98,  -98,  -98,  352,  -98,  -98,  354,  356,  353,  -98,
 /*   220 */   358,  359,  357,  -98,  361,  362,  360,  -98,  365,  363,
 /*   230 */   366,  -98,  367,  368,  370,  -98,  373,  -98,  372,  375,
 /*   240 */   377,  -98,  374,  -98,  379,  -98,  -98,  381,  380,  385,
 /*   250 */   -98,  386,  387,  388,  -98,  390,  391,  392,  -98,  -98,
 /*   260 */   393,  -98,  -98,  -98,  394,  -98,  -98,  -98,  -98,  395,
 /*   270 */   -98,  396,  -98,  -98,  -98,  -98,  -98,  -98,  -98,  -98,
 /*   280 */   -98,  -98,  397,  -98,  -98,  -98,  400,  -98,  -98,  -98,
 /*   290 */   304,  -98,  -98,  314,  -98,  -98,  -98,  317,  -98,  -98,
 /*   300 */   -98,  -98,  -98,  -98,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   451,  451,  451,  306,  451,  451,  377,  451,  307,  451,
 /*    10 */   308,  451,  371,  451,  451,  373,  451,  423,  451,  425,
 /*    20 */   428,  451,  427,  451,  426,  451,  451,  429,  424,  451,
 /*    30 */   451,  451,  430,  451,  433,  451,  434,  451,  435,  451,
 /*    40 */   436,  437,  438,  439,  440,  441,  442,  443,  432,  431,
 /*    50 */   372,  451,  451,  451,  398,  314,  393,  394,  395,  396,
 /*    60 */   397,  451,  399,  451,  406,  451,  407,  451,  408,  451,
 /*    70 */   451,  409,  451,  410,  451,  411,  451,  412,  451,  413,
 /*    80 */   451,  414,  451,  415,  451,  416,  451,  417,  451,  451,
 /*    90 */   451,  418,  451,  451,  419,  451,  420,  451,  451,  451,
 /*   100 */   421,  451,  451,  422,  451,  451,  451,  451,  400,  451,
 /*   110 */   401,  451,  451,  402,  451,  444,  445,  451,  451,  403,
 /*   120 */   451,  450,  448,  449,  446,  447,  451,  451,  451,  382,
 /*   130 */   451,  385,  451,  389,  451,  390,  451,  391,  451,  392,
 /*   140 */   451,  383,  386,  388,  451,  387,  451,  451,  384,  451,
 /*   150 */   378,  451,  381,  380,  379,  451,  451,  451,  374,  376,
 /*   160 */   375,  451,  451,  451,  405,  309,  451,  368,  451,  451,
 /*   170 */   370,  404,  369,  451,  405,  310,  451,  451,  311,  451,
 /*   180 */   365,  451,  451,  367,  366,  451,  451,  451,  451,  451,
 /*   190 */   312,  451,  315,  451,  324,  317,  451,  318,  451,  319,
 /*   200 */   451,  320,  451,  321,  322,  323,  325,  360,  356,  358,
 /*   210 */   326,  357,  355,  451,  451,  359,  360,  356,  358,  327,
 /*   220 */   360,  356,  358,  328,  360,  356,  358,  329,  360,  356,
 /*   230 */   358,  330,  360,  356,  358,  331,  360,  332,  362,  356,
 /*   240 */   358,  333,  451,  451,  451,  451,  361,  362,  356,  358,
 /*   250 */   334,  362,  356,  358,  335,  362,  356,  358,  336,  451,
 /*   260 */   451,  451,  337,  451,  451,  451,  338,  339,  340,  360,
 /*   270 */   341,  360,  342,  343,  344,  345,  346,  347,  348,  349,
 /*   280 */   350,  451,  451,  451,  351,  451,  451,  451,  352,  451,
 /*   290 */   451,  451,  353,  451,  363,  364,  451,  451,  451,  354,
 /*   300 */   316,  451,  451,  313,
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
  "ON",            "DOT",           "INNER",         "JOIN",        
  "OUTER",         "LEFT",          "RIGHT",         "NOT_EQUAL",   
  "UNEQUAL",       "LESSER_EQUAL",  "DNUM",          "ASC",         
  "DESC",          "error",         "terminal_statement",  "statement",   
  "selectstatement",  "optionalinsertfieldlist",  "insertgrouplist",  "setlist",     
  "optionalwhereclause",  "togrouplist",   "fielddescriptorlist",  "fieldlist",   
  "tableexpr",     "optionalquerymodifiers",  "fielddescriptor",  "fielddescriptortype",
  "optionalfielddescriptormodifierlist",  "literal",       "optionalprecision",  "optionalunsigned",
  "optionalzerofill",  "optionalfloatprecision",  "intnum",        "literallist", 
  "togroup",       "setexpr",       "expr",          "insertgroup", 
  "exprlist",      "labellist",     "field",         "joinclause",  
  "cond",          "tablename",     "whereclause",   "limitclause", 
  "havingclause",  "groupclause",   "orderclause",   "grouplist",   
  "orderlist",     "dblnum",        "orderelement",
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
 /*  79 */ "tableexpr ::= LPAREN selectstatement RPAREN",
 /*  80 */ "tableexpr ::= tableexpr joinclause tableexpr ON cond",
 /*  81 */ "tableexpr ::= tableexpr AS LABEL",
 /*  82 */ "tableexpr ::= tablename",
 /*  83 */ "tablename ::= LABEL DOT LABEL",
 /*  84 */ "tablename ::= LABEL",
 /*  85 */ "joinclause ::= INNER JOIN",
 /*  86 */ "joinclause ::= OUTER JOIN",
 /*  87 */ "joinclause ::= LEFT JOIN",
 /*  88 */ "joinclause ::= RIGHT JOIN",
 /*  89 */ "optionalquerymodifiers ::= optionalquerymodifiers whereclause",
 /*  90 */ "optionalquerymodifiers ::= optionalquerymodifiers limitclause",
 /*  91 */ "optionalquerymodifiers ::= optionalquerymodifiers havingclause",
 /*  92 */ "optionalquerymodifiers ::= optionalquerymodifiers groupclause",
 /*  93 */ "optionalquerymodifiers ::= optionalquerymodifiers orderclause",
 /*  94 */ "optionalquerymodifiers ::=",
 /*  95 */ "whereclause ::= WHERE cond",
 /*  96 */ "limitclause ::= LIMIT intnum COMMA intnum",
 /*  97 */ "havingclause ::= HAVING cond",
 /*  98 */ "groupclause ::= GROUP BY grouplist",
 /*  99 */ "orderclause ::= ORDER BY orderlist",
 /* 100 */ "optionalwhereclause ::= whereclause",
 /* 101 */ "optionalwhereclause ::=",
 /* 102 */ "cond ::= cond AND cond",
 /* 103 */ "cond ::= cond OR cond",
 /* 104 */ "cond ::= cond XOR cond",
 /* 105 */ "cond ::= expr EQUALS expr",
 /* 106 */ "cond ::= expr NOT_EQUAL expr",
 /* 107 */ "cond ::= expr UNEQUAL expr",
 /* 108 */ "cond ::= expr LESSER expr",
 /* 109 */ "cond ::= expr GREATER expr",
 /* 110 */ "cond ::= expr LESSER_EQUAL expr",
 /* 111 */ "cond ::= expr GREATER_EQUAL expr",
 /* 112 */ "cond ::= expr LIKE expr",
 /* 113 */ "cond ::= expr RLIKE expr",
 /* 114 */ "cond ::= expr BETWEEN expr AND expr",
 /* 115 */ "cond ::= expr NOT LIKE expr",
 /* 116 */ "cond ::= expr NOT RLIKE expr",
 /* 117 */ "cond ::= expr NOT BETWEEN expr AND expr",
 /* 118 */ "cond ::= LPAREN cond RPAREN",
 /* 119 */ "exprlist ::= exprlist COMMA expr",
 /* 120 */ "exprlist ::= expr",
 /* 121 */ "expr ::= literal",
 /* 122 */ "expr ::= LABEL DOT LABEL DOT LABEL",
 /* 123 */ "expr ::= LABEL DOT LABEL",
 /* 124 */ "expr ::= LABEL",
 /* 125 */ "expr ::= LABEL LPAREN exprlist RPAREN",
 /* 126 */ "expr ::= LPAREN expr RPAREN",
 /* 127 */ "expr ::= expr PLUS expr",
 /* 128 */ "expr ::= expr MINUS expr",
 /* 129 */ "expr ::= expr MUL expr",
 /* 130 */ "expr ::= expr DIV expr",
 /* 131 */ "expr ::= expr MOD expr",
 /* 132 */ "expr ::= DISTINCT expr",
 /* 133 */ "intnum ::= LNUM",
 /* 134 */ "intnum ::= HNUM",
 /* 135 */ "dblnum ::= DNUM",
 /* 136 */ "literal ::= STRING",
 /* 137 */ "literal ::= intnum",
 /* 138 */ "literal ::= dblnum",
 /* 139 */ "literal ::= NULL",
 /* 140 */ "grouplist ::= grouplist COMMA expr",
 /* 141 */ "grouplist ::= expr",
 /* 142 */ "orderlist ::= orderlist COMMA orderelement",
 /* 143 */ "orderlist ::= orderelement",
 /* 144 */ "orderelement ::= expr ASC",
 /* 145 */ "orderelement ::= expr DESC",
 /* 146 */ "orderelement ::= expr",
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
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
    case 46:
    case 47:
    case 48:
    case 49:
    case 50:
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
    case 56:
    case 57:
    case 58:
    case 59:
    case 60:
    case 61:
    case 62:
    case 63:
    case 64:
    case 65:
    case 66:
    case 67:
    case 68:
    case 69:
    case 70:
    case 71:
    case 72:
    case 73:
    case 74:
    case 75:
    case 76:
    case 77:
    case 78:
    case 79:
    case 80:
    case 81:
    case 82:
    case 83:
    case 84:
    case 85:
    case 86:
    case 87:
    case 88:
    case 89:
    case 90:
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
    case 96:
#line 357 "pdo_user_sql_parser.lemon"
{
	if ((yypminor->yy0).freeme) {
		efree((yypminor->yy0).token);
	}
}
#line 1107 "pdo_user_sql_parser.c"
      break;
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
    case 118:
    case 119:
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
    case 135:
    case 136:
    case 137:
    case 138:
#line 366 "pdo_user_sql_parser.lemon"
{ zval_ptr_dtor(&(yypminor->yy92)); }
#line 1149 "pdo_user_sql_parser.c"
      break;
    case 120:
    case 121:
#line 452 "pdo_user_sql_parser.lemon"
{ zval_ptr_dtor(&(yypminor->yy204)[0]); zval_ptr_dtor(&(yypminor->yy204)[1]); efree((yypminor->yy204)); }
#line 1155 "pdo_user_sql_parser.c"
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
  { 98, 2 },
  { 98, 1 },
  { 99, 1 },
  { 99, 5 },
  { 99, 6 },
  { 99, 5 },
  { 99, 3 },
  { 99, 3 },
  { 99, 6 },
  { 99, 3 },
  { 100, 5 },
  { 106, 3 },
  { 106, 1 },
  { 110, 3 },
  { 112, 3 },
  { 112, 3 },
  { 112, 3 },
  { 112, 3 },
  { 112, 2 },
  { 112, 2 },
  { 112, 0 },
  { 111, 1 },
  { 111, 4 },
  { 111, 4 },
  { 111, 4 },
  { 111, 4 },
  { 111, 4 },
  { 111, 4 },
  { 111, 2 },
  { 111, 4 },
  { 111, 4 },
  { 111, 4 },
  { 111, 4 },
  { 111, 4 },
  { 111, 4 },
  { 111, 1 },
  { 111, 1 },
  { 111, 2 },
  { 111, 2 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 111, 4 },
  { 111, 4 },
  { 111, 4 },
  { 111, 4 },
  { 115, 1 },
  { 115, 0 },
  { 116, 1 },
  { 116, 0 },
  { 114, 3 },
  { 114, 0 },
  { 117, 5 },
  { 117, 0 },
  { 119, 3 },
  { 119, 1 },
  { 105, 3 },
  { 105, 1 },
  { 120, 3 },
  { 103, 3 },
  { 103, 1 },
  { 121, 3 },
  { 102, 3 },
  { 102, 1 },
  { 123, 3 },
  { 125, 3 },
  { 125, 1 },
  { 101, 3 },
  { 101, 0 },
  { 107, 3 },
  { 107, 1 },
  { 126, 1 },
  { 126, 3 },
  { 108, 3 },
  { 108, 3 },
  { 108, 5 },
  { 108, 3 },
  { 108, 1 },
  { 129, 3 },
  { 129, 1 },
  { 127, 2 },
  { 127, 2 },
  { 127, 2 },
  { 127, 2 },
  { 109, 2 },
  { 109, 2 },
  { 109, 2 },
  { 109, 2 },
  { 109, 2 },
  { 109, 0 },
  { 130, 2 },
  { 131, 4 },
  { 132, 2 },
  { 133, 3 },
  { 134, 3 },
  { 104, 1 },
  { 104, 0 },
  { 128, 3 },
  { 128, 3 },
  { 128, 3 },
  { 128, 3 },
  { 128, 3 },
  { 128, 3 },
  { 128, 3 },
  { 128, 3 },
  { 128, 3 },
  { 128, 3 },
  { 128, 3 },
  { 128, 3 },
  { 128, 5 },
  { 128, 4 },
  { 128, 4 },
  { 128, 6 },
  { 128, 3 },
  { 124, 3 },
  { 124, 1 },
  { 122, 1 },
  { 122, 5 },
  { 122, 3 },
  { 122, 1 },
  { 122, 4 },
  { 122, 3 },
  { 122, 3 },
  { 122, 3 },
  { 122, 3 },
  { 122, 3 },
  { 122, 3 },
  { 122, 2 },
  { 118, 1 },
  { 118, 1 },
  { 137, 1 },
  { 113, 1 },
  { 113, 1 },
  { 113, 1 },
  { 113, 1 },
  { 135, 3 },
  { 135, 1 },
  { 136, 3 },
  { 136, 1 },
  { 138, 2 },
  { 138, 2 },
  { 138, 1 },
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
#line 363 "pdo_user_sql_parser.lemon"
{ pusp_do_terminal_statement(&return_value, yymsp[-1].minor.yy92, 1);   yy_destructor(36,&yymsp[0].minor);
}
#line 1530 "pdo_user_sql_parser.c"
        break;
      case 1:
#line 364 "pdo_user_sql_parser.lemon"
{ pusp_do_terminal_statement(&return_value, yymsp[0].minor.yy92, 0); }
#line 1535 "pdo_user_sql_parser.c"
        break;
      case 2:
      case 76:
      case 82:
      case 100:
      case 121:
      case 137:
      case 138:
#line 367 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = yymsp[0].minor.yy92; }
#line 1546 "pdo_user_sql_parser.c"
        break;
      case 3:
#line 368 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_insert_select_statement(pusp_zvalize_token(&yymsp[-2].minor.yy0), yymsp[-1].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(9,&yymsp[-4].minor);
  yy_destructor(10,&yymsp[-3].minor);
}
#line 1553 "pdo_user_sql_parser.c"
        break;
      case 4:
#line 369 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_insert_statement(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(9,&yymsp[-5].minor);
  yy_destructor(10,&yymsp[-4].minor);
  yy_destructor(37,&yymsp[-1].minor);
}
#line 1561 "pdo_user_sql_parser.c"
        break;
      case 5:
#line 370 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_update_statement(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-1].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(38,&yymsp[-4].minor);
  yy_destructor(39,&yymsp[-2].minor);
}
#line 1568 "pdo_user_sql_parser.c"
        break;
      case 6:
#line 371 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_delete_statement(pusp_zvalize_token(&yymsp[-1].minor.yy0), yymsp[0].minor.yy92);   yy_destructor(40,&yymsp[-2].minor);
}
#line 1574 "pdo_user_sql_parser.c"
        break;
      case 7:
#line 372 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_rename_statement(yymsp[0].minor.yy92);   yy_destructor(41,&yymsp[-2].minor);
  yy_destructor(42,&yymsp[-1].minor);
}
#line 1581 "pdo_user_sql_parser.c"
        break;
      case 8:
#line 373 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_create_statement(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-1].minor.yy92);   yy_destructor(43,&yymsp[-5].minor);
  yy_destructor(42,&yymsp[-4].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1590 "pdo_user_sql_parser.c"
        break;
      case 9:
#line 374 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_drop_statement(pusp_zvalize_token(&yymsp[0].minor.yy0));   yy_destructor(44,&yymsp[-2].minor);
  yy_destructor(42,&yymsp[-1].minor);
}
#line 1597 "pdo_user_sql_parser.c"
        break;
      case 10:
#line 377 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_select_statement(yymsp[-3].minor.yy92,yymsp[-1].minor.yy92,yymsp[0].minor.yy92);   yy_destructor(8,&yymsp[-4].minor);
  yy_destructor(45,&yymsp[-2].minor);
}
#line 1604 "pdo_user_sql_parser.c"
        break;
      case 11:
      case 59:
      case 67:
      case 74:
      case 119:
      case 140:
      case 142:
#line 380 "pdo_user_sql_parser.lemon"
{ add_next_index_zval(yymsp[-2].minor.yy92, yymsp[0].minor.yy92); yygotominor.yy92 = yymsp[-2].minor.yy92;   yy_destructor(11,&yymsp[-1].minor);
}
#line 1616 "pdo_user_sql_parser.c"
        break;
      case 12:
      case 60:
      case 68:
      case 75:
      case 120:
      case 141:
      case 143:
#line 381 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); array_init(yygotominor.yy92); add_next_index_zval(yygotominor.yy92, yymsp[0].minor.yy92); }
#line 1627 "pdo_user_sql_parser.c"
        break;
      case 13:
#line 384 "pdo_user_sql_parser.lemon"
{ add_assoc_zval(yymsp[-1].minor.yy92, "name", pusp_zvalize_token(&yymsp[-2].minor.yy0)); add_assoc_zval(yymsp[-1].minor.yy92, "flags", yymsp[0].minor.yy92); yygotominor.yy92 = yymsp[-1].minor.yy92; }
#line 1632 "pdo_user_sql_parser.c"
        break;
      case 14:
#line 387 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-2].minor.yy92, "not null", 1); yygotominor.yy92 = yymsp[-2].minor.yy92;   yy_destructor(1,&yymsp[-1].minor);
  yy_destructor(46,&yymsp[0].minor);
}
#line 1639 "pdo_user_sql_parser.c"
        break;
      case 15:
#line 388 "pdo_user_sql_parser.lemon"
{ add_assoc_zval(yymsp[-2].minor.yy92, "default", yymsp[0].minor.yy92); yygotominor.yy92 = yymsp[-2].minor.yy92;   yy_destructor(47,&yymsp[-1].minor);
}
#line 1645 "pdo_user_sql_parser.c"
        break;
      case 16:
#line 389 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-2].minor.yy92, "primary key", 1); yygotominor.yy92 = yymsp[-2].minor.yy92;   yy_destructor(48,&yymsp[-1].minor);
  yy_destructor(49,&yymsp[0].minor);
}
#line 1652 "pdo_user_sql_parser.c"
        break;
      case 17:
#line 390 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-2].minor.yy92, "unique key", 1); yygotominor.yy92 = yymsp[-2].minor.yy92;   yy_destructor(50,&yymsp[-1].minor);
  yy_destructor(49,&yymsp[0].minor);
}
#line 1659 "pdo_user_sql_parser.c"
        break;
      case 18:
#line 391 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-1].minor.yy92, "key", 1); yygotominor.yy92 = yymsp[-1].minor.yy92;   yy_destructor(49,&yymsp[0].minor);
}
#line 1665 "pdo_user_sql_parser.c"
        break;
      case 19:
#line 392 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-1].minor.yy92, "auto_increment", 1); yygotominor.yy92 = yymsp[-1].minor.yy92;   yy_destructor(51,&yymsp[0].minor);
}
#line 1671 "pdo_user_sql_parser.c"
        break;
      case 20:
#line 393 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); array_init(yygotominor.yy92); }
#line 1676 "pdo_user_sql_parser.c"
        break;
      case 21:
#line 396 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("bit", NULL, NULL);   yy_destructor(52,&yymsp[0].minor);
}
#line 1682 "pdo_user_sql_parser.c"
        break;
      case 22:
#line 397 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_num("int", yymsp[-2].minor.yy92, yymsp[-1].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(53,&yymsp[-3].minor);
}
#line 1688 "pdo_user_sql_parser.c"
        break;
      case 23:
#line 398 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_num("integer", yymsp[-2].minor.yy92, yymsp[-1].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(54,&yymsp[-3].minor);
}
#line 1694 "pdo_user_sql_parser.c"
        break;
      case 24:
#line 399 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_num("tinyint", yymsp[-2].minor.yy92, yymsp[-1].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(55,&yymsp[-3].minor);
}
#line 1700 "pdo_user_sql_parser.c"
        break;
      case 25:
#line 400 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_num("smallint", yymsp[-2].minor.yy92, yymsp[-1].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(56,&yymsp[-3].minor);
}
#line 1706 "pdo_user_sql_parser.c"
        break;
      case 26:
#line 401 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_num("mediumint", yymsp[-2].minor.yy92, yymsp[-1].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(57,&yymsp[-3].minor);
}
#line 1712 "pdo_user_sql_parser.c"
        break;
      case 27:
#line 402 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_num("bigint", yymsp[-2].minor.yy92, yymsp[-1].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(58,&yymsp[-3].minor);
}
#line 1718 "pdo_user_sql_parser.c"
        break;
      case 28:
#line 403 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("year", "precision", yymsp[0].minor.yy92);   yy_destructor(59,&yymsp[-1].minor);
}
#line 1724 "pdo_user_sql_parser.c"
        break;
      case 29:
#line 404 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_num("float", yymsp[-2].minor.yy92, yymsp[-1].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(60,&yymsp[-3].minor);
}
#line 1730 "pdo_user_sql_parser.c"
        break;
      case 30:
#line 405 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_num("real", yymsp[-2].minor.yy92, yymsp[-1].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(61,&yymsp[-3].minor);
}
#line 1736 "pdo_user_sql_parser.c"
        break;
      case 31:
#line 406 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_num("decimal", yymsp[-2].minor.yy92, yymsp[-1].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(62,&yymsp[-3].minor);
}
#line 1742 "pdo_user_sql_parser.c"
        break;
      case 32:
#line 407 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_num("double", yymsp[-2].minor.yy92, yymsp[-1].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(63,&yymsp[-3].minor);
}
#line 1748 "pdo_user_sql_parser.c"
        break;
      case 33:
#line 408 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("char", "length", yymsp[-1].minor.yy92);   yy_destructor(64,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1756 "pdo_user_sql_parser.c"
        break;
      case 34:
#line 409 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("varchar", "length", yymsp[-1].minor.yy92);   yy_destructor(65,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1764 "pdo_user_sql_parser.c"
        break;
      case 35:
#line 410 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("text", "date", NULL);   yy_destructor(66,&yymsp[0].minor);
}
#line 1770 "pdo_user_sql_parser.c"
        break;
      case 36:
#line 411 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("text", "time", NULL);   yy_destructor(67,&yymsp[0].minor);
}
#line 1776 "pdo_user_sql_parser.c"
        break;
      case 37:
#line 412 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("datetime", "precision", yymsp[0].minor.yy92);   yy_destructor(68,&yymsp[-1].minor);
}
#line 1782 "pdo_user_sql_parser.c"
        break;
      case 38:
#line 413 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("timestamp", "precision", yymsp[0].minor.yy92);   yy_destructor(69,&yymsp[-1].minor);
}
#line 1788 "pdo_user_sql_parser.c"
        break;
      case 39:
#line 414 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("text", "precision", NULL);   yy_destructor(70,&yymsp[0].minor);
}
#line 1794 "pdo_user_sql_parser.c"
        break;
      case 40:
#line 415 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "tiny", 1); yygotominor.yy92 = pusp_do_declare_type("text", "precision", p);   yy_destructor(71,&yymsp[0].minor);
}
#line 1800 "pdo_user_sql_parser.c"
        break;
      case 41:
#line 416 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "medium", 1); yygotominor.yy92 = pusp_do_declare_type("text", "precision", p);   yy_destructor(72,&yymsp[0].minor);
}
#line 1806 "pdo_user_sql_parser.c"
        break;
      case 42:
#line 417 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "long", 1); yygotominor.yy92 = pusp_do_declare_type("text", "precision", p);   yy_destructor(73,&yymsp[0].minor);
}
#line 1812 "pdo_user_sql_parser.c"
        break;
      case 43:
#line 418 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("blob", "precision", NULL);   yy_destructor(74,&yymsp[0].minor);
}
#line 1818 "pdo_user_sql_parser.c"
        break;
      case 44:
#line 419 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "tiny", 1); yygotominor.yy92 = pusp_do_declare_type("blob", "precision", p);   yy_destructor(75,&yymsp[0].minor);
}
#line 1824 "pdo_user_sql_parser.c"
        break;
      case 45:
#line 420 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "medium", 1); yygotominor.yy92 = pusp_do_declare_type("blob", "precision", p);   yy_destructor(76,&yymsp[0].minor);
}
#line 1830 "pdo_user_sql_parser.c"
        break;
      case 46:
#line 421 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "long", 1); yygotominor.yy92 = pusp_do_declare_type("blob", "precision", p);   yy_destructor(77,&yymsp[0].minor);
}
#line 1836 "pdo_user_sql_parser.c"
        break;
      case 47:
#line 422 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("binary", "length", yymsp[-1].minor.yy92);   yy_destructor(78,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1844 "pdo_user_sql_parser.c"
        break;
      case 48:
#line 423 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("varbinary", "length", yymsp[-1].minor.yy92);   yy_destructor(79,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1852 "pdo_user_sql_parser.c"
        break;
      case 49:
#line 424 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("set", "flags", yymsp[-1].minor.yy92);   yy_destructor(39,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1860 "pdo_user_sql_parser.c"
        break;
      case 50:
#line 425 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_declare_type("enum", "values", yymsp[-1].minor.yy92);   yy_destructor(80,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1868 "pdo_user_sql_parser.c"
        break;
      case 51:
#line 428 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); ZVAL_TRUE(yygotominor.yy92);   yy_destructor(81,&yymsp[0].minor);
}
#line 1874 "pdo_user_sql_parser.c"
        break;
      case 52:
      case 54:
#line 429 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); ZVAL_FALSE(yygotominor.yy92); }
#line 1880 "pdo_user_sql_parser.c"
        break;
      case 53:
#line 432 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); ZVAL_TRUE(yygotominor.yy92);   yy_destructor(82,&yymsp[0].minor);
}
#line 1886 "pdo_user_sql_parser.c"
        break;
      case 55:
      case 69:
      case 72:
      case 78:
      case 79:
      case 118:
      case 126:
#line 436 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = yymsp[-1].minor.yy92;   yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1899 "pdo_user_sql_parser.c"
        break;
      case 56:
      case 58:
      case 73:
      case 94:
      case 101:
#line 437 "pdo_user_sql_parser.lemon"
{ TSRMLS_FETCH(); yygotominor.yy92 = EG(uninitialized_zval_ptr); }
#line 1908 "pdo_user_sql_parser.c"
        break;
      case 57:
#line 440 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); array_init(yygotominor.yy92); add_assoc_zval(yygotominor.yy92, "length", yymsp[-3].minor.yy92); add_assoc_zval(yygotominor.yy92, "decimals", yymsp[-1].minor.yy92);   yy_destructor(31,&yymsp[-4].minor);
  yy_destructor(11,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1916 "pdo_user_sql_parser.c"
        break;
      case 61:
#line 448 "pdo_user_sql_parser.lemon"
{ pusp_do_push_labeled_zval(yymsp[-2].minor.yy92, yymsp[0].minor.yy204); yygotominor.yy92 = yymsp[-2].minor.yy92;   yy_destructor(11,&yymsp[-1].minor);
}
#line 1922 "pdo_user_sql_parser.c"
        break;
      case 62:
      case 65:
#line 449 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); array_init(yygotominor.yy92); pusp_do_push_labeled_zval(yygotominor.yy92, yymsp[0].minor.yy204); }
#line 1928 "pdo_user_sql_parser.c"
        break;
      case 63:
#line 453 "pdo_user_sql_parser.lemon"
{ zval **tmp = safe_emalloc(2, sizeof(zval*), 0); tmp[0] = pusp_zvalize_token(&yymsp[-2].minor.yy0); tmp[1] = pusp_zvalize_token(&yymsp[0].minor.yy0); yygotominor.yy204 = tmp;   yy_destructor(83,&yymsp[-1].minor);
}
#line 1934 "pdo_user_sql_parser.c"
        break;
      case 64:
#line 456 "pdo_user_sql_parser.lemon"
{ pusp_do_push_labeled_zval(yymsp[-2].minor.yy92, (zval**)yymsp[0].minor.yy204); yygotominor.yy92 = yymsp[-2].minor.yy92;   yy_destructor(11,&yymsp[-1].minor);
}
#line 1940 "pdo_user_sql_parser.c"
        break;
      case 66:
#line 461 "pdo_user_sql_parser.lemon"
{ zval **tmp = safe_emalloc(2, sizeof(zval*), 0); tmp[0] = pusp_zvalize_token(&yymsp[-2].minor.yy0); tmp[1] = yymsp[0].minor.yy92; yygotominor.yy204 = tmp;   yy_destructor(19,&yymsp[-1].minor);
}
#line 1946 "pdo_user_sql_parser.c"
        break;
      case 70:
#line 471 "pdo_user_sql_parser.lemon"
{ add_next_index_zval(yymsp[-2].minor.yy92, pusp_zvalize_token(&yymsp[0].minor.yy0)); yygotominor.yy92 = yymsp[-2].minor.yy92;   yy_destructor(11,&yymsp[-1].minor);
}
#line 1952 "pdo_user_sql_parser.c"
        break;
      case 71:
#line 472 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); array_init(yygotominor.yy92); add_next_index_zval(yygotominor.yy92, pusp_zvalize_token(&yymsp[0].minor.yy0)); }
#line 1957 "pdo_user_sql_parser.c"
        break;
      case 77:
#line 484 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); array_init(yygotominor.yy92); add_assoc_stringl(yygotominor.yy92, "type", "alias", sizeof("alias") - 1, 1); add_assoc_zval(yygotominor.yy92, "field", yymsp[-2].minor.yy92); add_assoc_zval(yygotominor.yy92, "as", pusp_zvalize_token(&yymsp[0].minor.yy0));   yy_destructor(15,&yymsp[-1].minor);
}
#line 1963 "pdo_user_sql_parser.c"
        break;
      case 80:
#line 489 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_join_expression(yymsp[-4].minor.yy92, yymsp[-3].minor.yy92, yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(84,&yymsp[-1].minor);
}
#line 1969 "pdo_user_sql_parser.c"
        break;
      case 81:
#line 490 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); array_init(yygotominor.yy92); add_assoc_stringl(yygotominor.yy92, "type", "alias", sizeof("alias") - 1, 1); add_assoc_zval(yygotominor.yy92, "table", yymsp[-2].minor.yy92); add_assoc_zval(yygotominor.yy92, "as", pusp_zvalize_token(&yymsp[0].minor.yy0));   yy_destructor(15,&yymsp[-1].minor);
}
#line 1975 "pdo_user_sql_parser.c"
        break;
      case 83:
      case 123:
#line 494 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_field(NULL, &yymsp[-2].minor.yy0, &yymsp[0].minor.yy0);   yy_destructor(85,&yymsp[-1].minor);
}
#line 1982 "pdo_user_sql_parser.c"
        break;
      case 84:
      case 124:
#line 495 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_field(NULL, NULL, &yymsp[0].minor.yy0); }
#line 1988 "pdo_user_sql_parser.c"
        break;
      case 85:
#line 506 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_zvalize_static_string("inner");   yy_destructor(86,&yymsp[-1].minor);
  yy_destructor(87,&yymsp[0].minor);
}
#line 1995 "pdo_user_sql_parser.c"
        break;
      case 86:
#line 507 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_zvalize_static_string("outer");   yy_destructor(88,&yymsp[-1].minor);
  yy_destructor(87,&yymsp[0].minor);
}
#line 2002 "pdo_user_sql_parser.c"
        break;
      case 87:
#line 508 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_zvalize_static_string("left");   yy_destructor(89,&yymsp[-1].minor);
  yy_destructor(87,&yymsp[0].minor);
}
#line 2009 "pdo_user_sql_parser.c"
        break;
      case 88:
#line 509 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_zvalize_static_string("right");   yy_destructor(90,&yymsp[-1].minor);
  yy_destructor(87,&yymsp[0].minor);
}
#line 2016 "pdo_user_sql_parser.c"
        break;
      case 89:
#line 512 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_add_query_modifier(yymsp[-1].minor.yy92, "where", yymsp[0].minor.yy92); }
#line 2021 "pdo_user_sql_parser.c"
        break;
      case 90:
#line 513 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_add_query_modifier(yymsp[-1].minor.yy92, "limit", yymsp[0].minor.yy92); }
#line 2026 "pdo_user_sql_parser.c"
        break;
      case 91:
#line 514 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_add_query_modifier(yymsp[-1].minor.yy92, "having", yymsp[0].minor.yy92); }
#line 2031 "pdo_user_sql_parser.c"
        break;
      case 92:
#line 515 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_add_query_modifier(yymsp[-1].minor.yy92, "group-by", yymsp[0].minor.yy92); }
#line 2036 "pdo_user_sql_parser.c"
        break;
      case 93:
#line 516 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_add_query_modifier(yymsp[-1].minor.yy92, "order-by", yymsp[0].minor.yy92); }
#line 2041 "pdo_user_sql_parser.c"
        break;
      case 95:
#line 520 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = yymsp[0].minor.yy92;   yy_destructor(6,&yymsp[-1].minor);
}
#line 2047 "pdo_user_sql_parser.c"
        break;
      case 96:
#line 523 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); array_init(yygotominor.yy92); add_assoc_zval(yygotominor.yy92, "from", yymsp[-2].minor.yy92); add_assoc_zval(yygotominor.yy92, "to", yymsp[0].minor.yy92);   yy_destructor(5,&yymsp[-3].minor);
  yy_destructor(11,&yymsp[-1].minor);
}
#line 2054 "pdo_user_sql_parser.c"
        break;
      case 97:
#line 526 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = yymsp[0].minor.yy92;   yy_destructor(7,&yymsp[-1].minor);
}
#line 2060 "pdo_user_sql_parser.c"
        break;
      case 98:
#line 529 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = yymsp[0].minor.yy92;   yy_destructor(2,&yymsp[-2].minor);
  yy_destructor(4,&yymsp[-1].minor);
}
#line 2067 "pdo_user_sql_parser.c"
        break;
      case 99:
#line 532 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = yymsp[0].minor.yy92;   yy_destructor(3,&yymsp[-2].minor);
  yy_destructor(4,&yymsp[-1].minor);
}
#line 2074 "pdo_user_sql_parser.c"
        break;
      case 102:
#line 539 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "and", yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(12,&yymsp[-1].minor);
}
#line 2080 "pdo_user_sql_parser.c"
        break;
      case 103:
#line 540 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "or", yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(13,&yymsp[-1].minor);
}
#line 2086 "pdo_user_sql_parser.c"
        break;
      case 104:
#line 541 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "xor", yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(14,&yymsp[-1].minor);
}
#line 2092 "pdo_user_sql_parser.c"
        break;
      case 105:
#line 543 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "=", yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(19,&yymsp[-1].minor);
}
#line 2098 "pdo_user_sql_parser.c"
        break;
      case 106:
#line 544 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "!=", yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(91,&yymsp[-1].minor);
}
#line 2104 "pdo_user_sql_parser.c"
        break;
      case 107:
#line 545 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "<>", yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(92,&yymsp[-1].minor);
}
#line 2110 "pdo_user_sql_parser.c"
        break;
      case 108:
#line 546 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "<", yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(20,&yymsp[-1].minor);
}
#line 2116 "pdo_user_sql_parser.c"
        break;
      case 109:
#line 547 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, ">", yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(21,&yymsp[-1].minor);
}
#line 2122 "pdo_user_sql_parser.c"
        break;
      case 110:
#line 548 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "<=", yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(93,&yymsp[-1].minor);
}
#line 2128 "pdo_user_sql_parser.c"
        break;
      case 111:
#line 549 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, ">=", yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(23,&yymsp[-1].minor);
}
#line 2134 "pdo_user_sql_parser.c"
        break;
      case 112:
#line 550 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "like", yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(17,&yymsp[-1].minor);
}
#line 2140 "pdo_user_sql_parser.c"
        break;
      case 113:
#line 551 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "rlike", yymsp[-2].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(18,&yymsp[-1].minor);
}
#line 2146 "pdo_user_sql_parser.c"
        break;
      case 114:
#line 552 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "between", yymsp[-4].minor.yy92, yymsp[-2].minor.yy92); add_assoc_zval(yygotominor.yy92, "op3", yymsp[0].minor.yy92);   yy_destructor(16,&yymsp[-3].minor);
  yy_destructor(12,&yymsp[-1].minor);
}
#line 2153 "pdo_user_sql_parser.c"
        break;
      case 115:
#line 553 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "not like", yymsp[-3].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(1,&yymsp[-2].minor);
  yy_destructor(17,&yymsp[-1].minor);
}
#line 2160 "pdo_user_sql_parser.c"
        break;
      case 116:
#line 554 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "not rlike", yymsp[-3].minor.yy92, yymsp[0].minor.yy92);   yy_destructor(1,&yymsp[-2].minor);
  yy_destructor(18,&yymsp[-1].minor);
}
#line 2167 "pdo_user_sql_parser.c"
        break;
      case 117:
#line 555 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy92, "not between", yymsp[-5].minor.yy92, yymsp[-2].minor.yy92);  add_assoc_zval(yygotominor.yy92, "op3", yymsp[0].minor.yy92);   yy_destructor(1,&yymsp[-4].minor);
  yy_destructor(16,&yymsp[-3].minor);
  yy_destructor(12,&yymsp[-1].minor);
}
#line 2175 "pdo_user_sql_parser.c"
        break;
      case 122:
#line 564 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_field(&yymsp[-4].minor.yy0, &yymsp[-2].minor.yy0, &yymsp[0].minor.yy0);   yy_destructor(85,&yymsp[-3].minor);
  yy_destructor(85,&yymsp[-1].minor);
}
#line 2182 "pdo_user_sql_parser.c"
        break;
      case 125:
#line 567 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_do_function(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-1].minor.yy92);   yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 2189 "pdo_user_sql_parser.c"
        break;
      case 127:
#line 569 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy92,yymsp[-2].minor.yy92,"+",yymsp[0].minor.yy92);   yy_destructor(29,&yymsp[-1].minor);
}
#line 2195 "pdo_user_sql_parser.c"
        break;
      case 128:
#line 570 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy92,yymsp[-2].minor.yy92,"-",yymsp[0].minor.yy92);   yy_destructor(30,&yymsp[-1].minor);
}
#line 2201 "pdo_user_sql_parser.c"
        break;
      case 129:
#line 571 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy92,yymsp[-2].minor.yy92,"*",yymsp[0].minor.yy92);   yy_destructor(25,&yymsp[-1].minor);
}
#line 2207 "pdo_user_sql_parser.c"
        break;
      case 130:
#line 572 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy92,yymsp[-2].minor.yy92,"/",yymsp[0].minor.yy92);   yy_destructor(26,&yymsp[-1].minor);
}
#line 2213 "pdo_user_sql_parser.c"
        break;
      case 131:
#line 573 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy92,yymsp[-2].minor.yy92,"%",yymsp[0].minor.yy92);   yy_destructor(27,&yymsp[-1].minor);
}
#line 2219 "pdo_user_sql_parser.c"
        break;
      case 132:
#line 574 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); array_init(yygotominor.yy92); add_assoc_stringl(yygotominor.yy92, "type", "distinct", sizeof("distinct") - 1, 1); add_assoc_zval(yygotominor.yy92, "distinct", yymsp[0].minor.yy92);   yy_destructor(24,&yymsp[-1].minor);
}
#line 2225 "pdo_user_sql_parser.c"
        break;
      case 133:
#line 577 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_zvalize_lnum(&yymsp[0].minor.yy0); }
#line 2230 "pdo_user_sql_parser.c"
        break;
      case 134:
#line 578 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_zvalize_hnum(&yymsp[0].minor.yy0); }
#line 2235 "pdo_user_sql_parser.c"
        break;
      case 135:
#line 581 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_zvalize_dnum(&yymsp[0].minor.yy0); }
#line 2240 "pdo_user_sql_parser.c"
        break;
      case 136:
#line 584 "pdo_user_sql_parser.lemon"
{ yygotominor.yy92 = pusp_zvalize_token(&yymsp[0].minor.yy0); }
#line 2245 "pdo_user_sql_parser.c"
        break;
      case 139:
#line 587 "pdo_user_sql_parser.lemon"
{ TSRMLS_FETCH(); yygotominor.yy92 = EG(uninitialized_zval_ptr);   yy_destructor(46,&yymsp[0].minor);
}
#line 2251 "pdo_user_sql_parser.c"
        break;
      case 144:
#line 598 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); add_assoc_stringl(yygotominor.yy92, "direction", "asc", sizeof("asc") - 1, 1); add_assoc_zval(yygotominor.yy92, "by", yymsp[-1].minor.yy92);   yy_destructor(95,&yymsp[0].minor);
}
#line 2257 "pdo_user_sql_parser.c"
        break;
      case 145:
#line 599 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); add_assoc_stringl(yygotominor.yy92, "direction", "desc", sizeof("desc") - 1, 1); add_assoc_zval(yygotominor.yy92, "by", yymsp[-1].minor.yy92);   yy_destructor(96,&yymsp[0].minor);
}
#line 2263 "pdo_user_sql_parser.c"
        break;
      case 146:
#line 600 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy92); add_assoc_null(yygotominor.yy92, "direction"); add_assoc_zval(yygotominor.yy92, "by", yymsp[0].minor.yy92); }
#line 2268 "pdo_user_sql_parser.c"
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
#line 351 "pdo_user_sql_parser.lemon"

	if (Z_TYPE_P(return_value) == IS_NULL) {
		/* Only throw error if we don't already have a statement */
		RETVAL_FALSE;
	}
#line 2332 "pdo_user_sql_parser.c"
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
