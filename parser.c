

#include "parser.h"
#include "tokenclass.h"
#include "stack.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static char *node_type_names[]={
	"NT_NULL",
	"NT_ROOT",
	"NT_UNIT",
	"NT_ARGSPEC",
	"NT_ARGSPECLIST",
	"NT_FUNCDECL",
	"NT_FUNCDEF",
	"NT_RETURN",
	"NT_DECL",
	"NT_STMT",
	"NT_BLOCK",
	"NT_EXPR",
	"NT_TYPESPEC",
	"NT_STRUCTSPEC",
	"NT_UNIONSPEC",
	"NT_STRUCT_UNION_MEMBER_LIST",
	"NT_NAME",
	"NT_NUMBER",
	"NT_LABEL",
	"NT_CASE",
	"NT_DEFAULT",
	"NT_DECLARATOR",
	"NT_POINTER_QUALIFIER",
	"NT_WHILE",
	"NT_DO",
	"NT_FOR",
	"NT_IF",
	"NT_SWITCH",
	"NT_GOTO",
	"NT_BREAK",
	"NT_CONTINUE",
	"NT_DECLDEF",
	"NT_OPERATOR",

	"NT_DUMMY"


};

static size_t parse_stmt(struct node_s *parent, struct token_s **tokens);
static size_t parse_expr(struct node_s *parent, struct token_s **tokens);
static size_t parse_block(struct node_s *parent, struct token_s **tokens);
static size_t parse_declarator(struct node_s *parent, struct token_s **tokens);
static size_t parse_declaration(struct node_s *parent, struct token_s **tokens);

static void log_node(struct node_s *node, const char *fmt, ...)
{
	va_list arg;
	int level=node->level;

	while(level--) (void)putc(' ', stderr);

	fprintf(stderr, "%-25s %3d/%3d %-10s %-10s: ", 
		node_type_names[node->type], 
		token_lno(node->token),
		token_cno(node->token),
		token_type(node->token), 
		token_text(node->token));
	va_start(arg, fmt);
	(void)vfprintf(stderr, fmt, arg);
	va_end(arg);
}

static void log_node_token(struct node_s *node, struct token_s *token, const char *fmt, ...)
{
	va_list arg;
	int level=node->level;

	while(level--) (void)putc(' ', stderr);

	//fprintf(stderr, "%-25s %-10s %-10s: ", 
	fprintf(stderr, "%-25s %3d/%3d %-10s %-10s: ", 
		node_type_names[node->type], 
		token_lno(token),
		token_cno(token),
		token_type(token), 
		token_text(token));
	va_start(arg, fmt);
	(void)vfprintf(stderr, fmt, arg);
	va_end(arg);
}

struct node_s *create_node(struct node_s *parent, int type)
{
	struct node_s *node=(struct node_s *)calloc(1, sizeof *node);
	if (!node) return NULL;

	node->parent=parent;
	node->level=node->parent->level+1;
	node->type=type;

	log_node(node, "create_node()\n");

	return node;
}

int free_node(struct node_s *node)
{
	size_t ix;

	log_node(node, "free_node(): Freeing from %s\n", 
		node_type_names[node->parent->type]);

	for (ix=0; ix<node->nsubnodes; ++ix) free(node->subnodes[ix]);

	free(node);

	return 0;
}

static int error_node(struct node_s *node, const char *fmt, ...)
{
	va_list arg;
	int level=node->level;

	while(level--) (void)putc(' ', stderr);

	fprintf(stderr, "ERROR (%s): ", node_type_names[node->type]);
	va_start(arg, fmt);
	(void)vfprintf(stderr, fmt, arg);
	va_end(arg);

	exit(EXIT_FAILURE);

	return free_node(node);
}

static int error_node_token(struct node_s *node, struct token_s *token, const char *fmt, ...)
{
	va_list arg;
	int level=node->level;

	while(level--) (void)putc(' ', stderr);

	fprintf(stderr, "ERROR (%-25s %-10s %-10s): ", 
		node_type_names[node->type], 
		token_type(token), 
		token_text(token));
	va_start(arg, fmt);
	(void)vfprintf(stderr, fmt, arg);
	va_end(arg);

	exit(EXIT_FAILURE);

	return free_node(node);
}

static int add_node(struct node_s *node)
{
	log_node(node, "add_node(): Adding to %s\n", 
		node_type_names[node->parent->type]);

	node->parent->subnodes=(struct node_s **)realloc(
		node->parent->subnodes, ++node->parent->nsubnodes * sizeof node);
	node->parent->subnodes[node->parent->nsubnodes-1]=node;

	return 0;
}

static void indent(int ind)
{
	int ix;

	for (ix=0; ix<ind; ++ix) (void)putchar(' ');
}

void print_node(struct node_s *node, int ind)
{
	size_t ix;

	indent(ind); printf("[%s", node_type_names[node->type]);

	if (node->token) {
		print_token(" ", node->token);
	} else (void)putchar('\n');

	for (ix=0; ix<node->nsubnodes; ++ix)
		print_node(node->subnodes[ix], ind+1);
	indent(ind); printf("]\n");
}

static const char *json_str(const char *str, char *buf, size_t len)
{
	size_t ix, bufix=0;

	// TODO: Check all bufix>len-n expressions for correctness
	if (bufix>len-2) return NULL;
	buf[bufix++]='"';
	for (ix=0; ix<strlen(str); ++ix) {
		switch (str[ix]) {
		case '\\':
		case '\0':
		case '\?':
		case '\"':
		case '\'':
			buf[bufix++]=str[ix];
			break;
		default:
			if (str[ix]<' ' || str[ix]>'\x7e') {
				if (bufix>len-3) return NULL;
				buf[bufix++]='\\';
				switch (str[ix]) {
				//case '\a':
				//	buf[bufix++]='a';
				case '\b':
					buf[bufix++]='b';
					break;
				case '\f':
					buf[bufix++]='f';
					break;
				case '\n':
					buf[bufix++]='n';
					break;
				case '\r':
					buf[bufix++]='r';
					break;
				case '\t':
					buf[bufix++]='t';
					break;
				//case '\v':
				//	buf[bufix++]='v';
				//	break;
				default:
					if (bufix>len-4) return NULL;
					sprintf(buf+ix, "u%04x", 
						(unsigned int)buf[bufix++]);

				}
			} else {
				if (bufix>len-2) return NULL;
				buf[bufix++]=str[ix];
			}
		}
	}

	buf[bufix++]='\"';
	buf[bufix]='\0';

	return buf;
}

void print_node_json(struct node_s *node, int ind)
{
	size_t ix;
	static char buf[1024*1024], buf2[1024*1024];

	indent(ind); printf("[{\"type\": %s, \"text\": %s}", 
		json_str(node_type_names[node->type], buf, 1024*1024), 
		json_str(token_text(node->token), buf2, 1024*1024));

	for (ix=0; ix<node->nsubnodes; ++ix) {
		printf(",\n"); 
		print_node_json(node->subnodes[ix], ind+1);
	}

	(void)putchar(']');
}

static size_t parse_name(struct node_s *parent, struct token_s **tokens)
{
	struct node_s *node;

	node=create_node(parent, NT_NAME);
	node->token=tokens[0];

	if (isname(tokens[0])) return add_node(node), (size_t)1;
	else return free_node(node), (size_t)0;
}

#if 0
static size_t parse_number(struct node_s *parent, struct token_s **tokens)
{
	struct node_s *node;

	node=create_node(parent, NT_NUMBER);
	node->token=tokens[0];

	if (isnumber(tokens[0])) return add_node(node), (size_t)1;
	else return free_node(node), (size_t)0;
}
#endif

static size_t parse_struct_union_member_list(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_STRUCT_UNION_MEMBER_LIST);
	node->token=tokens[ix];

	if (tokens[ix]->type==TT_LEFT_CURLY) {
		++ix;
	} else return error_node(node, "parse_struct_union_member_list(): Missing struct block start"), (size_t)0;

	while ((parsed=parse_declaration(node, tokens+ix))) {
		ix+=parsed;
	}

	if (tokens[ix]->type==TT_RIGHT_CURLY) {
		++ix;
		return add_node(node), ix;
	} else return error_node(node, "parse_struct_union_member_list(): Missing struct block end"), (size_t)0;
}

static size_t parse_struct_union_spec(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_STRUCTSPEC);
	node->token=tokens[ix];

	if (tokens[ix]->type==TT_STRUCT) {
		++ix;
	} else if (tokens[ix]->type==TT_UNION) {
		node->type=NT_UNIONSPEC;
		++ix;	
	} else return free_node(node), (size_t)0;

	if ((parsed=parse_name(node, tokens+ix))) ix+=parsed;

	if ((parsed=parse_struct_union_member_list(node, tokens+ix))) ix+=parsed;

	return add_node(node), ix;
}

static size_t parse_type_storage_qualifier(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0;

	if ((parsed=parse_struct_union_spec(parent, tokens))) return parsed;
	else if (istype(tokens[ix]) || isstorage(tokens[ix]) || isqualifier(tokens[ix])) return (size_t)1;
	return (size_t)0;
}


static size_t parse_typespec(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_TYPESPEC);
	node->token=tokens[ix];

	while ((parsed=parse_type_storage_qualifier(node, tokens+ix))) ix+=parsed;

	return ix;
}

static size_t parse_pointer_qualifier(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	struct node_s *node;

	node=create_node(parent, NT_POINTER_QUALIFIER);
	node->token=tokens[ix];

	if (tokens[ix]->type==TT_STAR_OP) {		/* Pointer ref. operator */
		return add_node(node), ++ix;
	}

	return free_node(node), (size_t)0;
}

static size_t parse_argspec(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_ARGSPEC);

	if ((parsed=parse_typespec(node, tokens+ix))) ix+=parsed;
	else return free_node(node), (size_t)0;

	if ((parsed=parse_declarator(node, tokens+ix))) ix+=parsed;
	else return error_node(node, "Missing declarator after typespec\n"), (size_t)0;

	return add_node(node), ix;
}

static size_t parse_argspeclist(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_ARGSPECLIST);

	if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
	else return free_node(node), (size_t)0;

	while ((parsed=parse_argspec(node, tokens+ix))) {
		ix+=parsed;
		if (tokens[ix]->type==TT_COMMA_OP) ++ix;
		else break;
	}

	if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
	else return error_node(node, "parse_argspeclist(): TT_RIGHT_PARANTHESIS - after argspec loop\n"), (size_t)0;

	return add_node(node), ix;
}

static size_t parse_declarator(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_DECLARATROR);
	node->token=tokens[ix];

	while ((parsed=parse_pointer_qualifier(node, tokens+ix))) {
		ix+=parsed;
	}

	if ((parsed=parse_name(node, tokens+ix))) ix+=parsed;
	else return free_node(node), (size_t)0;

	if (tokens[ix]->type==TT_LEFT_SQUARE) {
		++ix;
		if ((parsed=parse_expr(node, tokens+ix))) ix+=parsed;
		if (tokens[ix]->type!=TT_RIGHT_SQUARE) return free_node(node), (size_t)0;
		++ix;
	}

	if ((parsed=parse_argspeclist(node, tokens+ix))) ix+=parsed;

	return add_node(node), ix;
}

static size_t parse_declaration(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_DECL);
	node->token=tokens[ix];

	if ((parsed=parse_typespec(node, tokens+ix))) ix+=parsed;
	else return free_node(node), (size_t)0;

	while ((parsed=parse_declarator(node, tokens+ix))) {
		ix+=parsed;

		/* Bitfield specifier (only relevant for struct members) */
		if (tokens[ix]->type==TT_COLON_OP) {
			++ix;
			if ((parsed=parse_expr(node, tokens+ix))) ix+=parsed;
			else return error_node(node, "parse_declaration(): Missing bitfield size"), (size_t)0;
		}

		/* Initializer (Only relevant for regular declarations) */
		if (tokens[ix]->type==TT_ASSIGNMENT_OP) {
			++ix;
			if ((parsed=parse_name(node, tokens+ix))) ix+=parsed;
			else return error_node(node, "parse_declaration(): Missing initializer"), (size_t)0;
		}

		if (tokens[ix]->type==TT_COMMA_OP) ++ix;
		else break;
	}

	if (tokens[ix]->type==TT_SEMICOLON_OP) return add_node(node), ++ix;
	else return free_node(node), (size_t)0;
}

static size_t parse_typedef(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_DECL);
	node->token=tokens[ix];

	if (istypedef(tokens[ix])) ++ix;
	else return free_node(node), (size_t)0;

	if ((parsed=parse_typespec(node, tokens+ix))) ix+=parsed;
	else return free_node(node), (size_t)0;

	while ((parsed=parse_declarator(node, tokens+ix))) {
		ix+=parsed;

		if (tokens[ix]->type==TT_COMMA_OP) ++ix;
		else break;
	}

	if (tokens[ix]->type==TT_SEMICOLON_OP) return add_node(node), ++ix;
	else return free_node(node), (size_t)0;
}


enum {OP_ASSOC_NONE=0, OP_ASSOC_LEFT, OP_ASSOC_RIGHT};
enum {OP_UNARY, OP_RIGHT_UNARY, OP_BINARY/*, OP_TRINARY*/};
struct op_s {
	int op;		/* Operator token type */
	int prec;	/* Operator precedence (higher number gives higher precedence) */
	int assoc;	/* Operator associativity */
	int arity;	/* Operator arity */
};

static struct op_s op_table[]={
	{TT_PLUSPLUS_OP, 98, OP_ASSOC_LEFT, OP_RIGHT_UNARY},
	{TT_MINUSMINUS_OP, 98, OP_ASSOC_LEFT, OP_RIGHT_UNARY},
	{TT_LEFT_PARANTHESIS, 98, OP_ASSOC_LEFT, OP_UNARY},
	{TT_LEFT_SQUARE, 98, OP_ASSOC_LEFT, OP_UNARY},
	{TT_DOT_OP, 98, OP_ASSOC_LEFT, OP_BINARY},
	{TT_ARROW_OP, 98, OP_ASSOC_LEFT, OP_BINARY},
	/* Compound literal */
	/* typeid() */
	/* const_cast */
	/* dynamic_cats */
	/* reinterpret_cast */
	/* static_cast */

	{TT_PLUSPLUS_OP, 96, OP_ASSOC_RIGHT, OP_UNARY},
	{TT_MINUSMINUS_OP, 96, OP_ASSOC_RIGHT, OP_UNARY},
	{TT_PLUS_OP, 96, OP_ASSOC_RIGHT, OP_UNARY},
	{TT_MINUS_OP, 96, OP_ASSOC_RIGHT, OP_UNARY},
	{TT_NEGATION_OP, 96, OP_ASSOC_RIGHT, OP_UNARY},
	{TT_COMPLEMENT_OP, 96, OP_ASSOC_RIGHT, OP_UNARY},
	/* Type cast */
	{TT_STAR_OP, 96, OP_ASSOC_RIGHT, OP_UNARY},
	{TT_BIT_AND_OP, 96, OP_ASSOC_RIGHT, OP_UNARY},
	/* sizeof */
	/* _Alignof */
	/* new, new[] */
	/* delete, delete[] */

	/* .* */
	/* ->* */

	{TT_STAR_OP, 92, OP_ASSOC_LEFT, OP_BINARY},
	{TT_SLASH_OP, 92, OP_ASSOC_LEFT, OP_BINARY},
	{TT_PERCENT_OP, 92, OP_ASSOC_LEFT, OP_BINARY},

	{TT_PLUS_OP, 90, OP_ASSOC_LEFT, OP_BINARY},
	{TT_MINUS_OP, 90, OP_ASSOC_LEFT, OP_BINARY},

	{TT_LEFTSHIFT_OP, 88, OP_ASSOC_LEFT, OP_BINARY},
	{TT_RIGHTSHIFT_OP, 88, OP_ASSOC_LEFT, OP_BINARY},

	{TT_LESSTHAN_OP, 87, OP_ASSOC_LEFT, OP_BINARY},
	{TT_LESSTHAN_EQUAL_OP, 87, OP_ASSOC_LEFT, OP_BINARY},
	{TT_GREATERTHAN_OP, 87, OP_ASSOC_LEFT, OP_BINARY},
	{TT_GREATERTHAN_EQUAL_OP, 87, OP_ASSOC_LEFT, OP_BINARY},

	{TT_EQUAL_OP, 86, OP_ASSOC_LEFT, OP_BINARY},
	{TT_NOT_EQUAL_OP, 86, OP_ASSOC_LEFT, OP_BINARY},

	{TT_BIT_AND_OP, 84, OP_ASSOC_LEFT, OP_BINARY},

	{TT_BIT_XOR_OP, 82, OP_ASSOC_LEFT, OP_BINARY},

	{TT_BIT_OR_OP, 80, OP_ASSOC_LEFT, OP_BINARY},

	{TT_AND_OP, 78, OP_ASSOC_LEFT, OP_BINARY},

	{TT_OR_OP, 76, OP_ASSOC_LEFT, OP_BINARY},

	{TT_QUESTION_OP, 74, OP_ASSOC_RIGHT, OP_BINARY},
	{TT_COLON_OP, 74, OP_ASSOC_RIGHT, OP_BINARY},

	{TT_ASSIGNMENT_OP, 72, OP_ASSOC_RIGHT, OP_BINARY},
	{TT_PLUS_ASSIGNMENT_OP, 72, OP_ASSOC_RIGHT, OP_BINARY},
	{TT_MINUS_ASSIGNMENT_OP, 72, OP_ASSOC_RIGHT, OP_BINARY},
	{TT_STAR_ASSIGNMENT_OP, 72, OP_ASSOC_RIGHT, OP_BINARY},
	{TT_SLASH_ASSIGNMENT_OP, 72, OP_ASSOC_RIGHT, OP_BINARY},
	{TT_PERCENT_ASSIGNMENT_OP, 72, OP_ASSOC_RIGHT, OP_BINARY},
	{TT_LEFTSHIFT_ASSIGNMENT_OP, 72, OP_ASSOC_RIGHT, OP_BINARY},
	{TT_RIGHTSHIFT_ASSIGNMENT_OP, 72, OP_ASSOC_RIGHT, OP_BINARY},
	{TT_BIT_AND_ASSIGNMENT_OP, 72, OP_ASSOC_RIGHT, OP_BINARY},
	{TT_BIT_XOR_ASSIGNMENT_OP, 72, OP_ASSOC_RIGHT, OP_BINARY},
	{TT_BIT_OR_ASSIGNMENT_OP, 72, OP_ASSOC_RIGHT, OP_BINARY},

	/* throw */

	{TT_COMMA_OP, 72, OP_ASSOC_RIGHT, OP_BINARY}
};

static struct op_s *get_op(struct token_s *token)
{
	size_t ix;

	for (ix=0; ix<sizeof op_table/sizeof(struct op_s); ++ix) {
		if (op_table[ix].op==token->type) return &op_table[ix];
	}

	return NULL;
}

struct token_arities_s {
	int type;
	int left_unary;
	int right_unary;
	int binary;
};
#if 0
static struct token_arities_s token_arities[]={
	{TT_NULL, 0, 0, 0},
	{TT_END, 0, 0, 0},
	{TT_ERROR, 0, 0, 0},
	{TT_WHITESPACE,  0, 0, 0},
	{TT_PREPROCESSOR,  0, 0, 0},
	{TT_TEXTUAL, 0, 0, 0},
	{TT_RETURN, 0, 0, 0},
	{TT_BREAK, 0, 0, 0},
	{TT_CONTINUE, 0, 0, 0},
	{TT_GOTO, 0, 0, 0},
	{TT_IF, 0, 0, 0},
	{TT_ELSE, 0, 0, 0},
	{TT_WHILE, 0, 0, 0},
	{TT_DO, 0, 0, 0},
	{TT_FOR, 0, 0, 0},
	{TT_SWITCH, 0, 0, 0},
	{TT_CASE, 0, 0, 0},
	{TT_DEFAULT, 0, 0, 0},
	{TT_NUMBER,  0, 0, 0},
	{TT_STRING,  0, 0, 0},
	{TT_CHARACTER,  0, 0, 0},
	{TT_OPERATOR, 0, 0, 0},
	{TT_PLUS_OP, 1, 0, 1},
	{TT_PLUS_ASSIGNMENT_OP, 0, 0, 1},
	{TT_PLUSPLUS_OP, 1, 1, 0},
	{TT_MINUS_OP, 1, 0, 1},
	{TT_MINUS_ASSIGNMENT_OP, 0, 0, 1},
	{TT_ARROW_OP, 0, 0, 1},
	{TT_MINUSMINUS_OP, 1, 1, 0},
	{TT_STAR_OP, 1, 0, 1},
	{TT_STAR_ASSIGNMENT_OP, 0, 0, 1},
	{TT_SLASH_OP, 0, 0, 1},
	{TT_SLASH_ASSIGNMENT_OP, 0, 0, 1},
	{TT_PERCENT_OP, 0, 0, 1},
	{TT_PERCENT_ASSIGNMENT_OP, 0, 0, 1},
	{TT_ASSIGNMENT_OP, 0, 0, 1},
	{TT_EQUAL_OP, 0, 0, 1},
	{TT_NEGATION_OP, 1, 0, 0},
	{TT_COMPLEMENT_OP, 1, 0, 0},
	{TT_COMPLEMENT_ASSIGN_OP, 1, 0, 0},
	{TT_NOT_EQUAL_OP, 0, 0, 1},
	{TT_LESSTHAN_OP, 0, 0, 1},
	{TT_LESSTHAN_EQUAL_OP, 0, 0, 1},
	{TT_GREATERTHAN_OP, 0, 0, 1},
	{TT_GREATERTHAN_EQUAL_OP, 0, 0, 1},
	{TT_LEFTSHIFT_OP, 0, 0, 1},
	{TT_LEFTSHIFT_ASSIGNMENT_OP, 0, 0, 1},
	{TT_RIGHTSHIFT_OP, 0, 0, 1},
	{TT_RIGHTSHIFT_ASSIGNMENT_OP, 0, 0, 1},
	{TT_SEMICOLON_OP, 0, 0, 0},
	{TT_COMMA_OP, 0, 0, 1},
	{TT_DOT_OP, 0, 0, 1},
	{TT_AND_OP, 0, 0, 1},
	{TT_BIT_AND_OP, 1, 0, 1},
	{TT_BIT_AND_ASSIGNMENT_OP, 0, 0, 1},
	{TT_BIT_XOR_OP, 0, 0, 1},
	{TT_BIT_XOR_ASSIGNMENT_OP, 0, 0, 1},
	{TT_BIT_OR_OP, 0, 0, 1},
	{TT_BIT_OR_ASSIGNMENT_OP, 0, 0, 1},
	{TT_OR_OP, 0, 0, 1},
	{TT_LEFT_PARANTHESIS, 0, 0, 0},
	{TT_RIGHT_PARANTHESIS, 0, 0, 0},
	{TT_LEFT_SQUARE, 0, 0, 0},
	{TT_RIGHT_SQUARE, 0, 0, 0},
	{TT_LEFT_CURLY, 0, 0, 0},
	{TT_RIGHT_CURLY, 0, 0, 0},
	{TT_QUESTION_OP, 0, 1, 0},
	{TT_COLON_OP, 0, 0, 1},
	{TT_STRUCT, 0, 0, 0},
	{TT_UNION, 0, 0, 0},
};
#endif
#if 0
static int get_arity(size_t /*@unused@*/ix, struct token_s /*@unused@*/**tokens)
{
	return 0;
}
#endif

static size_t parse_expr(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	struct node_s *node;
	struct op_s *op;
	struct token_s *tmpop_token;
	STACK *op_stack, *match_stack, *rpn_stack;

	op_stack=stackalloc(32);		/* Operator stack */
	match_stack=stackalloc(32);		/* Stack tokens needing a matching token */
	rpn_stack=stackalloc(1024);		/* Parsed RPN stream */

	node=create_node(parent, NT_EXPR);
	node->token=tokens[ix];

	while (tokens[ix]->type!=TT_NULL) {
		log_node_token(node, tokens[ix], ">>>\n");
		if (isleft(tokens[ix])) {
			log_node_token(node, tokens[ix], ">>> left\n");
			stackpush(op_stack, tokens[ix]);
			stackpush(match_stack, tokens[ix]);
		} else if (isright(tokens[ix])) {
			log_node_token(node, tokens[ix], ">>> right\n");
			if (stacksize(match_stack)>0 
				&& ismatchingright(stacktop(match_stack), tokens[ix])) {

				stackpop(match_stack);
			} else break;

			while (stacksize(op_stack)>0 
				&& !ismatchingright(tokens[ix], stacktop(op_stack))) {
				log_node_token(node, tokens[ix], ">>> s\n");
				if (isleft(stacktop(op_stack))) {
					return error_node_token(node, tokens[ix], "Unmatched\n"), (size_t)0;
				}
				stackpush(rpn_stack, stackpop(op_stack));
			}
			if (stacksize(op_stack)<1 || !ismatchingright(tokens[ix], stacktop(op_stack)))
				break;

		} else if (isop(tokens[ix])) {
			if (tokens[ix]->type==TT_QUESTION_OP) stackpush(match_stack, tokens[ix]);
			else if (tokens[ix]->type==TT_COLON_OP) {
				if (stacksize(match_stack)>0 
					&& ismatchingright(stacktop(match_stack), tokens[ix])) {

					stackpop(match_stack);
				} else {
					if (stacksize(match_stack)==0) break;
					else return error_node_token(node, tokens[ix], "parse_expr(): Unexpected token\n"), (size_t)0;
				}
			}

			op=get_op(tokens[ix]);

			if (op->assoc==OP_ASSOC_RIGHT) {
				log_node_token(node, tokens[ix], ">>> right-assoc\n");
				while (stacksize(op_stack)>0 
					&& op->prec<get_op((tmpop_token=stackpop(op_stack)))->prec) {
					stackpush(rpn_stack, tmpop_token);
				}
			} else {
				log_node_token(node, tokens[ix], ">>> left-assoc\n");
				while (stacksize(op_stack)>0 
					&& op->prec<=get_op((tmpop_token=stackpop(op_stack)))->prec) {
					stackpush(rpn_stack, tmpop_token);
				}
			}

			stackpush(op_stack, tokens[ix]);
		} else if (isvalue(tokens[ix]) || isname(tokens[ix])) {
			stackpush(rpn_stack, tokens[ix]);
		} else {
			break;
		}

		++ix;
	}

	while (stacksize(op_stack)>0) {
		stackpush(rpn_stack, stackpop(op_stack));
	}

	stackfree(rpn_stack);
	stackfree(match_stack);
	stackfree(op_stack);

	return ix;
}

#if 0
static size_t parse_x(struct node_s /*@unused@*/*parent, struct token_s /*@unused@*/**tokens)
{
//	size_t ix=0;
//	size_t parsed;
//	struct node_s *node;

	//node=create_node(parent, NT_X);
	//node->token=tokens[ix];

	//if (ix) return add_node(node), ix;

	return /*free_node(node),*/ (size_t)0;
}
#endif

static size_t parse_return(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_RETURN);
	node->token=tokens[ix];

	if (tokens[ix]->type==TT_RETURN) {
		++ix;
		if ((parsed=parse_expr(node, tokens+ix))) ix+=parsed;
		else return free_node(node), (size_t)0;
		if (tokens[ix]->type==TT_SEMICOLON_OP) return add_node(node), ++ix;
		else return free_node(node), (size_t)0;
	} else return free_node(node), (size_t)0;
}

static size_t parse_while(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_WHILE);
	node->token=tokens[ix];

	if (tokens[ix]->type==TT_WHILE) {
		++ix;
		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
			++ix;
			if ((parsed=parse_expr(node, tokens+ix))) ix+=parsed;
			else return free_node(node), (size_t)0;
			if (!(tokens[ix]->type==TT_RIGHT_PARANTHESIS)) return free_node(node), (size_t)0;
			++ix;
			if ((parsed=parse_stmt(node, tokens+ix))) return add_node(node), ix+=parsed;
			else if ((parsed=parse_block(node, tokens+ix))) return add_node(node), ix+=parsed;
			else return free_node(node), (size_t)0;
		} else return free_node(node), (size_t)0;
	} else return free_node(node), (size_t)0;
}

static size_t parse_do(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_DO);
	node->token=tokens[ix];

	if (tokens[ix]->type==TT_DO) {
		++ix;
		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
			++ix;
			if ((parsed=parse_stmt(node, tokens+ix))) ix+=parsed;
			if (tokens[ix]->type==TT_WHILE) {
				if ((parsed=parse_expr(node, tokens+ix))) add_node(node), ix+=parsed;
				else return free_node(node), (size_t)0;
			} else free_node(node), (size_t)0;
		} else return free_node(node), (size_t)0;
	} else return free_node(node), (size_t)0;

	return free_node(node), (size_t)0;
}

static size_t parse_for(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_FOR);
	node->token=tokens[ix];

	if (tokens[ix]->type==TT_FOR) {
		++ix;
		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
			++ix;
			if ((parsed=parse_expr(node, tokens+ix))) ix+=parsed;
			else return free_node(node), (size_t)0;
			if (tokens[ix]->type==TT_SEMICOLON_OP) ++ix;
			else return free_node(node), (size_t)0;
			if ((parsed=parse_expr(node, tokens+ix))) ix+=parsed;
			else return free_node(node), (size_t)0;
			if (tokens[ix]->type==TT_SEMICOLON_OP) ++ix;
			else return free_node(node), (size_t)0;
			if ((parsed=parse_expr(node, tokens+ix))) ix+=parsed;
			else return free_node(node), (size_t)0;

			if (!(tokens[ix]->type==TT_RIGHT_PARANTHESIS)) return free_node(node), (size_t)0;
			if ((parsed=parse_stmt(node, tokens+ix))) return add_node(node), ix+=parsed;
			else return free_node(node), (size_t)0;
		} else return free_node(node), (size_t)0;
	} else return free_node(node), (size_t)0;
}

static size_t parse_if(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_IF);
	node->token=tokens[ix];

	if (tokens[ix]->type==TT_IF) {
		++ix;
		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
			++ix;
			if ((parsed=parse_expr(node, tokens+ix))) ix+=parsed;
			else return free_node(node), (size_t)0;
			if (!(tokens[ix]->type==TT_RIGHT_PARANTHESIS)) return free_node(node), (size_t)0;
			if ((parsed=parse_stmt(node, tokens+ix))) return add_node(node), ix+=parsed;
			else return free_node(node), (size_t)0;
		} else return free_node(node), (size_t)0;
	} else return free_node(node), (size_t)0;
}


static size_t parse_switch(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_SWITCH);
	node->token=tokens[ix];

	if (tokens[ix]->type==TT_SWITCH) {
		++ix;
		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
			++ix;
			if ((parsed=parse_expr(node, tokens+ix))) ix+=parsed;
			else return error_node(node, "parse_switch(): No condition\n"), (size_t)0;
			if (!(tokens[ix]->type==TT_RIGHT_PARANTHESIS)) return free_node(node), (size_t)0;
			++ix;
			if ((parsed=parse_block(node, tokens+ix))) return add_node(node), ix+=parsed;
			else return error_node(node, "parse_switch(): No switch block\n"), (size_t)0;
		} else return free_node(node), (size_t)0;
	} else return free_node(node), (size_t)0;
}

static size_t parse_goto(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	struct node_s *node;

	node=create_node(parent, NT_GOTO);
	node->token=tokens[ix];

	if (tokens[ix]->type==TT_GOTO) {
		++ix;
		if (!isname(tokens[ix])) 
			return error_node(node, "parse_goto(): No label specified\n"), (size_t)0;
		node->token=tokens[ix++];
		if (tokens[ix]->type!=TT_SEMICOLON_OP)
			return error_node(node, "parse_goto(): No terminating semicolon\n"), (size_t)0;
		return add_node(node), ++ix;
	} else return free_node(node), (size_t)0;
}

static size_t parse_break(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	struct node_s *node;

	node=create_node(parent, NT_BREAK);
	node->token=tokens[ix];

	if (tokens[ix]->type==TT_BREAK) {
		++ix;
		if (tokens[ix]->type!=TT_SEMICOLON_OP)
			return error_node(node, "parse_break(): No terminating semicolon\n"), (size_t)0;
		return add_node(node), ++ix;
	} else return free_node(node), (size_t)0;
}

static size_t parse_continue(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	struct node_s *node;

	node=create_node(parent, NT_CONTINUE);
	node->token=tokens[ix];

	if (tokens[ix]->type==TT_CONTINUE) {
		++ix;
		if (tokens[ix]->type!=TT_SEMICOLON_OP)
			return error_node(node, "parse_continue(): No terminating semicolon\n"), (size_t)0;
		return add_node(node), ++ix;
	} else return free_node(node), (size_t)0;
}

static size_t parse_stmt(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_STMT);

	node->token=tokens[0];

	if ((parsed=parse_return(node, tokens))
		|| (parsed=parse_declaration(node, tokens))
		|| (parsed=parse_typedef(node, tokens))
		|| (parsed=parse_while(node, tokens))
		|| (parsed=parse_do(node, tokens))
		|| (parsed=parse_for(node, tokens))
		|| (parsed=parse_if(node, tokens))
		|| (parsed=parse_switch(node, tokens))
		|| (parsed=parse_goto(node, tokens))
		|| (parsed=parse_break(node, tokens))
		|| (parsed=parse_continue(node, tokens))
		|| ((parsed=parse_expr(node, tokens)) && tokens[parsed]->type==TT_SEMICOLON_OP && ++parsed)) {

		return add_node(node), parsed;
	} else return free_node(node), (size_t)0;
}

static size_t parse_label(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_LABEL);

	node->token=tokens[ix];

	if ((parsed=parse_name(node, tokens))) {
		ix+=parsed;
		if (tokens[ix]->type==TT_COLON_OP) return add_node(node), ++ix;
	}

	return free_node(node), (size_t)0;
}

static size_t parse_case(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_CASE);

	node->token=tokens[ix];

	if (tokens[ix]->type==TT_CASE) {
		++ix;
		if (!(parsed=parse_expr(node, tokens+ix))) 
			return error_node(node, "parse_case(): No case expression\n"), (size_t)0;
		ix+=parsed;
		if (tokens[ix]->type==TT_COLON_OP) return add_node(node), ++ix;
		else error_node(node, "parse_case(): No terminating colon\n");
	} else return free_node(node), (size_t)0;

	return free_node(node), (size_t)0;
}

static size_t parse_default(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	struct node_s *node;

	node=create_node(parent, NT_DEFAULT);

	node->token=tokens[ix];

	if (tokens[ix]->type==TT_DEFAULT) {
		++ix;
		if (tokens[ix]->type==TT_COLON_OP) return add_node(node), ++ix;
		else error_node(node, "parse_default(): No terminating colon\n");
	} else return free_node(node), (size_t)0;

	return free_node(node), (size_t)0;
}

static size_t parse_block(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_BLOCK);
	node->token=tokens[ix];

	if (tokens[ix]->type==TT_LEFT_CURLY) {
		log_node(node, "TT_LEFT_CURLY\n");
		++ix;

		log_node(node, "Left curly ok\n");

		while((parsed=parse_stmt(node, tokens+ix))
			|| (parsed=parse_block(node, tokens+ix))
			|| (parsed=parse_case(node, tokens+ix))
			|| (parsed=parse_default(node, tokens+ix))
			|| (parsed=parse_label(node, tokens+ix))) ix+=parsed;

	} else return free_node(node), (size_t)0;

	if (tokens[ix]->type==TT_RIGHT_CURLY) ++ix;
	else return free_node(node), (size_t)0;

	return add_node(node), ix;
}

#if 0
static size_t parse_struct_union(struct node_s *parent, struct token_s **tokens)
{
	// TODO: Move this to declaration parsing

	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_STRUCT);

	if (tokens[ix]->type==TT_STRUCT) {
		log_node(node, "parse_struct_union() - 0.1\n");	
		++ix;
	} else if (tokens[ix]->type==TT_UNION) {
		log_node(node, "parse_struct_union() - 0.2\n");	
		node->type=NT_UNION;
		++ix;	
	}

	log_node(node, "parse_struct_union() - 1\n");
	if (ix) {
		log_node(node, "parse_struct_union() - 1.1\n");
		if ((parsed=parse_name(node, tokens+ix))) {
			log_node(node, "parse_struct_union() - 1.1.1\n");
			ix+=parsed;

		}
	}
	log_node(node, "parse_struct_union() - 2\n");

	if (tokens[ix]->type==TT_LEFT_CURLY) {
		++ix;
	} else return error_node(node, "parse_struct_union(): Missing struct block start"), (size_t)0;

	// TODO: Parse declarations

	if (tokens[ix]->type==TT_RIGHT_CURLY) {
		++ix;
	} else return error_node(node, "parse_struct_union(): Missing struct block end"), (size_t)0;

	return free_node(node), (size_t)0;
}
#endif

static size_t parse_funcdef(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_FUNCDEF);

	if ((parsed=parse_typespec(node, tokens+ix))) ix+=parsed;

	/* Argument list is ihncluded in parse_declarator */
	if ((parsed=parse_declarator(node, tokens+ix))) ix+=parsed;
	else return free_node(node), (size_t)0;

	/* Old-style c argument type declarations */
	while ((parsed=parse_declarator(node, tokens+ix))) ix+=parsed;

	if ((parsed=parse_block(node, tokens+ix))) ix+=parsed;
	else return free_node(node), (size_t)0;

	(void)add_node(node);
	
	return ix;
}

size_t parse(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0;
	size_t parsed;
	struct node_s *node;

	node=create_node(parent, NT_UNIT);

	while (tokens[ix]->type!=TT_END) {
		log_node(node, "parse() - Start of loop\n");
		if ((parsed=parse_funcdef(node, tokens+ix))
			|| (parsed=parse_declaration(node, tokens+ix))
			|| (parsed=parse_typedef(node, tokens+ix))) ix+=parsed;
		log_node(node, "parse() - End of loop\n");
	}

	(void)add_node(node);

	return ix;
}


