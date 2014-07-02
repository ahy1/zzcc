#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "token.h"
#include "tokenclass.h"
#include "strbuf.h"
#include "parser.h"

int main(int argc, char *argv[])
{
	struct token_s *token;
	FILE *fp;
	STRBUF *sb;
	struct token_s **tokens=NULL;
	size_t ntokens=0;
	struct node_s root_node;
	int lno=1, cno=0;
	struct token_s end_token={NULL, 0, TT_END, 0, 0, NULL, 0, 0};

	sb=sballoc(1024);

	if (argc>1) fp=fopen(argv[1], "r");
	else fp=stdin;

	while ((token=gettoken(fp, sb, &lno, &cno))) {
		//print_token("main()", token);

		/* TODO: Handle TT_UNKNOWN as error in the future */
		if (!istobeignored(token) && token->type!=TT_NULL) {
			//(void)puts(" Added");
			tokens=(struct token_s **)realloc(tokens, ++ntokens * sizeof *tokens);
			tokens[ntokens-1]=token;
		}

		//printf("[%s]\n", sbcstr(sb, token->sbix));
		//freetoken(token);
	}
	tokens=(struct token_s **)realloc(tokens, ++ntokens * sizeof *tokens);
	tokens[ntokens-1]=&end_token;

	memset(&root_node, 0, sizeof root_node);
	root_node.type=NT_ROOT;

	(void)parse(&root_node, tokens);

	//printf(" ====== Nodes\n");
	//print_node(&root_node, 0);

	//printf(" ====== Nodes (JSON)\n");
	print_node_json(&root_node, 0);

	(void)getchar();

	(void)sbfree(sb);

	return EXIT_SUCCESS;
}


