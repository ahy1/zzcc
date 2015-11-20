

#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <stdlib.h>

#include "token.h"

enum {
	TRANSLATION_UNIT,
	EXTERNAL_DECLARATION,
	FUNCTION_DEFINITION,
	COMPOUND_STATEMENT,
	BLOCK_ITEM_LIST,
	BLOCK_ITEM,
	STATEMENT,
	JUMP_STATEMENT,
	ITERATION_STATEMENT,
	SELECTION_STATEMENT,
	MULTIPLICATIVE_EXPRESSION,
	ADDITIVE_EXPRESSION,
	SHIFT_EXPRESSION,
	RELATIONAL_EXPRESSION,
	EQUALITY_EXPRESSION,
	AND_EXPRESSION,
	EXCLUSIVE_OR_EXPRESSION,
	INCLUSIVE_OR_EXPRESSION,
	LOGICAL_AND_EXPRESSION,
	LOGICAL_OR_EXPRESSION,
	CONDITIONAL_EXPRESSION,
	CONSTANT,
	STRING_LITERAL,
	PRIMARY_EXPRESSION,
	TYPE_NAME,
	DESIGNATOR,
	DESIGNATION,	
	DESIGNATION_INITIALIZER,
	INITIALIZER_LIST,
	POSTFIX_EXPRESSION,
	UNARY_OPERATOR,
	CAST_EXPRESSION,
	UNARY_EXPRESSION,	
	ASSIGNMENT_EXPRESSION,
	DIRECT_ABSTRACT_DECLARATOR,
	ABSTRACT_DECLARATOR,
	PARAMETER_DECLARATION,
	PARAMETER_LIST,
	PARAMETER_TYPE_LIST,
	STORAGE_CLASS_SPECIFIER,
	STRUCT_OR_UNION,
	SPECIFIER_QUALIFIER_LIST,
	STRUCT_DECLARATOR,
	STRUCT_DECLARATOR_LIST,
	STRUCT_DECLARATION,
	STRUCT_DECLARATION_LIST,
	STRUCT_OR_UNION_SPECIFIER,
	ENUMERATOR,
	ENUMERATOR_LIST,
	ENUM_SPECIFIER,
	TYPE_SPECIFIER,
	TYPE_QUALIFIER,
	TYPE_QUALIFIER_LIST,
	FUNCTION_SPECIFIER,
	DECLARATION_SPECIFIERS,
	POINTER,
	DIRECT_DECLARATOR,
	DECLARATOR,
	IDENTIFIER,
	IDENTIFIER_LIST,
	INITIALIZER,
	EXPRESSION,
	EXPRESSION_STATEMENT,
	INIT_DECLARATOR,
	INIT_DECLARATOR_LIST,
	DECLARATION,
	DECLARATION_LIST,
	LABELED_STATEMENT,

	NT_ANY,		/* Placeholder until real enum has been created */

	NT_ROOT,	/* Used by zzparser.c */
	NT_UNIT,	/* Top level */
	NT_DUMMY
};

struct node_s {
	struct node_s *parent;
	struct node_s *scope_parent;	/* The node surrounding scope */
	int type;
	int subtype;			/* For operators, the token type is here */
	int level;			/* Level in syntax three */
	struct token_s *token;		/* (optional) Token identifying data for this node */
	struct node_s **subnodes;
	size_t nsubnodes;
	const char **typealiases;
	size_t ntypealiases;
};

struct node_s *create_node(struct node_s *parent, int type, struct token_s *token);
size_t free_node(struct node_s *node);
void print_node(struct node_s *node, int ind);
void print_node_json(struct node_s *node, int ind);
size_t parse(struct node_s *parent, struct token_s **tokens);

#endif

