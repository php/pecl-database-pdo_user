#ifndef PDO_USER_SQL_PARSER_H
#define PDO_USER_SQL_PARSER_H
#include "pdo_user_sql_parser.h"
#endif

typedef struct _php_pdo_user_sql_token {
	unsigned char id;
	char *token;
	int token_len;
	int freeme:1;
} php_pdo_user_sql_token;

typedef struct _php_pdo_user_sql_tokenizer {
	char *start, *end;
} php_pdo_user_sql_tokenizer;

/* Tokens not identified by or used by the parser */
#define PU_END				0
#define PU_WHITESPACE		0xFF
#define PU_GRANT			0xFE
#define PU_REVOKE			0xFD
#define PU_IDENTIFIED		0xFC
#define PU_ALTER			0xFB
#define PU_ADD				0xFA
#define PU_VIEW				0xF9
#define PU_COLUMN			0xF8
#define PU_BEFORE			0xF7
#define PU_AFTER			0xF6
#define PU_ALL				0xF5

typedef struct _php_pdo_user_sql_token_label {
	unsigned char id;
	const char *label;
} php_pdo_user_sql_token_label;

extern php_pdo_user_sql_token_label php_pdo_user_sql_token_labels[];
int php_pdo_user_sql_get_token(php_pdo_user_sql_tokenizer *t, php_pdo_user_sql_token *token);
void *php_pdo_user_sql_parserAlloc(void *(*mallocProc)(size_t));
void php_pdo_user_sql_parser(void *yyp, int yymajor, php_pdo_user_sql_token yyminor, zval *return_value);
void php_pdo_user_sql_parserFree(void *p, void (*freeProc)(void*));

