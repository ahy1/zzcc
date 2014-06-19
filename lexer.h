

#ifndef LEXER_H_DEFINED
#define LEXER_H_DEFINED

#include "strbuf.h"

struct token_s {
	STRBUF *sb;
	size_t sbix;
	int type;
	int subtype;
	int encoding;
	const char *fname;
	int lno;
	int cno;
};

struct tokentype_s {
	int type;
	int subtype;
	int encoding;
	int (*firstchar)(int ch);
	int (*followingchar)(int ch);
	int (*stringok)(const char *str, int ch);
	int (*stringcomplete)(const char *str);
};

struct tokenreserved_s {
	int type;
	int subtype;
	int encoding;
	struct tokentype_s *basetype;
	char *name;
};

#endif


