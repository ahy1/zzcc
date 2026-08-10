

#include <stdlib.h>
#include <string.h>

#include "strbuf.h"

STRBUF *sballoc(size_t capacity)
{
	STRBUF *sb;

	if (!(sb=(STRBUF *)malloc(sizeof *sb))) return NULL;
	if (!(sb->addr=(char *)malloc(capacity))) {
		free(sb);
		return NULL;
	}

	sb->capacity=capacity;
	if (capacity>0) *sb->addr='\0';
	sb->ix=0;

	return sb;
}

STRBUF *sbrealloc(STRBUF *sb, size_t capacity)
{
	char *oldaddr=sb->addr;

	if (!(sb->addr=realloc(sb->addr, capacity))) {
		sb->addr=oldaddr;
		return NULL;
	}

	sb->capacity=capacity;

	return sb;
}

int sbfree(STRBUF *sb)
{
	if (sb) {
		if (sb->addr) free(sb->addr);
		free(sb);
		return 0;
	} else return -1;
}

STRBUF *sbexpand(STRBUF *sb, size_t needed_capacity)
{
	if (needed_capacity>sb->capacity) {
		if (needed_capacity<sb->capacity*2) sb=sbrealloc(sb, sb->capacity*2);
		else sb=realloc(sb, needed_capacity);
	}

	return sb;
}

int sbcat(STRBUF *sb, const char *str)
{
	size_t needed_capacity=sb->ix+strlen(str)+1;

	if (!(sb=sbexpand(sb, needed_capacity))) return -1;

	strcpy(sb->addr+sb->ix, str);
	sb->ix+=strlen(str);

	return 0;
}

int sbput(STRBUF *sb, int ch)
{
	size_t needed_capacity=sb->ix+2;

	if (!(sb=sbexpand(sb, needed_capacity))) return -1;

	sb->addr[sb->ix++]=(char)ch;
	sb->addr[sb->ix]='\0';

	return ch;
}

int sbstop(STRBUF *sb)
{
	++sb->ix;

	if (sb->ix>=sb->capacity) {
		if (!(sb=sbexpand(sb, sb->capacity+1))) return -1;
	}

	sb->addr[sb->ix]='\0';

	return 0;
}

size_t sbix(STRBUF *sb)
{
	return sb->ix;
}

int sbforeach(STRBUF *sb, size_t ix, int (*fn)(const char *))
{
	while (ix<sb->ix) {
		if (fn(sb->addr+ix)!=0) return -1;
		ix+=strlen(sb->addr+ix)+1;
	}

	return 0;
}

size_t sbsearch(STRBUF *sb, size_t ix, int (*fn)(const char *, const char *), const char *what)
{
	while (ix<sb->ix) {
		if (fn(sb->addr+ix, what)==0) return ix;
		ix+=strlen(sb->addr+ix)+1;
	}

	return (size_t)-1;		// TODO: What to return??
}

const char *sbcstr(STRBUF *sb, size_t ix)
{
	if (ix<sb->capacity) return sb->addr+ix;
	else return NULL;
}

int sbwritejson(STRBUF *sb, size_t ix, int (*fn)(const char *))
{
	const char *str;
	char *escaped=NULL, *p;

	fn("[\n");
	while (ix<sb->ix) {
		if (ix>0) fn(" ,");
		else fn("  ");
		str=sb->addr+ix;
		if(!(escaped=(char *)realloc(escaped, 3+strlen(str)*2))) return -1;	// Enough space for leading/ending '\"', possibly escaping every single character and '\0'
		escaped[0]='\"';
		p=escaped+1;
		while (*str) {
			switch (*str) {
				case '\'':
					*p++='\\'; *p++='\'';
					break;
				case '\"':
					*p++='\\'; *p++='\"';
					break;
				case '\n':
					*p++='\\'; *p++='n';
					break;
				case '\r':
					*p++='\\'; *p++='r';
					break;
				case '\f':
					*p++='\\'; *p++='f';
					break;
				case '\b':
					*p++='\\'; *p++='b';
					break;
				case '\t':
					*p++='\\'; *p++='t';
					break;
				case '\\':
					*p++='\\'; *p++='\\';
					break;
				default:;
					*p++=*str;

			}
			++str;
		}
		*p++='\"';
		*p++='\n';
		*p='\0';

		fn(escaped);

		ix+=strlen(sb->addr+ix)+1;
	}
	fn("]\n");

	free(escaped);

	return 0;
}

int sbwritecsv(STRBUF *sb, size_t ix, int sep, int quote, int (*fn)(const char *))
{
	const char *str;
	char *escaped=NULL, *p;
	char seps[2]={(char)sep, '\0'};

	while (ix<sb->ix) {
		if (ix>0) fn(seps);
		str=sb->addr+ix;
		if (!(escaped=(char *)realloc(escaped, 3+strlen(str)*2))) return -1;	// Enough space for leading/ending '\"', possibly escaping every single character and '\0'
		escaped[0]=(char)quote;
		p=escaped+1;
		while (*str) {
			if (*str==quote) {
				*p++=(char)quote;
			}
			*p++=*str++;
		}
		*p++=(char)quote;
		*p='\0';

		fn(escaped);

		ix+=strlen(sb->addr+ix)+1;
	}

	free(escaped);

	return 0;
}

