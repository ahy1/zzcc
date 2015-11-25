#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "token.h"
#include "tokenclass.h"
#include "strbuf.h"
#include "parser.h"
#include "parcom.h"

void test_int(const char *name, int expected, int actual)
{
	printf("  [%s]\n", name);

	if (expected==actual) puts("   OK");
	else printf("   FAILED: Expected %d, got %d\n", expected, actual);
}

void test_parcom(const char *testcode)
{
	int found=0;
	struct parser_s *parser;

	puts("Testing parser combinators");

	if (!testcode) testcode="ALL";

	if (!strcmp(testcode, "keyword") || !strcmp(testcode, "ALL")) {
		found=1;
		puts(" keyword (int)");
		parser=keyword("int");
		test_int("int i;\\n", 3, do_parse(parser, "int i;\n", 0));
		test_int("int", 3, do_parse(parser, "int", 0));
		test_int("integer", 0, do_parse(parser, "integer", 0));
	}
	
	if (!strcmp(testcode, "pattern-1") || !strcmp(testcode, "ALL")) {
		found=1;
		puts(" pattern ([:ident:][:idnum:]*)");
		parser=pattern("[:ident:][:idnum:]*");
		test_int("a", 1, do_parse(parser, "a", 0));
		test_int("aa", 2, do_parse(parser, "aa", 0));
		test_int("1aa", 0, do_parse(parser, "1aa", 0));
		test_int("a1aa", 4, do_parse(parser, "a1aa", 0));
	}
	
	if (!strcmp(testcode, "pattern-2") || !strcmp(testcode, "ALL")) {
		found=1;
		puts(" pattern ( *)");
		parser=pattern(" *");
		test_int(" ", 1, do_parse(parser, " ", 0));
		test_int("  ", 2, do_parse(parser, "  ", 0));
		test_int("   ", 3, do_parse(parser, "   ", 0));
		test_int(" .  ", 1, do_parse(parser, " .  ", 0));
	}
	
	if (!strcmp(testcode, "sequence") || !strcmp(testcode, "ALL")) {
		found=1;
		puts(" sequence (([:ident:][:idnum:]*), ( *), ([:digit:]))");
		parser=sequence(pattern("[:ident:][:idnum:]*"), 
			pattern(" *"), 
			pattern("[:digit:]"), 
			NULL);
		test_int("a 3", 3, do_parse(parser,"a 3", 0));
		test_int("a  3", 4, do_parse(parser,"a  3", 0));
		test_int("a1  3", 5, do_parse(parser,"a1  3", 0));
	}

	if (!strcmp(testcode, "Choice") || !strcmp(testcode, "ALL")) {
		found=1;
		puts(" choice (([:ident:][:idnum:]*), ([:digit:]*))");
		parser=choice(pattern("[:ident:][:idnum:]*"),
			pattern("[:digit:]*"),
			NULL);
		test_int("a1", 2, do_parse(parser, "a1", 0));
		test_int("99", 2, do_parse(parser, "99", 0));
		test_int("##", 0, do_parse(parser, "##", 0));
	}
	
	if (!found) fprintf(stderr, "Unknown test code %s\n", testcode);

}

void test(const char *module, const char *testcode)
{
	if (!module) {	/* Test all */
		test_parcom(testcode);
	} else if (!strcmp(module, "parcom")) test_parcom(testcode);
	else fprintf(stderr, "Unknown module name %s\n", module);
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
	const char *testmodule=NULL, *testcode=NULL;
	size_t parsed;

	if (argc>1 && !strcmp(argv[1], "-t")) {
		if (argc>2) testmodule=argv[2];
		if (argc>3) testcode=argv[3];
		test(testmodule, testcode);
		return EXIT_SUCCESS;
	}

	sb=sballoc(1024);

	if (argc>1) fp=fopen(argv[1], "r");
	else fp=stdin;

	if (!fp) {
		fprintf(stderr, "ERROR: Can\'t open file %s for reading\n", argv[1]);
		return EXIT_FAILURE;
	}

	while ((token=gettoken(fp, sb, &lno, &cno))) {
		/* TODO: Handle TT_UNKNOWN as error in the future */
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

	parsed=parse(&root_node, tokens);

	//printf(" ====== Nodes (JSON)\n");
	print_node_json(&root_node, 0);

	putchar('\n');

	/*(void)getchar();*/

	(void)sbfree(sb);

	printf("tokens=%d, parsed=%d\n", (int)ntokens-1, (int)parsed);

	return (ntokens-1u)==parsed ? EXIT_SUCCESS : EXIT_FAILURE;
}



