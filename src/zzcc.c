#include <stdio.h>
#include <string.h>

#include "gen.h"
#include "node.h"
#include "parser.h"
#include "strbuf.h"
#include "token.h"
#include "tokenclass.h"

static void skip_to_eol(FILE *infp, STRBUF *sb, int *lno, int *cno)
{
	struct token_s *token;

	while ((token=gettoken(infp, sb, lno, cno))) {
		if (token->type==TT_WHITESPACE && token->subtype==WTT_NEWLINEWS) break;
	}
}

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
	size_t parsed;

	sb=sballoc(1024);

	if (argc>1) fp=fopen(argv[1], "r");
	else fp=stdin;

	if (!fp) {
		fprintf(stderr, "ERROR: Can\'t open file %s for reading\n", argv[1]);
		return EXIT_FAILURE;
	}

	while ((token=gettoken(fp, sb, &lno, &cno))) {
		/* TODO: Handle TT_UNKNOWN as error in the future */

		if (token->type==TT_PREPROCESSOR) {
			skip_to_eol(fp, sb, &lno, &cno);
			continue;
		}

		if (!istobeignored(token) && token->type!=TT_NULL) {
			//(void)puts(" Added");
			tokens=(struct token_s **)realloc(tokens, ++ntokens * sizeof *tokens);
			tokens[ntokens-1]=token;
		}
	}
	tokens=(struct token_s **)realloc(tokens, ++ntokens * sizeof *tokens);
	tokens[ntokens-1]=&end_token;

	memset(&root_node, 0, sizeof root_node);
	root_node.type=NT_ROOT;
	root_node.has_token=0;

	parsed=parse(&root_node, tokens);

	gen_code(&root_node, "a.s");

	putchar('\n');

	/*(void)getchar();*/

	(void)sbfree(sb);

	/*printf("tokens=%d, parsed=%d\n", (int)ntokens-1, (int)parsed);*/

	return (ntokens-1u)==parsed ? EXIT_SUCCESS : EXIT_FAILURE;
}


