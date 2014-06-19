#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "token.h"
#include "strbuf.h"

static void error(const char *str)
{
	printf("%s", str);
	exit(EXIT_FAILURE);
}

enum {PPM_NORMAL, PPM_DIRECTIVE, PPM_INCLUDE, PPM_DEFINE, PPM_IF, PPM_IFDEF, PPM_IFNDEF, PPM_ELSE, PPM_ELIF, PPM_ENDIF, PPM_UNDEF, PPM_PRAGMA, PPM_LINE, PPM_ERROR};

static int preprocess_fp(STRBUF *sb, FILE *fp)
{
	struct token_s start_token={NULL, 0, TT_NULL, 0, 0, NULL, 0, 0};
	struct token_s *token, *prev_token=&start_token;
	int rowno=1, colno=0;
	int mode=PPM_NORMAL;
	const char *text;

	while ((token=gettoken(fp, sb, &rowno, &colno))) {
		text=token_text(token);
		if (mode==PPM_DIRECTIVE) {
			if (!strcmp(text, "include")) mode=PPM_INCLUDE;
			else if (!strcmp(text, "define")) mode=PPM_DEFINE;
			else if (!strcmp(text, "if")) mode=PPM_IF;
			else if (!strcmp(text, "ifdef")) mode=PPM_IFDEF;
			else if (!strcmp(text, "ifndef")) mode=PPM_IFNDEF;
			else if (!strcmp(text, "else")) mode=PPM_ELSE;
			else if (!strcmp(text, "elif")) mode=PPM_ELIF;
			else if (!strcmp(text, "endif")) mode=PPM_ENDIF;
			else if (!strcmp(text, "undef")) mode=PPM_UNDEF;
			else if (!strcmp(text, "pragma")) mode=PPM_PRAGMA;
			else if (!strcmp(text, "line")) mode=PPM_LINE;
			else if (!strcmp(text, "error")) mode=PPM_ERROR;
			else if (token->type==TT_WHITESPACE && token->subtype!=WTT_NEWLINEWS) continue;
			else error("Unknown preprocessor directive\n");
		} else if (token->type==TT_PREPROCESSOR) {
			if (prev_token->type==TT_WHITESPACE && prev_token->subtype==WTT_NEWLINEWS) mode=PPM_DIRECTIVE;
			else {
			}
		}
		print_token("main()", token);

		printf("[%s]\n", sbcstr(sb, token->sbix));

		prev_token=token;
	}

	return 0;
}

static int preprocess_file(STRBUF *sb, const char *fname)
{
	int ret;
	FILE *fp;

	if ((fp=fopen(fname, "r"))) {
		ret=preprocess_fp(sb, fp);
		(void)fclose(fp);
		return ret;
	} else return -1;
}

int main(int argc, char *argv[])
{
	STRBUF *sb;

	sb=sballoc(1024);

	if (argc>1) (void)preprocess_file(sb, argv[1]);
	else (void)preprocess_fp(sb, stdin);

	(void)getchar();

	(void)sbfree(sb);

	return EXIT_SUCCESS;
}


