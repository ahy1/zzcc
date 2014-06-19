
#ifndef STRBUF_H_INCLUDED
#define STRBUF_H_INCLUDED

#include <stdlib.h>

typedef struct {
	char *addr;
	size_t ix;			// '\0'-terminator of last token
	size_t capacity;
} STRBUF;

STRBUF *sballoc(size_t capacity);
STRBUF *sbrealloc(STRBUF *sb, size_t capacity);
int sbfree(STRBUF *sb);
STRBUF *sbexpand(STRBUF *sb, size_t needed_capacity);
int sbcat(STRBUF *sb, const char *str);
int sbput(STRBUF *sb, int ch);
int sbstop(STRBUF *sb);
size_t sbix(STRBUF *sb);
int sbforeach(STRBUF *sb, size_t ix, int (*fn)(const char *));
size_t sbsearch(STRBUF *sb, size_t ix, int (*fn)(const char *, const char *), const char *what);
const char *sbcstr(STRBUF *sb, size_t ix);
int sbwritejson(STRBUF *sb, size_t ix, int (*fn)(const char *));
int sbwritecsv(STRBUF *sb, size_t ix, int sep, int quote, int (*fn)(const char *));

#endif

