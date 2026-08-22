#include <gen_asm_amd64.h>

#include "node.h"
#include "token.h"

static void gen_expr_node(struct node_s *node, FILE *fp, int treg);

static struct {
	int taken;
	const char *name64, *name32, *name16, *name8;
} regs[] = {
	{0, .name64="rax", .name32="eax", .name16="ax", .name8="al"},
	{1, .name64="rbx", .name32="ebx", .name16="bx", .name8="bl"},
	{0, .name64="rcx", .name32="ecx", .name16="cx", .name8="cl"},
	{0, .name64="rdx", .name32="edx", .name16="dx", .name8="dl"},
	{0, .name64="rsi", .name32="esi", .name16="si", .name8="sil"},
	{0, .name64="rdi", .name32="edi", .name16="di", .name8="dil"},
	{1, .name64="rbp", .name32="ebp", .name16="bp", .name8="bpl"},
	{1, .name64="rsp", .name32="esp", .name16="sp", .name8="spl"},
	{0, .name64="r8", .name32="r8d", .name16="r8w", .name8="r8b"},
	{0, .name64="r9", .name32="r9d", .name16="r9w", .name8="r9b"},
	{0, .name64="r10", .name32="r10d", .name16="r10w", .name8="r10b"},
	{0, .name64="r11", .name32="r11d", .name16="r11w", .name8="r11b"},
	{1, .name64="r12", .name32="r12d", .name16="r12w", .name8="r12b"},
	{1, .name64="r13", .name32="r13d", .name16="r13w", .name8="r13b"},
	{1, .name64="r14", .name32="r14d", .name16="r14w", .name8="r14b"},
	{1, .name64="r15", .name32="r15d", .name16="r15w", .name8="r15b"}
};

static int get_free_reg(void)
{
	size_t ix;

	for (ix=0; ix<sizeof regs/sizeof regs[0]; ++ix) {
		if (!regs[ix].taken) return ix;
	}

	return -1;
}

static void gen_expr_add(struct node_s *node, FILE *fp, int treg)
{
	int rreg;

	if (node->nsubnodes!=2) return;

	gen_expr_node(node->subnodes[0], fp, treg);

	rreg = get_free_reg();
	regs[rreg].taken = 1;
	gen_expr_node(node->subnodes[1], fp, rreg);

	fprintf(fp, "\taddl %%%s, %%%s\n", regs[rreg].name32, regs[treg].name32);

	regs[rreg].taken = 0;
}

static void gen_expr_sub(struct node_s *node, FILE *fp, int treg)
{
	int rreg;

	if (node->nsubnodes!=2) return;

	gen_expr_node(node->subnodes[0], fp, treg);

	rreg = get_free_reg();
	regs[rreg].taken = 1;
	gen_expr_node(node->subnodes[1], fp, rreg);

	fprintf(fp, "\tsubl %%%s, %%%s\n", regs[rreg].name32, regs[treg].name32);

	regs[rreg].taken = 0;
}

static void gen_expr_mul(struct node_s *node, FILE *fp, int treg)
{
	int rreg;

	if (node->nsubnodes!=2) return;

	gen_expr_node(node->subnodes[0], fp, treg);

	rreg = get_free_reg();
	regs[rreg].taken = 1;
	gen_expr_node(node->subnodes[1], fp, rreg);

	fprintf(fp, "\timul %%%s, %%%s\n", regs[rreg].name32, regs[treg].name32);

	regs[rreg].taken = 0;
}

static void gen_expr_div(struct node_s *node, FILE *fp, int treg)
{
	int rreg;

	if (node->nsubnodes!=2) return;

	gen_expr_node(node->subnodes[0], fp, treg);

	rreg = get_free_reg();
	regs[rreg].taken = 1;
	gen_expr_node(node->subnodes[1], fp, rreg);

	fprintf(fp, "\tcqo\n");
	fprintf(fp, "\tidiv %%%s\n", regs[rreg].name32);
	if (treg!=0) { /* Different from EAX? */
		fprintf(fp, "\tmov %%eax, %%%s\n", regs[treg].name32);
	}

	regs[rreg].taken = 0;
}

static void gen_expr_mod(struct node_s *node, FILE *fp, int treg)
{
	int rreg;

	if (node->nsubnodes!=2) return;

	gen_expr_node(node->subnodes[0], fp, treg);

	rreg = get_free_reg();
	regs[rreg].taken = 1;
	gen_expr_node(node->subnodes[1], fp, rreg);

	fprintf(fp, "\tcqo\n");
	fprintf(fp, "\tidiv %%%s\n", regs[rreg].name32);
	if (treg!=3) { /* Different from EDX? */
		fprintf(fp, "\tmov %%edx, %%%s\n", regs[treg].name32);
	}

	regs[rreg].taken = 0;
}

static void gen_expr_node(struct node_s *node, FILE *fp, int treg)
{
	int intval;

	switch (node->type) {
	case CONSTANT:
		intval = atoi(token_text(node->token));
		fprintf(fp, "\tmovl $%d, %%%s\n", intval, regs[treg].name32);
		break;
	case ADD_EXPRESSION:
		gen_expr_add(node, fp, treg);
		break;
	case SUB_EXPRESSION:
		gen_expr_sub(node, fp, treg);
		break;
	case MUL_EXPRESSION:
		gen_expr_mul(node, fp, treg);
		break;
	case DIV_EXPRESSION:
		gen_expr_div(node, fp, treg);
		break;
	case MOD_EXPRESSION:
		gen_expr_mod(node, fp, treg);
		break;
	default:;
	}
}

static void gen_expr(struct node_s *node, FILE *fp, int treg)
{
	if (node->nsubnodes!=1) return;

	gen_expr_node(node->subnodes[0], fp, treg);
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
				regs[0].taken = 1;
				gen_expr(expr, fp, 0);
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
