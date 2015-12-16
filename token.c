
#include "token.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>


static char *token_type_names[]={
	"TT_NULL",
	"TT_END",
	"TT_ERROR",
	"TT_WHITESPACE", 
	"TT_PREPROCESSOR", 
	"TT_PREPROCESSOR_CONCAT",
	"TT_TEXTUAL",
	"TT_RETURN",
	"TT_BREAK",
	"TT_CONTINUE",
	"TT_GOTO",
	"TT_IF",
	"TT_ELSE",
	"TT_WHILE",
	"TT_DO",
	"TT_FOR",
	"TT_SWITCH",
	"TT_CASE",
	"TT_DEFAULT",
	"TT_NUMBER", 
	"TT_STRING", 
	"TT_CHARACTER", 
	"TT_OPERATOR",
	"TT_PLUS_OP",
	"TT_PLUS_ASSIGNMENT_OP",
	"TT_PLUSPLUS_OP",
	"TT_MINUS_OP",
	"TT_MINUS_ASSIGNMENT_OP",
	"TT_ARROW_OP",
	"TT_MINUSMINUS_OP",
	"TT_STAR_OP",
	"TT_STAR_ASSIGNMENT_OP",
	"TT_SLASH_OP",
	"TT_SLASH_ASSIGNMENT_OP",
	"TT_PERCENT_OP",
	"TT_PERCENT_ASSIGNMENT_OP",
	"TT_ASSIGNMENT_OP",
	"TT_EQUAL_OP",
	"TT_NEGATION_OP",
	"TT_COMPLEMENT_OP",
	"TT_COMPLEMENT_ASSIGN_OP",
	"TT_NOT_EQUAL_OP",
	"TT_LESSTHAN_OP",
	"TT_LESSTHAN_EQUAL_OP",
	"TT_GREATERTHAN_OP",
	"TT_GREATERTHAN_EQUAL_OP",
	"TT_LEFTSHIFT_OP",
	"TT_LEFTSHIFT_ASSIGNMENT_OP",
	"TT_RIGHTSHIFT_OP",
	"TT_RIGHTSHIFT_ASSIGNMENT_OP",
	"TT_SEMICOLON_OP",
	"TT_COMMA_OP",
	"TT_DOT_OP",
	"TT_AND_OP",
	"TT_BIT_AND_OP",
	"TT_BIT_AND_ASSIGNMENT_OP",
	"TT_BIT_XOR_OP",
	"TT_BIT_XOR_ASSIGNMENT_OP",
	"TT_BIT_OR_OP",
	"TT_BIT_OR_ASSIGNMENT_OP",
	"TT_OR_OP",
	"TT_LEFT_PARANTHESIS",
	"TT_RIGHT_PARANTHESIS",
	"TT_LEFT_SQUARE",
	"TT_RIGHT_SQUARE",
	"TT_LEFT_CURLY",
	"TT_RIGHT_CURLY",
	"TT_QUESTION_OP",
	"TT_COLON_OP",
	"TT_STRUCT",
	"TT_UNION",
	"TT_ENUM",
	"TT_INLINE",
	"TT_RESTRICT",
	"TT_CONST",
	"TT_VOLATILE",
	"TT_VOID",
	"TT_CHAR",
	"TT_SHORT",
	"TT_INT",
	"TT_LONG",
	"TT_FLOAT",
	"TT_DOUBLE",
	"TT_SIGNED",
	"TT_UNSIGNED",
	"TT_BOOL",
	"TT_COMPLEX",
	"TT_TYPEDEF",
	"TT_EXTERN",
	"TT_STATIC",
	"TT_AUTO",
	"TT_REGISTER",
	"TT_SIZEOF",
	"TT_ALIGNOF",
	"TT_ELIPSIS",
	"TT_INCLUDE",
};

/* << Rewrite for generic lexing 
*/

enum {PT_EXACT, PT_RE};
struct pattern_s {
	int type;						/* What type of pattern matching to use */
	char *text;						/* Text specifying the token pattern (matching algorithm depends on type) */
	int token_type, token_subtype, token_encoding;		/* What to report for this token */
	int state;						/* Lexer state (internal) */
};
#if 0
static struct token_s *fetchtoken(FILE *infp, STRBUF *bf, struct pattern_s *patterns, size_t npatterns)
{
	struct token_s *token;
	int ch;
	int chix=0;
	size_t pix=0;
	struct pattern_s *pattern;

	if (!(token=(struct token_s *)calloc(1, sizeof *token))) return NULL;

	while ((ch=getchar())!=EOF) {
		/* Loop through patterns to find all matches */

		for (pix=0; pix<npatterns; ++pix) {
			pattern=&patterns[pix];
			if (pattern->type==PT_EXACT) {
			}
		}

		++chix;
	}

	return token;
}
#endif
/* >>
*/

static int last_cno;

static int incpos(int ch, int *lno, int *cno)
{
	last_cno=*cno;

	if (ch=='\n') return ++*lno, *cno=1;
	else return ++*cno;
}

static int fetch(FILE *fp, int *lno, int *cno)
{
	int ch=getc(fp);

	(void)incpos(ch, lno, cno);

	return ch;
}

static int decpos(int ch, int *lno, int *cno)
{
	if (ch=='\n') return --*lno, *cno=last_cno;
	else return --*cno;
}

static int unfetch(int ch, FILE *fp, int *lno, int *cno)
{
	(void)decpos(ch, lno, cno);

	return ungetc(ch, fp);
}

struct token_s *gettoken(FILE *infp, STRBUF *sb, int *lno, int *cno)
{
	struct token_s *token;
	int ch;

	if (!(token=(struct token_s *)calloc(1, sizeof *token))) return NULL;

	token->sb=sb;
	token->sbix=sbix(sb);
	token->lno=*lno;
	token->cno=*cno;

	if ((ch=fetch(infp, lno, cno))==EOF) return NULL;

	switch (ch) {
	case ' ':
	case '\t':
	case '\n':
	case '\r':
	case '\f':
		/* Whitespace */
		token->type=TT_WHITESPACE;
		if (ch=='\n') token->subtype=WTT_NEWLINEWS;
		else token->subtype=WTT_WS;
		(void)sbput(token->sb, ch);
		while (isspace((ch=fetch(infp, lno, cno)))) {
			if (ch=='\n') token->subtype=WTT_NEWLINEWS;
			(void)sbput(token->sb, ch);
		}
		(void)unfetch(ch, infp, lno, cno);
		break;
	case '_': 
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': 
	case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': 
	case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
		/* Identifyer or keyword */
		token->type=TT_TEXTUAL;
		(void)sbput(token->sb, ch);
		while (isalnum((ch=fetch(infp, lno, cno))) || ch=='_') {(void)sbput(token->sb, ch);}

		if (!strcmp(sbcstr(token->sb, token->sbix), "return")) token->type=TT_RETURN;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "break")) token->type=TT_BREAK;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "continue")) token->type=TT_CONTINUE;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "goto")) token->type=TT_GOTO;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "if")) token->type=TT_IF;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "else")) token->type=TT_ELSE;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "while")) token->type=TT_WHILE;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "do")) token->type=TT_DO;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "for")) token->type=TT_FOR;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "switch")) token->type=TT_SWITCH;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "case")) token->type=TT_CASE;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "default")) token->type=TT_DEFAULT;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "struct")) token->type=TT_STRUCT;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "union")) token->type=TT_UNION;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "enum")) token->type=TT_ENUM;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "inline")) token->type=TT_INLINE;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "__inline")) token->type=TT_INLINE;		/* GNU thing */
		else if (!strcmp(sbcstr(token->sb, token->sbix), "restrict")) token->type=TT_RESTRICT;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "__restrict")) token->type=TT_RESTRICT;	/* GNU thing */
		else if (!strcmp(sbcstr(token->sb, token->sbix), "const")) token->type=TT_CONST;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "volatile")) token->type=TT_VOLATILE;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "void")) token->type=TT_VOID;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "char")) token->type=TT_CHAR;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "short")) token->type=TT_SHORT;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "int")) token->type=TT_INT;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "long")) token->type=TT_LONG;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "float")) token->type=TT_FLOAT;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "double")) token->type=TT_DOUBLE;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "signed")) token->type=TT_SIGNED;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "unsigned")) token->type=TT_UNSIGNED;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "_Bool")) token->type=TT_BOOL;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "_Complex")) token->type=TT_COMPLEX;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "typedef")) token->type=TT_TYPEDEF;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "extern")) token->type=TT_EXTERN;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "static")) token->type=TT_STATIC;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "auto")) token->type=TT_AUTO;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "register")) token->type=TT_REGISTER;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "sizeof")) token->type=TT_SIZEOF;
		else if (!strcmp(sbcstr(token->sb, token->sbix), "_Alignof")) token->type=TT_ALIGNOF;
		(void)unfetch(ch, infp, lno, cno);
		break;
	case '0':
		/* Octal or hexadecimal number */
		token->type=TT_NUMBER;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='x' || ch=='X') {
			token->encoding=NTE_HEX;
			(void)sbput(token->sb, ch);
		} else {
			token->encoding=NTE_OCTAL;
			if (isdigit(ch)) {
				(void)sbput(token->sb, ch);
				while(isdigit((ch=fetch(infp, lno, cno)))) {sbput(token->sb, ch);}
			}
			(void)unfetch(ch, infp, lno, cno);		// TODO: Check for postfix size specifier
		}
		break;
	case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
		/* Number */
		token->type=TT_NUMBER;
		(void)sbput(token->sb, ch);
		while (isdigit((ch=fetch(infp, lno, cno)))) {(void)sbput(token->sb, ch);}
		if (ch=='.') {
			(void)sbput(token->sb, ch);
			while (isdigit((ch=fetch(infp, lno, cno)))) {(void)sbput(token->sb, ch);}
			(void)unfetch(ch, infp, lno, cno);		// TODO: Check for postfix size specifier
		} else (void)unfetch(ch, infp, lno, cno);	// TODO: Check for postfix size specifier
		break;
	case '\"':
		/* String literal */
		token->type=TT_STRING;
		/*(void)sbput(token->sb, ch);*/
		while ((ch=fetch(infp, lno, cno))!='\"') {
			if (ch=='\\') {
				(void)sbput(token->sb, ch);
				ch=fetch(infp, lno, cno);
			}
			(void)sbput(token->sb, ch);
		}
		break;
	case '\'':
		/* Character literal */
		token->type=TT_CHARACTER;
		/*(void)sbput(token->sb, ch);*/
		while ((ch=fetch(infp, lno, cno))!='\'') {
			if (ch=='\\') {
				(void)sbput(token->sb, ch);
				ch=fetch(infp, lno, cno);
			}
			(void)sbput(token->sb, ch);
		}
		break;
	case '+':
		/* Addition operator */
		token->type=TT_PLUS_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='+') {
			token->type=TT_PLUSPLUS_OP;
			(void)sbput(token->sb, ch);
		} else if (ch=='=') {
			token->type=TT_PLUS_ASSIGNMENT_OP;
			(void)sbput(token->sb, ch);
		} else (void)unfetch(ch, infp, lno, cno);
		break;
	case '-':
		/* Subtraction or struct pointer member operator */
		token->type=TT_MINUS_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='-') {
			token->type=TT_MINUSMINUS_OP;
			(void)sbput(token->sb, ch);
		} else if (ch=='=') {
			token->type=TT_MINUS_ASSIGNMENT_OP;
			(void)sbput(token->sb, ch);
		} else if (ch=='>') {
			token->type=TT_ARROW_OP;
			(void)sbput(token->sb, ch);
		}  else (void)unfetch(ch, infp, lno, cno);
		break;
	case '*':
		/* Multiplication or address dereference operator */
		token->type=TT_STAR_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='=') {
			token->type=TT_STAR_ASSIGNMENT_OP;
			(void)sbput(token->sb, ch);
		} else (void)unfetch(ch, infp, lno, cno);
		break;
	case '/':
		/* Division operator or comment*/
		token->type=TT_SLASH_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='=') {
			token->type=TT_SLASH_ASSIGNMENT_OP;
			(void)sbput(token->sb, ch);
		} else if (ch=='/') {
			token->type=TT_WHITESPACE;
			token->subtype=WTT_LINECOMMENT;
			(void)sbput(token->sb, ch);
			while ((ch=fetch(infp, lno, cno))!=EOF && ch!='\n') {(void)sbput(token->sb, ch);}
			(void)sbput(token->sb, ch);
		} else if (ch=='*') {
			token->type=TT_WHITESPACE;
			token->subtype=WTT_BLOCKCOMMENT;
			(void)sbput(token->sb, ch);
			while (1) {
				while ((ch=fetch(infp, lno, cno))!=EOF && ch!='*') {(void)sbput(token->sb, ch);}
				(void)sbput(token->sb, ch);
				if ((ch=fetch(infp, lno, cno))=='/') break;
			}
			(void)sbput(token->sb, ch);
		} else (void)unfetch(ch, infp, lno, cno);
		break;
	case '%':
		/* Remainder operator */
		token->type=TT_PERCENT_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='=') {
			token->type=TT_PERCENT_ASSIGNMENT_OP;
			(void)sbput(token->sb, ch);
		} else (void)unfetch(ch, infp, lno, cno);
		break;
	case '=':
		/* Assignment or equal operator */
		token->type=TT_ASSIGNMENT_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='=') {
			token->type=TT_EQUAL_OP;
			(void)sbput(token->sb, ch);
		} else (void)unfetch(ch, infp, lno, cno);
		break;
	case '!':
		/* Negation or not equal operator */
		token->type=TT_NEGATION_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='=') {
			token->type=TT_NOT_EQUAL_OP;
			(void)sbput(token->sb, ch);
		} else (void)unfetch(ch, infp, lno, cno);
		break;
	case '~':
		/* Complement */
		token->type=TT_COMPLEMENT_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='=') {
			token->type=TT_COMPLEMENT_ASSIGN_OP;
			(void)sbput(token->sb, ch);
		} else (void)unfetch(ch, infp, lno, cno);
		break;
	case '<':
		/* Less-than or left-shift operator */
		token->type=TT_LESSTHAN_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='<') {
			token->type=TT_LEFTSHIFT_OP;
			if ((ch=fetch(infp, lno, cno))=='=') {
				token->type=TT_LEFTSHIFT_ASSIGNMENT_OP;
				(void)sbput(token->sb, ch);
			} else (void)unfetch(ch, infp, lno, cno);
		} else if (ch=='=') {
			token->type=TT_LESSTHAN_EQUAL_OP;
			(void)sbput(token->sb, ch);
		} else (void)unfetch(ch, infp, lno, cno);
		break;
	case '>':
		/* Greater-than or right-shift operator */
		token->type=TT_GREATERTHAN_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='>') {
			token->type=TT_RIGHTSHIFT_OP;
			(void)sbput(token->sb, ch);
			if ((ch=fetch(infp, lno, cno))=='=') {
				token->type=TT_RIGHTSHIFT_ASSIGNMENT_OP;
				(void)sbput(token->sb, ch);
			} else (void)unfetch(ch, infp, lno, cno);
		} else if (ch=='=') {
			token->type=TT_GREATERTHAN_EQUAL_OP;
			(void)sbput(token->sb, ch);
		}
		else (void)unfetch(ch, infp, lno, cno);
		break;
	case ';':
		token->type=TT_SEMICOLON_OP;
		(void)sbput(token->sb, ch);
		break;
	case ',':
		token->type=TT_COMMA_OP;
		(void)sbput(token->sb, ch);
		break;
	case '.':
		token->type=TT_DOT_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='.') {
			(void)sbput(token->sb, ch);
			if ((ch=fetch(infp, lno, cno))=='.') {
				token->type=TT_ELIPSIS;
				(void)sbput(token->sb, ch);
			} else {
				(void)unfetch(ch, infp, lno, cno);	/* TODO: Handle case when ".." (not elipsis, just 2 dots) */
			}
		} else (void)unfetch(ch, infp, lno, cno);
		break;
	case '&':
		token->type=TT_BIT_AND_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='=') {
			token->type=TT_BIT_AND_ASSIGNMENT_OP;
			(void)sbput(token->sb, ch);
		} else if (ch=='&') {
			token->type=TT_AND_OP;
			(void)sbput(token->sb, ch);
		} else (void)unfetch(ch, infp, lno, cno);
		break;
	case '^':
		token->type=TT_BIT_XOR_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='=') {
			token->type=TT_BIT_XOR_ASSIGNMENT_OP;
			(void)sbput(token->sb, ch);
		} else (void)unfetch(ch, infp, lno, cno);
		break;
	case '|':
		token->type=TT_BIT_OR_OP;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='=') {
			token->type=TT_BIT_OR_ASSIGNMENT_OP;
			(void)sbput(token->sb, ch);
		} else if (ch=='|') {
			token->type=TT_OR_OP;
			(void)sbput(token->sb, ch);
		} else (void)unfetch(ch, infp, lno, cno);
		break;
	case '(':
		token->type=TT_LEFT_PARANTHESIS;
		(void)sbput(token->sb, ch);
		break;
	case ')':
		token->type=TT_RIGHT_PARANTHESIS;
		(void)sbput(token->sb, ch);
		break;
	case '[':
		token->type=TT_LEFT_SQUARE;
		(void)sbput(token->sb, ch);
		break;
	case ']':
		token->type=TT_RIGHT_SQUARE;
		(void)sbput(token->sb, ch);
		break;
	case '{':
		token->type=TT_LEFT_CURLY;
		(void)sbput(token->sb, ch);
		break;
	case '}':
		token->type=TT_RIGHT_CURLY;
		(void)sbput(token->sb, ch);
		break;
	case '?':
		token->type=TT_QUESTION_OP;
		(void)sbput(token->sb, ch);
		break;
	case ':':
		token->type=TT_COLON_OP;
		(void)sbput(token->sb, ch);
		break;
	case '#':
		token->type=TT_PREPROCESSOR;
		(void)sbput(token->sb, ch);
		if ((ch=fetch(infp, lno, cno))=='#') {
			token->type=TT_PREPROCESSOR_CONCAT;
			(void)sbput(token->sb, ch);
		} else (void)unfetch(ch, infp, lno, cno);
		break;
	}

	(void)sbstop(token->sb);

	return token;
}

struct token_s *gettoken_include(FILE *infp, STRBUF *sb, int *lno, int *cno)
{
	struct token_s *token;
	int ch;

	if (!(token=(struct token_s *)calloc(1, sizeof *token))) return NULL;

	token->sb=sb;
	token->sbix=sbix(sb);
	token->lno=*lno;
	token->cno=*cno;

	if ((ch=fetch(infp, lno, cno))==EOF) return NULL;

	if (ch=='<') {
		token->type=TT_INCLUDE;
		while ((ch=fetch(infp, lno, cno))!='>' && ch!='\n') {
			(void)sbput(token->sb, ch);
		}
		(void)sbstop(token->sb);
		return token;
	} else {
		(void)unfetch(ch, infp, lno, cno);
		free(token);
		return gettoken(infp, sb, lno, cno);
	}
}

int freetoken(struct token_s *token)
{
	free(token);
	return 0;
}


const char *token_text(const struct token_s *token)
{
	if (!token) return "<NULL>";
	if (!token->sb) return "<NULL>";
	return sbcstr(token->sb, token->sbix);
}

const char *token_fname(const struct token_s *token)
{
	if (!token) return "<NULL>";
	if (!token->fname) return "-";
	return token->fname;
}

int token_lno(const struct token_s *token)
{
	if (!token) return -1;
	return token->lno;
}

int token_cno(const struct token_s *token)
{
	if (!token) return -1;
	return token->cno;
}

const char *token_type(const struct token_s *token)
{
	if (!token) return "<no-token>";
	return token_type_names[token->type];
}

void print_token(const char *prefix, const struct token_s *token)
{
	printf("%s %d:%d [%s](%d/%d)\n", prefix, token->lno, token->cno, token_text(token), token->type, token->subtype);
}


