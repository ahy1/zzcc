

#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <stdlib.h>

#include "token.h"
#include "node.h"

size_t type_name(struct node_s *parent, struct token_s **tokens);
size_t parse(struct node_s *parent, struct token_s **tokens);

#endif

