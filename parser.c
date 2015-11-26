
#include "parser.h"
#include "tokenclass.h"
#include "stack.h"
#include "json.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static char *node_type_names[]={
	"TRANSLATION_UNIT",
	"EXTERNAL_DECLARATION",
	"FUNCTION_DEFINITION",
	"COMPOUND_STATEMENT",
	"BLOCK_ITEM_LIST",
/*	"BLOCK_ITEM",*/
	"STATEMENT",
	"JUMP_STATEMENT",
	"ITERATION_STATEMENT",
	"SELECTION_STATEMENT",
	"MULTIPLICATIVE_EXPRESSION",
	"ADDITIVE_EXPRESSION",
	"SHIFT_EXPRESSION",
	"RELATIONAL_EXPRESSION",
	"EQUALITY_EXPRESSION",
	"AND_EXPRESSION",
	"EXCLUSIVE_OR_EXPRESSION",
	"INCLUSIVE_OR_EXPRESSION",
	"LOGICAL_AND_EXPRESSION",
	"LOGICAL_OR_EXPRESSION",
	"CONDITIONAL_EXPRESSION",
	"CONSTANT",
	"STRING_LITERAL",
	"PRIMARY_EXPRESSION",
	"TYPE_NAME",
	"DESIGNATOR",
	"DESIGNATION",
	"DESIGNATION_INITIALIZER",
	"INITIALIZER_LIST",
	"POSTFIX_EXPRESSION",
	"UNARY_OPERATOR",
	"CAST_EXPRESSION",
	"UNARY_EXPRESSION",
	"ASSIGNMENT_EXPRESSION",
	"DIRECT_ABSTRACT_DECLARATOR",
	"ABSTRACT_DECLARATOR",
	"PARAMETER_DECLARATION",
	"PARAMETER_LIST",
	"PARAMETER_TYPE_LIST",
	"STORAGE_CLASS_SPECIFIER",
	"STRUCT_OR_UNION",
	"SPECIFIER_QUALIFIER_LIST",
	"STRUCT_DECLARATOR",
	"STRUCT_DECLARATOR_LIST",
	"STRUCT_DECLARATION",
	"STRUCT_DECLARATION_LIST",
	"STRUCT_OR_UNION_SPECIFIER",
	"ENUMERATOR",
	"ENUMERATOR_LIST",
	"ENUM_SPECIFIER",
	"TYPEDEF_NAME",
	"TYPE_SPECIFIER",
	"TYPE_QUALIFIER",
	"TYPE_QUALIFIER_LIST",
	"FUNCTION_SPECIFIER",
	"DECLARATION_SPECIFIERS",
	"POINTER",
	"DIRECT_DECLARATOR",
	"DECLARATOR",
	"IDENTIFIER",
	"IDENTIFIER_LIST",
	"INITIALIZER",
	"EXPRESSION",
	"EXPRESSION_STATEMENT",
	"INIT_DECLARATOR",
	"INIT_DECLARATOR_LIST",
	"DECLARATION",
	"DECLARATION_LIST",
	"LABELED_STATEMENT",

	"NT_ANY",

	"NT_ROOT",
	"NT_UNIT",
	"NT_DUMMY"
};

struct node_s *g_root_node=NULL;

static void print_token_details(FILE *fp, struct token_s *token)
{
	fprintf(fp, "%3d/%2d [%-8s] %-25s", token_lno(token), token_cno(token),
		token_text(token), token_type(token));
}

static void print_node_type(FILE *fp, struct node_s *node)
{
	fprintf(stderr, "{%-26s}", node_type_names[node->type]); 
}

static void log_node_token(struct node_s *node, struct token_s *token, const char *fmt, ...)
{
	va_list arg;
	int level=node->level;

	print_token_details(stderr, token);

	while(level--) (void)putc(' ', stderr);

	print_node_type(stderr, node);

	va_start(arg, fmt);
	(void)vfprintf(stderr, fmt, arg);
	va_end(arg);
}

static void log_node(struct node_s *node, const char *fmt, ...)
{
	va_list arg;
	int level=node->level;

	print_token_details(stderr, node->token);

	while(level--) (void)putc(' ', stderr);

	print_node_type(stderr, node);

	va_start(arg, fmt);
	(void)vfprintf(stderr, fmt, arg);
	va_end(arg);
}

struct node_s *create_node(struct node_s *parent, int type, struct token_s *token)
{
	struct node_s *node=(struct node_s *)calloc(1, sizeof *node);
	if (!node) return NULL;

	node->parent=parent;
	node->scope_parent=parent->scope_parent;
	if (parent->type==COMPOUND_STATEMENT || parent->type==TRANSLATION_UNIT)
		node->scope_parent=parent;
	node->level=node->parent->level+1;
	node->type=type;
	node->in_typedef_declaration=node->parent->in_typedef_declaration;
	node->token=token;

	log_node_token(node, token, ". create_node() Trying on %s\n",
		node_type_names[node->parent->type]);

	return node;
}

size_t free_node(struct node_s *node)
{
	size_t ix;

	log_node(node, "- free_node(): Freeing from %s\n", 
		node_type_names[node->parent->type]);

	for (ix=0; ix<node->nsubnodes; ++ix) free(node->subnodes[ix]);

	free(node);

	return (size_t)0;
}

static size_t error_node(struct node_s *node, const char *fmt, ...)
{
	va_list arg;

	fprintf(stderr, "ERROR (%s): ", node_type_names[node->type]);
	va_start(arg, fmt);
	(void)vfprintf(stderr, fmt, arg);
	va_end(arg);

	exit(EXIT_FAILURE);

	return free_node(node);
}

static size_t error_node_token(struct node_s *node, struct token_s *token, const char *fmt, ...)
{
	va_list arg;

	fprintf(stderr, "ERROR (%s %d/%d %s %s): ", 
		node_type_names[node->type], 
		token_lno(token), token_cno(token),
		token_type(token), token_text(token));
	va_start(arg, fmt);
	(void)vfprintf(stderr, fmt, arg);
	va_end(arg);

	exit(EXIT_FAILURE);

	return free_node(node);
}

#if 0
static struct node_s *last_subnode(struct node_s *node)
{
	return node->nsubnodes>0 ? node->subnodes[node->nsubnodes-1] : NULL;
}
#endif

static int add_node(struct node_s *node)
{
	log_node(node, "+ add_node(): Adding to %s\n", 
		node_type_names[node->parent->type]);

	node->parent->subnodes=(struct node_s **)realloc(
		node->parent->subnodes, ++node->parent->nsubnodes * sizeof node);
	node->parent->subnodes[node->parent->nsubnodes-1]=node;

	return 0;
}

static int add_subnode(struct node_s *node)
{
	struct node_s *subnode;

	if (!(subnode=node->nsubnodes>0 ? node->subnodes[0] : NULL)) {
		fprintf(stderr, "ERROR: No subnode\n");
	}

	log_node(node, "+ add_subnode(): Adding subnode of %s to %s - freeing current node\n", 
		node_type_names[node->type],
		node_type_names[node->parent->type]);

	subnode->parent=node->parent;

	free(node);

	return add_node(subnode);


}

static int add_typealias(struct node_s *node)
{
	const char *text=token_text(node->token);

	log_node(node, "@ add_typealias(): Adding to %s - %s\n", 
		node_type_names[node->scope_parent->type], text);

	node->scope_parent->typealiases=(const char **)realloc(
		node->scope_parent->typealiases, ++node->scope_parent->ntypealiases * sizeof node);
	node->scope_parent->typealiases[node->scope_parent->ntypealiases-1]=text;

	return 0;
}

static int istypealias(const struct node_s *node, const struct token_s *token)
{
	size_t ix;

	if (node->scope_parent) {
		for (ix=0; ix<node->scope_parent->ntypealiases; ++ix) {
			if (!strcmp(token_text(token), node->scope_parent->typealiases[ix])) return 1;
		}
		return istypealias(node->scope_parent, token);
	}

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

void print_node_json(struct node_s *node, int ind)
{
	size_t ix;
	static char buf[1024*1024];

	indent(ind);
	fputs("[{\"type\": ", stdout);
	fputs(json_str(node_type_names[node->type], buf, 1024*1024), stdout);
	fputs(", \"token\": {\"type\": ", stdout);
	fputs(json_str(token_type(node->token), buf, 1024*1024), stdout);
	fputs(", \"text\": ", stdout);
	fputs(json_str(token_text(node->token), buf, 1024*1024), stdout);
	fputs(", \"fname\": ", stdout);
	fputs(json_str(token_fname(node->token), buf, 1024*1024), stdout);
	printf(", \"fpos\": \"%d,%d\"}}", token_lno(node->token), token_cno(node->token));

	for (ix=0; ix<node->nsubnodes; ++ix) {
		puts(","); 
		print_node_json(node->subnodes[ix], ind+1);
	}

	(void)putchar(']');
}

#if 0
typedef int (*predicate_t)(const struct token_s *);
/* Variable arguments should be pairs of type predicate_t and int, terminated by NULL
   The predicate is the test function. The int indicates if this token identifies the node (non-zero) */
static size_t generic_parse_tokensequence(struct node_s *parent, struct token_s **tokens,
	int node_type, ...)
{
	size_t ix=0;
	struct node_s *node=create_node(parent, node_type, NULL);
	predicate_t pred;
	int id;
	va_list argp;

	va_start(argp, node_type);

	while ((pred=va_arg(argp, predicate_t))) {
		if (pred(tokens[ix])) {
			if ((id=va_arg(argp, int))) node->token=tokens[ix];
			++ix;
		} else break;
	}

	va_end(argp);

	if (ix>0) return add_node(node), ix;
	else return free_node(node);
}
#endif

/* New approach <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

/* Helper parsers: */

static size_t separated(struct node_s *parent, struct token_s **tokens, int nt,
	size_t (*pf)(struct node_s *, struct token_s **),
	int tt_separator)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, nt, tokens[0]);

	fprintf(stderr, "separated() - 1 [%s]\n", token_text(tokens[ix]));

	if ((parsed=pf(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	fprintf(stderr, "separated() - 2 - ix=%d, [%s]\n", (int)ix, token_text(tokens[ix]));
	while (tokens[ix]->type==tt_separator) {
		fprintf(stderr, "separated() - 2a - ix=%d [%s]\n", (int)ix, token_text(tokens[ix]));
		++ix;

		if ((parsed=pf(node, tokens+ix))) ix+=parsed;
		else error_node(node, "[separated()] Unexpexted parse");
		fprintf(stderr, "separated() - 2b - ix=%d [%s]\n", (int)ix, token_text(tokens[ix]));
	}

	fprintf(stderr, "separated() - 3 - ix=%d [%s]\n", (int)ix, token_text(tokens[ix]));
	return add_node(node), ix;
}

static int is_any_of(int what, size_t num_alts, int *alts)
{
	size_t i;

	for (i=0u; i<num_alts; ++i) {
		if (what==alts[i]) return 1;
	}

	return 0;
}

static size_t separated_any_token(struct node_s *parent, struct token_s **tokens, int nt,
	size_t (*pf)(struct node_s *, struct token_s **),
	size_t num_separators, int *tt_separators)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, nt, tokens[0]);

	if ((parsed=pf(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	while (is_any_of(tokens[ix]->type, num_separators, tt_separators)) {
		++ix;

		if ((parsed=pf(node, tokens+ix))) ix+=parsed;
		else error_node(node, "[separated_any_token()] Unexpexted parse");
	}

	return add_node(node), ix;
}

static size_t many(struct node_s *parent, struct token_s **tokens, int nt,
	size_t (*pf)(struct node_s *, struct token_s **))
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, nt, tokens[0]);

	while ((parsed=pf(node, tokens+ix))) ix+=parsed;

	if (ix>0u) return add_node(node), ix;
	else return free_node(node);
}

static size_t any_of_2(struct node_s *parent, struct token_s **tokens, int nt,
	size_t (*pf1)(struct node_s *, struct token_s **),
	size_t (*pf2)(struct node_s *, struct token_s **))
{
	size_t parsed;
	struct node_s *node=create_node(parent, nt, tokens[0]);

	if ((parsed=pf1(node, tokens)) || (parsed=pf2(node, tokens))) return add_node(node), parsed;
	else return free_node(node);
}

static size_t postfix_token(struct node_s *parent, struct token_s **tokens, int nt,
	size_t (*pf)(struct node_s *, struct token_s **), int tt)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, nt, tokens[0]);

	fprintf(stderr, "postfix_token() - 1\n");

	if ((parsed=pf(node, tokens))) ix+=parsed;
	else return free_node(node);

	fprintf(stderr, "postfix_token() - 2 - ix=%d\n", (int)ix);
	if (tokens[ix]->type==tt) ++ix;
	else return free_node(node);

	fprintf(stderr, "postfix_token() - 3\n");
	return add_node(node), ix;
}

static size_t single_token(struct node_s *parent, struct token_s **tokens, int nt, int tt)
{
	struct node_s *node=create_node(parent, nt, tokens[0]);

	return tokens[0]->type==tt ? add_node(node),1u : free_node(node);
}

static size_t single_token_any_of(struct node_s *parent, struct token_s **tokens, int nt, size_t ntts, int *tts)
{
	struct node_s *node=create_node(parent, nt, tokens[0]);

	if (is_any_of(tokens[0]->type, ntts, tts)) return add_node(node), 1u;
	else return free_node(node);
}

static size_t prepostfix_tokens(struct node_s *parent, struct token_s **tokens, int nt,
	size_t (*pf)(struct node_s *, struct token_s **), int pre_tt, int post_tt)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, nt, tokens[ix]);

	if (tokens[ix]->type==pre_tt) ++ix;
	else return free_node(node);

	if ((parsed=pf(node, tokens))) ix+=parsed;
	else return free_node(node);

	if (tokens[ix]->type==post_tt) ++ix;
	else return free_node(node);

	return add_node(node), ix;
}

/* Expression parser: */

enum {ASSOC_NONE=0, ASSOC_LEFT, ASSOC_RIGHT};
enum {ARITY_NONE=0, ARITY_LEFT_UNARY, ARITY_RIGHT_UNARY, ARITY_BINARY, ARITY_TRINARY, ARITY_GROUPING, ARITY_GROUPING_END};
struct op_s {
	int tt, sec_tt;		/* sec_tt is token type for second operator in trinary operator */
	int prec;		/* High number means higher precedence */
	int assoc;
	int arity;
} ops[]={
	{.tt=TT_PLUSPLUS_OP, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_RIGHT_UNARY},
	{.tt=TT_MINUSMINUS_OP, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_RIGHT_UNARY},
	{.tt=TT_LEFT_PARANTHESIS, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_GROUPING},	/* Function call */
	{.tt=TT_LEFT_SQUARE, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_GROUPING},	/* Array subscripting */
	{.tt=TT_RIGHT_PARANTHESIS, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_GROUPING_END},	/* Function call */
	{.tt=TT_RIGHT_SQUARE, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_GROUPING_END},	/* Array subscripting */
	{.tt=TT_DOT_OP, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_ARROW_OP, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_PLUSPLUS_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_MINUSMINUS_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_PLUS_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_MINUS_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_NEGATION_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_COMPLEMENT_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	/* Type cast (how to handle this) */
	{.tt=TT_STAR_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_BIT_AND_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_SIZEOF, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},

	{.tt=TT_STAR_OP, .prec=90, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_SLASH_OP, .prec=90, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_PERCENT_OP, .prec=90, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_PLUS_OP, .prec=85, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_MINUS_OP, .prec=85, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_LEFTSHIFT_OP, .prec=80, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_RIGHTSHIFT_OP, .prec=80, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_LESSTHAN_OP, .prec=75, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_LESSTHAN_EQUAL_OP, .prec=75, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_GREATERTHAN_OP, .prec=75, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_GREATERTHAN_EQUAL_OP, .prec=75, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_EQUAL_OP, .prec=70, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_NOT_EQUAL_OP, .prec=70, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_BIT_AND_OP, .prec=65, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_BIT_XOR_OP, .prec=60, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_BIT_OR_OP, .prec=55, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_AND_OP, .prec=50, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_OR_OP, .prec=45, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_QUESTION_OP, .sec_tt=TT_COLON_OP, .prec=40, .assoc=ASSOC_RIGHT, .arity=ARITY_TRINARY},

	{.tt=TT_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_PLUS_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_MINUS_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_STAR_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_SLASH_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_PERCENT_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_LEFTSHIFT_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_RIGHTSHIFT_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_BIT_AND_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_BIT_XOR_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_BIT_OR_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},

	{.tt=TT_COMMA_OP, .prec=30, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

};

static struct op_s *getop(int tt, int arity)
{
	size_t i;

	for (i=0; i<sizeof ops/sizeof ops[0]; ++i) {
		if (tt==ops[i].tt && arity==ops[i].arity) return &ops[i];
	}

	return NULL;
}

/*static*/ size_t Xexpression(struct node_s *parent, struct token_s **tokens)
{
	size_t /*parsed, */ix=0u;
	struct node_s *node=create_node(parent, EXPRESSION, tokens[0]);
	int done=0;
	struct op_s start_op={.tt=0, .prec=1000, .assoc=ASSOC_NONE, .arity=ARITY_GROUPING};
	struct op_s value={.tt=0, .prec=1000, .assoc=ASSOC_NONE, .arity=ARITY_GROUPING_END};
	struct op_s *op, *prev_op=&start_op;
	int arity;

	while (!done) {
		if (prev_op->arity==ARITY_BINARY || prev_op->arity==ARITY_GROUPING)
			arity=ARITY_LEFT_UNARY;
		else arity=ARITY_BINARY;
		
		if((op=getop(tokens[ix]->type, arity))) {	/* Operator found */

		} else {					/* No operator found */
			op=&value;
		}
		/* TODO: Handle right unary */

		prev_op=op;
	}

	return free_node(node);
}

/* Actual parsers: */

static size_t identifier(struct node_s *parent, struct token_s **tokens);
static size_t type_specifier(struct node_s *parent, struct token_s **tokens);
static size_t expression(struct node_s *parent, struct token_s **tokens);
static size_t statement(struct node_s *parent, struct token_s **tokens);
static size_t compound_statement(struct node_s *parent, struct token_s **tokens);
static size_t declarator(struct node_s *parent, struct token_s **tokens);
static size_t declaration_specifiers(struct node_s *parent, struct token_s **tokens);
static size_t pointer(struct node_s *parent, struct token_s **tokens);
static size_t constant_expression(struct node_s *parent, struct token_s **tokens);
static size_t initializer(struct node_s *parent, struct token_s **tokens);
static size_t cast_expression(struct node_s *parent, struct token_s **tokens);
static size_t unary_expression(struct node_s *parent, struct token_s **tokens);
static size_t specifier_qualifier_list(struct node_s *parent, struct token_s **tokens);
static size_t abstract_declarator(struct node_s *parent, struct token_s **tokens);
static size_t parameter_type_list(struct node_s *parent, struct token_s **tokens);
static size_t type_qualifier(struct node_s *parent, struct token_s **tokens);


static size_t multiplicative_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated_any_token(parent, tokens, MULTIPLICATIVE_EXPRESSION, 
		cast_expression, 3, (int []) {TT_STAR_OP, TT_SLASH_OP, TT_PERCENT_OP});
}

static size_t additive_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated_any_token(parent, tokens, ADDITIVE_EXPRESSION, 
		multiplicative_expression, 2, (int []) {TT_PLUS_OP, TT_MINUS_OP});
}

static size_t shift_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated_any_token(parent, tokens, SHIFT_EXPRESSION, 
		additive_expression, 2, (int []) {TT_LEFTSHIFT_OP, TT_RIGHTSHIFT_OP});
}

static size_t relational_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated_any_token(parent, tokens, RELATIONAL_EXPRESSION, 
		shift_expression,
		4, (int []) {TT_LESSTHAN_OP, TT_GREATERTHAN_OP, TT_LESSTHAN_EQUAL_OP, TT_GREATERTHAN_EQUAL_OP});
}

static size_t equality_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated_any_token(parent, tokens, EQUALITY_EXPRESSION, 
		relational_expression, 2, (int []) {TT_EQUAL_OP, TT_NOT_EQUAL_OP});
}

static size_t and_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, AND_EXPRESSION, equality_expression, TT_BIT_AND_OP);
}

static size_t exclusive_or_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, EXCLUSIVE_OR_EXPRESSION, and_expression, TT_BIT_XOR_OP);
}

static size_t inclusive_or_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, INCLUSIVE_OR_EXPRESSION, exclusive_or_expression, TT_BIT_OR_OP);
}

static size_t logical_and_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, LOGICAL_AND_EXPRESSION, inclusive_or_expression, TT_AND_OP);
}

static size_t logical_or_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, LOGICAL_OR_EXPRESSION, logical_and_expression, TT_OR_OP);
}

static size_t conditional_expression(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, CONDITIONAL_EXPRESSION, tokens[0]);

	if ((parsed=logical_or_expression(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if (tokens[ix]->type==TT_QUESTION_OP) {
		++ix;

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else error_node(node, "[conditional_expression()] Expected expression");

		if (tokens[ix]->type==TT_COLON_OP) ++ix;
		else error_node(node, "[conditional_expression()] Expected colon");

		if ((parsed=conditional_expression(node, tokens+ix))) ix+=parsed;
		else error_node(node, "[conditional_expression()] Expected conditional-expression");
	}

	return add_node(node), ix;
}

static size_t constant(struct node_s *parent, struct token_s **tokens)
{
	return single_token_any_of(parent, tokens, CONSTANT, 
		2, (int []) {TT_CHARACTER, TT_NUMBER});
}

static size_t string_literal(struct node_s *parent, struct token_s **tokens)
{
	return single_token(parent, tokens, STRING_LITERAL, TT_STRING);
}

static size_t primary_expression(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed;
	struct node_s *node=create_node(parent, PRIMARY_EXPRESSION, tokens[0]);

	if ((parsed=identifier(node, tokens))
		|| (parsed=constant(node, tokens))
		|| (parsed=string_literal(node, tokens))) return add_node(node), parsed;
	else if ((parsed=prepostfix_tokens(node, tokens, PRIMARY_EXPRESSION,
		expression, TT_LEFT_PARANTHESIS, TT_RIGHT_PARANTHESIS))) return add_subnode(node), parsed;
	else return free_node(node);
}

static size_t type_name(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, TYPE_NAME, tokens[0]);

	if ((parsed=specifier_qualifier_list(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if ((parsed=abstract_declarator(node, tokens+ix))) ix+=parsed;

	return add_node(node), ix;
}

static size_t designator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DESIGNATOR, tokens[0]);

	if (tokens[ix]->type==TT_LEFT_SQUARE) {
		++ix;

		if ((parsed=constant_expression(node, tokens+ix))) ix+=parsed;
		else return free_node(node);

		if (tokens[ix]->type==TT_RIGHT_SQUARE) ++ix;
		else return free_node(node);
	} else if (tokens[ix]->type==TT_DOT_OP) {
		++ix;

		if ((parsed=identifier(node, tokens+ix))) ix+=parsed;
		else return free_node(node);
	} else return free_node(node);

	return add_node(node), ix;
}

static size_t designation(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DESIGNATION, tokens[0]);

	if ((parsed=designator(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if (tokens[ix]->type==TT_ASSIGNMENT_OP) ++ix;
	else return free_node(node);

	return add_node(node), ix;
}

static size_t designation_initializer(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DESIGNATION_INITIALIZER, tokens[0]);

	if ((parsed=designation(node, tokens+ix))) ix+=parsed;

	if ((parsed=initializer(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	return add_node(node), ix;
}

static size_t initializer_list(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, INITIALIZER_LIST, designation_initializer, TT_COMMA_OP);
}

static size_t postfix_expression(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, POSTFIX_EXPRESSION, tokens[0]);

	if ((parsed=primary_expression(node, tokens+ix))) ix+=parsed;
	else if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
		++ix;

		if ((parsed=type_name(node, tokens+ix))) ix+=parsed;
		else return free_node(node);

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else return free_node(node);

		if (tokens[ix]->type==TT_LEFT_CURLY) ++ix;
		else error_node(node, "[postfix_expression()] Expected opening curly bracket");

		if ((parsed=initializer_list(node, tokens+ix))) ix+=parsed;
		else error_node(node, "[postfix_expression()] Expected initializer-list");

		if (tokens[ix]->type==TT_COMMA_OP) ++ix;

		if (tokens[ix]->type==TT_RIGHT_CURLY) ++ix;
		else error_node(node, "[postfix_expression()] Expected closing curly bracket");
	} else {
		/* TODO: Handle left-recursive grammar */
		return free_node(node);
	}

	return add_node(node), ix;
}

static size_t unary_operator(struct node_s *parent, struct token_s **tokens)
{
	int tts[]={TT_BIT_AND_OP, TT_STAR_OP, TT_PLUS_OP, TT_MINUS_OP, TT_COMPLEMENT_OP, TT_NEGATION_OP};
	struct node_s *node=create_node(parent, UNARY_OPERATOR, tokens[0]);

	if (is_any_of(tokens[0]->type, sizeof tts/sizeof tts[0], tts)) return add_node(node), 1u;
	else return free_node(node);
}

static size_t cast_expression(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, CAST_EXPRESSION, tokens[0]);

	while (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
		++ix;

		if ((parsed=type_name(node, tokens+ix))) ix+=parsed;
		else return free_node(node);

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected closing paranthesis");
	}

	if ((parsed=unary_expression(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	return add_node(node), ix;
}

static size_t unary_expression(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, UNARY_EXPRESSION, tokens[0]);

	if ((parsed=postfix_expression(node, tokens+ix))) ix+=parsed;
	else if (tokens[ix]->type==TT_PLUSPLUS_OP || tokens[ix]->type==TT_MINUSMINUS_OP) {
		++ix;
		
		if ((parsed=unary_expression(node, tokens+ix))) ix+=parsed;
		else error_node(node, "[unary_expression()] Expected unary-expression");
	} else if ((parsed=unary_operator(node, tokens+ix))) {
		ix+=parsed;

		if ((parsed=cast_expression(node, tokens+ix))) ix+=parsed;
		else error_node(node, "[unary_expression()] Expected cast-expression");
	} else if(tokens[ix]->type==TT_SIZEOF) {
		++ix;

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
			++ix;

			if ((parsed=type_name(node, tokens+ix))) ix+=parsed;
			else error_node(node, "[unary_expression()] Expected type-name");

			if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
			else error_node(node, "[unary_expression()] Expected right paranthesis");
		} else if ((parsed=unary_expression(node, tokens+ix))) ix+=parsed;
		else error_node(node, "[unary_expression()] Expected unary-expression or paranthesized type-name");
	}

	return add_node(node), ix;
}

static size_t assignment_expression(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, ASSIGNMENT_EXPRESSION, tokens[0]);

	if ((parsed=unary_expression(node, tokens+ix))) {
		ix+=parsed;

		if (tokens[ix]->type==TT_ASSIGNMENT_OP) {
			++ix;

			if ((parsed=assignment_expression(node, tokens+ix))) ix+=parsed;
			else error_node(node, "[assignment_expression()] Expected assignment-expression");
		} else ix=0u;
	}

	if (ix==0u && (parsed=conditional_expression(node, tokens+ix))) {
		ix+=parsed;
	}

	return ix>0u ? (add_node(node), ix) : free_node(node);
}

static size_t direct_abstract_declarator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DIRECT_ABSTRACT_DECLARATOR, tokens[0]);
	int tt;

	if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
		++ix;

		if ((parsed=abstract_declarator(node, tokens+ix))) ix+=parsed;
		else return free_node(node);

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node(node, "Expected closing paranthesis");
	}

	while ((tt=tokens[ix]->type==TT_LEFT_SQUARE) || (tt=tokens[ix]->type==TT_LEFT_PARANTHESIS)) {
		++ix;

		if (tt==TT_LEFT_SQUARE) {
			if (tokens[ix]->type==TT_STAR_OP) ++ix;
			else if ((parsed=assignment_expression(node, tokens+ix))) ix+=parsed;
			else error_node(node, "Expected assignment expression or *");

			if (tokens[ix]->type==TT_RIGHT_SQUARE) ++ix;
			else error_node(node, "Expected closing square bracket");
		} else if (tt==TT_LEFT_PARANTHESIS) {
			if ((parsed=parameter_type_list(node, tokens+ix))) ix+=parsed;
			else error_node(node, "Expected parameter type list");

			if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
			else error_node(node, "Expected closing paranthesis");
		}	/* No other type is possible */
	}

	return ix>0u ? (add_node(node), ix) : free_node(node);
}

static size_t abstract_declarator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, ABSTRACT_DECLARATOR, tokens[0]);

	if ((parsed=pointer(node, tokens+ix))) ix+=parsed;

	if ((parsed=direct_abstract_declarator(node, tokens+ix))) ix+=parsed;

	if (ix>0u) return add_node(node), ix;
	else return free_node(node);
}

static size_t parameter_declaration(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, PARAMETER_DECLARATION, tokens[0]);

	if ((parsed=declaration_specifiers(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if ((parsed=declarator(node, tokens+ix))
		|| (parsed=abstract_declarator(node, tokens+ix))) {
		ix+=parsed;

		return add_node(node), ix;
	}

	return add_node(node), ix;
}

static size_t parameter_list(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, PARAMETER_LIST, parameter_declaration, TT_COMMA_OP);
}

static size_t parameter_type_list(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, PARAMETER_TYPE_LIST, tokens[0]);

	if ((parsed=parameter_list(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if (tokens[ix]->type==TT_COMMA_OP) {
		++ix;

		if (tokens[ix]->type==TT_ELIPSIS) ++ix;
		else return free_node(node);
	}

	return add_node(node), ix;
}

static size_t storage_class_specifier(struct node_s *parent, struct token_s **tokens)
{
	int tts[]={TT_TYPEDEF, TT_EXTERN, TT_STATIC, TT_AUTO, TT_REGISTER};

	return single_token_any_of(parent, tokens, STORAGE_CLASS_SPECIFIER, sizeof tts/sizeof tts[0], tts);
}

static size_t struct_or_union(struct node_s *parent, struct token_s **tokens)
{
	int tts[]={TT_STRUCT, TT_UNION};

	return single_token_any_of(parent, tokens, STRUCT_OR_UNION, sizeof tts/sizeof tts[0], tts);
}


static size_t specifier_qualifier_list(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, SPECIFIER_QUALIFIER_LIST, tokens[0]);

	while ((parsed=type_specifier(node, tokens+ix)) || (parsed=type_qualifier(node, tokens+ix))) ix+=parsed;

	return ix>0u ? (add_node(node), ix) : free_node(node);
}

static size_t struct_declarator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, STRUCT_DECLARATOR, tokens[0]);

	if ((parsed=declarator(node, tokens+ix))) ix+=parsed;

	if (tokens[ix]->type==TT_COLON_OP) {
		++ix;

		if ((parsed=constant_expression(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected constant-expression");
	}

	if (ix>0u) return add_node(node), ix;
	else return free_node(node);
}

static size_t struct_declarator_list(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, STRUCT_DECLARATOR_LIST, struct_declarator, TT_COMMA_OP);
}

static size_t struct_declaration(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, STRUCT_DECLARATION, tokens[0]);

	fprintf(stderr, "struct_declaration() - 1\n");

	if ((parsed=specifier_qualifier_list(node, tokens+ix))) ix+=parsed;
	else return free_node(node);
	fprintf(stderr, "struct_declaration() - 2\n");

	if ((parsed=struct_declarator_list(node, tokens+ix))) ix+=parsed;
	else return free_node(node);
	fprintf(stderr, "struct_declaration() - 3\n");

	if (tokens[ix]->type==TT_SEMICOLON_OP) ++ix;
	else return free_node(node);
	fprintf(stderr, "struct_declaration() - 4\n");

	return add_node(node), ix;
}

static size_t struct_declaration_list(struct node_s *parent, struct token_s **tokens)
{
	return many(parent, tokens, STRUCT_DECLARATION_LIST, struct_declaration);
}

static size_t struct_or_union_specifier(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	int id_or_list=0;
	struct node_s *node=create_node(parent, STRUCT_OR_UNION_SPECIFIER, tokens[0]);

	if ((parsed=struct_or_union(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if ((parsed=identifier(node, tokens+ix))) {ix+=parsed; id_or_list=1;}

	if (tokens[ix]->type==TT_LEFT_CURLY) {
		++ix;
		id_or_list=1;

		if ((parsed=struct_declaration_list(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected struct declarator list");

		if (tokens[ix]->type==TT_RIGHT_CURLY) ++ix;
		else error_node(node, "Expected right curly brace");
	}

	if (!id_or_list) error_node(node, "Expected struct/union tag name or struct declaration list");

	return add_node(node), ix;
}

static size_t enumerator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, ENUMERATOR, tokens[0]);

	if ((parsed=identifier(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if (tokens[ix]->type==TT_ASSIGNMENT_OP) {
		++ix;
	
		if ((parsed=constant_expression(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected constant expression");
	}

	return add_node(node), ix;
}

static size_t enumerator_list(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, ENUMERATOR_LIST, enumerator, TT_COMMA_OP);
}

static size_t enum_specifier(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, ENUM_SPECIFIER, tokens[0]);

	if (tokens[ix]->type==TT_ENUM) {
		++ix;

		if ((parsed=identifier(node, tokens+ix))) ix+=parsed;

		if (tokens[ix]->type==TT_LEFT_CURLY) {
			++ix;

			if ((parsed=enumerator_list(node, tokens+ix))) ix+=parsed;
			else error_node(node, "Expected enumerator list");

			if (tokens[ix]->type==TT_COMMA_OP) ++ix;
			
			if (tokens[ix]->type==TT_RIGHT_CURLY) ++ix;
			else error_node(node, "Expected closing brace");
		}

		if (ix < 2) error_node(node, "Expected identifier or enumerator list after enum");

		return add_node(node), ix;
	} else return free_node(node);
}

static size_t typedef_name(struct node_s *parent, struct token_s **tokens)
{
	struct node_s *node=create_node(parent, TYPEDEF_NAME, tokens[0]);

	if (istypealias(node, tokens[0])) return add_node(node), 1u;
	else return free_node(node);
}

static size_t type_specifier(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, TYPE_SPECIFIER, tokens[0]);

	switch(tokens[ix]->type) {
	case TT_VOID:
	case TT_CHAR: case TT_SHORT: case TT_INT: case TT_LONG:
	case TT_FLOAT: case TT_DOUBLE:
	case TT_SIGNED: case TT_UNSIGNED:
	case TT_BOOL:
	case TT_COMPLEX:
		return add_node(node), 1u;
	default:
		if ((parsed=struct_or_union_specifier(node, tokens+ix))
			|| (parsed=enum_specifier(node, tokens+ix))
			|| (parsed=typedef_name(node, tokens+ix))) return add_node(node), parsed;
		else return free_node(node);
	}
}

static size_t type_qualifier(struct node_s *parent, struct token_s **tokens)
{
	struct node_s *node=create_node(parent, TYPE_QUALIFIER, tokens[0]);

	if (tokens[0]->type==TT_CONST
		|| tokens[0]->type==TT_RESTRICT
		|| tokens[0]->type==TT_VOLATILE) return add_node(node), 1u;
	else return free_node(node);
}

static size_t type_qualifier_list(struct node_s *parent, struct token_s **tokens)
{
	return many(parent, tokens, TYPE_QUALIFIER_LIST, type_qualifier);
}

static size_t function_specifier(struct node_s *parent, struct token_s **tokens)
{
	return single_token(parent, tokens, FUNCTION_SPECIFIER, TT_INLINE);
}

static size_t declaration_specifiers(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DECLARATION_SPECIFIERS, tokens[0]);

	while ((parsed=storage_class_specifier(node, tokens+ix))
		|| (parsed=type_specifier(node, tokens+ix))
		|| (parsed=type_qualifier(node, tokens+ix))
		|| (parsed=function_specifier(node, tokens+ix))) ix+=parsed;

	if (ix>0u) return add_node(node), ix;
	else return free_node(node);
}

static size_t pointer(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, POINTER, tokens[0]);

	if (tokens[ix]->type==TT_STAR_OP) ++ix;
	else return free_node(node);

	if ((parsed=type_qualifier_list(node, tokens+ix))) ix+=parsed;

	if ((parsed=pointer(node, tokens+ix))) ix+=parsed;

	return add_node(node), ix;
}

static size_t identifier(struct node_s *parent, struct token_s **tokens)
{
	struct node_s *node=create_node(parent, IDENTIFIER, tokens[0]);

	if (tokens[0]->type==TT_TEXTUAL) {
		if (node->in_typedef_declaration) {
			add_typealias(node);
		}

		return add_node(node), 1u;
	} else return free_node(node);
/*
	return single_token(parent, tokens, IDENTIFIER, TT_TEXTUAL);*/
}

static size_t identifier_list(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, IDENTIFIER_LIST, identifier, TT_COMMA_OP);
}

static size_t direct_declarator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DIRECT_DECLARATOR, tokens[0]);
	int tt;

	/* identifier 
	| "(" declarator ")" 
	| direct-declarator "[" type-qualifier-list? assignment-expression? "]"
	| direct-declarator "[" "static" type-qualifier-list? assignment-expression? "]"
	| direct-declarator "[" type-qualifier-list? "static" assignment-expression? "]"
	| direct-declarator "[" type-qualifier-list? "*" "]"
	| direct-declarator "(" parameter-type-list ")"
	| direct-declarator "(" identifier-list? ")" */

	if ((parsed=identifier(node, tokens+ix))) {
		ix+=parsed;
	} else if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
		++ix;

		if ((parsed=declarator(node, tokens+ix))) {
			ix+=parsed;

			if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
			else return free_node(node);/*ix=0u;*/
		} else return free_node(node); /*ix=0u;*/
	}

	while ((tt=tokens[ix]->type)==TT_LEFT_PARANTHESIS || (tt=tokens[ix]->type)==TT_LEFT_SQUARE) {
		++ix;
		if (tt==TT_LEFT_SQUARE) {
			if (tokens[ix]->type==TT_STATIC) ++ix;

			if ((parsed=type_qualifier_list(node, tokens+ix))) ix+=parsed;

			/* TODO: Handle type-qualifier-list "*" */

			if (tokens[ix]->type==TT_STATIC) ++ix;

			if ((parsed=assignment_expression(node, tokens+ix))) ix+=parsed;

			if (tokens[ix]->type==TT_RIGHT_SQUARE) ++ix;
			else error_node(node, "Expected right square bracket");
		} else {	/* Paranthesis */
			if ((parsed=parameter_type_list(node, tokens+ix))) ix+=parsed;
			else if ((parsed=identifier_list(node, tokens+ix))) ix+=parsed;

			if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
			else return free_node(node);
		}
	}

	if(ix>0u) return add_node(node), ix;
	else return free_node(node);
}

static size_t declarator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DECLARATOR, tokens[0]);

	if ((parsed=pointer(node, tokens+ix))) ix+=parsed;

	if ((parsed=direct_declarator(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	return add_node(node), ix;
}

static size_t initializer(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, INITIALIZER, tokens[0]);

	if ((parsed=assignment_expression(node, tokens))) ix+=parsed;
	else if ((parsed=initializer_list(node, tokens))) {
		ix+=parsed;

		if (tokens[ix]->type==TT_COMMA_OP) ++ix;
	} else return free_node(node);

	return add_node(node), ix;
}

static size_t init_declarator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, INIT_DECLARATOR, tokens[0]);

	if ((parsed=declarator(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if (tokens[ix]->type==TT_ASSIGNMENT_OP) {
		++ix;

		if ((parsed=initializer(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected initializer");
	}

	return add_node(node), ix;
}

static size_t init_declarator_list(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, INIT_DECLARATOR_LIST, init_declarator, TT_COMMA_OP);
}

static size_t declaration(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DECLARATION, tokens[0]);

	if (tokens[0]->type==TT_TYPEDEF) node->in_typedef_declaration=1;

	if ((parsed=declaration_specifiers(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if ((parsed=init_declarator_list(node, tokens+ix))) ix+=parsed;

	if (tokens[ix]->type==TT_SEMICOLON_OP) ++ix;
	else return free_node(node); /*error_node(node, "Expected semicolon - ; at end of declaration");*/

	return add_node(node), ix;
}

static size_t declaration_list(struct node_s *parent, struct token_s **tokens)
{
	return many(parent, tokens, DECLARATION_LIST, declaration);
}

static size_t constant_expression(struct node_s *parent, struct token_s **tokens)
{
	return conditional_expression(parent, tokens);	/* TODO: Use node type for cond. expr. */
}

static size_t labeled_statement(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, LABELED_STATEMENT, tokens[0]);

	if ((parsed=identifier(node, tokens+ix))) {
		ix+=parsed;

		if (tokens[ix]->type==TT_COLON_OP) ++ix;
		else return free_node(node);
	} else if (tokens[ix]->type==TT_CASE) {
		++ix;

		if ((parsed=constant_expression(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected constant expression after \"case\"");

		if (tokens[ix]->type==TT_COLON_OP) ++ix;
		else error_node(node, "Expected colon - : after constant expression in \"case\" statement");
	} else if (tokens[ix]->type==TT_DEFAULT) {
		++ix;

		if (tokens[ix]->type==TT_COLON_OP) ++ix;
		else error_node(node, "Expected colon - : after \"default\"");
	} else return free_node(node);

	if ((parsed=statement(node, tokens+ix))) ix+=parsed;	/* Made this optional */

	return add_node(node), ix;
}

static size_t expression(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, EXPRESSION, assignment_expression, TT_COMMA_OP);
}

static size_t expression_statement(struct node_s *parent, struct token_s **tokens)
{
	return postfix_token(parent, tokens, EXPRESSION_STATEMENT, expression, TT_SEMICOLON_OP);
}

static size_t selection_statement(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, SELECTION_STATEMENT, tokens[0]);

	if (tokens[ix]->type==TT_IF) {
		++ix;

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
		else error_node(node, "Expected left paranthesis - ( after if");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected condition expression for if statement");

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node(node, "Expected right paranthesis - ) after condition expression");

		if ((parsed=statement(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected statement in if statement");

		if (tokens[ix]->type==TT_ELSE) {
			++ix;

			if ((parsed=statement(node, tokens+ix))) ix+=parsed;
			else error_node(node, "Expected statement for else clause in if statement");
		}

		return add_node(node), ix;
	} else if (tokens[ix]->type==TT_SWITCH) {
		++ix;

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
		else error_node(node, "Expected left paranthesis - ( after switch");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected condition expression for switch statement");

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node(node, "Expected right paranthesis - ) after condition expression");

		if ((parsed=statement(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected statement in switch statement");

		return add_node(node), ix;
	} else return free_node(node);
}

static size_t iteration_statement(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, ITERATION_STATEMENT, tokens[0]);

	if (tokens[ix]->type==TT_WHILE) {
		++ix;

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
		else error_node(node, "Expected left paranthesis");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected expression");

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node(node, "Expected right parantheses");

		if ((parsed=statement(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected statement");
	} else if (tokens[ix]->type==TT_DO) {
		++ix;

		if ((parsed=statement(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected statement");

		if (tokens[ix]->type==TT_WHILE) ++ix;
		else error_node(node, "Expected \"while\" keyword");

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
		else error_node(node, "Expected left paranthesis");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected expression");

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node(node, "Expected right parantheses");

		if (tokens[ix]->type==TT_SEMICOLON_OP) ++ix;
		else error_node(node, "Expected semicolon");
	} else if (tokens[ix]->type==TT_FOR) {
		++ix;

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
		else error_node(node, "Expected left paranthesis");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else if ((parsed=declaration(node, tokens+ix))) ix+=parsed;

		if (tokens[ix]->type==TT_SEMICOLON_OP) ++ix;
		else error_node(node, "Expected semicolon");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;

		if (tokens[ix]->type==TT_SEMICOLON_OP) ++ix;
		else error_node(node, "Expected semicolon");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node(node, "Expected right paranthesis");

		if ((parsed=statement(node, tokens+ix))) ix+=parsed;
		else error_node(node, "Expected statement");
	} else return free_node(node);

	return add_node(node), ix;
}

static size_t jump_statement(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, JUMP_STATEMENT, tokens[0]);

	if (tokens[ix]->type==TT_GOTO) {
		if (tokens[++ix]->type==TT_TEXTUAL && tokens[++ix]->type==TT_SEMICOLON_OP) return add_node(node), ++ix;
		else error_node_token(node, tokens[ix], "Expected identifier and semicolon after goto");
	} else if (tokens[ix]->type==TT_CONTINUE || tokens[ix]->type==TT_BREAK) {
		if (tokens[++ix]->type==TT_SEMICOLON_OP) return add_node(node), ++ix;
		else error_node_token(node, tokens[ix], "Expected semicolon after continue/break");
	} else if (tokens[ix]->type==TT_RETURN) {
		++ix;

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected expression after return");

		if (tokens[ix]->type==TT_SEMICOLON_OP) return add_node(node), ++ix;
		else error_node_token(node, tokens[ix], "Expected semicolon after return expression");
	}

	return free_node(node);
}

static size_t statement(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, STATEMENT, tokens[0]);

	if ((parsed=labeled_statement(node, tokens+ix))
		|| (parsed=compound_statement(node, tokens+ix))
		|| (parsed=expression_statement(node, tokens+ix))
		|| (parsed=selection_statement(node, tokens+ix))
		|| (parsed=iteration_statement(node, tokens+ix))
		|| (parsed=jump_statement(node, tokens+ix))) return add_node(node), parsed;
	else return free_node(node);
}

static size_t block_item(struct node_s *parent, struct token_s **tokens)
{
	/* Node-less parse rule */
	size_t parsed;

	if ((parsed=declaration(parent, tokens)) || (parsed=statement(parent, tokens)))
		return parsed;
	else return 0u;
}

static size_t block_item_list(struct node_s *parent, struct token_s **tokens)
{
	fprintf(stderr, "block_item_list()\n");
	return many(parent, tokens, BLOCK_ITEM_LIST, block_item);
}

static size_t compound_statement(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, COMPOUND_STATEMENT, tokens[0]);

	if (tokens[ix]->type==TT_LEFT_CURLY) ++ix;
	else return free_node(node);

	if ((parsed=block_item_list(node, tokens+ix))) ix+=parsed;

	if (tokens[ix]->type==TT_RIGHT_CURLY) ++ix;
	else error_node(node, "Missing right curly brace - } at end of compound statement");

	return add_node(node), ix;
}

static size_t function_definition(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, FUNCTION_DEFINITION, tokens[0]);

	if ((parsed=declaration_specifiers(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if ((parsed=declarator(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if ((parsed=declaration_list(node, tokens+ix))) ix+=parsed;

	if ((parsed=compound_statement(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	return add_node(node), ix;
}

static size_t external_declaration(struct node_s *parent, struct token_s **tokens)
{
	return any_of_2(parent, tokens, EXTERNAL_DECLARATION, function_definition, declaration);
}

static size_t translation_unit(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, TRANSLATION_UNIT, tokens[0]);

	while ((parsed=external_declaration(node, tokens+ix))) ix+=parsed;

	return add_node(node), ix;
}

size_t parse(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed;
	struct node_s *node;

	g_root_node=node=create_node(parent, NT_UNIT, tokens[0]);

	if ((parsed=translation_unit(node, tokens))) return add_node(node), parsed;
	else return free_node(node);
}

/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */

