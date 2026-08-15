#include "gen.h"

#include "gen_asm_amd64.h"

#include <stdio.h>

void gen_code(struct node_s *node, const char *target)
{
	FILE *fp;

	fp = fopen(target, "wb");

	if (fp) {
		gen_code_asm_amd64(node, fp);
		fclose(fp);
	}
}
