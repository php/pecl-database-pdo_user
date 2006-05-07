#include "php.h"
#include "php_pdo_user_sql.h"

static void php_pdo_user_unquote(php_pdo_user_sql_token *token)
{
	char *out = emalloc(token->token_len + 1), *p = out;
	int i;

	token->token_len--;
	for(i = 1; i < token->token_len; i++) {
		switch (token->token[i]) {
			case '\\':
				if ((i + 1) >= token->token_len) {
					/* EOL */
					*(p++) = '\\';
					goto done;
				}
				switch (token->token[++i]) {
					case 'r':	*(p++) = '\r';		goto next_iter;
					case 'n':	*(p++) = '\n';		goto next_iter;
					case 't':	*(p++) = '\t';		goto next_iter;
					case '\\':	*(p++) = '\\';		goto next_iter;
					case 'x':
					{
						unsigned char val = 0;

						if (++i >= token->token_len) {
							*(p++) = 'x';
							goto done;
						}
						if (token->token[i] >= '0' && token->token[i] <= '9') {
							val = token->token[i] - '0';
						} else if (token->token[i] >= 'A' && token->token[i] <= 'F') {
							val = 10 + token->token[i] - 'A';
						} else if (token->token[i] >= 'a' && token->token[i] <= 'f') {
							val = 10 + token->token[i] - 'a';
						} else {
							*(p++) = 'x';
							i--;
							goto next_iter;
						}
						if (++i >= token->token_len) {
							*(p++) = val;
							goto done;
						}
						if (token->token[i] >= '0' && token->token[i] <= '9') {
							val <<= 4;
							val |= token->token[i] - '0';
						} else if (token->token[i] >= 'A' && token->token[i] <= 'F') {
							val <<= 4;
							val |= 10 + token->token[i] - 'A';
						} else if (token->token[i] >= 'a' && token->token[i] <= 'f') {
							val <<= 4;
							val |= 10 + token->token[i] - 'a';
						} else {
							*(p++) = val;
							i--;
							goto next_iter;
						}
						*(p++) = val;
						goto next_iter;
					}
					default: /* local-quote, octal, and non-escaped */
					{
						unsigned char val = 0;

						if (token->token[i] >= '0' && token->token[i] <= '7') {
							val = token->token[i] - '0';
						} else {
							*(p++) = token->token[i];
							goto next_iter;
						}
						if (++i >= token->token_len) {
							*(p++) = val;
							goto done;
						}
						if (token->token[i] >= '0' && token->token[i] <= '7') {
							val <<= 3;
							val |= token->token[i] - '0';
						} else {
							*(p++) = val;
							i--;
							goto next_iter;
						}
						if (val > 0x1F) {
							*(p++) = val;
							goto next_iter;
						}
						if (++i >= token->token_len) {
							*(p++) = val;
							goto done;
						}
						if (token->token[i] >= '0' && token->token[i] <= '7') {
							val <<= 3;
							val |= token->token[i] - '0';
						} else {
							*(p++) = val;
							i--;
							goto next_iter;
						}
						*(p++) = val;
						goto next_iter;
					}
				}
			default:
				*(p++) = token->token[i];
		}
next_iter:
		continue; /* avoids warning */
	}

done:
	token->token = out;
	token->token_len = p - out;
	*p = 0;

	token->freeme = 1;
}

#define RET(num) { token->token_len = t->start - token->token; token->id = num; return (num); }
#define RET_UNESC(num) { token->token_len = t->start - token->token; token->id = num; php_pdo_user_unquote(token); return (num); }

#define YYCTYPE		char
#define YYCURSOR	(t->start)
#define YYLIMIT		(t->end)
#define YYFILL(n)
#define YYMARKER	marker

int php_pdo_user_sql_get_token(php_pdo_user_sql_tokenizer *t, php_pdo_user_sql_token *token)
{
	token->freeme = 0;
	token->token = t->start;
	char *marker = t->start;

/*!re2c
	ESCBT		= [\\][`];
	ESCQQ		= [\\]["];
	ESCQ		= [\\]['];
	ESCSL		= [\\][\\];
	ESCSEQ		= [\\][rntx0-7];
	LNUMS		= [0-9];
	HNUMS		= [0-9A-Fa-f];
	LABELCHAR	= [0-9a-zA-Z_\200-\377];
	LABELSTART	= [a-zA-Z_\200-\377];
	ANYNOEOF	= [\001-\377];
	EOF			= [\000];

	/* SQL Verbs */
	'select'								{ RET(PU_SELECT); }
	'insert'								{ RET(PU_INSERT); }
	'update'								{ RET(PU_UPDATE); }
	'delete'								{ RET(PU_DELETE); }
	'create'								{ RET(PU_CREATE); }
	'rename'								{ RET(PU_RENAME); }
	'alter'									{ RET(PU_ALTER); }
	'grant'									{ RET(PU_GRANT); }
	'revoke'								{ RET(PU_REVOKE); }
	'add'									{ RET(PU_ADD); }
	'drop'									{ RET(PU_DROP); }

	/* SQL Nouns */
	'table'									{ RET(PU_TABLE); }
	'view'									{ RET(PU_VIEW); }
	'column'								{ RET(PU_COLUMN); }
	'key'									{ RET(PU_KEY); }
	'null'									{ RET(PU_NULL); }
	'auto_increment'						{ RET(PU_AUTO_INCREMENT); }
	'all'									{ RET(PU_ALL); }

	/* SQL Prepositions */
	'into'									{ RET(PU_INTO); }
	'from'									{ RET(PU_FROM); }
	'default'								{ RET(PU_DEFAULT); }
	'where'									{ RET(PU_WHERE); }
	'group'									{ RET(PU_GROUP); }
	'limit'									{ RET(PU_LIMIT); }
	'having'								{ RET(PU_HAVING); }
	'order'									{ RET(PU_ORDER); }
	'by'									{ RET(PU_BY); }
	'as'									{ RET(PU_AS); }
	'to'									{ RET(PU_TO); }
	'values'								{ RET(PU_VALUES); }
	'unique'								{ RET(PU_UNIQUE); }
	'primary'								{ RET(PU_PRIMARY); }
	'before'								{ RET(PU_BEFORE); }
	'after'									{ RET(PU_AFTER); }
	'identified'							{ RET(PU_IDENTIFIED); }
	'distinct'								{ RET(PU_DISTINCT); }

	/* SQL Types */
	'int'									{ RET(PU_INT); }
	'bit'									{ RET(PU_BIT); }
	'char'									{ RET(PU_CHAR); }
	'varchar'								{ RET(PU_VARCHAR); }
	'datetime'								{ RET(PU_DATETIME); }
	'date'									{ RET(PU_DATE); }
	'time'									{ RET(PU_TIME); }
	'timestamp'								{ RET(PU_TIMESTAMP); }
	'double'								{ RET(PU_DOUBLE); }
	'float'									{ RET(PU_FLOAT); }
	'text'									{ RET(PU_TEXT); }
	'blob'									{ RET(PU_BLOB); }
	'set'									{ RET(PU_SET); }
	'enum'									{ RET(PU_ENUM); }	

	/* SQL Logicals */
	"!="									{ RET(PU_NOT_EQUAL); }
	"<>"									{ RET(PU_UNEQUAL); }
	"<="									{ RET(PU_LESSER_EQUAL); }
	">="									{ RET(PU_GREATER_EQUAL); }
	'like'									{ RET(PU_LIKE); }
	'rlike'									{ RET(PU_RLIKE); }
	'not'									{ RET(PU_NOT); }
	'and'									{ RET(PU_AND); }
	'or'									{ RET(PU_OR); }
	'xor'									{ RET(PU_XOR); }

	/* Single char tokens */
	[+]										{ RET(PU_PLUS); }
	[-]										{ RET(PU_MINUS); }
	[*]										{ RET(PU_MUL); }
	[/]										{ RET(PU_DIV); }
	[%]										{ RET(PU_MOD); }
	[(]										{ RET(PU_LPAREN); }
	[)]										{ RET(PU_RPAREN); }
	[,]										{ RET(PU_COMMA); }
	[=]										{ RET(PU_EQUALS); }
	[;]										{ RET(PU_SEMICOLON); }

	([`] (ESCBT|ESCSL|ESCSEQ|ANYNOEOF\[\\`])* [`])	{ RET_UNESC(PU_LABEL); }
	(["] (ESCQQ|ESCSL|ESCSEQ|ANYNOEOF\[\\"])* ["])	{ RET_UNESC(PU_STRING); }
	(['] (ESCQ|ESCSL|ESCSEQ|ANYNOEOF\[\\'])* ['])	{ RET_UNESC(PU_STRING); }
	[\s\r\n\t ]+							{ RET(PU_WHITESPACE); }
	('0x' HNUMS+)							{ RET(PU_HNUM); }
	LNUMS+									{ RET(PU_LNUM); }
	(LABELSTART LABELCHAR*)					{ RET(PU_LABEL); }
	EOF										{ RET(PU_END); }
*/
}

#define PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(v)	{ v, #v },
 
php_pdo_user_sql_token_label php_pdo_user_sql_token_labels[] = {
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_END)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_LABEL)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_STRING)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_WHITESPACE)

	/* Verbs */
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_SELECT)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_INSERT)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_UPDATE)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_DELETE)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_CREATE)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_RENAME)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_ALTER)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_GRANT)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_REVOKE)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_ADD)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_DROP)

	/* Nouns */
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_TABLE)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_VIEW)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_COLUMN)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_KEY)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_NULL)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_AUTO_INCREMENT)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_HNUM)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_LNUM)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_ALL)

	/* Prepositions */
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_INTO)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_FROM)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(	PU_DEFAULT)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_WHERE)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_GROUP)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_LIMIT)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_HAVING)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_ORDER)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_BY)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_AS)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_TO)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_VALUES)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_UNIQUE)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_PRIMARY)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_BEFORE)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_AFTER)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_IDENTIFIED)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_DISTINCT)

	/* SQL Types */
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_INT)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_BIT)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_CHAR)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_VARCHAR)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_DATETIME)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_DATE)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_TIME)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_TIMESTAMP)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_DOUBLE)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_FLOAT)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_TEXT)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_BLOB)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_SET)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_ENUM)

	/* Logicals */
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_NOT_EQUAL) 	/* != */
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_UNEQUAL)		/* <> */
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_LESSER_EQUAL)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_GREATER_EQUAL)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_LIKE)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_RLIKE)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_NOT)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_AND)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_OR)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_XOR)

	/* Single Character Tokens */
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_PLUS)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_MINUS)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_MUL)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_DIV)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_MOD)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_LPAREN)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_RPAREN)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_COMMA)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_EQUALS)
	PHP_PDO_USER_SQL_TOKEN_LABEL_ENTRY(PU_SEMICOLON)

	{ 0, NULL }
};
