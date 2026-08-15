#include "gen.h"

#include "node.h"
#include "token.h"

#include <stdio.h>

void gen_code_asm_amd64(struct node_s *node, FILE *fp)
{
	struct node_s *tunit, *funcdef, *funcid;
	size_t ix;

	if (!(tunit=get_subnode_by_typepath(node, (int[]) {NT_UNIT, TRANSLATION_UNIT}, 2))) return;

	fprintf(fp, "\t.text\n");
	fprintf(fp, "\tp2align 4\n");
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

			fprintf(fp, "\tmovq %%rbp,%%rsp\n");
			fprintf(fp, "\tpopq %%rbp\n");
			fprintf(fp, "\tret\n");
		}
	}
}

void gen_code(struct node_s *node, const char *target)
{
	FILE *fp;

	fp = fopen(target, "wb");

	if (fp) {
		gen_code_asm_amd64(node, fp);
		fclose(fp);
	}
}
