
#include "parcom.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>	/* For test logging */
#include <stdarg.h>

static char *mystrdup(const char *s)
{
	char *d=malloc(strlen(s)+1);

	if (!d) return NULL;

	strcpy(d, s);

	return d;
}

int myisblank(int ch)	/* isblank() not supported in VS2005 */
{
	return ch==' ' || ch=='\t';
}

/* keyword parsing */

static size_t parse_keyword(const void *opts, const char *b, size_t nbs)
{
	const char *kw=opts, *p=b;

	for (;*kw && *p && *kw==*p; ++kw, ++p) ;

	if (*kw=='\0' && !isalnum((int)*p)) return p-b;

	return 0;

}

static void free_keyword(struct parser_s *parser)
{
	free(parser->opts);
	free(parser);
}

struct parser_s *keyword(const char *str)
{
	struct parser_s *parser=malloc(sizeof *parser);

	if (!parser) return NULL;

	parser->opts=mystrdup(str);
	parser->parse=parse_keyword;
	parser->free=free_keyword;

	return parser;
}


/* pattern parsing */

/* Patterns:
 *
 * \a         - a
 * .          - any char
 * [abc...z]  - any of abc...z
 * [^abc...z] - any not abc...z
 * a?         - 0 or 1 instance of a
 * a*         - 0 or more instances of a
 * a+         - 1 or more instances of a
 * [:alnum:]  - a-zA-Z0-9
 * [:alpha:]  - a-zA-Z
 * [:ascii:]  - 00-7F
 * [:blank:]  - <space><tab>
 * [:cntrl:]  - 00-1F,7F
 * [:digit:]  - 0-9
 * [:graph:]  - 21-7E
 * [:ident:]  - a-zA-Z_
 * [:idnum:]  - a-zA-Z_0-9
 * [:lower:]  - a-z
 * [:print:]  - 20-7E
 * [:punct:]  - !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~
 * [:space:]  - <space><tab><cr><lf><vt><ff>
 * [:upper:]  - A-Z
 * [:xdigit:] - 0-9a-zA-Z
*/

static int match_ch(const char *pattern, int ch)
{
//	printf("match_ch(%s, %c)\n", pattern, ch);

	switch(*pattern) {
	case '\\':
		return (*++pattern==ch) ? 2 : -2;
	case '.':
		return ch ? 1 : -1;
	case '[':
		++pattern;
		if (*pattern==':') {
			++pattern;
			if (!strncmp(pattern, "alnum:]", 7)) return isalnum(ch) ? 9 : -9;
			else if (!strncmp(pattern, "alpha:]", 7)) return isalpha(ch) ? 9 : -9;
			else if (!strncmp(pattern, "ascii:]", 7)) return isascii(ch) ? 9 : -9;
			else if (!strncmp(pattern, "blank:]", 7)) return myisblank(ch) ? 9 : -9;
			else if (!strncmp(pattern, "cntrl:]", 7)) return iscntrl(ch) ? 9 : -9;
			else if (!strncmp(pattern, "digit:]", 7)) return isdigit(ch) ? 9 : -9;
			else if (!strncmp(pattern, "graph:]", 7)) return isgraph(ch) ? 9 : -9;
			else if (!strncmp(pattern, "ident:]", 7)) return (isalpha(ch)||ch=='_') ? 9 : -9;
			else if (!strncmp(pattern, "idnum:]", 7)) return (isalnum(ch)||ch=='_') ? 9 : -9;
			else if (!strncmp(pattern, "lower:]", 7)) return islower(ch) ? 9 : -9;
			else if (!strncmp(pattern, "print:]", 7)) return isprint(ch) ? 9 : -9;
			else if (!strncmp(pattern, "punct:]", 7)) return ispunct(ch) ? 9 : -9;
			else if (!strncmp(pattern, "space:]", 7)) return isspace(ch) ? 9 : -9;
			else if (!strncmp(pattern, "upper:]", 7)) return isupper(ch) ? 9 : -9;
			else if (!strncmp(pattern, "xdigit:]", 8)) return isxdigit(ch) ? 10 : -10;
			else return 0;
		} else if (*pattern=='^') {		/* Not any of */
			return 0;
		} else {				/* Any of */
			return 0;
		}
		break;
	default:
		return *pattern==ch ? 1 : -1;
	}

}

static size_t parse_pattern(const void *opts, const char *b, size_t nbs)
{
	const char *kw=opts, *p=b;
	int mlen;
	int modifier=0;

//	printf("   parse_pattern(%s) - pattern=[%s]\n", b, kw);

	while (*kw) {
//		printf("     matching *kw=[%c], b=[%s]\n", *kw, b);
		mlen=match_ch(kw, *p);

		if (mlen>0) {
//			puts("Match");

			/* Match */
			if (!modifier) modifier=*(kw+mlen);
//			printf(" pre modifier=[%c]\n", modifier);
			if (modifier=='?') {
				kw+=mlen+1;
				++p;
				modifier=0;
			} else if (modifier=='*') {
				++p;
			} else if (modifier=='+') {
				++p;
				modifier='*';
			} else {
				kw+=mlen;
				++p;
				modifier=0;
			}
//			printf(" post modifier=[%c] p=[%s]\n", modifier, p);
		} else if (mlen<0) {
//			puts("No match");

			/* No match */
			if (!modifier) modifier=*(kw-mlen);
//			printf(" modifier=[%c]\n", modifier);

			if (modifier=='?') {
				kw-=mlen-1;
				modifier=0;
			} else if (modifier=='*') {
				kw-=mlen-1;
				modifier=0;
			} else if (modifier=='+') {
				return (size_t)0;
			} else {
				return (size_t)0;
			}
		} else {
			/* ERROR */
			return (size_t)-1;
		}
	}

	return p-b;
}

static void free_pattern(struct parser_s *parser)
{
	free(parser->opts);
	free(parser);
}

struct parser_s *pattern(const char *str)
{
	struct parser_s *parser=malloc(sizeof *parser);

	if (!parser) return NULL;

	parser->opts=mystrdup(str);
	parser->parse=parse_pattern;
	parser->free=free_pattern;

	return parser;
}


/* sequence combinator */

struct sequence_s {
	struct parser_s **parsers;
	size_t nparsers;
};

static size_t parse_sequence(const void *opts, const char *b, size_t nbs)
{
	const struct sequence_s *seq=opts;
	struct parser_s **par;
	size_t nparsed, total, nparsers;

	for (par=seq->parsers, nparsers=seq->nparsers, total=0; nparsers; ++par, --nparsers) {
//		printf("    -- tick [%s] nparsers=%d\n", b, (int)nparsers);
		nparsed=(*par)->parse((*par)->opts, b, nbs);
//		printf("     nparsed=%d\n", (int)nparsed);
		if ((int)nparsed>0) {
//			printf("         b=[%s]\n", b);
			total+=nparsed;
			b+=nparsed;
			nbs-=nparsed;
//			printf("         :b=[%s] nbs=%d\n", b, (int)nbs);
		} else return 0;
	}

//	printf("      > total=%d\n", (int)total);

	return total;
}

static void free_sequence(struct parser_s *parser)
{
	struct sequence_s *seq=parser->opts;
	struct parser_s **par;

	for (par=seq->parsers; seq->nparsers; ++par, --seq->nparsers)
		(*par)->free(*par);
	free(seq->parsers);
	free(parser->opts);
	free(parser);
}

struct parser_s *sequence(struct parser_s *first, ...)
{
	struct parser_s *parser=malloc(sizeof *parser);
	struct sequence_s *sequence=malloc(sizeof *sequence);
	va_list args;

	if (!sequence || !parser) return NULL;

	sequence->parsers=malloc(sizeof *sequence->parsers);
	if (!sequence->parsers) return NULL;
	sequence->nparsers=1;

	sequence->parsers[0]=first;

	va_start(args, first);

	while ((first=va_arg(args, struct parser_s *))!=NULL) {
		sequence->parsers=realloc(sequence->parsers,
			++sequence->nparsers * sizeof *sequence->parsers);
		if (!sequence->parsers) return NULL;

		sequence->parsers[sequence->nparsers-1]=first;
	}

	va_end(args);

	parser->opts=sequence;
	parser->parse=parse_sequence;
	parser->free=free_sequence;

	return parser;
}


/* Choise combinator */

struct choice_s {
	struct parser_s **parsers;
	size_t nparsers;
};

static size_t parse_choice(const void *opts, const char *b, size_t nbs)
{
	const struct choice_s *cho=opts;
	struct parser_s **par;
	size_t nparsed, nparsers;

	for (par=cho->parsers, nparsers=cho->nparsers; nparsers; ++par, --nparsers) {
		nparsed=(*par)->parse((*par)->opts, b, nbs);
		if ((int)nparsed>0) return nparsed;
	}

	return 0;
}

static void free_choice(struct parser_s *parser)
{
	struct choice_s *cho=parser->opts;
	struct parser_s **par;

	for (par=cho->parsers; cho->nparsers; ++par, --cho->nparsers)
		(*par)->free(*par);
	free(cho->parsers);
	free(parser->opts);
	free(parser);
}

struct parser_s *choice(struct parser_s *first, ...)
{
	struct parser_s *parser=malloc(sizeof *parser);
	struct sequence_s *choice=malloc(sizeof *choice);
	va_list args;

	if (!choice || !parser) return NULL;

	choice->parsers=malloc(sizeof *choice->parsers);
	if (!choice->parsers) return NULL;
	choice->nparsers=1;

	choice->parsers[0]=first;

	va_start(args, first);

	while ((first=va_arg(args, struct parser_s *))!=NULL) {
		choice->parsers=realloc(choice->parsers,
			++choice->nparsers * sizeof *choice->parsers);
		if (!choice->parsers) return NULL;

		choice->parsers[choice->nparsers-1]=first;
	}

	va_end(args);

	parser->opts=choice;
	parser->parse=parse_choice;
	parser->free=free_choice;

	return parser;
}




/* Support */

size_t do_parse(struct parser_s *parser, const char *b, size_t nbs)
{
	if (!nbs) nbs=strlen(b);

	return parser->parse(parser->opts, b, nbs);
}




