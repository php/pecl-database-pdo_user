$(srcdir)/pdo_user_sql_tokenizer.c: $(srcdir)/pdo_user_sql_tokenizer.re
	@(cd $(top_srcdir); $(RE2C) -b -o $(srcdir)/pdo_user_sql_tokenizer.c $(srcdir)/pdo_user_sql_tokenizer.re)

