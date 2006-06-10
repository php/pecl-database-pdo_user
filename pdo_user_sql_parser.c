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
#define YYNOCODE 141
#define YYACTIONTYPE unsigned short int
#define php_pdo_user_sql_parserTOKENTYPE php_pdo_user_sql_token
typedef union {
  php_pdo_user_sql_parserTOKENTYPE yy0;
  zval* yy8;
  zval** yy142;
  int yy281;
} YYMINORTYPE;
#define YYSTACKDEPTH 100
#define php_pdo_user_sql_parserARG_SDECL zval *return_value;
#define php_pdo_user_sql_parserARG_PDECL ,zval *return_value
#define php_pdo_user_sql_parserARG_FETCH zval *return_value = yypParser->return_value
#define php_pdo_user_sql_parserARG_STORE yypParser->return_value = return_value
#define YYNSTATE 314
#define YYNRULE 152
#define YYERRORSYMBOL 98
#define YYERRSYMDT yy281
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
 /*     0 */   299,   38,   40,   42,  314,   18,   29,   38,   40,   42,
 /*    10 */    37,   18,   29,  216,  217,  226,  230,  234,  238,  242,
 /*    20 */   246,  248,  257,  261,  265,  269,  273,  277,  278,  279,
 /*    30 */   281,  283,  284,  285,  286,  287,  288,  289,  290,  291,
 /*    40 */   295,  306,   95,   44,  163,   59,   60,   61,   62,   63,
 /*    50 */    35,  157,   33,   34,   47,   54,    5,   91,   87,   89,
 /*    60 */    73,   79,   81,   95,   85,   50,   38,   40,   42,   37,
 /*    70 */    18,   29,  125,  126,  133,   31,   33,   34,   91,   87,
 /*    80 */    89,   73,   79,   81,    9,   85,    6,   38,   40,   42,
 /*    90 */    44,   18,   29,  315,  165,   44,    7,   35,   20,   33,
 /*   100 */    34,   47,  104,   20,   33,   34,   47,   54,    4,  467,
 /*   110 */     1,    3,   50,   30,   46,  133,   55,   50,    8,   16,
 /*   120 */   133,   64,   19,   66,   68,   70,  176,   48,  132,    2,
 /*   130 */   143,  156,   75,   77,   83,  164,   15,  171,  131,  183,
 /*   140 */   186,  152,  195,  311,  150,  135,   49,  137,  139,  141,
 /*   150 */   206,   10,  305,   75,   77,   83,   93,   48,  301,  145,
 /*   160 */    30,   46,   16,   18,   29,   30,   46,  102,   13,   38,
 /*   170 */    40,   42,   53,   18,   29,   56,   49,   11,   19,   27,
 /*   180 */    38,   40,   42,   48,   18,   29,  135,  124,  137,  139,
 /*   190 */   141,  135,   25,  137,  139,  141,  208,  210,  214,  212,
 /*   200 */   215,  122,   49,  128,   19,  167,   38,   40,   42,   48,
 /*   210 */    18,   29,   19,   28,   22,   14,   58,   48,  100,   96,
 /*   220 */    98,   28,  169,   26,  130,  146,  114,  120,   49,  108,
 /*   230 */    64,  112,   19,   57,  129,   19,   49,   48,   12,   24,
 /*   240 */    48,   72,  162,   19,   72,   21,   19,   65,   48,  159,
 /*   250 */    67,   48,   72,  175,  145,   72,   49,   19,   69,   49,
 /*   260 */    19,   71,   48,   33,   34,   48,  107,   49,   19,   72,
 /*   270 */    49,   19,  105,   48,   23,  113,   48,  119,   25,  181,
 /*   280 */   124,   49,   19,  149,   49,   54,   32,   48,  109,   19,
 /*   290 */   116,   72,   49,  110,   48,   49,  127,  151,  156,   33,
 /*   300 */    34,   47,  153,  174,  145,   19,   49,  111,  130,  146,
 /*   310 */    48,   19,   50,   49,   17,  115,   48,   19,   19,   19,
 /*   320 */    52,  182,   48,   48,   48,   19,   51,   36,   39,   49,
 /*   330 */    48,  158,  199,  188,   41,   49,  310,   66,   68,   70,
 /*   340 */   185,   49,   49,   49,  117,   19,   19,   19,  194,   49,
 /*   350 */    48,   48,   48,  106,   43,   45,   74,  121,   19,   19,
 /*   360 */   123,   46,  129,   48,   48,  136,  181,   76,   78,   49,
 /*   370 */    49,   49,   19,  134,  209,   19,  138,   48,  140,   48,
 /*   380 */    48,   80,   49,   49,   82,   19,   19,  144,   19,  201,
 /*   390 */    48,   48,  142,   48,   84,   86,   49,   88,   49,   49,
 /*   400 */   303,  161,   19,  147,  148,  154,  200,   48,   24,   49,
 /*   410 */    49,   90,   49,   19,   19,   19,   19,  302,   48,   48,
 /*   420 */    48,   48,   92,   94,   97,   99,   49,   19,  303,  155,
 /*   430 */   160,  170,   48,  166,  168,  172,  101,   49,   49,   49,
 /*   440 */    49,   19,  304,   19,   19,  309,   48,   48,   48,   48,
 /*   450 */   103,   49,  118,  180,  305,  173,  178,  177,  179,   48,
 /*   460 */   308,  184,   64,  187,  191,   49,   49,   49,   49,  189,
 /*   470 */   192,  190,  193,  196,  197,  198,  202,  203,   49,  204,
 /*   480 */   207,  205,  211,  223,  213,  218,  222,  219,  221,  225,
 /*   490 */   220,  228,  227,  224,  252,  231,  229,  232,  235,  233,
 /*   500 */   236,  239,  237,  254,  240,  256,  270,  243,  241,  244,
 /*   510 */   247,  245,  272,  274,  249,  251,  250,  276,  292,  294,
 /*   520 */   253,  296,  298,  255,  259,  258,  260,  263,  262,  264,
 /*   530 */   267,  266,  268,  300,  307,  280,  313,  271,  275,  312,
 /*   540 */   282,  293,  297,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    39,   25,   26,   27,    0,   29,   30,   25,   26,   27,
 /*    10 */    28,   29,   30,   52,   53,   54,   55,   56,   57,   58,
 /*    20 */    59,   60,   61,   62,   63,   64,   65,   66,   67,   68,
 /*    30 */    69,   70,   71,   72,   73,   74,   75,   76,   77,   78,
 /*    40 */    79,   80,    1,   24,   25,  131,  132,  133,  134,  135,
 /*    50 */    31,   32,   33,   34,   35,    8,   10,   16,   17,   18,
 /*    60 */    19,   20,   21,    1,   23,   46,   25,   26,   27,   28,
 /*    70 */    29,   30,   96,   97,   15,   32,   33,   34,   16,   17,
 /*    80 */    18,   19,   20,   21,   37,   23,   32,   25,   26,   27,
 /*    90 */    24,   29,   30,    0,   31,   24,  102,   31,   32,   33,
 /*   100 */    34,   35,   31,   32,   33,   34,   35,    8,    9,   99,
 /*   110 */   100,  101,   46,   94,   95,   15,  108,   46,  101,   11,
 /*   120 */    15,    6,  114,   12,   13,   14,   11,  119,   28,   36,
 /*   130 */   101,  123,   91,   92,   93,  127,   28,   38,  109,   40,
 /*   140 */    41,   11,   43,   44,   85,   86,  138,   88,   89,   90,
 /*   150 */     1,  103,  114,   91,   92,   93,   12,  119,  120,  130,
 /*   160 */    94,   95,   11,   29,   30,   94,   95,   12,   31,   25,
 /*   170 */    26,   27,  124,   29,   30,   45,  138,   11,  114,   28,
 /*   180 */    25,   26,   27,  119,   29,   30,   86,  123,   88,   89,
 /*   190 */    90,   86,   31,   88,   89,   90,   47,   48,   49,   50,
 /*   200 */    51,  137,  138,  139,  114,   11,   25,   26,   27,  119,
 /*   210 */    29,   30,  114,  123,   32,  125,  110,  119,   16,   17,
 /*   220 */    18,  123,   28,  125,   31,   32,    2,    3,  138,    5,
 /*   230 */     6,    7,  114,  109,  128,  114,  138,  119,  124,   32,
 /*   240 */   119,  123,   25,  114,  123,   84,  114,  129,  119,   32,
 /*   250 */   129,  119,  123,  105,  130,  123,  138,  114,  129,  138,
 /*   260 */   114,  129,  119,   33,   34,  119,  123,  138,  114,  123,
 /*   270 */   138,  114,  129,  119,   84,  129,  119,  123,   31,  131,
 /*   280 */   123,  138,  114,  109,  138,    8,  119,  119,  119,  114,
 /*   290 */   136,  123,  138,   11,  119,  138,  139,  129,  123,   33,
 /*   300 */    34,   35,  127,  104,  130,  114,  138,  119,   31,   32,
 /*   310 */   119,  114,   46,  138,  123,    4,  119,  114,  114,  114,
 /*   320 */   123,  122,  119,  119,  119,  114,  123,  123,  123,  138,
 /*   330 */   119,   84,  107,  106,  123,  138,  111,   12,   13,   14,
 /*   340 */   105,  138,  138,  138,   11,  114,  114,  114,  121,  138,
 /*   350 */   119,  119,  119,   28,  123,  123,  123,    4,  114,  114,
 /*   360 */    11,   95,  128,  119,  119,   87,  131,  123,  123,  138,
 /*   370 */   138,  138,  114,   32,  114,  114,   87,  119,   87,  119,
 /*   380 */   119,  123,  138,  138,  123,  114,  114,   28,  114,   11,
 /*   390 */   119,  119,   87,  119,  123,  123,  138,  123,  138,  138,
 /*   400 */    11,   25,  114,   84,   32,   15,   28,  119,   32,  138,
 /*   410 */   138,  123,  138,  114,  114,  114,  114,   28,  119,  119,
 /*   420 */   119,  119,  123,  123,  123,  123,  138,  114,   11,   32,
 /*   430 */    84,   32,  119,  126,   32,   32,  123,  138,  138,  138,
 /*   440 */   138,  114,  114,  114,  114,   28,  119,  119,  119,  119,
 /*   450 */   123,  138,  123,  123,  114,   39,   32,  122,   19,  119,
 /*   460 */   120,   32,    6,   42,   32,  138,  138,  138,  138,   11,
 /*   470 */    83,  121,   32,   42,   32,   31,  111,   32,  138,  112,
 /*   480 */    46,  113,   49,   31,   49,  115,   81,  116,   82,   28,
 /*   490 */   117,  116,  115,  119,   31,  115,  117,  116,  115,  117,
 /*   500 */   116,  115,  117,   11,  116,   28,   31,  115,  117,  116,
 /*   510 */   115,  117,   28,   31,  118,  117,  116,   28,   31,   28,
 /*   520 */   119,   31,   28,  119,  116,  118,  117,  116,  118,  117,
 /*   530 */   116,  118,  117,   31,   31,  115,   32,  119,  119,   42,
 /*   540 */   115,  119,  119,
};
#define YY_SHIFT_USE_DFLT (-40)
static const short yy_shift_ofst[] = {
 /*     0 */    99,   93,    4,  -40,   46,   54,   63,   47,  -40,  137,
 /*    10 */   166,  137,  -40,   66,  108,  -40,   66,  181,   66,  -40,
 /*    20 */   161,  182,  190,  207,  -40,   66,  151,  -40,  181,   66,
 /*    30 */    43,  -40,  -40,  -40,  -40,   66,  -18,  -40,   66,  134,
 /*    40 */    66,  134,   66,  134,   66,  181,  -40,  -40,  -40,  -40,
 /*    50 */   -40,  -40,  -40,  -40,   19,  130,  193,  105,  224,  -40,
 /*    60 */   -40,  -40,  -40,  -40,   71,  111,   71,  -40,   71,  -40,
 /*    70 */    71,  -40,   62,   66,  181,   66,  181,   66,  181,   66,
 /*    80 */   181,   66,  181,   66,  181,   66,  181,   66,  181,   66,
 /*    90 */   181,   66,  144,   66,  181,  202,   66,  181,   66,  181,
 /*   100 */    66,  155,   66,  181,   71,  325,  -40,   41,  230,  282,
 /*   110 */   230,  -40,   71,  111,  311,   66,  333,   66,  181,  181,
 /*   120 */   353,   66,  349,   66,  -24,  -40,  -40,  -40,  -40,  193,
 /*   130 */   277,  100,  -40,  341,  -40,  278,  -40,  289,  -40,  291,
 /*   140 */   -40,  305,  -40,  359,  -40,  -40,  319,  372,  -40,   59,
 /*   150 */    71,  111,   19,  390,  397,  -40,  181,  247,  217,  346,
 /*   160 */   376,  -40,  -40,  -40,  390,  399,  194,  402,  -40,  -40,
 /*   170 */   -40,  403,  416,  424,  115,  -40,  424,  -40,  439,   66,
 /*   180 */   181,  -40,  -40,  429,  456,  -40,  421,  432,  458,  432,
 /*   190 */   -40,  387,  440,  -40,  -40,  431,  442,  444,  445,  378,
 /*   200 */   -40,  445,  -40,  -39,  -40,  149,  434,  -40,  266,  -40,
 /*   210 */   433,  -40,  435,  -40,  -40,  -40,  -40,  452,  405,  406,
 /*   220 */   -40,  -40,  -40,  230,  461,  -40,  452,  405,  406,  -40,
 /*   230 */   452,  405,  406,  -40,  452,  405,  406,  -40,  452,  405,
 /*   240 */   406,  -40,  452,  405,  406,  -40,  452,  -40,  463,  405,
 /*   250 */   406,  -40,  230,  492,  230,  477,  -40,  463,  405,  406,
 /*   260 */   -40,  463,  405,  406,  -40,  463,  405,  406,  -40,  475,
 /*   270 */   230,  484,  -40,  482,  230,  489,  -40,  -40,  -40,  452,
 /*   280 */   -40,  452,  -40,  -40,  -40,  -40,  -40,  -40,  -40,  -40,
 /*   290 */   -40,  487,  230,  491,  -40,  490,  230,  494,  -40,  502,
 /*   300 */   266,  389,  -40,  266,  -40,  -40,  503,  266,  417,  -40,
 /*   310 */   -40,  497,  504,  -40,
};
#define YY_REDUCE_USE_DFLT (-87)
static const short yy_reduce_ofst[] = {
 /*     0 */    10,  -87,  -87,  -87,  -87,  -87,   -6,   17,  -87,   48,
 /*    10 */   -87,  114,  -87,   90,  -87,  -87,  191,  -87,  197,  -87,
 /*    20 */   -87,  -87,  -87,  -87,  -87,   98,  -87,  -87,  -87,  203,
 /*    30 */   167,  -87,  -87,  -87,  -87,  204,  -87,  -87,  205,  -87,
 /*    40 */   211,  -87,  231,  -87,  232,  -87,  -87,  -87,  -87,  -87,
 /*    50 */   -87,  -87,  -87,  -87,    8,  -87,  124,  106,  -86,  -87,
 /*    60 */   -87,  -87,  -87,  -87,  118,  -87,  121,  -87,  129,  -87,
 /*    70 */   132,  -87,  -87,  233,  -87,  244,  -87,  245,  -87,  258,
 /*    80 */   -87,  261,  -87,  271,  -87,  272,  -87,  274,  -87,  288,
 /*    90 */   -87,  299,  -87,  300,  -87,  -87,  301,  -87,  302,  -87,
 /*   100 */   313,  -87,  327,  -87,  143,  -87,  -87,  -87,  169,  -87,
 /*   110 */   188,  -87,  146,  -87,  -87,  154,  -87,  329,  -87,  -87,
 /*   120 */   -87,   64,  -87,  157,  -87,  -87,  -87,  -87,  -87,  174,
 /*   130 */    29,  234,  -87,  -87,  -87,  -87,  -87,  -87,  -87,  -87,
 /*   140 */   -87,  -87,  -87,  -87,  -87,  -87,  -87,  -87,  -87,  234,
 /*   150 */   168,  -87,  175,  -87,  -87,  -87,  -87,  -87,  -87,  -87,
 /*   160 */   -87,  -87,  -87,  -87,  -87,  307,  -87,  -87,  -87,  -87,
 /*   170 */   -87,  -87,  -87,  199,  148,  -87,  335,  -87,  -87,  330,
 /*   180 */   -87,  -87,  -87,  -87,  235,  -87,  -87,  227,  -87,  350,
 /*   190 */   -87,  -87,  -87,  -87,  -87,  -87,  -87,  -87,  225,  -87,
 /*   200 */   -87,  365,  -87,  367,  368,  -87,  -87,  -87,  260,  -87,
 /*   210 */   -87,  -87,  -87,  -87,  -87,  -87,  -87,  370,  371,  373,
 /*   220 */   -87,  -87,  -87,  374,  -87,  -87,  377,  375,  379,  -87,
 /*   230 */   380,  381,  382,  -87,  383,  384,  385,  -87,  386,  388,
 /*   240 */   391,  -87,  392,  393,  394,  -87,  395,  -87,  396,  400,
 /*   250 */   398,  -87,  401,  -87,  404,  -87,  -87,  407,  408,  409,
 /*   260 */   -87,  410,  411,  412,  -87,  413,  414,  415,  -87,  -87,
 /*   270 */   418,  -87,  -87,  -87,  419,  -87,  -87,  -87,  -87,  420,
 /*   280 */   -87,  425,  -87,  -87,  -87,  -87,  -87,  -87,  -87,  -87,
 /*   290 */   -87,  -87,  422,  -87,  -87,  -87,  423,  -87,  -87,  -87,
 /*   300 */    38,  -87,  -87,  328,  -87,  -87,  -87,  340,  -87,  -87,
 /*   310 */   -87,  -87,  -87,  -87,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   466,  466,  466,  316,  466,  466,  387,  466,  317,  466,
 /*    10 */   318,  466,  381,  466,  466,  383,  466,  436,  466,  438,
 /*    20 */   441,  466,  440,  466,  439,  466,  466,  444,  437,  466,
 /*    30 */   466,  442,  443,  452,  453,  466,  466,  445,  466,  448,
 /*    40 */   466,  449,  466,  450,  466,  451,  454,  455,  456,  457,
 /*    50 */   458,  447,  446,  382,  466,  466,  466,  411,  324,  406,
 /*    60 */   407,  408,  409,  410,  466,  412,  466,  419,  466,  420,
 /*    70 */   466,  421,  466,  466,  422,  466,  423,  466,  424,  466,
 /*    80 */   425,  466,  426,  466,  427,  466,  428,  466,  429,  466,
 /*    90 */   430,  466,  466,  466,  431,  466,  466,  432,  466,  433,
 /*   100 */   466,  466,  466,  434,  466,  466,  435,  466,  466,  466,
 /*   110 */   466,  413,  466,  414,  466,  466,  415,  466,  459,  460,
 /*   120 */   466,  466,  416,  466,  465,  463,  464,  461,  462,  466,
 /*   130 */   466,  466,  395,  466,  398,  466,  402,  466,  403,  466,
 /*   140 */   404,  466,  405,  466,  396,  399,  401,  466,  400,  466,
 /*   150 */   466,  397,  466,  388,  466,  394,  390,  441,  466,  440,
 /*   160 */   466,  391,  392,  393,  389,  466,  466,  466,  384,  386,
 /*   170 */   385,  466,  466,  466,  418,  319,  466,  378,  466,  466,
 /*   180 */   380,  417,  379,  466,  418,  320,  466,  466,  321,  466,
 /*   190 */   375,  466,  466,  377,  376,  466,  466,  466,  466,  466,
 /*   200 */   322,  466,  325,  466,  334,  327,  466,  328,  466,  329,
 /*   210 */   466,  330,  466,  331,  332,  333,  335,  370,  366,  368,
 /*   220 */   336,  367,  365,  466,  466,  369,  370,  366,  368,  337,
 /*   230 */   370,  366,  368,  338,  370,  366,  368,  339,  370,  366,
 /*   240 */   368,  340,  370,  366,  368,  341,  370,  342,  372,  366,
 /*   250 */   368,  343,  466,  466,  466,  466,  371,  372,  366,  368,
 /*   260 */   344,  372,  366,  368,  345,  372,  366,  368,  346,  466,
 /*   270 */   466,  466,  347,  466,  466,  466,  348,  349,  350,  370,
 /*   280 */   351,  370,  352,  353,  354,  355,  356,  357,  358,  359,
 /*   290 */   360,  466,  466,  466,  361,  466,  466,  466,  362,  466,
 /*   300 */   466,  466,  363,  466,  373,  374,  466,  466,  466,  364,
 /*   310 */   326,  466,  466,  323,
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
  "DOT",           "ON",            "INNER",         "JOIN",        
  "OUTER",         "LEFT",          "RIGHT",         "NOT_EQUAL",   
  "UNEQUAL",       "LESSER_EQUAL",  "COLON",         "DNUM",        
  "ASC",           "DESC",          "error",         "terminal_statement",
  "statement",     "selectstatement",  "optionalinsertfieldlist",  "insertgrouplist",
  "setlist",       "optionalwhereclause",  "togrouplist",   "fielddescriptorlist",
  "fieldlist",     "tableexpr",     "optionalquerymodifiers",  "fielddescriptor",
  "fielddescriptortype",  "optionalfielddescriptormodifierlist",  "literal",       "optionalprecision",
  "optionalunsigned",  "optionalzerofill",  "optionalfloatprecision",  "intnum",      
  "literallist",   "togroup",       "setexpr",       "expr",        
  "insertgroup",   "exprlist",      "labellist",     "field",       
  "joinclause",    "cond",          "tablename",     "whereclause", 
  "limitclause",   "havingclause",  "groupclause",   "orderclause", 
  "grouplist",     "orderlist",     "dblnum",        "orderelement",
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
 /* 108 */ "cond ::= expr EQUALS expr",
 /* 109 */ "cond ::= expr NOT_EQUAL expr",
 /* 110 */ "cond ::= expr UNEQUAL expr",
 /* 111 */ "cond ::= expr LESSER expr",
 /* 112 */ "cond ::= expr GREATER expr",
 /* 113 */ "cond ::= expr LESSER_EQUAL expr",
 /* 114 */ "cond ::= expr GREATER_EQUAL expr",
 /* 115 */ "cond ::= expr LIKE expr",
 /* 116 */ "cond ::= expr RLIKE expr",
 /* 117 */ "cond ::= expr BETWEEN expr AND expr",
 /* 118 */ "cond ::= expr NOT LIKE expr",
 /* 119 */ "cond ::= expr NOT RLIKE expr",
 /* 120 */ "cond ::= expr NOT BETWEEN expr AND expr",
 /* 121 */ "cond ::= LPAREN cond RPAREN",
 /* 122 */ "exprlist ::= exprlist COMMA expr",
 /* 123 */ "exprlist ::= expr",
 /* 124 */ "expr ::= literal",
 /* 125 */ "expr ::= LABEL DOT LABEL DOT LABEL",
 /* 126 */ "expr ::= LABEL DOT LABEL",
 /* 127 */ "expr ::= LABEL",
 /* 128 */ "expr ::= COLON LABEL",
 /* 129 */ "expr ::= COLON intnum",
 /* 130 */ "expr ::= LABEL LPAREN exprlist RPAREN",
 /* 131 */ "expr ::= LPAREN expr RPAREN",
 /* 132 */ "expr ::= expr PLUS expr",
 /* 133 */ "expr ::= expr MINUS expr",
 /* 134 */ "expr ::= expr MUL expr",
 /* 135 */ "expr ::= expr DIV expr",
 /* 136 */ "expr ::= expr MOD expr",
 /* 137 */ "expr ::= DISTINCT expr",
 /* 138 */ "intnum ::= LNUM",
 /* 139 */ "intnum ::= HNUM",
 /* 140 */ "dblnum ::= DNUM",
 /* 141 */ "literal ::= STRING",
 /* 142 */ "literal ::= intnum",
 /* 143 */ "literal ::= dblnum",
 /* 144 */ "literal ::= NULL",
 /* 145 */ "grouplist ::= grouplist COMMA expr",
 /* 146 */ "grouplist ::= expr",
 /* 147 */ "orderlist ::= orderlist COMMA orderelement",
 /* 148 */ "orderlist ::= orderelement",
 /* 149 */ "orderelement ::= expr ASC",
 /* 150 */ "orderelement ::= expr DESC",
 /* 151 */ "orderelement ::= expr",
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
    case 97:
#line 368 "pdo_user_sql_parser.lemon"
{
	if ((yypminor->yy0).freeme) {
		efree((yypminor->yy0).token);
	}
}
#line 1131 "pdo_user_sql_parser.c"
      break;
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
    case 120:
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
    case 139:
#line 377 "pdo_user_sql_parser.lemon"
{ zval_ptr_dtor(&(yypminor->yy8)); }
#line 1173 "pdo_user_sql_parser.c"
      break;
    case 121:
    case 122:
#line 463 "pdo_user_sql_parser.lemon"
{ zval_ptr_dtor(&(yypminor->yy142)[0]); zval_ptr_dtor(&(yypminor->yy142)[1]); efree((yypminor->yy142)); }
#line 1179 "pdo_user_sql_parser.c"
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
  { 99, 2 },
  { 99, 1 },
  { 100, 1 },
  { 100, 5 },
  { 100, 6 },
  { 100, 5 },
  { 100, 3 },
  { 100, 3 },
  { 100, 6 },
  { 100, 3 },
  { 101, 5 },
  { 107, 3 },
  { 107, 1 },
  { 111, 3 },
  { 113, 3 },
  { 113, 3 },
  { 113, 3 },
  { 113, 3 },
  { 113, 2 },
  { 113, 2 },
  { 113, 0 },
  { 112, 1 },
  { 112, 4 },
  { 112, 4 },
  { 112, 4 },
  { 112, 4 },
  { 112, 4 },
  { 112, 4 },
  { 112, 2 },
  { 112, 4 },
  { 112, 4 },
  { 112, 4 },
  { 112, 4 },
  { 112, 4 },
  { 112, 4 },
  { 112, 1 },
  { 112, 1 },
  { 112, 2 },
  { 112, 2 },
  { 112, 1 },
  { 112, 1 },
  { 112, 1 },
  { 112, 1 },
  { 112, 1 },
  { 112, 1 },
  { 112, 1 },
  { 112, 1 },
  { 112, 4 },
  { 112, 4 },
  { 112, 4 },
  { 112, 4 },
  { 116, 1 },
  { 116, 0 },
  { 117, 1 },
  { 117, 0 },
  { 115, 3 },
  { 115, 0 },
  { 118, 5 },
  { 118, 0 },
  { 120, 3 },
  { 120, 1 },
  { 106, 3 },
  { 106, 1 },
  { 121, 3 },
  { 104, 3 },
  { 104, 1 },
  { 122, 3 },
  { 103, 3 },
  { 103, 1 },
  { 124, 3 },
  { 126, 3 },
  { 126, 1 },
  { 102, 3 },
  { 102, 0 },
  { 108, 3 },
  { 108, 1 },
  { 127, 1 },
  { 127, 5 },
  { 127, 3 },
  { 127, 1 },
  { 127, 3 },
  { 109, 3 },
  { 109, 3 },
  { 109, 5 },
  { 109, 3 },
  { 109, 1 },
  { 130, 3 },
  { 130, 1 },
  { 128, 2 },
  { 128, 2 },
  { 128, 2 },
  { 128, 2 },
  { 110, 2 },
  { 110, 2 },
  { 110, 2 },
  { 110, 2 },
  { 110, 2 },
  { 110, 0 },
  { 131, 2 },
  { 132, 4 },
  { 133, 2 },
  { 134, 3 },
  { 135, 3 },
  { 105, 1 },
  { 105, 0 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 3 },
  { 129, 5 },
  { 129, 4 },
  { 129, 4 },
  { 129, 6 },
  { 129, 3 },
  { 125, 3 },
  { 125, 1 },
  { 123, 1 },
  { 123, 5 },
  { 123, 3 },
  { 123, 1 },
  { 123, 2 },
  { 123, 2 },
  { 123, 4 },
  { 123, 3 },
  { 123, 3 },
  { 123, 3 },
  { 123, 3 },
  { 123, 3 },
  { 123, 3 },
  { 123, 2 },
  { 119, 1 },
  { 119, 1 },
  { 138, 1 },
  { 114, 1 },
  { 114, 1 },
  { 114, 1 },
  { 114, 1 },
  { 136, 3 },
  { 136, 1 },
  { 137, 3 },
  { 137, 1 },
  { 139, 2 },
  { 139, 2 },
  { 139, 1 },
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
#line 374 "pdo_user_sql_parser.lemon"
{ pusp_do_terminal_statement(&return_value, yymsp[-1].minor.yy8, 1);   yy_destructor(36,&yymsp[0].minor);
}
#line 1559 "pdo_user_sql_parser.c"
        break;
      case 1:
#line 375 "pdo_user_sql_parser.lemon"
{ pusp_do_terminal_statement(&return_value, yymsp[0].minor.yy8, 0); }
#line 1564 "pdo_user_sql_parser.c"
        break;
      case 2:
      case 76:
      case 85:
      case 103:
      case 124:
      case 142:
      case 143:
#line 378 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = yymsp[0].minor.yy8; }
#line 1575 "pdo_user_sql_parser.c"
        break;
      case 3:
#line 379 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_insert_select_statement(pusp_zvalize_token(&yymsp[-2].minor.yy0), yymsp[-1].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(9,&yymsp[-4].minor);
  yy_destructor(10,&yymsp[-3].minor);
}
#line 1582 "pdo_user_sql_parser.c"
        break;
      case 4:
#line 380 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_insert_statement(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(9,&yymsp[-5].minor);
  yy_destructor(10,&yymsp[-4].minor);
  yy_destructor(37,&yymsp[-1].minor);
}
#line 1590 "pdo_user_sql_parser.c"
        break;
      case 5:
#line 381 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_update_statement(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-1].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(38,&yymsp[-4].minor);
  yy_destructor(39,&yymsp[-2].minor);
}
#line 1597 "pdo_user_sql_parser.c"
        break;
      case 6:
#line 382 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_delete_statement(pusp_zvalize_token(&yymsp[-1].minor.yy0), yymsp[0].minor.yy8);   yy_destructor(40,&yymsp[-2].minor);
}
#line 1603 "pdo_user_sql_parser.c"
        break;
      case 7:
#line 383 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_rename_statement(yymsp[0].minor.yy8);   yy_destructor(41,&yymsp[-2].minor);
  yy_destructor(42,&yymsp[-1].minor);
}
#line 1610 "pdo_user_sql_parser.c"
        break;
      case 8:
#line 384 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_create_statement(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-1].minor.yy8);   yy_destructor(43,&yymsp[-5].minor);
  yy_destructor(42,&yymsp[-4].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1619 "pdo_user_sql_parser.c"
        break;
      case 9:
#line 385 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_drop_statement(pusp_zvalize_token(&yymsp[0].minor.yy0));   yy_destructor(44,&yymsp[-2].minor);
  yy_destructor(42,&yymsp[-1].minor);
}
#line 1626 "pdo_user_sql_parser.c"
        break;
      case 10:
#line 388 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_select_statement(yymsp[-3].minor.yy8,yymsp[-1].minor.yy8,yymsp[0].minor.yy8);   yy_destructor(8,&yymsp[-4].minor);
  yy_destructor(45,&yymsp[-2].minor);
}
#line 1633 "pdo_user_sql_parser.c"
        break;
      case 11:
      case 59:
      case 67:
      case 74:
      case 122:
      case 145:
      case 147:
#line 391 "pdo_user_sql_parser.lemon"
{ add_next_index_zval(yymsp[-2].minor.yy8, yymsp[0].minor.yy8); yygotominor.yy8 = yymsp[-2].minor.yy8;   yy_destructor(11,&yymsp[-1].minor);
}
#line 1645 "pdo_user_sql_parser.c"
        break;
      case 12:
      case 60:
      case 68:
      case 75:
      case 123:
      case 146:
      case 148:
#line 392 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); array_init(yygotominor.yy8); add_next_index_zval(yygotominor.yy8, yymsp[0].minor.yy8); }
#line 1656 "pdo_user_sql_parser.c"
        break;
      case 13:
#line 395 "pdo_user_sql_parser.lemon"
{ add_assoc_zval(yymsp[-1].minor.yy8, "name", pusp_zvalize_token(&yymsp[-2].minor.yy0)); add_assoc_zval(yymsp[-1].minor.yy8, "flags", yymsp[0].minor.yy8); yygotominor.yy8 = yymsp[-1].minor.yy8; }
#line 1661 "pdo_user_sql_parser.c"
        break;
      case 14:
#line 398 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-2].minor.yy8, "not null", 1); yygotominor.yy8 = yymsp[-2].minor.yy8;   yy_destructor(1,&yymsp[-1].minor);
  yy_destructor(46,&yymsp[0].minor);
}
#line 1668 "pdo_user_sql_parser.c"
        break;
      case 15:
#line 399 "pdo_user_sql_parser.lemon"
{ add_assoc_zval(yymsp[-2].minor.yy8, "default", yymsp[0].minor.yy8); yygotominor.yy8 = yymsp[-2].minor.yy8;   yy_destructor(47,&yymsp[-1].minor);
}
#line 1674 "pdo_user_sql_parser.c"
        break;
      case 16:
#line 400 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-2].minor.yy8, "primary key", 1); yygotominor.yy8 = yymsp[-2].minor.yy8;   yy_destructor(48,&yymsp[-1].minor);
  yy_destructor(49,&yymsp[0].minor);
}
#line 1681 "pdo_user_sql_parser.c"
        break;
      case 17:
#line 401 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-2].minor.yy8, "unique key", 1); yygotominor.yy8 = yymsp[-2].minor.yy8;   yy_destructor(50,&yymsp[-1].minor);
  yy_destructor(49,&yymsp[0].minor);
}
#line 1688 "pdo_user_sql_parser.c"
        break;
      case 18:
#line 402 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-1].minor.yy8, "key", 1); yygotominor.yy8 = yymsp[-1].minor.yy8;   yy_destructor(49,&yymsp[0].minor);
}
#line 1694 "pdo_user_sql_parser.c"
        break;
      case 19:
#line 403 "pdo_user_sql_parser.lemon"
{ add_next_index_string(yymsp[-1].minor.yy8, "auto_increment", 1); yygotominor.yy8 = yymsp[-1].minor.yy8;   yy_destructor(51,&yymsp[0].minor);
}
#line 1700 "pdo_user_sql_parser.c"
        break;
      case 20:
#line 404 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); array_init(yygotominor.yy8); }
#line 1705 "pdo_user_sql_parser.c"
        break;
      case 21:
#line 407 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("bit", NULL, NULL);   yy_destructor(52,&yymsp[0].minor);
}
#line 1711 "pdo_user_sql_parser.c"
        break;
      case 22:
#line 408 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_num("int", yymsp[-2].minor.yy8, yymsp[-1].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(53,&yymsp[-3].minor);
}
#line 1717 "pdo_user_sql_parser.c"
        break;
      case 23:
#line 409 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_num("integer", yymsp[-2].minor.yy8, yymsp[-1].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(54,&yymsp[-3].minor);
}
#line 1723 "pdo_user_sql_parser.c"
        break;
      case 24:
#line 410 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_num("tinyint", yymsp[-2].minor.yy8, yymsp[-1].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(55,&yymsp[-3].minor);
}
#line 1729 "pdo_user_sql_parser.c"
        break;
      case 25:
#line 411 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_num("smallint", yymsp[-2].minor.yy8, yymsp[-1].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(56,&yymsp[-3].minor);
}
#line 1735 "pdo_user_sql_parser.c"
        break;
      case 26:
#line 412 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_num("mediumint", yymsp[-2].minor.yy8, yymsp[-1].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(57,&yymsp[-3].minor);
}
#line 1741 "pdo_user_sql_parser.c"
        break;
      case 27:
#line 413 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_num("bigint", yymsp[-2].minor.yy8, yymsp[-1].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(58,&yymsp[-3].minor);
}
#line 1747 "pdo_user_sql_parser.c"
        break;
      case 28:
#line 414 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("year", "precision", yymsp[0].minor.yy8);   yy_destructor(59,&yymsp[-1].minor);
}
#line 1753 "pdo_user_sql_parser.c"
        break;
      case 29:
#line 415 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_num("float", yymsp[-2].minor.yy8, yymsp[-1].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(60,&yymsp[-3].minor);
}
#line 1759 "pdo_user_sql_parser.c"
        break;
      case 30:
#line 416 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_num("real", yymsp[-2].minor.yy8, yymsp[-1].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(61,&yymsp[-3].minor);
}
#line 1765 "pdo_user_sql_parser.c"
        break;
      case 31:
#line 417 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_num("decimal", yymsp[-2].minor.yy8, yymsp[-1].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(62,&yymsp[-3].minor);
}
#line 1771 "pdo_user_sql_parser.c"
        break;
      case 32:
#line 418 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_num("double", yymsp[-2].minor.yy8, yymsp[-1].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(63,&yymsp[-3].minor);
}
#line 1777 "pdo_user_sql_parser.c"
        break;
      case 33:
#line 419 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("char", "length", yymsp[-1].minor.yy8);   yy_destructor(64,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1785 "pdo_user_sql_parser.c"
        break;
      case 34:
#line 420 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("varchar", "length", yymsp[-1].minor.yy8);   yy_destructor(65,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1793 "pdo_user_sql_parser.c"
        break;
      case 35:
#line 421 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("text", "date", NULL);   yy_destructor(66,&yymsp[0].minor);
}
#line 1799 "pdo_user_sql_parser.c"
        break;
      case 36:
#line 422 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("text", "time", NULL);   yy_destructor(67,&yymsp[0].minor);
}
#line 1805 "pdo_user_sql_parser.c"
        break;
      case 37:
#line 423 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("datetime", "precision", yymsp[0].minor.yy8);   yy_destructor(68,&yymsp[-1].minor);
}
#line 1811 "pdo_user_sql_parser.c"
        break;
      case 38:
#line 424 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("timestamp", "precision", yymsp[0].minor.yy8);   yy_destructor(69,&yymsp[-1].minor);
}
#line 1817 "pdo_user_sql_parser.c"
        break;
      case 39:
#line 425 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("text", "precision", NULL);   yy_destructor(70,&yymsp[0].minor);
}
#line 1823 "pdo_user_sql_parser.c"
        break;
      case 40:
#line 426 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "tiny", 1); yygotominor.yy8 = pusp_do_declare_type("text", "precision", p);   yy_destructor(71,&yymsp[0].minor);
}
#line 1829 "pdo_user_sql_parser.c"
        break;
      case 41:
#line 427 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "medium", 1); yygotominor.yy8 = pusp_do_declare_type("text", "precision", p);   yy_destructor(72,&yymsp[0].minor);
}
#line 1835 "pdo_user_sql_parser.c"
        break;
      case 42:
#line 428 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "long", 1); yygotominor.yy8 = pusp_do_declare_type("text", "precision", p);   yy_destructor(73,&yymsp[0].minor);
}
#line 1841 "pdo_user_sql_parser.c"
        break;
      case 43:
#line 429 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("blob", "precision", NULL);   yy_destructor(74,&yymsp[0].minor);
}
#line 1847 "pdo_user_sql_parser.c"
        break;
      case 44:
#line 430 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "tiny", 1); yygotominor.yy8 = pusp_do_declare_type("blob", "precision", p);   yy_destructor(75,&yymsp[0].minor);
}
#line 1853 "pdo_user_sql_parser.c"
        break;
      case 45:
#line 431 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "medium", 1); yygotominor.yy8 = pusp_do_declare_type("blob", "precision", p);   yy_destructor(76,&yymsp[0].minor);
}
#line 1859 "pdo_user_sql_parser.c"
        break;
      case 46:
#line 432 "pdo_user_sql_parser.lemon"
{ zval *p; MAKE_STD_ZVAL(p); ZVAL_STRING(p, "long", 1); yygotominor.yy8 = pusp_do_declare_type("blob", "precision", p);   yy_destructor(77,&yymsp[0].minor);
}
#line 1865 "pdo_user_sql_parser.c"
        break;
      case 47:
#line 433 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("binary", "length", yymsp[-1].minor.yy8);   yy_destructor(78,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1873 "pdo_user_sql_parser.c"
        break;
      case 48:
#line 434 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("varbinary", "length", yymsp[-1].minor.yy8);   yy_destructor(79,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1881 "pdo_user_sql_parser.c"
        break;
      case 49:
#line 435 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("set", "flags", yymsp[-1].minor.yy8);   yy_destructor(39,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1889 "pdo_user_sql_parser.c"
        break;
      case 50:
#line 436 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_declare_type("enum", "values", yymsp[-1].minor.yy8);   yy_destructor(80,&yymsp[-3].minor);
  yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1897 "pdo_user_sql_parser.c"
        break;
      case 51:
#line 439 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); ZVAL_TRUE(yygotominor.yy8);   yy_destructor(81,&yymsp[0].minor);
}
#line 1903 "pdo_user_sql_parser.c"
        break;
      case 52:
      case 54:
#line 440 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); ZVAL_FALSE(yygotominor.yy8); }
#line 1909 "pdo_user_sql_parser.c"
        break;
      case 53:
#line 443 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); ZVAL_TRUE(yygotominor.yy8);   yy_destructor(82,&yymsp[0].minor);
}
#line 1915 "pdo_user_sql_parser.c"
        break;
      case 55:
      case 69:
      case 72:
      case 81:
      case 82:
      case 121:
      case 131:
#line 447 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = yymsp[-1].minor.yy8;   yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1928 "pdo_user_sql_parser.c"
        break;
      case 56:
      case 58:
      case 73:
      case 97:
      case 104:
#line 448 "pdo_user_sql_parser.lemon"
{ TSRMLS_FETCH(); yygotominor.yy8 = EG(uninitialized_zval_ptr); }
#line 1937 "pdo_user_sql_parser.c"
        break;
      case 57:
#line 451 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); array_init(yygotominor.yy8); add_assoc_zval(yygotominor.yy8, "length", yymsp[-3].minor.yy8); add_assoc_zval(yygotominor.yy8, "decimals", yymsp[-1].minor.yy8);   yy_destructor(31,&yymsp[-4].minor);
  yy_destructor(11,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 1945 "pdo_user_sql_parser.c"
        break;
      case 61:
#line 459 "pdo_user_sql_parser.lemon"
{ pusp_do_push_labeled_zval(yymsp[-2].minor.yy8, yymsp[0].minor.yy142); yygotominor.yy8 = yymsp[-2].minor.yy8;   yy_destructor(11,&yymsp[-1].minor);
}
#line 1951 "pdo_user_sql_parser.c"
        break;
      case 62:
      case 65:
#line 460 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); array_init(yygotominor.yy8); pusp_do_push_labeled_zval(yygotominor.yy8, yymsp[0].minor.yy142); }
#line 1957 "pdo_user_sql_parser.c"
        break;
      case 63:
#line 464 "pdo_user_sql_parser.lemon"
{ zval **tmp = safe_emalloc(2, sizeof(zval*), 0); tmp[0] = pusp_zvalize_token(&yymsp[-2].minor.yy0); tmp[1] = pusp_zvalize_token(&yymsp[0].minor.yy0); yygotominor.yy142 = tmp;   yy_destructor(83,&yymsp[-1].minor);
}
#line 1963 "pdo_user_sql_parser.c"
        break;
      case 64:
#line 467 "pdo_user_sql_parser.lemon"
{ pusp_do_push_labeled_zval(yymsp[-2].minor.yy8, (zval**)yymsp[0].minor.yy142); yygotominor.yy8 = yymsp[-2].minor.yy8;   yy_destructor(11,&yymsp[-1].minor);
}
#line 1969 "pdo_user_sql_parser.c"
        break;
      case 66:
#line 472 "pdo_user_sql_parser.lemon"
{ zval **tmp = safe_emalloc(2, sizeof(zval*), 0); tmp[0] = pusp_zvalize_token(&yymsp[-2].minor.yy0); tmp[1] = yymsp[0].minor.yy8; yygotominor.yy142 = tmp;   yy_destructor(19,&yymsp[-1].minor);
}
#line 1975 "pdo_user_sql_parser.c"
        break;
      case 70:
#line 482 "pdo_user_sql_parser.lemon"
{ add_next_index_zval(yymsp[-2].minor.yy8, pusp_zvalize_token(&yymsp[0].minor.yy0)); yygotominor.yy8 = yymsp[-2].minor.yy8;   yy_destructor(11,&yymsp[-1].minor);
}
#line 1981 "pdo_user_sql_parser.c"
        break;
      case 71:
#line 483 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); array_init(yygotominor.yy8); add_next_index_zval(yygotominor.yy8, pusp_zvalize_token(&yymsp[0].minor.yy0)); }
#line 1986 "pdo_user_sql_parser.c"
        break;
      case 77:
      case 125:
#line 495 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_field(&yymsp[-4].minor.yy0, &yymsp[-2].minor.yy0, &yymsp[0].minor.yy0);   yy_destructor(84,&yymsp[-3].minor);
  yy_destructor(84,&yymsp[-1].minor);
}
#line 1994 "pdo_user_sql_parser.c"
        break;
      case 78:
      case 86:
      case 126:
#line 496 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_field(NULL, &yymsp[-2].minor.yy0, &yymsp[0].minor.yy0);   yy_destructor(84,&yymsp[-1].minor);
}
#line 2002 "pdo_user_sql_parser.c"
        break;
      case 79:
      case 87:
      case 127:
#line 497 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_field(NULL, NULL, &yymsp[0].minor.yy0); }
#line 2009 "pdo_user_sql_parser.c"
        break;
      case 80:
#line 498 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); array_init(yygotominor.yy8); add_assoc_stringl(yygotominor.yy8, "type", "alias", sizeof("alias") - 1, 1); add_assoc_zval(yygotominor.yy8, "field", yymsp[-2].minor.yy8); add_assoc_zval(yygotominor.yy8, "as", pusp_zvalize_token(&yymsp[0].minor.yy0));   yy_destructor(15,&yymsp[-1].minor);
}
#line 2015 "pdo_user_sql_parser.c"
        break;
      case 83:
#line 503 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_join_expression(yymsp[-4].minor.yy8, yymsp[-3].minor.yy8, yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(85,&yymsp[-1].minor);
}
#line 2021 "pdo_user_sql_parser.c"
        break;
      case 84:
#line 504 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); array_init(yygotominor.yy8); add_assoc_stringl(yygotominor.yy8, "type", "alias", sizeof("alias") - 1, 1); add_assoc_zval(yygotominor.yy8, "table", yymsp[-2].minor.yy8); add_assoc_zval(yygotominor.yy8, "as", pusp_zvalize_token(&yymsp[0].minor.yy0));   yy_destructor(15,&yymsp[-1].minor);
}
#line 2027 "pdo_user_sql_parser.c"
        break;
      case 88:
#line 520 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_zvalize_static_string("inner");   yy_destructor(86,&yymsp[-1].minor);
  yy_destructor(87,&yymsp[0].minor);
}
#line 2034 "pdo_user_sql_parser.c"
        break;
      case 89:
#line 521 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_zvalize_static_string("outer");   yy_destructor(88,&yymsp[-1].minor);
  yy_destructor(87,&yymsp[0].minor);
}
#line 2041 "pdo_user_sql_parser.c"
        break;
      case 90:
#line 522 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_zvalize_static_string("left");   yy_destructor(89,&yymsp[-1].minor);
  yy_destructor(87,&yymsp[0].minor);
}
#line 2048 "pdo_user_sql_parser.c"
        break;
      case 91:
#line 523 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_zvalize_static_string("right");   yy_destructor(90,&yymsp[-1].minor);
  yy_destructor(87,&yymsp[0].minor);
}
#line 2055 "pdo_user_sql_parser.c"
        break;
      case 92:
#line 526 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_add_query_modifier(yymsp[-1].minor.yy8, "where", yymsp[0].minor.yy8); }
#line 2060 "pdo_user_sql_parser.c"
        break;
      case 93:
#line 527 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_add_query_modifier(yymsp[-1].minor.yy8, "limit", yymsp[0].minor.yy8); }
#line 2065 "pdo_user_sql_parser.c"
        break;
      case 94:
#line 528 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_add_query_modifier(yymsp[-1].minor.yy8, "having", yymsp[0].minor.yy8); }
#line 2070 "pdo_user_sql_parser.c"
        break;
      case 95:
#line 529 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_add_query_modifier(yymsp[-1].minor.yy8, "group-by", yymsp[0].minor.yy8); }
#line 2075 "pdo_user_sql_parser.c"
        break;
      case 96:
#line 530 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_add_query_modifier(yymsp[-1].minor.yy8, "order-by", yymsp[0].minor.yy8); }
#line 2080 "pdo_user_sql_parser.c"
        break;
      case 98:
#line 534 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = yymsp[0].minor.yy8;   yy_destructor(6,&yymsp[-1].minor);
}
#line 2086 "pdo_user_sql_parser.c"
        break;
      case 99:
#line 537 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); array_init(yygotominor.yy8); add_assoc_zval(yygotominor.yy8, "from", yymsp[-2].minor.yy8); add_assoc_zval(yygotominor.yy8, "to", yymsp[0].minor.yy8);   yy_destructor(5,&yymsp[-3].minor);
  yy_destructor(11,&yymsp[-1].minor);
}
#line 2093 "pdo_user_sql_parser.c"
        break;
      case 100:
#line 540 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = yymsp[0].minor.yy8;   yy_destructor(7,&yymsp[-1].minor);
}
#line 2099 "pdo_user_sql_parser.c"
        break;
      case 101:
#line 543 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = yymsp[0].minor.yy8;   yy_destructor(2,&yymsp[-2].minor);
  yy_destructor(4,&yymsp[-1].minor);
}
#line 2106 "pdo_user_sql_parser.c"
        break;
      case 102:
#line 546 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = yymsp[0].minor.yy8;   yy_destructor(3,&yymsp[-2].minor);
  yy_destructor(4,&yymsp[-1].minor);
}
#line 2113 "pdo_user_sql_parser.c"
        break;
      case 105:
#line 553 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "and", yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(12,&yymsp[-1].minor);
}
#line 2119 "pdo_user_sql_parser.c"
        break;
      case 106:
#line 554 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "or", yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(13,&yymsp[-1].minor);
}
#line 2125 "pdo_user_sql_parser.c"
        break;
      case 107:
#line 555 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "xor", yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(14,&yymsp[-1].minor);
}
#line 2131 "pdo_user_sql_parser.c"
        break;
      case 108:
#line 557 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "=", yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(19,&yymsp[-1].minor);
}
#line 2137 "pdo_user_sql_parser.c"
        break;
      case 109:
#line 558 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "!=", yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(91,&yymsp[-1].minor);
}
#line 2143 "pdo_user_sql_parser.c"
        break;
      case 110:
#line 559 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "<>", yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(92,&yymsp[-1].minor);
}
#line 2149 "pdo_user_sql_parser.c"
        break;
      case 111:
#line 560 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "<", yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(20,&yymsp[-1].minor);
}
#line 2155 "pdo_user_sql_parser.c"
        break;
      case 112:
#line 561 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, ">", yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(21,&yymsp[-1].minor);
}
#line 2161 "pdo_user_sql_parser.c"
        break;
      case 113:
#line 562 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "<=", yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(93,&yymsp[-1].minor);
}
#line 2167 "pdo_user_sql_parser.c"
        break;
      case 114:
#line 563 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, ">=", yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(23,&yymsp[-1].minor);
}
#line 2173 "pdo_user_sql_parser.c"
        break;
      case 115:
#line 564 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "like", yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(17,&yymsp[-1].minor);
}
#line 2179 "pdo_user_sql_parser.c"
        break;
      case 116:
#line 565 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "rlike", yymsp[-2].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(18,&yymsp[-1].minor);
}
#line 2185 "pdo_user_sql_parser.c"
        break;
      case 117:
#line 566 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "between", yymsp[-4].minor.yy8, yymsp[-2].minor.yy8); add_assoc_zval(yygotominor.yy8, "op3", yymsp[0].minor.yy8);   yy_destructor(16,&yymsp[-3].minor);
  yy_destructor(12,&yymsp[-1].minor);
}
#line 2192 "pdo_user_sql_parser.c"
        break;
      case 118:
#line 567 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "not like", yymsp[-3].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(1,&yymsp[-2].minor);
  yy_destructor(17,&yymsp[-1].minor);
}
#line 2199 "pdo_user_sql_parser.c"
        break;
      case 119:
#line 568 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "not rlike", yymsp[-3].minor.yy8, yymsp[0].minor.yy8);   yy_destructor(1,&yymsp[-2].minor);
  yy_destructor(18,&yymsp[-1].minor);
}
#line 2206 "pdo_user_sql_parser.c"
        break;
      case 120:
#line 569 "pdo_user_sql_parser.lemon"
{ DO_COND(yygotominor.yy8, "not between", yymsp[-5].minor.yy8, yymsp[-2].minor.yy8);  add_assoc_zval(yygotominor.yy8, "op3", yymsp[0].minor.yy8);   yy_destructor(1,&yymsp[-4].minor);
  yy_destructor(16,&yymsp[-3].minor);
  yy_destructor(12,&yymsp[-1].minor);
}
#line 2214 "pdo_user_sql_parser.c"
        break;
      case 128:
#line 581 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_placeholder(pusp_zvalize_token(&yymsp[0].minor.yy0));   yy_destructor(94,&yymsp[-1].minor);
}
#line 2220 "pdo_user_sql_parser.c"
        break;
      case 129:
#line 582 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_placeholder(yymsp[0].minor.yy8);   yy_destructor(94,&yymsp[-1].minor);
}
#line 2226 "pdo_user_sql_parser.c"
        break;
      case 130:
#line 583 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_do_function(pusp_zvalize_token(&yymsp[-3].minor.yy0), yymsp[-1].minor.yy8);   yy_destructor(31,&yymsp[-2].minor);
  yy_destructor(28,&yymsp[0].minor);
}
#line 2233 "pdo_user_sql_parser.c"
        break;
      case 132:
#line 585 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy8,yymsp[-2].minor.yy8,"+",yymsp[0].minor.yy8);   yy_destructor(29,&yymsp[-1].minor);
}
#line 2239 "pdo_user_sql_parser.c"
        break;
      case 133:
#line 586 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy8,yymsp[-2].minor.yy8,"-",yymsp[0].minor.yy8);   yy_destructor(30,&yymsp[-1].minor);
}
#line 2245 "pdo_user_sql_parser.c"
        break;
      case 134:
#line 587 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy8,yymsp[-2].minor.yy8,"*",yymsp[0].minor.yy8);   yy_destructor(25,&yymsp[-1].minor);
}
#line 2251 "pdo_user_sql_parser.c"
        break;
      case 135:
#line 588 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy8,yymsp[-2].minor.yy8,"/",yymsp[0].minor.yy8);   yy_destructor(26,&yymsp[-1].minor);
}
#line 2257 "pdo_user_sql_parser.c"
        break;
      case 136:
#line 589 "pdo_user_sql_parser.lemon"
{ DO_MATHOP(yygotominor.yy8,yymsp[-2].minor.yy8,"%",yymsp[0].minor.yy8);   yy_destructor(27,&yymsp[-1].minor);
}
#line 2263 "pdo_user_sql_parser.c"
        break;
      case 137:
#line 590 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); array_init(yygotominor.yy8); add_assoc_stringl(yygotominor.yy8, "type", "distinct", sizeof("distinct") - 1, 1); add_assoc_zval(yygotominor.yy8, "distinct", yymsp[0].minor.yy8);   yy_destructor(24,&yymsp[-1].minor);
}
#line 2269 "pdo_user_sql_parser.c"
        break;
      case 138:
#line 593 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_zvalize_lnum(&yymsp[0].minor.yy0); }
#line 2274 "pdo_user_sql_parser.c"
        break;
      case 139:
#line 594 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_zvalize_hnum(&yymsp[0].minor.yy0); }
#line 2279 "pdo_user_sql_parser.c"
        break;
      case 140:
#line 597 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_zvalize_dnum(&yymsp[0].minor.yy0); }
#line 2284 "pdo_user_sql_parser.c"
        break;
      case 141:
#line 600 "pdo_user_sql_parser.lemon"
{ yygotominor.yy8 = pusp_zvalize_token(&yymsp[0].minor.yy0); }
#line 2289 "pdo_user_sql_parser.c"
        break;
      case 144:
#line 603 "pdo_user_sql_parser.lemon"
{ TSRMLS_FETCH(); yygotominor.yy8 = EG(uninitialized_zval_ptr);   yy_destructor(46,&yymsp[0].minor);
}
#line 2295 "pdo_user_sql_parser.c"
        break;
      case 149:
#line 614 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); add_assoc_stringl(yygotominor.yy8, "direction", "asc", sizeof("asc") - 1, 1); add_assoc_zval(yygotominor.yy8, "by", yymsp[-1].minor.yy8);   yy_destructor(96,&yymsp[0].minor);
}
#line 2301 "pdo_user_sql_parser.c"
        break;
      case 150:
#line 615 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); add_assoc_stringl(yygotominor.yy8, "direction", "desc", sizeof("desc") - 1, 1); add_assoc_zval(yygotominor.yy8, "by", yymsp[-1].minor.yy8);   yy_destructor(97,&yymsp[0].minor);
}
#line 2307 "pdo_user_sql_parser.c"
        break;
      case 151:
#line 616 "pdo_user_sql_parser.lemon"
{ MAKE_STD_ZVAL(yygotominor.yy8); add_assoc_null(yygotominor.yy8, "direction"); add_assoc_zval(yygotominor.yy8, "by", yymsp[0].minor.yy8); }
#line 2312 "pdo_user_sql_parser.c"
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
#line 2376 "pdo_user_sql_parser.c"
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
