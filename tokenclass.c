

#include "tokenclass.h"

#include "token.h"

#include <string.h>


int istype(const struct token_s *token)
{
	return token->type==TT_TEXTUAL 
		&& (!strcmp(sbcstr(token->sb, token->sbix), "void")
			|| !strcmp(sbcstr(token->sb, token->sbix), "char")
			|| !strcmp(sbcstr(token->sb, token->sbix), "short")
			|| !strcmp(sbcstr(token->sb, token->sbix), "int")
			|| !strcmp(sbcstr(token->sb, token->sbix), "long")
			|| !strcmp(sbcstr(token->sb, token->sbix), "float")
			|| !strcmp(sbcstr(token->sb, token->sbix), "double")
			|| !strcmp(sbcstr(token->sb, token->sbix), "signed")
			|| !strcmp(sbcstr(token->sb, token->sbix), "unsigned"));
}

int isstorage(const struct token_s *token)
{
	return token->type==TT_TEXTUAL 
		&& (!strcmp(sbcstr(token->sb, token->sbix), "auto")
			|| !strcmp(sbcstr(token->sb, token->sbix), "register")
			|| !strcmp(sbcstr(token->sb, token->sbix), "static")
			|| !strcmp(sbcstr(token->sb, token->sbix), "extern"));
}

int istypedef(const struct token_s *token)
{
	return token->type==TT_TEXTUAL
		&& (!strcmp(sbcstr(token->sb, token->sbix), "typedef"));
}

#if 0
int isstruct(const struct token_s *token)
{
	return token->type==TT_TEXTUAL
		&& (!strcmp(sbcstr(token->sb, token->sbix), "struct"));
}

int isunion(const struct token_s *token)
{
	return token->type==TT_TEXTUAL
		&& (!strcmp(sbcstr(token->sb, token->sbix), "union"));
}
#endif

int isqualifier(const struct token_s *token)
{
	return token->type==TT_TEXTUAL 
		&& (!strcmp(sbcstr(token->sb, token->sbix), "const")
			|| !strcmp(sbcstr(token->sb, token->sbix), "volatile"));
}

int isname(const struct token_s *token)
{
	return token->type==TT_TEXTUAL;
}

int isnumber(const struct token_s *token)
{
	return token->type==TT_NUMBER;
}

int isws(const struct token_s *token)
{
	return token->type==TT_WHITESPACE;
}

int isop(const struct token_s *token)
{
	switch(token->type) {
	case TT_PLUS_OP:
	case TT_PLUS_ASSIGNMENT_OP:
	case TT_PLUSPLUS_OP:
	case TT_MINUS_OP:
	case TT_MINUS_ASSIGNMENT_OP:
	case TT_MINUSMINUS_OP:
	case TT_ARROW_OP:
	case TT_STAR_OP:
	case TT_STAR_ASSIGNMENT_OP:
	case TT_SLASH_OP:
	case TT_SLASH_ASSIGNMENT_OP:
	case TT_PERCENT_OP:
	case TT_PERCENT_ASSIGNMENT_OP:
	case TT_ASSIGNMENT_OP:
	case TT_EQUAL_OP:
	case TT_NEGATION_OP:
	case TT_COMPLEMENT_OP:
	case TT_COMPLEMENT_ASSIGN_OP:
	case TT_NOT_EQUAL_OP:
	case TT_LESSTHAN_OP:
	case TT_LESSTHAN_EQUAL_OP:
	case TT_GREATERTHAN_OP:
	case TT_GREATERTHAN_EQUAL_OP:
	case TT_LEFTSHIFT_OP:
	case TT_LEFTSHIFT_ASSIGNMENT_OP:
	case TT_RIGHTSHIFT_OP:
	case TT_RIGHTSHIFT_ASSIGNMENT_OP:
	//case TT_SEMICOLON_OP:				/* Not really an operator, but fits in somehow */
	case TT_COMMA_OP:
	case TT_DOT_OP:
	case TT_AND_OP:
	case TT_BIT_AND_OP:
	case TT_BIT_AND_ASSIGNMENT_OP:
	case TT_BIT_XOR_OP:
	case TT_BIT_XOR_ASSIGNMENT_OP:
	case TT_BIT_OR_OP:
	case TT_BIT_OR_ASSIGNMENT_OP:
	case TT_OR_OP:
	//case TT_LEFT_PARANTHESIS:
	//case TT_RIGHT_PARANTHESIS:
	//case TT_LEFT_SQUARE:
	//case TT_RIGHT_SQUARE:
	//case TT_LEFT_CURLY:
	//case TT_RIGHT_CURLY:
	case TT_QUESTION_OP:
	case TT_COLON_OP:
		return 1;
	default:
		return 0;
	}
}

int isstart(const struct token_s *token)
{
	switch (token->type) {
	case TT_NULL:
	case TT_LEFT_PARANTHESIS:
	case TT_LEFT_SQUARE:
	//case TT_LEFT_CURLY:
		return 1;
	default:
		return 0;
	}
}

int isend(const struct token_s *token)
{
	switch (token->type) {
	case TT_END:
	case TT_RIGHT_PARANTHESIS:
	case TT_RIGHT_SQUARE:
	//case TT_RIGHT_CURLY:
		return 1;
	default:
		return 0;
	}
}

int isleft(const struct token_s *token)
{
	switch (token->type) {
	case TT_LEFT_PARANTHESIS:
	case TT_LEFT_SQUARE:
	//case TT_LEFT_CURLY:
		return 1;
	default:
		return 0;
	}
}

int isright(const struct token_s *token)
{
	switch (token->type) {
	case TT_RIGHT_PARANTHESIS:
	case TT_RIGHT_SQUARE:
	//case TT_RIGHT_CURLY:
		return 1;
	default:
		return 0;
	}
}
int ismatchingright(const struct token_s *start, const struct token_s *end)
{
	return (start->type==TT_LEFT_PARANTHESIS && end->type==TT_RIGHT_PARANTHESIS)
		|| (start->type==TT_LEFT_SQUARE && end->type==TT_RIGHT_SQUARE)
	//	|| (start->type==TT_LEFT_CURLY && end->type==TT_RIGHT_CURLY)
		|| (start->type==TT_QUESTION_OP && end->type==TT_COLON_OP);
}

int isbracket(const struct token_s *token)
{
	switch (token->type) {
	case TT_LEFT_PARANTHESIS:
	case TT_RIGHT_PARANTHESIS:
	case TT_LEFT_SQUARE:
	case TT_RIGHT_SQUARE:
	case TT_LEFT_CURLY:
	case TT_RIGHT_CURLY:
		return 1;
	default:
		return 0;
	}
}

int isvalue(const struct token_s *token)
{
	return token->type==TT_NUMBER || token->type==TT_STRING || token->type==TT_CHARACTER;
}

