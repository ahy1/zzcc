
#include <stdarg.h>

#include "stack.h"
#include "json.h"
#include "node.h"



static const char *node_type_names[]={
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
	"ARGUMENT_EXPRESSION_LIST",
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
	"ALIGNMENT_SPECIFIER",
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

	"NT_NON_STANDARD_ATTRIBUTE",
	"NT_NON_STANDARD_ASM",

	"NT_ANY",

	"NT_ROOT",
	"NT_UNIT",
	"NT_DUMMY"
};

const char *node_type_name(struct node_s *node)
{
	return node ? node_type_names[node->type] : "";
}

static void print_node_type(FILE *fp, struct node_s *node)
{
	fprintf(stderr, "{%-26s}", node_type_names[node->type]); 
}

void log_node_token(struct node_s *node, struct token_s *token, const char *fmt, ...)
{
	va_list arg;
	int level=node->level;

	fprintf(stderr, "%02d %3d/%2d [%-8s] %-25s", level, token_lno(token), token_cno(token),
		token_text(token), token_type(token));

	while(level--) (void)putc(' ', stderr);

	print_node_type(stderr, node);

	va_start(arg, fmt);
	(void)vfprintf(stderr, fmt, arg);
	va_end(arg);
}

#define NODE_SETS_ALLOC
#define NODE_SET_SIZE 16

struct nodeset_s {
	struct node_s nodes[NODE_SET_SIZE];
	size_t nnodes;
};

STACK *nodesets=NULL;

static struct node_s *nodealloc(void)
{
#ifdef NODE_SETS_ALLOC
	struct nodeset_s *nodeset;

	if (!nodesets) {
		nodesets=stackalloc(1u);
		nodeset=(struct nodeset_s *)calloc(1, sizeof *nodeset);
		stackpush(nodesets, (void *)nodeset);
	} else nodeset=(struct nodeset_s *)stacktop(nodesets);

	if (nodeset->nnodes >= NODE_SET_SIZE) {
		nodeset=(struct nodeset_s *)calloc(1, sizeof *nodeset);
		stackpush(nodesets, (void *)nodeset);
	}

	return &nodeset->nodes[nodeset->nnodes++];
#else
	struct node_s *node=(struct node_s *)calloc(1, sizeof *node);

	if (!node) {
		fprintf(stderr, "ERROR: Out of memory\n");
		exit(EXIT_FAILURE);
	}

	return node;
#endif
}

static void nodefree(struct node_s *node)
{
#ifdef NODE_SETS_ALLOC
	struct nodeset_s *nodeset;

	nodeset=(struct nodeset_s *)stacktop(nodesets);

	if (nodeset->nnodes > 0u) --nodeset->nnodes;	/* TODO: Handle empty nodeset? */
#else
	free(node);
#endif
}

struct node_s *create_node(struct node_s *parent, int type, struct token_s *token)
{
	struct node_s *node=nodealloc();
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

struct node_s *create_mergeable_node(struct node_s *parent, int type, struct token_s *token)
{
	struct node_s *node=create_node(parent, type, token);

	node->mergeable=1;

	return node;
}

void set_node_token(struct node_s *node, struct token_s *token)
{
	node->token=token;
}

size_t error_node_token(struct node_s *node, struct token_s *token, const char *fmt, ...)
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

int add_node(struct node_s *node)
{
	log_node_token(node, node->token, "+ add_node(): Adding to %s\n", 
		node_type_names[node->parent->type]);

	if (node->mergeable && node->nsubnodes==1u) {
		log_node_token(node, node->token, "+ add_node(): Merging subnode %s\n", 
			node_type_names[node->subnodes[0]->type]);

		/* TODO: This node will not be freed. Consider if it is needed to save space */
		node->subnodes[0]->parent=node->parent;
		node=node->subnodes[0];
	}

	node->parent->subnodes=(struct node_s **)realloc(
		node->parent->subnodes, ++node->parent->nsubnodes * sizeof node);
	node->parent->subnodes[node->parent->nsubnodes-1]=node;

	return 0;
}

size_t free_node(struct node_s *node)
{
	size_t ix;

	log_node_token(node, node->token, "- free_node(): Freeing from %s\n", 
		node_type_names[node->parent->type]);

	if (node->parent 
			&& node->parent->nsubnodes > 0u
			&& node->parent->subnodes[node->parent->nsubnodes]==node) {
		--node->parent->nsubnodes;
	}

	for (ix=0; ix<node->nsubnodes; ++ix) nodefree(node->subnodes[ix]);

	nodefree(node);

	return (size_t)0;
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



