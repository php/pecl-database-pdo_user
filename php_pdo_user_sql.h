typedef struct _php_pdo_user_sql_token {
	unsigned char id;
	char *token;
	int token_len;
	int freeme:1;
} php_pdo_user_sql_token;

typedef struct _php_pdo_user_sql_tokenizer {
	char *start, *end;
} php_pdo_user_sql_tokenizer;

enum php_pdo_user_sql_token_num {
	PU_END,
	PU_LABEL,
	PU_STRING,
	PU_WHITESPACE,

	/* Verbs */
	PU_SELECT,
	PU_INSERT,
	PU_UPDATE,
	PU_DELETE,
	PU_CREATE,
	PU_RENAME,
	PU_ALTER,
	PU_GRANT,
	PU_REVOKE,
	PU_ADD,
	PU_DROP,

	/* Nouns */
	PU_TABLE,
	PU_VIEW,
	PU_COLUMN,
	PU_KEY,
	PU_NULL,
	PU_AUTO_INCREMENT,
	PU_HNUM,
	PU_LNUM,
	PU_ALL,

	/* Prepositions */
	PU_INTO,
	PU_FROM,
	PU_DEFAULT,
	PU_WHERE,
	PU_GROUP,
	PU_LIMIT,
	PU_HAVING,
	PU_ORDER,
	PU_BY,
	PU_AS,
	PU_TO,
	PU_VALUES,
	PU_UNIQUE,
	PU_PRIMARY,
	PU_BEFORE,
	PU_AFTER,
	PU_IDENTIFIED,
	PU_DISTINCT,
	PU_INNER,
	PU_OUTER,
	PU_LEFT,
	PU_RIGHT,
	PU_JOIN,
	PU_ON,
	PU_ASC,
	PU_DESC,

	/* SQL Types */
	PU_INT,
	PU_BIT,
	PU_CHAR,
	PU_VARCHAR,
	PU_DATETIME,
	PU_DATE,
	PU_TIME,
	PU_TIMESTAMP,
	PU_DOUBLE,
	PU_FLOAT,
	PU_TEXT,
	PU_BLOB,
	PU_SET,
	PU_ENUM,

	/* Logicals */
	PU_NOT_EQUAL, 	/* != */
	PU_UNEQUAL,		/* <> */
	PU_LESSER_EQUAL,
	PU_GREATER_EQUAL,
	PU_LIKE,
	PU_RLIKE,
	PU_NOT,
	PU_AND,
	PU_OR,
	PU_XOR,

	/* Single Character Tokens */
	PU_PLUS,
	PU_MINUS,
	PU_MUL,
	PU_DIV,
	PU_MOD,
	PU_LPAREN,
	PU_RPAREN,
	PU_COMMA,
	PU_EQUALS,
	PU_SEMICOLON
};

typedef struct _php_pdo_user_sql_token_label {
	unsigned char id;
	const char *label;
} php_pdo_user_sql_token_label;

extern php_pdo_user_sql_token_label php_pdo_user_sql_token_labels[];
int php_pdo_user_sql_get_token(php_pdo_user_sql_tokenizer *t, php_pdo_user_sql_token *token);
