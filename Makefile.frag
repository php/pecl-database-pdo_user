$(srcdir)/pdo_user_sql_tokenizer.c: $(srcdir)/pdo_user_sql_tokenizer.re
	@(cd $(srcdir); $(RE2C) -b -o pdo_user_sql_tokenizer.c pdo_user_sql_tokenizer.re)

