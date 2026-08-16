#include <gen_asm_amd64.h>

#include "node.h"
#include "token.h"

static void gen_expr(struct node_s *node, FILE *fp)
{
	int intval;

	if (node->nsubnodes!=1) return;

	switch (node->subnodes[0]->type) {
	case CONSTANT:
		intval = atoi(token_text(node->subnodes[0]->token));
		fprintf(fp, "\tmovl $%d, %%eax\n", intval);
	default:;
	}
}

static void gen_block(struct node_s *node, FILE *fp)
{
	struct node_s *expr;
	size_t ix;

	for (ix=0; ix<node->nsubnodes; ++ix) {
		switch (node->subnodes[ix]->type) {
		case STATEMENT:
			switch (node->subnodes[ix]->token->type) {
			case TT_RETURN:
				if (!(expr=get_subnode_by_type(node->subnodes[ix], EXPRESSION))) return;
				gen_expr(expr, fp);
				break;
			}
			break;
		default:;
		}
	}
}

void gen_code_asm_amd64(struct node_s *node, FILE *fp)
{
	struct node_s *tunit, *funcdef, *funcid, *block;
	size_t ix;

	if (!(tunit=get_subnode_by_typepath(node, (int[]) {NT_UNIT, TRANSLATION_UNIT}, 2))) return;

	fprintf(fp, "\t.text\n");
	fprintf(fp, "\t.p2align 4\n");
	fprintf(fp, "\n");

	for (ix=0; ix<tunit->nsubnodes; ++ix) {
		if (tunit->subnodes[ix]->type==FUNCTION_DEFINITION) {
			funcdef = tunit->subnodes[ix];
			fprintf(stderr, " >>> Function definiton\n");
			print_node(funcdef, 16);

			if (!(funcid=get_subnode_by_typepath(funcdef, (int[]) {DECLARATOR, DIRECT_DECLARATOR, IDENTIFIER}, 3))) return;

			fprintf(fp, "\t.global %s\n", token_text(funcid->token));
			fprintf(fp, "%s:\n", token_text(funcid->token));
			fprintf(fp, "\tpushq %%rbp\n");
			fprintf(fp, "\tmovq %%rsp,%%rbp\n");

			if (!(block=get_subnode_by_typepath(funcdef, (int[]) {COMPOUND_STATEMENT, BLOCK_ITEM_LIST}, 2))) return;
			gen_block(block, fp);

			fprintf(fp, "\tmovq %%rbp,%%rsp\n");
			fprintf(fp, "\tpopq %%rbp\n");
			fprintf(fp, "\tret\n");
		}
	}
}
