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

static zval *pusp_do_placeholder(zval *placeholder)
{
	zval *ret;

	MAKE_STD_ZVAL(ret);
	array_init(ret);
	add_assoc_zval(ret, "placeholder", placeholder);

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

#line 356 "pdo_user_sql_parser.c"
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
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
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
#define YYNOCODE 143
#define YYACTIONTYPE unsigned short int
#define php_pdo_user_sql_parserTOKENTYPE php_pdo_user_sql_token
typedef union {
  php_pdo_user_sql_parserTOKENTYPE yy0;
  zval** yy174;
  zval* yy252;
  int yy285;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define php_pdo_user_sql_parserARG_SDECL zval *return_value;
#define php_pdo_user_sql_parserARG_PDECL ,zval *return_value
#define php_pdo_user_sql_parserARG_FETCH zval *return_value = yypParser->return_value
#define php_pdo_user_sql_parserARG_STORE yypParser->return_value = return_value
#define YYNSTATE 321
#define YYNRULE 155
#define YYERRORSYMBOL 99
#define YYERRSYMDT yy285
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
 /*     0 */   213,  160,   21,   22,   23,  319,   18,   19,  237,  238,
 /*    10 */   239,  240,  241,  283,   77,   80,   83,   86,   89,   92,
 /*    20 */    95,   96,   99,  102,  105,  205,  207,  301,  302,  108,
 /*    30 */   109,  305,  306,  307,  308,  309,  310,  311,  312,  209,
 /*    40 */   211,  214,  431,    6,  431,  431,   16,  431,  431,  431,
 /*    50 */     5,   24,  264,    1,  431,  431,  431,  431,   20,  155,
 /*    60 */   226,  227,  230,   17,   49,    1,  164,   21,   22,   23,
 /*    70 */   431,   18,   19,  233,  248,  249,   46,  181,  431,  432,
 /*    80 */   220,  432,  432,  263,  432,  432,  432,    8,    9,   10,
 /*    90 */   185,  432,  432,  432,  432,  188,  167,  191,  192,  170,
 /*   100 */   196,  215,  190,  246,   21,   22,   23,  432,   18,   19,
 /*   110 */   193,  221,  221,   46,  181,  432,  231,  231,  226,  227,
 /*   120 */   141,  129,   59,  229,  183,  274,  268,  431,  431,  144,
 /*   130 */   431,  431,  431,    1,  154,  194,  232,  232,   21,   22,
 /*   140 */    23,  228,   18,   19,   35,   33,   34,   26,   29,   30,
 /*   150 */    47,   32,  159,   21,   22,   23,  228,   18,   19,  477,
 /*   160 */   146,  217,   51,    1,  432,  432,    7,  432,  432,  432,
 /*   170 */   433,  259,  433,  433,   56,  433,  433,  433,  110,   24,
 /*   180 */    18,   19,  433,  433,  433,  433,   20,  148,  226,  227,
 /*   190 */   230,  226,  227,  230,  218,   21,   22,   23,  433,   18,
 /*   200 */    19,  233,  180,   49,  233,   73,  433,   24,  144,  225,
 /*   210 */    57,  322,  175,  271,   20,  148,  226,  227,  230,   72,
 /*   220 */    27,   28,   31,   35,   33,   34,   26,   29,   30,  233,
 /*   230 */    32,  259,   21,   22,   23,    7,   18,   19,  153,  159,
 /*   240 */    74,   21,   22,   23,  221,   18,   19,  163,   60,  231,
 /*   250 */    59,  229,  175,  141,  229,  433,  433,  186,  433,  433,
 /*   260 */   433,  427,   50,  427,  427,  252,  427,  427,  427,  232,
 /*   270 */   224,  226,  227,  427,  427,  427,  427,  219,   59,  229,
 /*   280 */   270,  171,   15,  176,   24,  177,  178,  179,    4,  427,
 /*   290 */     2,   11,  148,  226,  227,  230,  247,  427,   72,   27,
 /*   300 */    28,   31,  429,   58,  429,  429,  233,  429,  429,  429,
 /*   310 */   175,    8,    9,   10,  429,  429,  429,  429,   36,   39,
 /*   320 */    37,   38,   48,  176,  259,  177,  178,  179,  199,  202,
 /*   330 */   429,   21,   22,   23,  221,   18,   19,  166,  429,  231,
 /*   340 */   203,    6,  204,  121,  206,  262,  427,  427,   40,  427,
 /*   350 */   427,  427,  222,  187,  208,   59,  229,  174,  236,  232,
 /*   360 */   251,   21,   22,   23,  210,   18,   19,  212,  172,  173,
 /*   370 */   266,   61,    7,   12,   44,  200,  281,  201,  282,  117,
 /*   380 */    17,  176,  267,  177,  178,  179,  269,  429,  429,  158,
 /*   390 */   429,  429,  429,  419,  156,  419,  419,  223,  419,  419,
 /*   400 */   419,  272,   25,  221,  276,    8,    9,   10,  231,   76,
 /*   410 */    45,   63,  127,  221,  147,  278,  286,   78,  231,  244,
 /*   420 */   231,  419,  127,  285,  149,   79,  275,  315,  232,  419,
 /*   430 */   421,  284,  421,  421,   81,  421,  421,  421,  232,  424,
 /*   440 */   232,   82,    8,    9,   10,  422,  288,  422,  422,   84,
 /*   450 */   422,  422,  422,   87,  221,  221,   25,   64,  421,  231,
 /*   460 */   231,   85,  221,  116,  116,   88,  421,  231,   90,  118,
 /*   470 */   242,  116,  289,  422,  290,  424,   91,  115,  291,  232,
 /*   480 */   232,  422,   94,   93,  125,  321,  292,  232,  293,  316,
 /*   490 */   221,  221,  221,  221,  231,  231,  231,  231,  231,  116,
 /*   500 */   114,  116,  121,  221,  221,  243,  143,  119,  231,  231,
 /*   510 */   303,   97,  129,  126,  232,  232,  232,  232,  232,  250,
 /*   520 */    45,  221,  221,  294,  304,  120,  231,  231,  232,  232,
 /*   530 */   116,  235,   98,  100,  221,  101,  145,  318,  296,  231,
 /*   540 */   221,  103,  297,  234,  104,  231,  232,  232,  221,  122,
 /*   550 */   106,  221,  107,  231,   70,  298,  231,  150,  221,  232,
 /*   560 */   151,  221,   71,  231,  168,  232,  231,  152,  165,  222,
 /*   570 */   128,  169,  221,  232,  245,  221,  232,  231,   62,   13,
 /*   580 */   231,  130,  221,  232,  131,  221,  232,  231,    3,   14,
 /*   590 */   231,  111,  253,  221,  112,  254,  221,  232,  231,  255,
 /*   600 */   232,  231,  113,  221,  258,  132,  221,  232,  231,  182,
 /*   610 */   232,  231,  133,  256,  221,  134,  257,  221,  232,  231,
 /*   620 */   221,  232,  231,  135,  260,  231,  136,  184,  232,  123,
 /*   630 */   261,  232,  221,  157,  265,  221,  189,  231,  221,  232,
 /*   640 */   231,  137,  232,  231,  138,  232,  221,  139,   52,   41,
 /*   650 */    53,  231,   54,   75,  195,  124,  221,  232,  273,  221,
 /*   660 */   232,  231,  197,  232,  231,  140,  198,  317,  142,  317,
 /*   670 */    55,  232,  231,  161,  231,  162,  279,  277,  280,  287,
 /*   680 */    65,  232,  295,   66,  232,  299,   67,  300,  313,   68,
 /*   690 */   216,  314,  232,   69,  232,   42,   43,  478,  320,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    39,  108,   25,   26,   27,  112,   29,   30,  132,  133,
 /*    10 */   134,  135,  136,   52,   53,   54,   55,   56,   57,   58,
 /*    20 */    59,   60,   61,   62,   63,   64,   65,   66,   67,   68,
 /*    30 */    69,   70,   71,   72,   73,   74,   75,   76,   77,   78,
 /*    40 */    79,   80,    0,   31,    2,    3,  111,    5,    6,    7,
 /*    50 */    31,   24,   25,    8,   12,   13,   14,   15,   31,   32,
 /*    60 */    33,   34,   35,   11,  129,    8,    9,   25,   26,   27,
 /*    70 */    28,   29,   30,   46,   97,   98,   31,   32,   36,    0,
 /*    80 */    28,    2,    3,   25,    5,    6,    7,   12,   13,   14,
 /*    90 */    32,   12,   13,   14,   15,   38,   84,   40,   41,  102,
 /*   100 */    43,   44,   32,   28,   25,   26,   27,   28,   29,   30,
 /*   110 */   107,  115,  115,   31,   32,   36,  120,  120,   33,   34,
 /*   120 */   124,  124,   95,   96,  128,  122,  106,   85,   86,    1,
 /*   130 */    88,   89,   90,    8,  137,   32,  140,  140,   25,   26,
 /*   140 */    27,   28,   29,   30,   16,   17,   18,   19,   20,   21,
 /*   150 */   110,   23,  132,   25,   26,   27,   28,   29,   30,  100,
 /*   160 */   101,  102,   37,    8,   85,   86,    6,   88,   89,   90,
 /*   170 */     0,  131,    2,    3,   32,    5,    6,    7,  113,   24,
 /*   180 */    29,   30,   12,   13,   14,   15,   31,   32,   33,   34,
 /*   190 */    35,   33,   34,   35,  102,   25,   26,   27,   28,   29,
 /*   200 */    30,   46,  102,  129,   46,   31,   36,   24,    1,  120,
 /*   210 */   110,    0,   15,  106,   31,   32,   33,   34,   35,   91,
 /*   220 */    92,   93,   94,   16,   17,   18,   19,   20,   21,   46,
 /*   230 */    23,  131,   25,   26,   27,    6,   29,   30,  109,  132,
 /*   240 */    11,   25,   26,   27,  115,   29,   30,   36,  103,  120,
 /*   250 */    95,   96,   15,  124,   96,   85,   86,  128,   88,   89,
 /*   260 */    90,    0,  105,    2,    3,   28,    5,    6,    7,  140,
 /*   270 */    32,   33,   34,   12,   13,   14,   15,  125,   95,   96,
 /*   280 */   123,  120,   85,   86,   24,   88,   89,   90,   11,   28,
 /*   290 */    31,   31,   32,   33,   34,   35,  120,   36,   91,   92,
 /*   300 */    93,   94,    0,  110,    2,    3,   46,    5,    6,    7,
 /*   310 */    15,   12,   13,   14,   12,   13,   14,   15,   12,   16,
 /*   320 */    17,   18,   45,   86,  131,   88,   89,   90,    1,  120,
 /*   330 */    28,   25,   26,   27,  115,   29,   30,  104,   36,  120,
 /*   340 */   120,   31,  120,  124,  120,   25,   85,   86,   12,   88,
 /*   350 */    89,   90,   32,   11,  120,   95,   96,  138,  125,  140,
 /*   360 */   141,   25,   26,   27,  120,   29,   30,  120,    2,    3,
 /*   370 */    28,    5,    6,    7,   47,   48,   49,   50,   51,  139,
 /*   380 */    11,   86,   32,   88,   89,   90,  123,   85,   86,  127,
 /*   390 */    88,   89,   90,    0,   84,    2,    3,   28,    5,    6,
 /*   400 */     7,  122,   11,  115,  112,   12,   13,   14,  120,   11,
 /*   410 */    11,   31,  124,  115,  126,  115,   81,  116,  120,   28,
 /*   420 */   120,   28,  124,   82,  126,  117,   28,   28,  140,   36,
 /*   430 */     0,  118,    2,    3,  116,    5,    6,    7,  140,    0,
 /*   440 */   140,  117,   12,   13,   14,    0,  118,    2,    3,  116,
 /*   450 */     5,    6,    7,  116,  115,  115,   11,   31,   28,  120,
 /*   460 */   120,  117,  115,  124,  124,  117,   36,  120,  116,  130,
 /*   470 */   130,  124,  118,   28,  118,   36,  117,  130,  118,  140,
 /*   480 */   140,   36,  117,  116,  114,    0,  118,  140,  116,  115,
 /*   490 */   115,  115,  115,  115,  120,  120,  120,  120,  120,  124,
 /*   500 */   124,  124,  124,  115,  115,  130,  130,  130,  120,  120,
 /*   510 */   116,  119,  124,  124,  140,  140,  140,  140,  140,  141,
 /*   520 */    11,  115,  115,  118,  116,  137,  120,  120,  140,  140,
 /*   530 */   124,  124,  117,  119,  115,  117,  130,   28,  118,  120,
 /*   540 */   115,  119,  118,  124,  117,  120,  140,  140,  115,  124,
 /*   550 */   119,  115,  117,  120,   32,  118,  120,  124,  115,  140,
 /*   560 */   124,  115,   11,  120,   32,  140,  120,  124,   10,   32,
 /*   570 */   124,   84,  115,  140,   28,  115,  140,  120,   11,    4,
 /*   580 */   120,  124,  115,  140,  124,  115,  140,  120,    4,   11,
 /*   590 */   120,  124,   32,  115,  124,   87,  115,  140,  120,   87,
 /*   600 */   140,  120,  124,  115,   28,  124,  115,  140,  120,   84,
 /*   610 */   140,  120,  124,   87,  115,  124,   87,  115,  140,  120,
 /*   620 */   115,  140,  120,  124,   32,  120,  124,   15,  140,  124,
 /*   630 */    32,  140,  115,   84,   32,  115,   32,  120,  115,  140,
 /*   640 */   120,  124,  140,  120,  124,  140,  115,  124,   39,   19,
 /*   650 */    32,  120,   42,   11,   83,  124,  115,  140,   32,  115,
 /*   660 */   140,  120,   42,  140,  120,  124,   32,  115,  124,  115,
 /*   670 */    31,  140,  120,  121,  120,  121,   49,   46,   49,   28,
 /*   680 */    11,  140,   28,   31,  140,   28,   31,   28,   28,   31,
 /*   690 */    42,   28,  140,   31,  140,   31,   31,  142,   32,
};
#define YY_SHIFT_USE_DFLT (-40)
#define YY_SHIFT_MAX 216
static const short yy_shift_ofst[] = {
 /*     0 */    57,   27,  155,  183,   27,  183,  183,  260,  260,  260,
 /*    10 */   260,  260,  260,  183,  183,  260,  366,  183,  183,  183,
 /*    20 */   183,  183,  183,  183,  183,  183,  183,  183,  183,  183,
 /*    30 */   183,  183,  183,  183,  183,  183,  183,  183,  183,  183,
 /*    40 */   183,  183,  158,  158,  158,  158,   45,  295,   82,   82,
 /*    50 */   229,   19,   70,  160,  103,  142,  -39,  237,  197,  238,
 /*    60 */   125,   85,   85,   85,   85,   85,   85,   85,   85,   85,
 /*    70 */   174,   19,  259,  350,   70,  103,  142,  380,  335,  341,
 /*    80 */   380,  335,  341,  380,  335,  341,  380,  335,  341,  380,
 /*    90 */   335,  341,  380,  335,  341,  380,  426,  335,  341,  426,
 /*   100 */   335,  341,  426,  335,  341,  426,  335,  341,  380,  380,
 /*   110 */   -40,   42,   79,  170,  128,  261,  207,  302,  393,  430,
 /*   120 */   445,  -23,  113,  306,  336,  327,  216,  216,  216,  216,
 /*   130 */   216,  216,  216,  216,  216,  216,  216,  216,  216,  216,
 /*   140 */   216,  216,  216,   75,  303,  299,  211,   52,   12,  369,
 /*   150 */   151,  151,  151,  277,  391,  310,   58,  320,  342,  439,
 /*   160 */   398,  399,  509,  485,  558,  522,  551,  532,  487,  537,
 /*   170 */   546,  567,  575,  584,  578,  560,  508,  512,  526,  529,
 /*   180 */   576,  525,  592,  612,  598,  549,  612,  602,  604,  609,
 /*   190 */   630,  618,  610,  642,  571,  626,  620,  634,  639,  631,
 /*   200 */   627,  629,  651,  669,  654,  652,  657,  655,  659,  658,
 /*   210 */   660,  662,  663,  664,  665,  648,  666,
};
#define YY_REDUCE_USE_DFLT (-125)
#define YY_REDUCE_MAX 110
static const short yy_reduce_ofst[] = {
 /*     0 */    59,  129,   -3,  219,   -4,  288,  298,  339,  340,  347,
 /*    10 */   375,  376,  377,  388,  378,  406, -124,  389,  407,  419,
 /*    20 */   425,  433,  436,  443,  446,  457,  460,  467,  470,  478,
 /*    30 */   481,  488,  491,  499,  502,  505,  517,  520,  523,  531,
 /*    40 */   541,  544,  552,  554,  300,  374,  100,  -65,   40,  193,
 /*    50 */    20,  233,  157,  107,    3, -107,   65,   74,   74,   89,
 /*    60 */    92,  161,  176,  209,  220,  222,  224,  234,  244,  247,
 /*    70 */   145,  152,  240,  262,  263,  279,  292,  301,  308,  313,
 /*    80 */   318,  324,  328,  333,  344,  354,  337,  348,  356,  352,
 /*    90 */   359,  360,  367,  365,  368,  372,  392,  415,  405,  414,
 /*   100 */   418,  420,  422,  427,  424,  431,  435,  437,  394,  408,
 /*   110 */   370,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*    10 */   476,  476,  476,  476,  476,  476,  331,  476,  476,  476,
 /*    20 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*    30 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*    40 */   476,  476,  476,  476,  476,  476,  476,  418,  476,  476,
 /*    50 */   425,  476,  476,  425,  476,  476,  476,  476,  476,  476,
 /*    60 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*    70 */   394,  476,  476,  476,  476,  476,  476,  377,  373,  375,
 /*    80 */   377,  373,  375,  377,  373,  375,  377,  373,  375,  377,
 /*    90 */   373,  375,  377,  373,  375,  377,  379,  373,  375,  379,
 /*   100 */   373,  375,  379,  373,  375,  379,  373,  375,  377,  377,
 /*   110 */   341,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*   120 */   476,  475,  476,  476,  476,  334,  444,  445,  459,  470,
 /*   130 */   469,  430,  434,  435,  436,  437,  438,  439,  440,  441,
 /*   140 */   442,  397,  387,  476,  476,  404,  476,  476,  449,  476,
 /*   150 */   456,  457,  458,  476,  476,  449,  476,  476,  476,  476,
 /*   160 */   476,  476,  476,  476,  476,  476,  325,  476,  448,  476,
 /*   170 */   476,  476,  476,  476,  423,  476,  476,  476,  476,  476,
 /*   180 */   476,  408,  476,  395,  476,  448,  396,  476,  476,  476,
 /*   190 */   476,  476,  476,  328,  476,  476,  476,  476,  476,  476,
 /*   200 */   476,  476,  476,  476,  476,  476,  476,  476,  476,  476,
 /*   210 */   476,  476,  476,  476,  476,  476,  476,  323,  324,  388,
 /*   220 */   390,  446,  447,  452,  450,  451,  462,  463,  453,  464,
 /*   230 */   465,  466,  467,  468,  455,  454,  389,  413,  414,  415,
 /*   240 */   416,  417,  426,  428,  460,  461,  443,  420,  473,  474,
 /*   250 */   471,  472,  402,  405,  409,  410,  411,  412,  403,  406,
 /*   260 */   407,  401,  398,  399,  400,  391,  393,  392,  326,  385,
 /*   270 */   386,  327,  382,  384,  383,  329,  332,  335,  336,  337,
 /*   280 */   338,  339,  340,  342,  343,  374,  372,  376,  344,  345,
 /*   290 */   346,  347,  348,  349,  350,  378,  351,  352,  353,  354,
 /*   300 */   355,  356,  357,  358,  359,  360,  361,  362,  363,  364,
 /*   310 */   365,  366,  367,  368,  369,  370,  380,  381,  371,  333,
 /*   320 */   330,
};
#define YY_SZ_ACTTAB (int)(sizeof(yy_action)/sizeof(yy_action[0]))

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
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
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
  "DOT",           "ON",            "INNER",         "JOIN",        
  "OUTER",         "LEFT",          "RIGHT",         "IN",          
  "NOT_EQUAL",     "UNEQUAL",       "LESSER_EQUAL",  "COLON",       
  "DNUM",          "ASC",           "DESC",          "error",       
  "terminal_statement",  "statement",     "selectstatement",  "optionalinsertfieldlist",
  "insertgrouplist",  "setlist",       "optionalwhereclause",  "togrouplist", 
  "fielddescriptorlist",  "fieldlist",     "tableexpr",     "optionalquerymodifiers",
  "fielddescriptor",  "fielddescriptortype",  "optionalfielddescriptormodifierlist",  "literal",     
  "optionalprecision",  "optionalunsigned",  "optionalzerofill",  "optionalfloatprecision",
  "intnum",        "literallist",   "togroup",       "setexpr",     
  "expr",          "insertgroup",   "exprlist",      "labellist",   
  "field",         "joinclause",    "cond",          "tablename",   
  "whereclause",   "limitclause",   "havingclause",  "groupclause", 
  "orderclause",   "grouplist",     "orderlist",     "inexpr",      
  "dblnum",        "orderelement",
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
 /*  77 */ "field ::= LABEL DOT LABEL DOT MUL",
 /*  78 */ "field ::= LABEL DOT MUL",
 /*  79 */ "field ::= MUL",
 /*  80 */ "field ::= field AS LABEL",
 /*  81 */ "tableexpr ::= LPAREN tableexpr RPAREN",
 /*  82 */ "tableexpr ::= LPAREN selectstatement RPAREN",
 /*  83 */ "tableexpr ::= tableexpr joinclause tableexpr ON cond",
 /*  84 */ "tableexpr ::= tableexpr AS LABEL",
 /*  85 */ "tableexpr ::= tablename",
 /*  86 */ "tablename ::= LABEL DOT LABEL",
 /*  87 */ "tablename ::= LABEL",
 /*  88 */ "joinclause ::= INNER JOIN",
 /*  89 */ "joinclause ::= OUTER JOIN",
 /*  90 */ "joinclause ::= LEFT JOIN",
 /*  91 */ "joinclause ::= RIGHT JOIN",
 /*  92 */ "optionalquerymodifiers ::= optionalquerymodifiers whereclause",
 /*  93 */ "optionalquerymodifiers ::= optionalquerymodifiers limitclause",
 /*  94 */ "optionalquerymodifiers ::= optionalquerymodifiers havingclause",
 /*  95 */ "optionalquerymodifiers ::= optionalquerymodifiers groupclause",
 /*  96 */ "optionalquerymodifiers ::= optionalquerymodifiers orderclause",
 /*  97 */ "optionalquerymodifiers ::=",
 /*  98 */ "whereclause ::= WHERE cond",
 /*  99 */ "limitclause ::= LIMIT intnum COMMA intnum",
 /* 100 */ "havingclause ::= HAVING cond",
 /* 101 */ "groupclause ::= GROUP BY grouplist",
 /* 102 */ "orderclause ::= ORDER BY orderlist",
 /* 103 */ "optionalwhereclause ::= whereclause",
 /* 104 */ "optionalwhereclause ::=",
 /* 105 */ "cond ::= cond AND cond",
 /* 106 */ "cond ::= cond OR cond",
 /* 107 */ "cond ::= cond XOR cond",
 /* 108 */ "cond ::= expr IN inexpr",
 /* 109 */ "cond ::= expr EQUALS expr",
 /* 110 */ "cond ::= expr NOT_EQUAL expr",
 /* 111 */ "cond ::= expr UNEQUAL expr",
 /* 112 */ "cond ::= expr LESSER expr",
 /* 113 */ "cond ::= expr GREATER expr",
 /* 114 */ "cond ::= expr LESSER_EQUAL expr",
 /* 115 */ "cond ::= expr GREATER_EQUAL expr",
 /* 116 */ "cond ::= expr LIKE expr",
 /* 117 */ "cond ::= expr RLIKE expr",
 /* 118 */ "cond ::= expr BETWEEN expr AND expr",
 /* 119 */ "cond ::= expr NOT LIKE expr",
 /* 120 */ "cond ::= expr NOT RLIKE expr",
 /* 121 */ "cond ::= expr NOT BETWEEN expr AND expr",
 /* 122 */ "cond ::= LPAREN cond RPAREN",
 /* 123 */ "exprlist ::= exprlist COMMA expr",
 /* 124 */ "exprlist ::= expr",
 /* 125 */ "expr ::= literal",
 /* 126 */ "expr ::= LABEL DOT LABEL DOT LABEL",
 /* 127 */ "expr ::= LABEL DOT LABEL",
 /* 128 */ "expr ::= LABEL",
 /* 129 */ "expr ::= COLON LABEL",
 /* 130 */ "expr ::= COLON intnum",
 /* 131 */ "expr ::= LABEL LPAREN exprlist RPAREN",
 /* 132 */ "expr ::= LPAREN expr RPAREN",
 /* 133 */ "expr ::= expr PLUS expr",
 /* 134 */ "expr ::= expr MINUS expr",
 /* 135 */ "expr ::= expr MUL expr",
 /* 136 */ "expr ::= expr DIV expr",
 /* 137 */ "expr ::= expr MOD expr",
 /* 138 */ "expr ::= DISTINCT expr",
 /* 139 */ "inexpr ::= LPAREN grouplist RPAREN",
 /* 140 */ "inexpr ::= LPAREN selectstatement RPAREN",
 /* 141 */ "intnum ::= LNUM",
 /* 142 */ "intnum ::= HNUM",
 /* 143 */ "dblnum ::= DNUM",
 /* 144 */ "literal ::= STRING",
 /* 145 */ "literal ::= intnum",
 /* 146 */ "literal ::= dblnum",
 /* 147 */ "literal ::= NULL",
 /* 148 */ "grouplist ::= grouplist COMMA expr",
 /* 149 */ "grouplist ::= expr",
 /* 150 */ "orderlist ::= orderlist COMMA orderelement",
 /* 151 */ "orderlist ::= orderelement",
 /* 152 */ "orderelement ::= expr ASC",
 /* 153 */ "orderelement ::= expr DESC",
 /* 154 */ "orderelement ::= expr",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

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
#if YYSTACKDEPTH<=0
    yyGrowStack(pParser);
#endif
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
    case 97:
    case 98:
#line 368 "pdo_user_sql_parser.lemon"
{
	if ((yypminor->yy0).freeme) {
		efree((yypminor->yy0).token);
	}
}
#line 1158 "pdo_user_sql_parser.c"
      break;
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
    case 120:
    case 121:
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
    case 139:
    case 140:
    case 141:
#line 377 "pdo_user_sql_parser.lemon"
{ zval_ptr_dtor(&(yypminor->yy252)); }
#line 1201 "pdo_user_sql_parser.c"
      break;
    case 122:
    case 123:
#line 463 "pdo_user_sql_parser.lemon"
{ zval_ptr_dtor(&(yypminor->yy174)[0]); zval_ptr_dtor(&(yypminor->yy174)[1]); efree((yypminor->yy174)); }
#line 1207 "pdo_user_sql_parser.c"
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
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
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
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>YY_SHIFT_MAX || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  if( iLookAhead==YYNOCODE ){
    return YY_NO_ACTION;
  }
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
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
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && yy_lookahead[j]==YYWILDCARD ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
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
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  /* int stateno = pParser->yystack[pParser->yyidx].stateno; */
 
  if( stateno>YY_REDUCE_MAX ||
      (i = yy_reduce_ofst[stateno])==YY_REDUCE_USE_DFLT ){
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
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor){
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
#if YYSTACKDEPTH>0 
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser, yypMinor);
      return;
    }
  }
#endif
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
  { 100, 2 },
  { 100, 1 },
  { 101, 1 },
  { 101, 5 },
  { 101, 6 },
  { 101, 5 },
  { 101, 3 },
  { 101, 3 },
  { 101, 6 },
  { 101, 3 },
  { 102, 5 },
  { 108, 3 },
  { 108, 1 },
  { 112, 3 },
  { 114, 3 },
  { 114, 3 },
  { 114, 3 },
  { 114, 3 },
  { 114, 2 },
  { 114, 2 },
  { 114, 0 },
  { 113, 1 },
  { 113, 4 },
  { 113, 4 },
  { 113, 4 },
  { 113, 4 },
  { 113, 4 },
  { 113, 4 },
  { 113, 2 },
  { 113, 4 },
  { 113, 4 },
  { 113, 4 },
  { 113, 4 },
  { 113, 4 },
  { 113, 4 },
  { 113, 1 },
  { 113, 1 },
  { 113, 2 },
  { 113, 2 },
  { 113, 1 },
  { 113, 1 },
  { 113, 1 },
  { 113, 1 },
  { 113, 1 },
  { 113, 1 },
  { 113, 1 },
  { 113, 1 },
  { 113, 4 },
  { 113, 4 },
  { 113, 4 },
  { 113, 4 },
  { 117, 1 },
  { 117, 0 },
  { 118, 1 },
  { 118, 0 },
  { 116, 3 },
  { 116, 0 },
  { 119, 5 },
  { 119, 0 },
  { 121, 3 },
  { 121, 1 },
  { 107, 3 },
  { 107, 1 },
  { 122, 3 },
  { 105, 3 },
  { 105, 1 },
  { 123, 3 },
  { 104, 3 },
  { 104, 1 },
  { 125, 3 },
  { 127, 3 },
  { 127, 1 },
  { 103, 3 },
  { 103, 0 },
  { 109, 3 },
  { 109, 1 },
  { 128, 1 },
  { 128, 5 },
  { 128, 3 },
  { 128, 1 },
  { 128, 3 },
  { 110, 3 },
  { 110, 3 },
  { 110, 5 },
  { 110, 3 },
  { 110, 1 },
  { 131, 3 },
  { 131, 1 },
  { 129, 2 },
  { 129, 2 },
  { 129, 2 },
  { 129, 2 },
  { 111, 2 },
  { 111, 2 },
  { 111, 2 },
  { 111, 2 },
  { 111, 2 },
  { 111, 0 },
  { 132, 2 },
  { 133, 4 },
  { 134, 2 },
  { 135, 3 },
  { 136, 3 },
  { 106, 1 },
  { 106, 0 },
  { 130, 3 },
  { 130, 3 },
  { 130, 3 },
  { 130, 3 },
  { 130, 3 },
  { 130, 3 },
  { 130, 3 },
  { 130, 3 },
  { 130, 3 },
  { 130, 3 },
  { 130, 3 },
  { 130, 3 },
  { 130, 3 },
  { 130, 5 },
  { 130, 4 },
  { 130, 4 },
  { 130, 6 },
  { 130, 3 },
  { 126, 3 },
  { 126, 1 },
  { 124, 1 },
  { 124, 5 },
  { 124, 3 },
  { 124, 1 },
  { 124, 2 },
  { 124, 2 },
  { 124, 4 },
  { 124, 3 },
  { 124, 3 },
  { 124, 3 },
  { 124, 3 },
  { 124, 3 },
  { 124, 3 },
  { 124, 2 },
  { 139, 3 },
  { 139, 3 },
  { 120, 1 },
  { 120, 1 },
  { 140, 1 },
  { 115, 1 },
  { 115, 1 },
  { 115, 1 },
  { 115, 1 },
  { 137, 3 },
  { 137, 1 },
  { 138, 3 },
  { 138, 1 },
  { 141, 2 },
  { 141, 2 },
  { 141, 1 },
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
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  **
  ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
  ** without this code, their parser segfaults.  I'm not sure what there
  ** parser is doing to make this happen.  This is the second bug report
  ** from wireshark this week.  Clearly they are stressing Lemon in ways
  ** that it has not been previously stressed...  (SQLite ticket #2172)
  */
  memset(&yygotominor, 0, sizeof(yygotominor));


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
#line 374 "pdo_user_sql_parser.lemon"
{ pusp_do_terminal_statement(&return_value, yymsp[-1].minor.yy252, 1);   yy_destructor(36,&yymsp[0].minor);
}
#line 1629 "pdo_user_sql_parser.c"
        break;
      case 1:
#line 375 "pdo_user_sql_parser.lemon"
{ pusp_do_terminal_statement(&return_value, yymsp[0].minor.yy252, 0); }
#line 1634 "pdo_user_sql_parser.c"
        break;
      case 2:
      case 76:
      case 85:
      case 103:
      case 125:
      case 145:
      case 146:
#line 378 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = yymsp[0].minor.yy252; }
#line 1645 "pdo_user_sql_parser.c"
        break;
      case 3:
#line 379 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_insert_select_statement(pusp_zvalize_token(&yymsp[-2].minor.yy0), yymsp[-1].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(9,&yymsp[-4].minor);
  yy_destructor(10,&yymsp[-3].minor);
}
#line 1652 "pdo_user_sql_parser.c"
        break;
      case 4:
#line 380 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_insert_statement(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(9,&yymsp[-5].minor);
  yy_destructor(10,&yymsp[-4].minor);
  yy_destructor(37,&yymsp[-1].minor);
}
#line 1660 "pdo_user_sql_parser.c"
        break;
      case 5:
#line 381 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_update_statement(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-1].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(38,&yymsp[-4].minor);
  yy_destructor(39,&yymsp[-2].minor);
}
#line 1667 "pdo_user_sql_parser.c"
        break;
      case 6:
#line 382 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_delete_statement(pusp_zvalize_token(&yymsp[-1].minor.yy0), yymsp[0].minor.yy252);   yy_destructor(40,&yymsp[-2].minor);
}
#line 1673 "pdo_user_sql_parser.c"
        break;
      case 7:
#line 383 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_rename_statement(yymsp[0].minor.yy252);   yy_destructor(41,&yymsp[-2].minor);
  yy_destructor(42,&yymsp[-1].minor);
}
#line 1680 "pdo_user_sql_parser.c"
        break;
      case 8:
#line 384 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_create_statement(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-1].minor.yy252);   yy_destructor(43,&yymsp[-5].minor);
  yy_destructor(42,&yymsp[-4].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1689 "pdo_user_sql_parser.c"
        break;
      case 9:
#line 385 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_drop_statement(pusp_zvalize_token(&yymsp[0].minor.yy0));   yy_destructor(44,&yymsp[-2].minor);
  yy_destructor(42,&yymsp[-1].minor);
}
#line 1696 "pdo_user_sql_parser.c"
        break;
      case 10:
#line 388 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_select_statement(yymsp[-3].minor.yy252,yymsp[-1].minor.yy252,yymsp[0].minor.yy252);   yy_destructor(8,&yymsp[-4].minor);
  yy_destructor(45,&yymsp[-2].minor);
}
#line 1703 "pdo_user_sql_parser.c"
        break;
      case 11:
      case 59:
      case 67:
      case 74:
      case 123:
      case 148:
      case 150:
#line 391 "pdo_user_sql_parser.lemon"
{ add_next_index_zval(yymsp[-2].minor.yy252, yymsp[0].minor.yy252); yygotominor.yy252 = yymsp[-2].minor.yy252;   yy_destructor(11,&yymsp[-1].minor);
}
#line 1715 "pdo_user_sql_parser.c"
        break;
      case 12:
      case 60:
      case 68:
      case 75:
      case 124:
      case 149:
      case 151:
#line 392 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); array_init(yygotominor.yy252); add_next_index_zval(yygotominor.yy252, yymsp[0].minor.yy252); }
#line 1726 "pdo_user_sql_parser.c"
        break;
      case 13:
#line 395 "pdo_user_sql_parser.lemon"
{ add_assoc_zval(yymsp[-1].minor.yy252, "name", pusp_zvalize_token(&yymsp[-2].minor.yy0)); add_assoc_zval(yymsp[-1].minor.yy252, "flags", yymsp[0].minor.yy252); yygotominor.yy252 = yymsp[-1].minor.yy252; }
#line 1731 "pdo_user_sql_parser.c"
        break;
      case 14:
#line 398 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-2].minor.yy252, "not null", 1); yygotominor.yy252 = yymsp[-2].minor.yy252;   yy_destructor(1,&yymsp[-1].minor);
  yy_destructor(46,&yymsp[0].minor);
}
#line 1738 "pdo_user_sql_parser.c"
        break;
      case 15:
#line 399 "pdo_user_sql_parser.lemon"
{ add_assoc_zval(yymsp[-2].minor.yy252, "default", yymsp[0].minor.yy252); yygotominor.yy252 = yymsp[-2].minor.yy252;   yy_destructor(47,&yymsp[-1].minor);
}
#line 1744 "pdo_user_sql_parser.c"
        break;
      case 16:
#line 400 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-2].minor.yy252, "primary key", 1); yygotominor.yy252 = yymsp[-2].minor.yy252;   yy_destructor(48,&yymsp[-1].minor);
  yy_destructor(49,&yymsp[0].minor);
}
#line 1751 "pdo_user_sql_parser.c"
        break;
      case 17:
#line 401 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-2].minor.yy252, "unique key", 1); yygotominor.yy252 = yymsp[-2].minor.yy252;   yy_destructor(50,&yymsp[-1].minor);
  yy_destructor(49,&yymsp[0].minor);
}
#line 1758 "pdo_user_sql_parser.c"
        break;
      case 18:
#line 402 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-1].minor.yy252, "key", 1); yygotominor.yy252 = yymsp[-1].minor.yy252;   yy_destructor(49,&yymsp[0].minor);
}
#line 1764 "pdo_user_sql_parser.c"
        break;
      case 19:
#line 403 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-1].minor.yy252, "auto_increment", 1); yygotominor.yy252 = yymsp[-1].minor.yy252;   yy_destructor(51,&yymsp[0].minor);
}
#line 1770 "pdo_user_sql_parser.c"
        break;
      case 20:
#line 404 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); array_init(yygotominor.yy252); }
#line 1775 "pdo_user_sql_parser.c"
        break;
      case 21:
#line 407 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("bit", NULL, NULL);   yy_destructor(52,&yymsp[0].minor);
}
#line 1781 "pdo_user_sql_parser.c"
        break;
      case 22:
#line 408 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_num("int", yymsp[-2].minor.yy252, yymsp[-1].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(53,&yymsp[-3].minor);
}
#line 1787 "pdo_user_sql_parser.c"
        break;
      case 23:
#line 409 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_num("integer", yymsp[-2].minor.yy252, yymsp[-1].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(54,&yymsp[-3].minor);
}
#line 1793 "pdo_user_sql_parser.c"
        break;
      case 24:
#line 410 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_num("tinyint", yymsp[-2].minor.yy252, yymsp[-1].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(55,&yymsp[-3].minor);
}
#line 1799 "pdo_user_sql_parser.c"
        break;
      case 25:
#line 411 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_num("smallint", yymsp[-2].minor.yy252, yymsp[-1].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(56,&yymsp[-3].minor);
}
#line 1805 "pdo_user_sql_parser.c"
        break;
      case 26:
#line 412 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_num("mediumint", yymsp[-2].minor.yy252, yymsp[-1].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(57,&yymsp[-3].minor);
}
#line 1811 "pdo_user_sql_parser.c"
        break;
      case 27:
#line 413 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_num("bigint", yymsp[-2].minor.yy252, yymsp[-1].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(58,&yymsp[-3].minor);
}
#line 1817 "pdo_user_sql_parser.c"
        break;
      case 28:
#line 414 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("year", "precision", yymsp[0].minor.yy252);   yy_destructor(59,&yymsp[-1].minor);
}
#line 1823 "pdo_user_sql_parser.c"
        break;
      case 29:
#line 415 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_num("float", yymsp[-2].minor.yy252, yymsp[-1].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(60,&yymsp[-3].minor);
}
#line 1829 "pdo_user_sql_parser.c"
        break;
      case 30:
#line 416 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_num("real", yymsp[-2].minor.yy252, yymsp[-1].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(61,&yymsp[-3].minor);
}
#line 1835 "pdo_user_sql_parser.c"
        break;
      case 31:
#line 417 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_num("decimal", yymsp[-2].minor.yy252, yymsp[-1].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(62,&yymsp[-3].minor);
}
#line 1841 "pdo_user_sql_parser.c"
        break;
      case 32:
#line 418 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_num("double", yymsp[-2].minor.yy252, yymsp[-1].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(63,&yymsp[-3].minor);
}
#line 1847 "pdo_user_sql_parser.c"
        break;
      case 33:
#line 419 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("char", "length", yymsp[-1].minor.yy252);   yy_destructor(64,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1855 "pdo_user_sql_parser.c"
        break;
      case 34:
#line 420 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("varchar", "length", yymsp[-1].minor.yy252);   yy_destructor(65,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1863 "pdo_user_sql_parser.c"
        break;
      case 35:
#line 421 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("text", "date", NULL);   yy_destructor(66,&yymsp[0].minor);
}
#line 1869 "pdo_user_sql_parser.c"
        break;
      case 36:
#line 422 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("text", "time", NULL);   yy_destructor(67,&yymsp[0].minor);
}
#line 1875 "pdo_user_sql_parser.c"
        break;
      case 37:
#line 423 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("datetime", "precision", yymsp[0].minor.yy252);   yy_destructor(68,&yymsp[-1].minor);
}
#line 1881 "pdo_user_sql_parser.c"
        break;
      case 38:
#line 424 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("timestamp", "precision", yymsp[0].minor.yy252);   yy_destructor(69,&yymsp[-1].minor);
}
#line 1887 "pdo_user_sql_parser.c"
        break;
      case 39:
#line 425 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("text", "precision", NULL);   yy_destructor(70,&yymsp[0].minor);
}
#line 1893 "pdo_user_sql_parser.c"
        break;
      case 40:
#line 426 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "tiny", 1); yygotominor.yy252 = pusp_do_declare_type("text", "precision", p);   yy_destructor(71,&yymsp[0].minor);
}
#line 1899 "pdo_user_sql_parser.c"
        break;
      case 41:
#line 427 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "medium", 1); yygotominor.yy252 = pusp_do_declare_type("text", "precision", p);   yy_destructor(72,&yymsp[0].minor);
}
#line 1905 "pdo_user_sql_parser.c"
        break;
      case 42:
#line 428 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "long", 1); yygotominor.yy252 = pusp_do_declare_type("text", "precision", p);   yy_destructor(73,&yymsp[0].minor);
}
#line 1911 "pdo_user_sql_parser.c"
        break;
      case 43:
#line 429 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("blob", "precision", NULL);   yy_destructor(74,&yymsp[0].minor);
}
#line 1917 "pdo_user_sql_parser.c"
        break;
      case 44:
#line 430 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "tiny", 1); yygotominor.yy252 = pusp_do_declare_type("blob", "precision", p);   yy_destructor(75,&yymsp[0].minor);
}
#line 1923 "pdo_user_sql_parser.c"
        break;
      case 45:
#line 431 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "medium", 1); yygotominor.yy252 = pusp_do_declare_type("blob", "precision", p);   yy_destructor(76,&yymsp[0].minor);
}
#line 1929 "pdo_user_sql_parser.c"
        break;
      case 46:
#line 432 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "long", 1); yygotominor.yy252 = pusp_do_declare_type("blob", "precision", p);   yy_destructor(77,&yymsp[0].minor);
}
#line 1935 "pdo_user_sql_parser.c"
        break;
      case 47:
#line 433 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("binary", "length", yymsp[-1].minor.yy252);   yy_destructor(78,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1943 "pdo_user_sql_parser.c"
        break;
      case 48:
#line 434 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("varbinary", "length", yymsp[-1].minor.yy252);   yy_destructor(79,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1951 "pdo_user_sql_parser.c"
        break;
      case 49:
#line 435 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("set", "flags", yymsp[-1].minor.yy252);   yy_destructor(39,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1959 "pdo_user_sql_parser.c"
        break;
      case 50:
#line 436 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_declare_type("enum", "values", yymsp[-1].minor.yy252);   yy_destructor(80,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1967 "pdo_user_sql_parser.c"
        break;
      case 51:
#line 439 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); ZVAL_TRUE(yygotominor.yy252);   yy_destructor(81,&yymsp[0].minor);
}
#line 1973 "pdo_user_sql_parser.c"
        break;
      case 52:
      case 54:
#line 440 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); ZVAL_FALSE(yygotominor.yy252); }
#line 1979 "pdo_user_sql_parser.c"
        break;
      case 53:
#line 443 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); ZVAL_TRUE(yygotominor.yy252);   yy_destructor(82,&yymsp[0].minor);
}
#line 1985 "pdo_user_sql_parser.c"
        break;
      case 55:
      case 69:
      case 72:
      case 81:
      case 82:
      case 122:
      case 132:
      case 139:
      case 140:
#line 447 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = yymsp[-1].minor.yy252;   yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 2000 "pdo_user_sql_parser.c"
        break;
      case 56:
      case 58:
      case 73:
      case 97:
      case 104:
#line 448 "pdo_user_sql_parser.lemon"
{ TSRMLS_FETCH(); yygotominor.yy252 = EG(uninitialized_zval_ptr); }
#line 2009 "pdo_user_sql_parser.c"
        break;
      case 57:
#line 451 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); array_init(yygotominor.yy252); add_assoc_zval(yygotominor.yy252, "length", yymsp[-3].minor.yy252); add_assoc_zval(yygotominor.yy252, "decimals", yymsp[-1].minor.yy252);   yy_destructor(31,&yymsp[-4].minor);
  yy_destructor(11,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 2017 "pdo_user_sql_parser.c"
        break;
      case 61:
#line 459 "pdo_user_sql_parser.lemon"
{ pusp_do_push_labeled_zval(yymsp[-2].minor.yy252, yymsp[0].minor.yy174); yygotominor.yy252 = yymsp[-2].minor.yy252;   yy_destructor(11,&yymsp[-1].minor);
}
#line 2023 "pdo_user_sql_parser.c"
        break;
      case 62:
      case 65:
#line 460 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); array_init(yygotominor.yy252); pusp_do_push_labeled_zval(yygotominor.yy252, yymsp[0].minor.yy174); }
#line 2029 "pdo_user_sql_parser.c"
        break;
      case 63:
#line 464 "pdo_user_sql_parser.lemon"
{ zval **tmp = safe_emalloc(2, sizeof(zval*), 0); tmp[0] = pusp_zvalize_token(&yymsp[-2].minor.yy0); tmp[1] = pusp_zvalize_token(&yymsp[0].minor.yy0); yygotominor.yy174 = tmp;   yy_destructor(83,&yymsp[-1].minor);
}
#line 2035 "pdo_user_sql_parser.c"
        break;
      case 64:
#line 467 "pdo_user_sql_parser.lemon"
{ pusp_do_push_labeled_zval(yymsp[-2].minor.yy252, (zval**)yymsp[0].minor.yy174); yygotominor.yy252 = yymsp[-2].minor.yy252;   yy_destructor(11,&yymsp[-1].minor);
}
#line 2041 "pdo_user_sql_parser.c"
        break;
      case 66:
#line 472 "pdo_user_sql_parser.lemon"
{ zval **tmp = safe_emalloc(2, sizeof(zval*), 0); tmp[0] = pusp_zvalize_token(&yymsp[-2].minor.yy0); tmp[1] = yymsp[0].minor.yy252; yygotominor.yy174 = tmp;   yy_destructor(19,&yymsp[-1].minor);
}
#line 2047 "pdo_user_sql_parser.c"
        break;
      case 70:
#line 482 "pdo_user_sql_parser.lemon"
{ add_next_index_zval(yymsp[-2].minor.yy252, pusp_zvalize_token(&yymsp[0].minor.yy0)); yygotominor.yy252 = yymsp[-2].minor.yy252;   yy_destructor(11,&yymsp[-1].minor);
}
#line 2053 "pdo_user_sql_parser.c"
        break;
      case 71:
#line 483 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); array_init(yygotominor.yy252); add_next_index_zval(yygotominor.yy252, pusp_zvalize_token(&yymsp[0].minor.yy0)); }
#line 2058 "pdo_user_sql_parser.c"
        break;
      case 77:
      case 126:
#line 495 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_field(&yymsp[-4].minor.yy0, &yymsp[-2].minor.yy0, &yymsp[0].minor.yy0);   yy_destructor(84,&yymsp[-3].minor);
  yy_destructor(84,&yymsp[-1].minor);
}
#line 2066 "pdo_user_sql_parser.c"
        break;
      case 78:
      case 86:
      case 127:
#line 496 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_field(NULL, &yymsp[-2].minor.yy0, &yymsp[0].minor.yy0);   yy_destructor(84,&yymsp[-1].minor);
}
#line 2074 "pdo_user_sql_parser.c"
        break;
      case 79:
      case 87:
      case 128:
#line 497 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_field(NULL, NULL, &yymsp[0].minor.yy0); }
#line 2081 "pdo_user_sql_parser.c"
        break;
      case 80:
#line 498 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); array_init(yygotominor.yy252); add_assoc_stringl(yygotominor.yy252, "type", "alias", sizeof("alias") - 1, 1); add_assoc_zval(yygotominor.yy252, "field", yymsp[-2].minor.yy252); add_assoc_zval(yygotominor.yy252, "as", pusp_zvalize_token(&yymsp[0].minor.yy0));   yy_destructor(15,&yymsp[-1].minor);
}
#line 2087 "pdo_user_sql_parser.c"
        break;
      case 83:
#line 503 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_join_expression(yymsp[-4].minor.yy252, yymsp[-3].minor.yy252, yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(85,&yymsp[-1].minor);
}
#line 2093 "pdo_user_sql_parser.c"
        break;
      case 84:
#line 504 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); array_init(yygotominor.yy252); add_assoc_stringl(yygotominor.yy252, "type", "alias", sizeof("alias") - 1, 1); add_assoc_zval(yygotominor.yy252, "table", yymsp[-2].minor.yy252); add_assoc_zval(yygotominor.yy252, "as", pusp_zvalize_token(&yymsp[0].minor.yy0));   yy_destructor(15,&yymsp[-1].minor);
}
#line 2099 "pdo_user_sql_parser.c"
        break;
      case 88:
#line 520 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_zvalize_static_string("inner");   yy_destructor(86,&yymsp[-1].minor);
  yy_destructor(87,&yymsp[0].minor);
}
#line 2106 "pdo_user_sql_parser.c"
        break;
      case 89:
#line 521 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_zvalize_static_string("outer");   yy_destructor(88,&yymsp[-1].minor);
  yy_destructor(87,&yymsp[0].minor);
}
#line 2113 "pdo_user_sql_parser.c"
        break;
      case 90:
#line 522 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_zvalize_static_string("left");   yy_destructor(89,&yymsp[-1].minor);
  yy_destructor(87,&yymsp[0].minor);
}
#line 2120 "pdo_user_sql_parser.c"
        break;
      case 91:
#line 523 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_zvalize_static_string("right");   yy_destructor(90,&yymsp[-1].minor);
  yy_destructor(87,&yymsp[0].minor);
}
#line 2127 "pdo_user_sql_parser.c"
        break;
      case 92:
#line 526 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_add_query_modifier(yymsp[-1].minor.yy252, "where", yymsp[0].minor.yy252); }
#line 2132 "pdo_user_sql_parser.c"
        break;
      case 93:
#line 527 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_add_query_modifier(yymsp[-1].minor.yy252, "limit", yymsp[0].minor.yy252); }
#line 2137 "pdo_user_sql_parser.c"
        break;
      case 94:
#line 528 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_add_query_modifier(yymsp[-1].minor.yy252, "having", yymsp[0].minor.yy252); }
#line 2142 "pdo_user_sql_parser.c"
        break;
      case 95:
#line 529 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_add_query_modifier(yymsp[-1].minor.yy252, "group-by", yymsp[0].minor.yy252); }
#line 2147 "pdo_user_sql_parser.c"
        break;
      case 96:
#line 530 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_add_query_modifier(yymsp[-1].minor.yy252, "order-by", yymsp[0].minor.yy252); }
#line 2152 "pdo_user_sql_parser.c"
        break;
      case 98:
#line 534 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = yymsp[0].minor.yy252;   yy_destructor(6,&yymsp[-1].minor);
}
#line 2158 "pdo_user_sql_parser.c"
        break;
      case 99:
#line 537 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); array_init(yygotominor.yy252); add_assoc_zval(yygotominor.yy252, "from", yymsp[-2].minor.yy252); add_assoc_zval(yygotominor.yy252, "to", yymsp[0].minor.yy252);   yy_destructor(5,&yymsp[-3].minor);
  yy_destructor(11,&yymsp[-1].minor);
}
#line 2165 "pdo_user_sql_parser.c"
        break;
      case 100:
#line 540 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = yymsp[0].minor.yy252;   yy_destructor(7,&yymsp[-1].minor);
}
#line 2171 "pdo_user_sql_parser.c"
        break;
      case 101:
#line 543 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = yymsp[0].minor.yy252;   yy_destructor(2,&yymsp[-2].minor);
  yy_destructor(4,&yymsp[-1].minor);
}
#line 2178 "pdo_user_sql_parser.c"
        break;
      case 102:
#line 546 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = yymsp[0].minor.yy252;   yy_destructor(3,&yymsp[-2].minor);
  yy_destructor(4,&yymsp[-1].minor);
}
#line 2185 "pdo_user_sql_parser.c"
        break;
      case 105:
#line 553 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "and", yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(12,&yymsp[-1].minor);
}
#line 2191 "pdo_user_sql_parser.c"
        break;
      case 106:
#line 554 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "or", yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(13,&yymsp[-1].minor);
}
#line 2197 "pdo_user_sql_parser.c"
        break;
      case 107:
#line 555 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "xor", yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(14,&yymsp[-1].minor);
}
#line 2203 "pdo_user_sql_parser.c"
        break;
      case 108:
#line 557 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "IN", yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(91,&yymsp[-1].minor);
}
#line 2209 "pdo_user_sql_parser.c"
        break;
      case 109:
#line 558 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "=", yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(19,&yymsp[-1].minor);
}
#line 2215 "pdo_user_sql_parser.c"
        break;
      case 110:
#line 559 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "!=", yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(92,&yymsp[-1].minor);
}
#line 2221 "pdo_user_sql_parser.c"
        break;
      case 111:
#line 560 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "<>", yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(93,&yymsp[-1].minor);
}
#line 2227 "pdo_user_sql_parser.c"
        break;
      case 112:
#line 561 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "<", yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(20,&yymsp[-1].minor);
}
#line 2233 "pdo_user_sql_parser.c"
        break;
      case 113:
#line 562 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, ">", yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(21,&yymsp[-1].minor);
}
#line 2239 "pdo_user_sql_parser.c"
        break;
      case 114:
#line 563 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "<=", yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(94,&yymsp[-1].minor);
}
#line 2245 "pdo_user_sql_parser.c"
        break;
      case 115:
#line 564 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, ">=", yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(23,&yymsp[-1].minor);
}
#line 2251 "pdo_user_sql_parser.c"
        break;
      case 116:
#line 565 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "like", yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(17,&yymsp[-1].minor);
}
#line 2257 "pdo_user_sql_parser.c"
        break;
      case 117:
#line 566 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "rlike", yymsp[-2].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(18,&yymsp[-1].minor);
}
#line 2263 "pdo_user_sql_parser.c"
        break;
      case 118:
#line 567 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "between", yymsp[-4].minor.yy252, yymsp[-2].minor.yy252); add_assoc_zval(yygotominor.yy252, "op3", yymsp[0].minor.yy252);   yy_destructor(16,&yymsp[-3].minor);
  yy_destructor(12,&yymsp[-1].minor);
}
#line 2270 "pdo_user_sql_parser.c"
        break;
      case 119:
#line 568 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "not like", yymsp[-3].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(1,&yymsp[-2].minor);
  yy_destructor(17,&yymsp[-1].minor);
}
#line 2277 "pdo_user_sql_parser.c"
        break;
      case 120:
#line 569 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "not rlike", yymsp[-3].minor.yy252, yymsp[0].minor.yy252);   yy_destructor(1,&yymsp[-2].minor);
  yy_destructor(18,&yymsp[-1].minor);
}
#line 2284 "pdo_user_sql_parser.c"
        break;
      case 121:
#line 570 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy252, "not between", yymsp[-5].minor.yy252, yymsp[-2].minor.yy252);  add_assoc_zval(yygotominor.yy252, "op3", yymsp[0].minor.yy252);   yy_destructor(1,&yymsp[-4].minor);
  yy_destructor(16,&yymsp[-3].minor);
  yy_destructor(12,&yymsp[-1].minor);
}
#line 2292 "pdo_user_sql_parser.c"
        break;
      case 129:
#line 582 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_placeholder(pusp_zvalize_token(&yymsp[0].minor.yy0));   yy_destructor(95,&yymsp[-1].minor);
}
#line 2298 "pdo_user_sql_parser.c"
        break;
      case 130:
#line 583 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_placeholder(yymsp[0].minor.yy252);   yy_destructor(95,&yymsp[-1].minor);
}
#line 2304 "pdo_user_sql_parser.c"
        break;
      case 131:
#line 584 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_do_function(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-1].minor.yy252);   yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 2311 "pdo_user_sql_parser.c"
        break;
      case 133:
#line 586 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy252,yymsp[-2].minor.yy252,"+",yymsp[0].minor.yy252);   yy_destructor(29,&yymsp[-1].minor);
}
#line 2317 "pdo_user_sql_parser.c"
        break;
      case 134:
#line 587 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy252,yymsp[-2].minor.yy252,"-",yymsp[0].minor.yy252);   yy_destructor(30,&yymsp[-1].minor);
}
#line 2323 "pdo_user_sql_parser.c"
        break;
      case 135:
#line 588 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy252,yymsp[-2].minor.yy252,"*",yymsp[0].minor.yy252);   yy_destructor(25,&yymsp[-1].minor);
}
#line 2329 "pdo_user_sql_parser.c"
        break;
      case 136:
#line 589 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy252,yymsp[-2].minor.yy252,"/",yymsp[0].minor.yy252);   yy_destructor(26,&yymsp[-1].minor);
}
#line 2335 "pdo_user_sql_parser.c"
        break;
      case 137:
#line 590 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy252,yymsp[-2].minor.yy252,"%",yymsp[0].minor.yy252);   yy_destructor(27,&yymsp[-1].minor);
}
#line 2341 "pdo_user_sql_parser.c"
        break;
      case 138:
#line 591 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); array_init(yygotominor.yy252); add_assoc_stringl(yygotominor.yy252, "type", "distinct", sizeof("distinct") - 1, 1); add_assoc_zval(yygotominor.yy252, "distinct", yymsp[0].minor.yy252);   yy_destructor(24,&yymsp[-1].minor);
}
#line 2347 "pdo_user_sql_parser.c"
        break;
      case 141:
#line 598 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_zvalize_lnum(&yymsp[0].minor.yy0); }
#line 2352 "pdo_user_sql_parser.c"
        break;
      case 142:
#line 599 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_zvalize_hnum(&yymsp[0].minor.yy0); }
#line 2357 "pdo_user_sql_parser.c"
        break;
      case 143:
#line 602 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_zvalize_dnum(&yymsp[0].minor.yy0); }
#line 2362 "pdo_user_sql_parser.c"
        break;
      case 144:
#line 605 "pdo_user_sql_parser.lemon"
{ yygotominor.yy252 = pusp_zvalize_token(&yymsp[0].minor.yy0); }
#line 2367 "pdo_user_sql_parser.c"
        break;
      case 147:
#line 608 "pdo_user_sql_parser.lemon"
{ TSRMLS_FETCH(); yygotominor.yy252 = EG(uninitialized_zval_ptr);   yy_destructor(46,&yymsp[0].minor);
}
#line 2373 "pdo_user_sql_parser.c"
        break;
      case 152:
#line 619 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); add_assoc_stringl(yygotominor.yy252, "direction", "asc", sizeof("asc") - 1, 1); add_assoc_zval(yygotominor.yy252, "by", yymsp[-1].minor.yy252);   yy_destructor(97,&yymsp[0].minor);
}
#line 2379 "pdo_user_sql_parser.c"
        break;
      case 153:
#line 620 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); add_assoc_stringl(yygotominor.yy252, "direction", "desc", sizeof("desc") - 1, 1); add_assoc_zval(yygotominor.yy252, "by", yymsp[-1].minor.yy252);   yy_destructor(98,&yymsp[0].minor);
}
#line 2385 "pdo_user_sql_parser.c"
        break;
      case 154:
#line 621 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy252); add_assoc_null(yygotominor.yy252, "direction"); add_assoc_zval(yygotominor.yy252, "by", yymsp[0].minor.yy252); }
#line 2390 "pdo_user_sql_parser.c"
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
#line 362 "pdo_user_sql_parser.lemon"

	if (Z_TYPE_P(return_value) == IS_NULL) {
		/* Only throw error if we don't already have a statement */
		RETVAL_FALSE;
	}
#line 2454 "pdo_user_sql_parser.c"
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
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      memset(&yyminorunion, 0, sizeof(yyminorunion));
      yyStackOverflow(yypParser, &yyminorunion);
      return;
    }
#endif
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
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
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
