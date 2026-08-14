#include "gen.h"

#include "node.h"
#include "token.h"

#include <stdio.h>

void gen_code_asm_amd64(struct node_s *node, FILE *fp)
{
	struct node_s *tunit, *funcdef, *funcid;
	size_t ix;

	if (node->type!=NT_ROOT
			|| node->nsubnodes!=1
			|| node->subnodes[0]->type!=NT_UNIT
			|| node->subnodes[0]->nsubnodes!=1
			|| node->subnodes[0]->subnodes[0]->type!=TRANSLATION_UNIT) {
		return;
	}

	fprintf(fp, "\t.text\n");
	fprintf(fp, "\tp2align 4\n");
	fprintf(fp, "\n");

	tunit = node->subnodes[0]->subnodes[0];

	for (ix=0; ix<tunit->nsubnodes; ++ix) {
		if (tunit->subnodes[ix]->type==FUNCTION_DEFINITION) {
			funcdef = tunit->subnodes[ix];
			fprintf(stderr, " >>> Function definiton\n");
			print_node(funcdef, 16);

			if (!(funcid=get_subnode_by_typepath(funcdef, (int[]) {DECLARATOR, DIRECT_DECLARATOR, IDENTIFIER}, 3))) return;

			fprintf(fp, "\t.global %s\n", token_text(funcid->token));
			fprintf(fp, "%s:\n", token_text(funcid->token));
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
