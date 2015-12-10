
#ifndef TOKEN_H_INCLUDED
#define TOKEN_H_INCLUDED

#include "strbuf.h"

#include <stdio.h>

enum {
	TT_NULL, 		/* Dummy token marking start of stream */
	TT_END,			/* Dummy token marking end of stream */
	TT_ERROR,
	TT_WHITESPACE, 
	TT_PREPROCESSOR,
	TT_PREPROCESSOR_CONCAT,
	TT_TEXTUAL,
	TT_RETURN,
	TT_BREAK,
	TT_CONTINUE,
	TT_GOTO,
	TT_IF,
	TT_ELSE,
	TT_WHILE,
	TT_DO,
	TT_FOR,
	TT_SWITCH,
	TT_CASE,
	TT_DEFAULT,
	TT_NUMBER, 
	TT_STRING, 
	TT_CHARACTER, 
	TT_OPERATOR,
	TT_PLUS_OP,
	TT_PLUS_ASSIGNMENT_OP,
	TT_PLUSPLUS_OP,
	TT_MINUS_OP,
	TT_MINUS_ASSIGNMENT_OP,
	TT_ARROW_OP,
	TT_MINUSMINUS_OP,
	TT_STAR_OP,
	TT_STAR_ASSIGNMENT_OP,
	TT_SLASH_OP,
	TT_SLASH_ASSIGNMENT_OP,
	TT_PERCENT_OP,
	TT_PERCENT_ASSIGNMENT_OP,
	TT_ASSIGNMENT_OP,
	TT_EQUAL_OP,
	TT_NEGATION_OP,
	TT_COMPLEMENT_OP,
	TT_COMPLEMENT_ASSIGN_OP,
	TT_NOT_EQUAL_OP,
	TT_LESSTHAN_OP,
	TT_LESSTHAN_EQUAL_OP,
	TT_GREATERTHAN_OP,
	TT_GREATERTHAN_EQUAL_OP,
	TT_LEFTSHIFT_OP,
	TT_LEFTSHIFT_ASSIGNMENT_OP,
	TT_RIGHTSHIFT_OP,
	TT_RIGHTSHIFT_ASSIGNMENT_OP,
	TT_SEMICOLON_OP,				/* Not really an operator, but fits in somehow */
	TT_COMMA_OP,
	TT_DOT_OP,
	TT_AND_OP,
	TT_BIT_AND_OP,
	TT_BIT_AND_ASSIGNMENT_OP,
	TT_BIT_XOR_OP,
	TT_BIT_XOR_ASSIGNMENT_OP,
	TT_BIT_OR_OP,
	TT_BIT_OR_ASSIGNMENT_OP,
	TT_OR_OP,
	TT_LEFT_PARANTHESIS,
	TT_RIGHT_PARANTHESIS,
	TT_LEFT_SQUARE,
	TT_RIGHT_SQUARE,
	TT_LEFT_CURLY,
	TT_RIGHT_CURLY,
	TT_QUESTION_OP,
	TT_COLON_OP,
	TT_STRUCT,
	TT_UNION,
	TT_ENUM,
	TT_INLINE,
	TT_RESTRICT,
	TT_CONST,
	TT_VOLATILE,
	TT_VOID,
	TT_CHAR,
	TT_SHORT,
	TT_INT,
	TT_LONG,
	TT_FLOAT,
	TT_DOUBLE,
	TT_SIGNED,
	TT_UNSIGNED,
	TT_BOOL,
	TT_COMPLEX,
	TT_TYPEDEF,
	TT_EXTERN,
	TT_STATIC,
	TT_AUTO,
	TT_REGISTER,
	TT_SIZEOF,
	TT_ALIGNOF,
	TT_ELIPSIS,
	TT_INCLUDE,
};

/* Numeric sub-types */
enum {
	NTT_INT,
	NTT_SHORT,
	NTT_LONG,
	NTT_LONGLONG,
	NTT_UINT,
	NTT_USHORT,
	NTT_ULONG,
	NTT_ULONGLONG,
	NTT_FLOAT,
	NTT_DOUBLE
};

/* Numeric encodings */
enum {
	NTE_DECIMAL,
	NTE_OCTAL,
	NTE_HEX
};

/* Whitespace sub-types */
enum {
	WTT_WS,
	WTT_NEWLINEWS,
	WTT_BLOCKCOMMENT,
	WTT_LINECOMMENT
};

struct token_s {
	STRBUF *sb;
	size_t sbix;
	int type;
	int subtype;
	int encoding;
	const char *fname;
	int lno;
	int cno;
};

struct token_s *gettoken(FILE *infp, STRBUF *sb, int *lno, int *cno);
struct token_s *gettoken_include(FILE *infp, STRBUF *sb, int *lno, int *cno);
int freetoken(struct token_s *token);
const char *token_text(const struct token_s *token);
const char *token_fname(const struct token_s *token);
int token_lno(const struct token_s *token);
int token_cno(const struct token_s *token);
const char *token_type(const struct token_s *token);
void print_token(const char *prefix, const struct token_s *token);


#endif

