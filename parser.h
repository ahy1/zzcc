

#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <stdlib.h>

#include "token.h"

enum {
	NT_NULL,
	NT_ROOT,
	NT_UNIT,
	NT_ARGSPEC,
	NT_ARGSPECLIST,
	NT_FUNCDECL,
	NT_FUNCDEF,
	NT_RETURN,
	NT_DECL,
	NT_STMT,
	NT_BLOCK,
	NT_EXPR,
	NT_TYPESPEC,
	NT_STRUCTSPEC,
	NT_UNIONSPEC,
	NT_STRUCT_UNION_MEMBER_LIST,
	NT_NAME,
	NT_NUMBER,
	NT_LABEL,
	NT_DEFAULT,
	NT_CASE,
	NT_DECLARATROR,
	NT_POINTER_QUALIFIER,
	NT_WHILE,
	NT_DO,
	NT_FOR,
	NT_IF,
	NT_SWITCH,
	NT_GOTO,
	NT_BREAK,
	NT_CONTINUE,
	NT_DECLDEF,
	NT_OPERATOR,

	/*NT_POST_INCREMENT,
	NT_POST_DECREMENT,
	NT_FUNC_CALL,
	NT_SUBSCRIPT,
	NT_ELEMENT_SELECT,
	NT_POINTER_ELEMENT_SELECT,
	NT_PRE_INCREMENT,
	NT_PRE_DECREMENT,
	NT_UNARY_PLUS,
	NT_UNARY_MINUS,
	NT_NOT,
	NT_COMPLEMENT,
	NT_POINTER_DEREF,
	NT_ADDRESS_OFF,
	NT_MULTIPLICATION,
	NT_DIVISION,
	NT_REMAINDER,
	NT_ADDITION,
	NT_SUBTRACTION,
	NT_LEFTSHIFT,
	NT_RIGHTSHIFT,
	NT_LESSTHAN,
	NT_LESSTHAN_EQUAL,
	NT_GREATERTHAN,
	NT_GREATERTHAN_EQUAL,
	NT_EQUAL,
	NT_NOT_EQUAL,
	NT_BIT_AND,
	NT_BIT_XOR,
	NT_BIT_OR,
	NT_AND,
	NT_OR,
	NT_EXPR_IF,
	NT_EXPR_ELSE,
	NT_ASSIGNMENT,
	NT_ADDITION_ASSIGNMENT,
	NT_SUBTRACTION_ASSIGNMENT,
	NT_MULTIPLICATION_ASSIGNMENT,
	NT_DIVISION_ASSIGNMENT,
	NT_REMAINDER_ASSIGNMENT,
	NT_LEFTSHIFT_ASSIGNMENT,
	NT_RIGHTSHIFT_ASSIGNMENT,
	NT_BIT_AND_ASSIGNMENT,
	NT_BIT_XOR_ASSIGNMENT,
	NT_BIT_OR_ASSIGNMENT,
	NT_COMMA,*/

	NT_DUMMY
};

struct node_s {
	struct node_s *parent;
	struct node_s *scope_parent;	/* The nodes surrounding scope */
	int type;
	int subtype;			/* For operators, the token type is here */
	int level;			/* Level in syntax three */
	struct token_s *token;		/* (optional) Token identifying data for this node */
	struct node_s **subnodes;
	size_t nsubnodes;
	const char **typealiases;
	size_t ntypealiases;
};

struct node_s *create_node(struct node_s *parent, int type);
int free_node(struct node_s *node);
void print_node(struct node_s *node, int ind);
void print_node_json(struct node_s *node, int ind);
size_t parse(struct node_s *parent, struct token_s **tokens);

#endif

