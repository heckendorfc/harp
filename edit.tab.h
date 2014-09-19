#define TOK_TEXT 257
#define TOK_QUOTE 258
#define TOK_QUOTE_STR 259
#define TOK_WHITESPACE 260
#define TOK_COMMA 261
#define TOK_WORD 262
#define TOK_SEMICOLON 263
#define TOK_OPAR 264
#define TOK_CPAR 265
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union{
	int number;
	char *word;
	wordchain_t *wc;
	wordlist_t *wl;
	arglist_t *arg;
	command_t *command;
	commandline_t *commandline;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;
