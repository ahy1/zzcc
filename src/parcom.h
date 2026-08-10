

#ifndef PARCOM_C_INCLUDED
#define PARCOM_C_INCLUDED

#include <stdlib.h>

struct parser_s {
	void *opts;
	size_t (*parse)(const void *opts, const char *b, size_t nbs);
	void (*free)(struct parser_s *parser);
};

struct parser_s *keyword(const char *str);
struct parser_s *pattern(const char *str);
struct parser_s *sequence(struct parser_s *first, ...);
struct parser_s *choice(struct parser_s *first, ...);

size_t do_parse(struct parser_s *parser, const char *b, size_t nbs);


#endif
