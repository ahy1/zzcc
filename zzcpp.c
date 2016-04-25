#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <libgen.h>	/* dirname() */
#include <limits.h>

#include "token.h"
#include "strbuf.h"
#include "json.h"

static int preprocess_file(STRBUF *sb, const char *fname);

static void failure(const char *str)
{
	printf("%s", str);
	exit(EXIT_FAILURE);
}

static void failuref(const char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	vfprintf(stderr, fmt, arg);
	va_end(arg);

	exit(EXIT_FAILURE);
}

enum {PPM_NORMAL,
	PPM_DIRECTIVE,
	PPM_INCLUDE,
	PPM_DEFINE,
	PPM_DEFINE_VALUE,
	PPM_IF,
	PPM_IFDEF,
	PPM_IFNDEF,
	PPM_ELSE,
	PPM_ELIF,
	PPM_ENDIF,
	PPM_UNDEF,
	PPM_PRAGMA,
	PPM_LINE,
	PPM_ERROR};

struct define_s {
	struct token_s *name;
	struct token_s **values;
	int nvalues;
};

struct define_s *defines=NULL;
size_t ndefines=0;

int *conditionals=NULL;
size_t nconditionals=0;

static struct define_s *get_define(struct token_s *token)
{
	int n;

	for (n=0; n<ndefines; ++n) {
		fprintf(stderr, " >> get_define() [%s] [%s]\n", token_text(token), token_text(defines[n].name));
		if (!strcmp(token_text(token), token_text(defines[n].name))) return &defines[n];
	}

	return NULL;
}

static void put_token(struct token_s *token)
{
	struct define_s *define;
	int n;
	static char b[1024*1024];

	if ((define=get_define(token))) {
		fprintf(stderr, " **** YES!\n");
		for (n=0; n<define->nvalues; ++n) {
			put_token(define->values[n]);
		}
	} else {
		if (token->type!=TT_WHITESPACE)
			printf("[%s]\n", json_str(token_text(token), b, 1024*1024));
	}
}

static int file_exists(const char *fname)
{
    FILE *fp;
    if ((fp=fopen(fname, "r"))) {
        fclose(fp);
        return 1;
    } else return 0;
}

static char *lib_file_paths[]={
	"/usr/include"
};

static char *prj_file_paths[]={
	"."
};

static const char *lookup_file(size_t npaths, char *paths[], const char *fname)
{
	size_t i;
	static char fpath[PATH_MAX];	/* TODO: Static ok? */

	for (i=0u; i<npaths; ++i) {
		strcpy(fpath, paths[i]);
		strcat(fpath, "/");
		strcat(fpath, fname);	/* TODO: Don't trust PATH_MAX */

		if (file_exists(fpath)) return fpath;	/* TODO: Add path to check */
	}

	return NULL;
}

static const char *lookup_lib_file(const char *fname)
{
	return lookup_file(sizeof lib_file_paths/sizeof lib_file_paths[0], lib_file_paths, fname);
}

static const char *lookup_prj_file(const char *fname)
{
	return lookup_file(sizeof prj_file_paths/sizeof prj_file_paths[0], prj_file_paths, fname);
}

static int preprocess_fp(STRBUF *sb, FILE *fp)
{
	struct token_s start_token={NULL, 0, TT_NULL, 0, 0, NULL, 0, 0};
	struct token_s *token, *prev_token=&start_token;
	int lno=1, cno=0;
	int mode=PPM_NORMAL;
	const char *text;
	struct token_s *define_name=NULL;
	struct token_s **define_values=NULL;
	int define_nvalues=0;
	const char *fpath;
	size_t ix;

	while ((token=
		(mode==PPM_INCLUDE 
			?gettoken_include(fp, sb, &lno, &cno)
			:gettoken(fp, sb, &lno, &cno)))) {
		text=token_text(token);

		switch (mode) {
		case PPM_NORMAL:
			fprintf(stderr, "m NORMAL [%s]\n", token_text(token));
			if (token->type==TT_PREPROCESSOR) {
				fprintf(stderr, "tt PREPROCESSOR [%s]\n", token_text(token));
				if (prev_token->type==TT_WHITESPACE 
					&& prev_token->subtype==WTT_NEWLINEWS)
					mode=PPM_DIRECTIVE;
			} else if (token->type==TT_PREPROCESSOR_CONCAT) {
				/* TODO: */
			} else if (token->type!=TT_WHITESPACE) {
				put_token(token);
			}
			break;
		case PPM_DIRECTIVE:
			fprintf(stderr, "m DIRECTIVE [%s]\n", token_text(token));
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
			else if (token->type==TT_WHITESPACE 
				&& token->subtype!=WTT_NEWLINEWS) continue;
			else failure("Unknown preprocessor directive\n");
			break;
		case PPM_INCLUDE:
			fprintf(stderr, "m INCLUDE [%s]\n", token_text(token));
			if (token->type!=TT_WHITESPACE) {
				if (token->type==TT_INCLUDE) {
					if(!(fpath=lookup_lib_file(token_text(token))))
						failuref("Can\'t find file %s\n", token_text(token));
				} else if (token->type==TT_STRING) {
					if(!(fpath=lookup_prj_file(token_text(token))))
						failuref("Can\'t find file %s\n", token_text(token));
				} else failure("Expected include specification\n");

				preprocess_file(sb, fpath);

				mode=PPM_NORMAL;
			}
			break;
		case PPM_DEFINE:
			fprintf(stderr, "m DEFINE [%s]\n", token_text(token));
			if (token->type!=TT_WHITESPACE) {
				define_name=token;
				mode=PPM_DEFINE_VALUE;
			}
			break;
		case PPM_DEFINE_VALUE:
			fprintf(stderr, "m DEFINE_VALUE [%s]\n", token_text(token));
			if (token->type==TT_WHITESPACE) {
				if (token->subtype==WTT_NEWLINEWS) {
					fprintf(stderr, " -- End of define value\n");
					defines=realloc(defines, ++ndefines * sizeof defines[0]);
					defines[ndefines-1].name=define_name;
					defines[ndefines-1].values=define_values;
					defines[ndefines-1].nvalues=define_nvalues;

					define_name=NULL;
					define_values=NULL;
					define_nvalues=0;
					mode=PPM_NORMAL;
				}
			} else {
				fprintf(stderr, " -- Define value token\n");
				define_values=realloc(define_values, ++define_nvalues * sizeof define_values[0]);
				define_values[define_nvalues-1]=token;
			}
			break;
		case PPM_IF:
			/* TODO: Push expression */
			break;
		case PPM_IFDEF:
			fprintf(stderr, "m IFDEF [%s]\n", token_text(token));
			if (get_define(token)) {
				fprintf(stderr, "m IFDEF [%s] Found it\n", token_text(token));
				/* TODO: Push true */
			} else {
				fprintf(stderr, "m IFDEF [%s] Didn\'t find it\n", token_text(token));
				/* TODO: Push false */
			}
			break;
		case PPM_IFNDEF:
			fprintf(stderr, "m IFNDEF [%s]\n", token_text(token));
			if (get_define(token)) {
				fprintf(stderr, "m IFNDEF [%s] Found it\n", token_text(token));
				/* TODO: Push false */
			} else {
				fprintf(stderr, "m IFNDEF [%s] Didn\'t find it\n", token_text(token));
				/* TODO: Push true */
			}
			break;
		case PPM_ELSE:
			/* TODO: Pop, then push opposite */
			break;
		case PPM_ELIF:
			/* TODO: Pop, then push expression && opposite */
			break;
		case PPM_ENDIF:
			/* TODO: Pop */
			break;
		case PPM_UNDEF:
			for (ix=0; ix<ndefines; ++ix) {
				if (!strcmp(token_text(defines[ndefines].name), token_text(token))) {
					defines[ndefines].name=NULL;
				}
			}
			break;
		case PPM_PRAGMA:
			break;
		case PPM_LINE:
			break;
		case PPM_ERROR:
			fprintf(stderr, "ERROR: %s\n", token_text(token));
			exit(EXIT_FAILURE);
		default:
			if (token->type==TT_PREPROCESSOR) {
				if (prev_token->type==TT_WHITESPACE && prev_token->subtype==WTT_NEWLINEWS) mode=PPM_DIRECTIVE;
				else {
				}
			}
		}
		/*print_token("main()", token);*/

		/*printf("[%s]\n", sbcstr(sb, token->sbix));*/

		prev_token=token;
	}

	return 0;
}

static int preprocess_file(STRBUF *sb, const char *fname)
{
	int ret;
	FILE *fp;

	fprintf(stderr, "preprocess_file(%s)\n", fname);

	if ((fp=fopen(fname, "r"))) {
		ret=preprocess_fp(sb, fp);
		(void)fclose(fp);
		return ret;
	} else {
		failuref("Can\'t open file %s\n", fname);
		return -1;
	}
}

int main(int argc, char *argv[])
{
	STRBUF *sb;

	sb=sballoc(1024);

	if (argc>1) {
		prj_file_paths[0]=dirname(strdup(argv[1]));

		(void)preprocess_file(sb, argv[1]);
	} else (void)preprocess_fp(sb, stdin);

	(void)sbfree(sb);

	return EXIT_SUCCESS;
}


