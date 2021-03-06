
#include "parser.h"
#include "tokenclass.h"

#include <stdlib.h>
#include <string.h>

/*#define LOG_PARSER(s) fprintf(stderr, " >> %s (%d) >> %s >> ix=%d >> %s\n", __func__, node->level, (s), (int)ix, token_text(tokens[ix]))*/
#define LOG_PARSER(s) 

struct node_s *g_root_node=NULL;

static int add_typealias_text(struct node_s *scope_parent, const char *text)
{
	scope_parent->typealiases=(const char **)realloc(
		scope_parent->typealiases, ++scope_parent->ntypealiases * sizeof(struct node_s *));
	scope_parent->typealiases[scope_parent->ntypealiases-1]=text;

	return 0;
}

static int add_typealias_node(struct node_s *node)
{
	const char *text=token_text(node->token);

	log_node_token(node, node->token, "@ add_typealias(): Adding to %s - %s\n", 
		node_type_name(node->scope_parent));

	return add_typealias_text(node->scope_parent, text);
}

static int istypealias(const struct node_s *node, const struct token_s *token)
{
	size_t ix;

	if (node->scope_parent) {
		for (ix=0; ix<node->scope_parent->ntypealiases; ++ix) {
			if (!strcmp(token_text(token), node->scope_parent->typealiases[ix])) return 1;
		}
		return istypealias(node->scope_parent, token);
	}

	return 0;
}

/* Helper parsers: */

static size_t separated(struct node_s *parent, struct token_s **tokens, int nt,
	size_t (*pf)(struct node_s *, struct token_s **),
	int tt_separator, int opt_end_separator, int mergeable)
{
	/* opt_end_separator: 1 => include opt. end sep., 2=> don't include */
	size_t parsed, ix=0u;
	struct node_s *node=mergeable ? create_mergeable_node(parent, nt, tokens[0]) : create_node(parent, nt, tokens[0]);

	LOG_PARSER("1");

	if ((parsed=pf(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	LOG_PARSER("2");
	while (tokens[ix]->type==tt_separator) {
		++ix;
		LOG_PARSER("2a");

		if ((parsed=pf(node, tokens+ix))) ix+=parsed;
		else {
			if(opt_end_separator==1) return add_node(node), ix;
			else if (opt_end_separator==2) return add_node(node), ix-1;
			else error_node_token(node, tokens[ix], "[separated()] Unexpexted parse");
		}
		LOG_PARSER("2b");
	}

	LOG_PARSER("3");
	return add_node(node), ix;
}

static int is_any_of(int what, size_t num_alts, int *alts)
{
	size_t i;

	for (i=0u; i<num_alts; ++i) {
		if (what==alts[i]) return 1;
	}

	return 0;
}

static size_t separated_any_token(struct node_s *parent, struct token_s **tokens, int nt,
	size_t (*pf)(struct node_s *, struct token_s **),
	size_t num_separators, int *tt_separators,
	int mergeable)
{
	size_t parsed, ix=0u;
	struct node_s *node=mergeable ? create_mergeable_node(parent, nt, tokens[0]) : create_node(parent, nt, tokens[0]);
	int separator_cnt=0;

	if ((parsed=pf(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	while (is_any_of(tokens[ix]->type, num_separators, tt_separators)) {
		++ix;
		++separator_cnt;

		if ((parsed=pf(node, tokens+ix))) ix+=parsed;
		else {
			if (separator_cnt>1) return add_node(node), ix-1;
			else return free_node(node);
		}
	}

	return add_node(node), ix;
}

static size_t many(struct node_s *parent, struct token_s **tokens, int nt,
	size_t (*pf)(struct node_s *, struct token_s **), int mergeable)
{
	size_t parsed, ix=0u;
	struct node_s *node=mergeable ? create_mergeable_node(parent, nt, tokens[0]) : create_node(parent, nt, tokens[0]);

	while ((parsed=pf(node, tokens+ix))) ix+=parsed;

	if (ix>0u) return add_node(node), ix;
	else return free_node(node);
}

static size_t any_of_2(struct node_s *parent, struct token_s **tokens, int nt,
	size_t (*pf1)(struct node_s *, struct token_s **),
	size_t (*pf2)(struct node_s *, struct token_s **),
	int mergeable)
{
	size_t parsed;
	struct node_s *node=mergeable ? create_mergeable_node(parent, nt, tokens[0]) : create_node(parent, nt, tokens[0]);

	if ((parsed=pf1(node, tokens)) || (parsed=pf2(node, tokens))) return add_node(node), parsed;
	else return free_node(node);
}

static size_t optional_postfix_token(struct node_s *parent, struct token_s **tokens, int nt,
	size_t (*pf)(struct node_s *, struct token_s **), int tt)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, nt, tokens[0]);

	if ((parsed=pf(node, tokens+ix))) ix+=parsed;

	if (tokens[ix]->type==tt) ++ix;
	else return free_node(node);

	return add_node(node), ix;
}

static size_t single_token(struct node_s *parent, struct token_s **tokens, int nt, int tt)
{
	struct node_s *node=create_node(parent, nt, tokens[0]);

	return tokens[0]->type==tt ? add_node(node),1u : free_node(node);
}

static size_t single_token_any_of(struct node_s *parent, struct token_s **tokens, int nt, size_t ntts, int *tts)
{
	struct node_s *node=create_node(parent, nt, tokens[0]);

	if (is_any_of(tokens[0]->type, ntts, tts)) return add_node(node), 1u;
	else return free_node(node);
}

static size_t prepostfix_tokens(struct node_s *parent, struct token_s **tokens, int nt,
	size_t (*pf)(struct node_s *, struct token_s **), int pre_tt, int post_tt, int mergeable)
{
	size_t parsed, ix=0u;
	struct node_s *node=mergeable ? create_mergeable_node(parent, nt, tokens[0]) : create_node(parent, nt, tokens[0]);

	LOG_PARSER("1");
	if (tokens[ix]->type==pre_tt) ++ix;
	else return free_node(node);

	LOG_PARSER("2");
	if ((parsed=pf(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	LOG_PARSER("3");
	if (tokens[ix]->type==post_tt) ++ix;
	else return free_node(node);

	LOG_PARSER("4");
	return add_node(node), ix;
}

/* Actual parsers: */

static size_t identifier(struct node_s *parent, struct token_s **tokens);
static size_t type_specifier(struct node_s *parent, struct token_s **tokens, int *include_typedef);
static size_t expression(struct node_s *parent, struct token_s **tokens);
static size_t statement(struct node_s *parent, struct token_s **tokens);
static size_t compound_statement(struct node_s *parent, struct token_s **tokens);
static size_t declarator(struct node_s *parent, struct token_s **tokens);
static size_t declaration_specifiers(struct node_s *parent, struct token_s **tokens);
static size_t pointer(struct node_s *parent, struct token_s **tokens);
static size_t constant_expression(struct node_s *parent, struct token_s **tokens);
static size_t initializer(struct node_s *parent, struct token_s **tokens);
static size_t cast_expression(struct node_s *parent, struct token_s **tokens);
static size_t unary_expression(struct node_s *parent, struct token_s **tokens);
static size_t specifier_qualifier_list(struct node_s *parent, struct token_s **tokens);
static size_t abstract_declarator(struct node_s *parent, struct token_s **tokens);
static size_t parameter_type_list(struct node_s *parent, struct token_s **tokens);
static size_t type_qualifier(struct node_s *parent, struct token_s **tokens);
static size_t assignment_expression(struct node_s *parent, struct token_s **tokens);


static size_t attribute(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, NT_NON_STANDARD_ATTRIBUTE, tokens[0]);

	if (tokens[ix]->type==TT_TEXTUAL && !strcmp(token_text(tokens[ix]), "__attribute__")) ++ix;
	else return free_node(node);

	if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
	else error_node_token(node, tokens[ix], "Expected left paranthesis");

	if ((parsed=expression(node, tokens+ix))) ix+=parsed;
	else error_node_token(node, tokens[ix], "Expected expression");

	if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
	else error_node_token(node, tokens[ix], "Expected right paranthesis");

	return /*add_node(node), */ix;
}

static size_t inline_asm(struct node_s *parent, struct token_s **tokens)
{
	size_t ix=0u;
	struct node_s *node=create_node(parent, NT_NON_STANDARD_ASM, tokens[0]);

	LOG_PARSER("Start");

	if (tokens[ix]->type==TT_TEXTUAL && !strcmp(token_text(tokens[ix]), "__asm__")) ++ix;
	else return free_node(node);

	if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
	else error_node_token(node, tokens[ix], "Expected left paranthesis");

	while (tokens[ix]->type==TT_STRING) ++ix;

	if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
	else error_node_token(node, tokens[ix], "Expected right paranthesis");

	LOG_PARSER("Returning");

	return /*add_node(node), */ix;
}

static size_t multiplicative_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated_any_token(parent, tokens, MULTIPLICATIVE_EXPRESSION, 
		cast_expression, 3, (int []) {TT_STAR_OP, TT_SLASH_OP, TT_PERCENT_OP}, 1);
}

static size_t additive_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated_any_token(parent, tokens, ADDITIVE_EXPRESSION, 
		multiplicative_expression, 2, (int []) {TT_PLUS_OP, TT_MINUS_OP}, 1);
}

static size_t shift_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated_any_token(parent, tokens, SHIFT_EXPRESSION, 
		additive_expression, 2, (int []) {TT_LEFTSHIFT_OP, TT_RIGHTSHIFT_OP}, 1);
}

static size_t relational_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated_any_token(parent, tokens, RELATIONAL_EXPRESSION, 
		shift_expression,
		4, (int []) {TT_LESSTHAN_OP, TT_GREATERTHAN_OP, TT_LESSTHAN_EQUAL_OP, TT_GREATERTHAN_EQUAL_OP}, 1);
}

static size_t equality_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated_any_token(parent, tokens, EQUALITY_EXPRESSION, 
		relational_expression, 2, (int []) {TT_EQUAL_OP, TT_NOT_EQUAL_OP}, 1);
}

static size_t and_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, AND_EXPRESSION, equality_expression, TT_BIT_AND_OP, 0, 1);
}

static size_t exclusive_or_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, EXCLUSIVE_OR_EXPRESSION, and_expression, TT_BIT_XOR_OP, 0, 1);
}

static size_t inclusive_or_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, INCLUSIVE_OR_EXPRESSION, exclusive_or_expression, TT_BIT_OR_OP, 0, 1);
}

static size_t logical_and_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, LOGICAL_AND_EXPRESSION, inclusive_or_expression, TT_AND_OP, 0, 1);
}

static size_t logical_or_expression(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, LOGICAL_OR_EXPRESSION, logical_and_expression, TT_OR_OP, 0, 1);
}

static size_t conditional_expression(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_mergeable_node(parent, CONDITIONAL_EXPRESSION, tokens[0]);

	LOG_PARSER("Start");

	if ((parsed=logical_or_expression(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if (tokens[ix]->type==TT_QUESTION_OP) {
		++ix;

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "[conditional_expression()] Expected expression");

		if (tokens[ix]->type==TT_COLON_OP) ++ix;
		else error_node_token(node, tokens[ix], "[conditional_expression()] Expected colon");

		if ((parsed=conditional_expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "[conditional_expression()] Expected conditional-expression");
	}

	LOG_PARSER("Returning");

	return add_node(node), ix;
}

static size_t primary_expression(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed;
	struct node_s *node=create_mergeable_node(parent, PRIMARY_EXPRESSION, tokens[0]);

	if ((parsed=identifier(node, tokens))
		|| (parsed=single_token_any_of(node, tokens, CONSTANT, 2, (int []){TT_CHARACTER, TT_NUMBER}))
		|| (parsed=single_token(node, tokens, STRING_LITERAL, TT_STRING))) return add_node(node), parsed;
	else if ((parsed=prepostfix_tokens(node, tokens, PRIMARY_EXPRESSION,
		expression, TT_LEFT_PARANTHESIS, TT_RIGHT_PARANTHESIS, 1))) return add_node(node), parsed;
	else return free_node(node);
}

size_t type_name(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, TYPE_NAME, tokens[0]);

	if ((parsed=specifier_qualifier_list(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if ((parsed=abstract_declarator(node, tokens+ix))) ix+=parsed;

	return add_node(node), ix;
}

static size_t designator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DESIGNATOR, tokens[0]);

	if (tokens[ix]->type==TT_LEFT_SQUARE) {
		++ix;

		if ((parsed=constant_expression(node, tokens+ix))) ix+=parsed;
		else return free_node(node);

		if (tokens[ix]->type==TT_RIGHT_SQUARE) ++ix;
		else return free_node(node);
	} else if (tokens[ix]->type==TT_DOT_OP) {
		++ix;

		if ((parsed=identifier(node, tokens+ix))) ix+=parsed;
		else return free_node(node);
	} else return free_node(node);

	return add_node(node), ix;
}

static size_t designation(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DESIGNATION, tokens[0]);

	if ((parsed=designator(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if (tokens[ix]->type==TT_ASSIGNMENT_OP) ++ix;
	else return free_node(node);

	return add_node(node), ix;
}

static size_t designation_initializer(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DESIGNATION_INITIALIZER, tokens[0]);

	if ((parsed=designation(node, tokens+ix))) ix+=parsed;

	if ((parsed=initializer(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	return add_node(node), ix;
}

static size_t initializer_list(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, INITIALIZER_LIST, designation_initializer, TT_COMMA_OP, 1, 0);
}

static size_t postfix_expression(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_mergeable_node(parent, POSTFIX_EXPRESSION, tokens[0]);
	int found_postfix;

	LOG_PARSER("Start");

	if ((parsed=primary_expression(node, tokens+ix))) ix+=parsed;
	else if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
		++ix;

		LOG_PARSER("Found left paren");

		if ((parsed=type_name(node, tokens+ix))) ix+=parsed;
		else return free_node(node);

		LOG_PARSER("Parsed type name");

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else return free_node(node);

		LOG_PARSER("Found right paren");

		if (tokens[ix]->type==TT_LEFT_CURLY) ++ix;
		else return free_node(node);

		LOG_PARSER("Found left curly");

		if ((parsed=initializer_list(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "[postfix_expression()] Expected initializer-list");

		LOG_PARSER("Parsed initializer list");

		if (tokens[ix]->type==TT_RIGHT_CURLY) ++ix;
		else error_node_token(node, tokens[ix], "[postfix_expression()] Expected closing curly bracket");

		LOG_PARSER("Found right curly");

	}

	LOG_PARSER("Moving on to actual postfix part");
      
	/* The actual postfix part */	
	if (ix>0u) {
		LOG_PARSER("Yes we have parsed something already");
		found_postfix=1;

		do {
			switch(tokens[ix]->type) {
			case TT_LEFT_SQUARE:
				++ix;
				if ((parsed=expression(node, tokens+ix))) ix+=parsed;
				else error_node_token(node, tokens[ix], "[postfix_expression()] Expected expression");
				if (tokens[ix]->type==TT_RIGHT_SQUARE) ++ix;
				else error_node_token(node, tokens[ix], "[postfix_expression()] Expected closing square bracket");
				break;
			case TT_LEFT_PARANTHESIS:
				++ix;
				if ((parsed=separated(node, tokens+ix, ARGUMENT_EXPRESSION_LIST, assignment_expression, TT_COMMA_OP, 0, 0))) 
					ix+=parsed;
				if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
				else error_node_token(node, tokens[ix], "[postfix_expression()] Expected closing paranthesis");
				break;
			case TT_DOT_OP:
			case TT_ARROW_OP:
				++ix;
				if ((parsed=identifier(node, tokens+ix))) ix+=parsed;
				else error_node_token(node, tokens[ix], "[postfix_expression()] Expected identifier");
				break;
			case TT_PLUSPLUS_OP:
				++ix;
				break;
			case TT_MINUSMINUS_OP:
				++ix;
				break;
			default:
				found_postfix=0;
			}
		} while (found_postfix);

		LOG_PARSER("Returning");
		return add_node(node), ix;
	} else return free_node(node);
}

static size_t unary_operator(struct node_s *parent, struct token_s **tokens)
{
	int tts[]={TT_BIT_AND_OP, TT_STAR_OP, TT_PLUS_OP, TT_MINUS_OP, TT_COMPLEMENT_OP, TT_NEGATION_OP};
	struct node_s *node=create_node(parent, UNARY_OPERATOR, tokens[0]);

	if (is_any_of(tokens[0]->type, sizeof tts/sizeof tts[0], tts)) return add_node(node), 1u;
	else return free_node(node);
}

static size_t cast_expression(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_mergeable_node(parent, CAST_EXPRESSION, tokens[0]);

	/*
	 * : unary_expression
	 * | ( type_name ) cast_expression
	 */

	LOG_PARSER("start");

	if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
		++ix;

		LOG_PARSER("Found left paren");
		if ((parsed=type_name(node, tokens+ix))) {
			ix+=parsed;

			LOG_PARSER("Parsed type name");

			if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
			else error_node_token(node, tokens[ix], "Expected closing paranthesis");

			LOG_PARSER("Found right paren");

			if ((parsed=cast_expression(node, tokens+ix))) {
				ix+=parsed;
				return add_node(node), ix;
			}

			LOG_PARSER("Gave up (type_name) cast_expression");
		}
		LOG_PARSER("No left parenthesis starting ( type_name ) cast_expression");
	}

	ix=0u;
	LOG_PARSER("Trying unary_expression");

	if ((parsed=unary_expression(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	LOG_PARSER("After trying unary_expression");

	return add_node(node), ix;
}

static size_t unary_expression(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_mergeable_node(parent, UNARY_EXPRESSION, tokens[0]);

	/*
	 * : postfix_expression
	 * | ++ unary_expression
	 * | -- unary_expression
	 * | unary_operator cast_expression
	 * | sizeof unary_expression
	 * | sizeof ( type_name )
	 * | alignof ( type_name )
	 */

	LOG_PARSER("Start");

	if ((parsed=postfix_expression(node, tokens+ix))) ix+=parsed;
	else if (tokens[ix]->type==TT_PLUSPLUS_OP || tokens[ix]->type==TT_MINUSMINUS_OP) {
		++ix;

		LOG_PARSER("Found ++/--");
		
		if ((parsed=unary_expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "[unary_expression()] Expected unary-expression");
	} else if ((parsed=unary_operator(node, tokens+ix))) {
		ix+=parsed;

		LOG_PARSER("Parsed unary operator");

		if ((parsed=cast_expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "[unary_expression()] Expected cast-expression");
	} else if (tokens[ix]->type==TT_SIZEOF) {
		++ix;

		LOG_PARSER("Found sizeof");

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
			++ix;

			if ((parsed=type_name(node, tokens+ix))) ix+=parsed;
			else if ((parsed=unary_expression(node, tokens+ix))) ix+=parsed;	/* TODO: This is ugly. Refactor! */
			else error_node_token(node, tokens[ix], "[unary_expression()] Expected type-name");

			if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
			else error_node_token(node, tokens[ix], "[unary_expression()] Expected right paranthesis");
		} else if ((parsed=unary_expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "[unary_expression()] Expected unary-expression or paranthesized type-name");
	} else if (tokens[ix]->type==TT_ALIGNOF) {
		++ix;

		LOG_PARSER("Found alignof");

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
			++ix;

			if ((parsed=type_name(node, tokens+ix))) ix+=parsed;
			else error_node_token(node, tokens[ix], "[unary_expression()] Expected type-name");

			if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
			else error_node_token(node, tokens[ix], "[unary_expression()] Expected right paranthesis");
		} else error_node_token(node, tokens[ix], "[unary_expression()] Expected paranthesized type-name");
	} else return free_node(node);

	LOG_PARSER("Returning");

	return add_node(node), ix;
}

static size_t assignment_expression(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_mergeable_node(parent, ASSIGNMENT_EXPRESSION, tokens[0]);

	LOG_PARSER("Start");

	/* Skip attribute to avoid parsing it as assignment expression */
	if ((parsed=attribute(node, tokens+ix))) ix+=parsed;

	set_node_token(node, tokens[ix]);

	if ((parsed=unary_expression(node, tokens+ix))) {
		ix+=parsed;
		LOG_PARSER("1");

		if (is_any_of(tokens[ix]->type, 11, (int []){
				TT_ASSIGNMENT_OP,
				TT_STAR_ASSIGNMENT_OP,
				TT_SLASH_ASSIGNMENT_OP,
				TT_PERCENT_ASSIGNMENT_OP,
				TT_PLUS_ASSIGNMENT_OP,
				TT_MINUS_ASSIGNMENT_OP,
				TT_LEFTSHIFT_ASSIGNMENT_OP,
				TT_RIGHTSHIFT_ASSIGNMENT_OP,
				TT_BIT_AND_ASSIGNMENT_OP,
				TT_BIT_XOR_ASSIGNMENT_OP,
				TT_BIT_OR_ASSIGNMENT_OP})) {
			++ix;
			LOG_PARSER("1a");

			if ((parsed=assignment_expression(node, tokens+ix))) ix+=parsed;
			else error_node_token(node, tokens[ix], 
					"[assignment_expression()] Expected assignment-expression");

			LOG_PARSER("1b");

			return add_node(node), ix;
		} else ix=0u;
	} 

	LOG_PARSER("Trying conditional?");
	
	if ((parsed=conditional_expression(node, tokens+ix))) {
		ix+=parsed;
		LOG_PARSER("Parsed conditional expression");
	}

	LOG_PARSER("Returning");

	return ix>0u ? add_node(node), ix : free_node(node);
}

static size_t direct_abstract_declarator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DIRECT_ABSTRACT_DECLARATOR, tokens[0]);
	int tt;

	if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
		++ix;

		if ((parsed=abstract_declarator(node, tokens+ix))) ix+=parsed;
		else return free_node(node);

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected closing paranthesis");
	}

	while ((tt=tokens[ix]->type)==TT_LEFT_SQUARE || tokens[ix]->type==TT_LEFT_PARANTHESIS) {
		++ix;

		if (tt==TT_LEFT_SQUARE) {
			if (tokens[ix]->type==TT_STAR_OP) ++ix;
			else if ((parsed=assignment_expression(node, tokens+ix))) ix+=parsed;

			if (tokens[ix]->type==TT_RIGHT_SQUARE) ++ix;
			else error_node_token(node, tokens[ix], "Expected closing square bracket");
		} else if (tt==TT_LEFT_PARANTHESIS) {
			if ((parsed=parameter_type_list(node, tokens+ix))) ix+=parsed;
			else error_node_token(node, tokens[ix], "Expected parameter type list");

			if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
			else error_node_token(node, tokens[ix], "Expected closing paranthesis");
		}	/* No other type is possible */
	}

	return ix>0u ? (add_node(node), ix) : free_node(node);
}

static size_t abstract_declarator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, ABSTRACT_DECLARATOR, tokens[0]);

	if ((parsed=pointer(node, tokens+ix))) ix+=parsed;

	if ((parsed=direct_abstract_declarator(node, tokens+ix))) ix+=parsed;

	if (ix>0u) return add_node(node), ix;
	else return free_node(node);
}

static size_t parameter_declaration(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, PARAMETER_DECLARATION, tokens[0]);

	if ((parsed=declaration_specifiers(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if ((parsed=declarator(node, tokens+ix))
		|| (parsed=abstract_declarator(node, tokens+ix))) {
		ix+=parsed;

		return add_node(node), ix;
	}

	return add_node(node), ix;
}

static size_t parameter_type_list(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, PARAMETER_TYPE_LIST, tokens[0]);

	if ((parsed=separated(node, tokens+ix, PARAMETER_LIST, parameter_declaration, TT_COMMA_OP, 2, 0))) ix+=parsed;
	else return free_node(node);

	if (tokens[ix]->type==TT_COMMA_OP) {
		++ix;

		if (tokens[ix]->type==TT_ELLIPSIS) ++ix;
		else return free_node(node);
	}

	return add_node(node), ix;
}

static size_t specifier_qualifier_list(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, SPECIFIER_QUALIFIER_LIST, tokens[0]);
	int include_typedef=1;

	while ((parsed=type_specifier(node, tokens+ix, &include_typedef)) || (parsed=type_qualifier(node, tokens+ix)))
		ix+=parsed;

	return ix>0u ? (add_node(node), ix) : free_node(node);
}

static size_t struct_declarator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, STRUCT_DECLARATOR, tokens[0]);

	if ((parsed=declarator(node, tokens+ix))) ix+=parsed;

	if (tokens[ix]->type==TT_COLON_OP) {
		++ix;

		if ((parsed=constant_expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected constant-expression");
	}

	if (ix>0u) return add_node(node), ix;
	else return free_node(node);
}

static size_t struct_declaration(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, STRUCT_DECLARATION, tokens[0]);

	LOG_PARSER("Start");

	if ((parsed=specifier_qualifier_list(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	LOG_PARSER("Parsed specifier qualifier list");

	if ((parsed=separated(node, tokens+ix, STRUCT_DECLARATOR_LIST, struct_declarator, TT_COMMA_OP, 0, 0))) ix+=parsed;

	LOG_PARSER("Parsed struct declarator list");

	if (tokens[ix]->type==TT_SEMICOLON_OP) ++ix;
	else return free_node(node);

	LOG_PARSER("Found semicolon");

	return add_node(node), ix;
}

static size_t struct_or_union_specifier(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	int id_or_list=0;
	struct node_s *node=create_node(parent, STRUCT_OR_UNION_SPECIFIER, tokens[0]);

	if ((parsed=single_token_any_of(node, tokens, STRUCT_OR_UNION, 2u, (int []){TT_STRUCT, TT_UNION}))) ix+=parsed;
	else return free_node(node);

	if ((parsed=identifier(node, tokens+ix))) {ix+=parsed; id_or_list=1;}

	if (tokens[ix]->type==TT_LEFT_CURLY) {
		++ix;
		id_or_list=1;

		if ((parsed=many(node, tokens+ix, STRUCT_DECLARATION_LIST, struct_declaration, 0))) ix+=parsed;

		if (tokens[ix]->type==TT_RIGHT_CURLY) ++ix;
		else error_node_token(node, tokens[ix], "Expected right curly brace");
	}

	if (!id_or_list) error_node_token(node, tokens[ix], "Expected struct/union tag name or struct declaration list");

	return add_node(node), ix;
}

static size_t enumerator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, ENUMERATOR, tokens[0]);

	if ((parsed=identifier(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if (tokens[ix]->type==TT_ASSIGNMENT_OP) {
		++ix;
	
		if ((parsed=constant_expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected constant expression");
	}

	return add_node(node), ix;
}

static size_t enum_specifier(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, ENUM_SPECIFIER, tokens[0]);

	if (tokens[ix]->type==TT_ENUM) {
		++ix;

		if ((parsed=identifier(node, tokens+ix))) ix+=parsed;

		if (tokens[ix]->type==TT_LEFT_CURLY) {
			++ix;

			if ((parsed=separated(node, tokens+ix, ENUMERATOR_LIST, enumerator, TT_COMMA_OP, 1, 0))) ix+=parsed;
			else error_node_token(node, tokens[ix], "Expected enumerator list");

			if (tokens[ix]->type==TT_COMMA_OP) ++ix;
			
			if (tokens[ix]->type==TT_RIGHT_CURLY) ++ix;
			else error_node_token(node, tokens[ix], "Expected closing brace");
		}

		if (ix < 2) error_node_token(node, tokens[ix], "Expected identifier or enumerator list after enum");

		return add_node(node), ix;
	} else return free_node(node);
}

static size_t typedef_name(struct node_s *parent, struct token_s **tokens)
{
	struct node_s *node=create_node(parent, TYPEDEF_NAME, tokens[0]);

	if (istypealias(node, tokens[0])) return add_node(node), 1u;
	else return free_node(node);
}

static size_t type_specifier(struct node_s *parent, struct token_s **tokens, int *include_typedef)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, TYPE_SPECIFIER, tokens[0]);

	switch(tokens[ix]->type) {
	case TT_VOID:
	case TT_CHAR: case TT_SHORT: case TT_INT: case TT_LONG:
	case TT_FLOAT: case TT_DOUBLE:
	case TT_SIGNED: case TT_UNSIGNED:
	case TT_BOOL:
	case TT_COMPLEX:
		return *include_typedef=0, add_node(node), 1u;
	default:
		if ((parsed=struct_or_union_specifier(node, tokens+ix))
			|| (parsed=enum_specifier(node, tokens+ix))) return *include_typedef=0, add_node(node), parsed;
		else if(*include_typedef && (parsed=typedef_name(node, tokens+ix))) return *include_typedef=0, add_node(node), parsed;
		else return free_node(node);
	}
}

static size_t type_qualifier(struct node_s *parent, struct token_s **tokens)
{
	struct node_s *node=create_node(parent, TYPE_QUALIFIER, tokens[0]);

	if (tokens[0]->type==TT_CONST
		|| tokens[0]->type==TT_RESTRICT
		|| tokens[0]->type==TT_VOLATILE) return add_node(node), 1u;
	else return free_node(node);
}

static size_t function_specifier(struct node_s *parent, struct token_s **tokens)
{
	struct node_s *node=create_node(parent, FUNCTION_SPECIFIER, tokens[0]);

	return is_any_of(tokens[0]->type, 2, (int []){TT_INLINE, TT_NORETURN})
			? add_node(node), 1u
			: free_node(node);

}

static size_t alignment_specifier(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0; 
	struct node_s *node=create_node(parent, ALIGNMENT_SPECIFIER, tokens[0]);

	if (tokens[ix]->type==TT_ALIGNOF) {
		++ix;

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected opening paranthesis");

		if ((parsed=type_name(node, tokens+ix))
				|| (parsed=constant_expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected type name or constant expression");

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected closing paranthesis");

		return add_node(node), ix;
	} else return free_node(node);
}

static size_t type_qualifier_list(struct node_s *parent, struct token_s **tokens)
{
	return many(parent, tokens, TYPE_QUALIFIER_LIST, type_qualifier, 0);
}

static size_t declaration_specifiers(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DECLARATION_SPECIFIERS, tokens[0]);
	int include_typedef=1;

	/* TODO: Cleanup and update to C11 */

	while ((parsed=single_token_any_of(node, tokens+ix, STORAGE_CLASS_SPECIFIER, 5u, (int []){
			TT_TYPEDEF, TT_EXTERN, TT_STATIC, TT_AUTO, TT_REGISTER}))
		|| (parsed=type_specifier(node, tokens+ix, &include_typedef))
		|| (parsed=type_qualifier(node, tokens+ix))
		|| (parsed=function_specifier(node, tokens+ix))
		|| (parsed=single_token(node, tokens+ix, FUNCTION_SPECIFIER, TT_INLINE))
		|| (parsed=alignment_specifier(node, tokens+ix))
		|| (parsed=attribute(node, tokens+ix))) ix+=parsed;

	if (ix>0u) return add_node(node), ix;
	else return free_node(node);
}

static size_t pointer(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, POINTER, tokens[0]);

	if (tokens[ix]->type==TT_STAR_OP) ++ix;
	else return free_node(node);

	if ((parsed=type_qualifier_list(node, tokens+ix))) ix+=parsed;

	if ((parsed=pointer(node, tokens+ix))) ix+=parsed;

	return add_node(node), ix;
}

static size_t identifier(struct node_s *parent, struct token_s **tokens)
{
	struct node_s *node=create_node(parent, IDENTIFIER, tokens[0]);

	if (tokens[0]->type==TT_TEXTUAL) {
		if (node->in_typedef_declaration) {
			add_typealias_node(node);
		}

		return add_node(node), 1u;
	} else return free_node(node);
}

static size_t direct_declarator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DIRECT_DECLARATOR, tokens[0]);
	int tt;

	/* identifier 
	| "(" declarator ")" 
	| direct-declarator "[" type-qualifier-list? assignment-expression? "]"
	| direct-declarator "[" "static" type-qualifier-list? assignment-expression? "]"
	| direct-declarator "[" type-qualifier-list? "static" assignment-expression? "]"
	| direct-declarator "[" type-qualifier-list? "*" "]"
	| direct-declarator "(" parameter-type-list ")"
	| direct-declarator "(" identifier-list? ")" */

	if ((parsed=identifier(node, tokens+ix))) {
		ix+=parsed;
	} else if (tokens[ix]->type==TT_LEFT_PARANTHESIS) {
		++ix;

		if ((parsed=declarator(node, tokens+ix))) {
			ix+=parsed;

			if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
			else return free_node(node);
		} else return free_node(node);
	}

	while ((tt=tokens[ix]->type)==TT_LEFT_PARANTHESIS || (tt=tokens[ix]->type)==TT_LEFT_SQUARE) {
		++ix;
		if (tt==TT_LEFT_SQUARE) {
			if (tokens[ix]->type==TT_STATIC) ++ix;

			if ((parsed=type_qualifier_list(node, tokens+ix))) ix+=parsed;

			/* TODO: Handle type-qualifier-list "*" */

			if (tokens[ix]->type==TT_STATIC) ++ix;

			if ((parsed=assignment_expression(node, tokens+ix))) ix+=parsed;

			if (tokens[ix]->type==TT_RIGHT_SQUARE) ++ix;
			else error_node_token(node, tokens[ix], "Expected right square bracket");
		} else {	/* Paranthesis */
			if ((parsed=parameter_type_list(node, tokens+ix))) {
				ix+=parsed;
			} else if ((parsed=separated(node, tokens+ix, IDENTIFIER_LIST, identifier, TT_COMMA_OP, 0, 0))) ix+=parsed;

			if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
			else return free_node(node);

			if ((parsed=inline_asm(node, tokens+ix))) ix+=parsed;	/* GNU __asm__ ( .. ) at end of function decl. */
		}
	}

	if(ix>0u) return add_node(node), ix;
	else return free_node(node);
}

static size_t declarator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DECLARATOR, tokens[0]);

	if ((parsed=attribute(node, tokens+ix))) ix+=parsed;

	if ((parsed=pointer(node, tokens+ix))) ix+=parsed;

	if ((parsed=attribute(node, tokens+ix))) ix+=parsed;

	set_node_token(node, tokens[ix]);

	if ((parsed=direct_declarator(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if ((parsed=attribute(node, tokens+ix))) ix+=parsed;

	return add_node(node), ix;
}

static size_t initializer(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, INITIALIZER, tokens[0]);

	LOG_PARSER("Start");

	if ((parsed=assignment_expression(node, tokens+ix))) ix+=parsed;
	else if (tokens[ix]->type==TT_LEFT_CURLY) {
		++ix;

		LOG_PARSER("Found left curly");

		if ((parsed=initializer_list(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Exprected initializer list");

		LOG_PARSER("Parsed initializer list");

		if (tokens[ix]->type==TT_RIGHT_CURLY) ++ix;
		else error_node_token(node, tokens[ix], "Expected closing curly brace");

		LOG_PARSER("Founde right curly");
	} else return free_node(node);

	LOG_PARSER("Returning");

	return add_node(node), ix;
}

static size_t init_declarator(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, INIT_DECLARATOR, tokens[0]);

	LOG_PARSER("Start");

	if ((parsed=declarator(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	LOG_PARSER("Parsed declarator");

	if (tokens[ix]->type==TT_ASSIGNMENT_OP) {
		++ix;

		LOG_PARSER("Found assignment op");

		if ((parsed=initializer(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected initializer");

		LOG_PARSER("Parsed initializer");
	}

	LOG_PARSER("Returning");

	return add_node(node), ix;
}

static size_t declaration(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, DECLARATION, tokens[0]);

	if (tokens[0]->type==TT_TYPEDEF) node->in_typedef_declaration=1;

	if ((parsed=declaration_specifiers(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if ((parsed=separated(node, tokens+ix, INIT_DECLARATOR_LIST, init_declarator, TT_COMMA_OP, 0, 0))) ix+=parsed;

	if (tokens[ix]->type==TT_SEMICOLON_OP) ++ix;
	else return free_node(node);

	return add_node(node), ix;
}

static size_t constant_expression(struct node_s *parent, struct token_s **tokens)
{
	return conditional_expression(parent, tokens);	/* TODO: Use node type for cond. expr. */
}

static size_t labeled_statement(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, LABELED_STATEMENT, tokens[0]);

	if ((parsed=identifier(node, tokens+ix))) {
		ix+=parsed;

		if (tokens[ix]->type==TT_COLON_OP) ++ix;
		else return free_node(node);
	} else if (tokens[ix]->type==TT_CASE) {
		++ix;

		if ((parsed=constant_expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected constant expression after \"case\"");

		if (tokens[ix]->type==TT_COLON_OP) ++ix;
		else error_node_token(node, tokens[ix], "Expected colon - : after constant expression in \"case\" statement");
	} else if (tokens[ix]->type==TT_DEFAULT) {
		++ix;

		if (tokens[ix]->type==TT_COLON_OP) ++ix;
		else error_node_token(node, tokens[ix], "Expected colon - : after \"default\"");
	} else return free_node(node);

	if ((parsed=statement(node, tokens+ix))) ix+=parsed;	/* Made this optional */

	return add_node(node), ix;
}

static size_t expression(struct node_s *parent, struct token_s **tokens)
{
	return separated(parent, tokens, EXPRESSION, assignment_expression, TT_COMMA_OP, 0, 1);
}

static size_t selection_statement(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, SELECTION_STATEMENT, tokens[0]);

	if (tokens[ix]->type==TT_IF) {
		++ix;

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected left paranthesis - ( after if");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected condition expression for if statement");

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected right paranthesis - ) after condition expression");

		if ((parsed=statement(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected statement in if statement");

		if (tokens[ix]->type==TT_ELSE) {
			++ix;

			if ((parsed=statement(node, tokens+ix))) ix+=parsed;
			else error_node_token(node, tokens[ix], "Expected statement for else clause in if statement");
		}

		return add_node(node), ix;
	} else if (tokens[ix]->type==TT_SWITCH) {
		++ix;

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected left paranthesis - ( after switch");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected condition expression for switch statement");

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected right paranthesis - ) after condition expression");

		if ((parsed=statement(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected statement in switch statement");

		return add_node(node), ix;
	} else return free_node(node);
}

static size_t iteration_statement(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, ITERATION_STATEMENT, tokens[0]);

	if (tokens[ix]->type==TT_WHILE) {
		++ix;

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected left paranthesis");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected expression");

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected right parantheses");

		if ((parsed=statement(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected statement");
	} else if (tokens[ix]->type==TT_DO) {
		++ix;

		if ((parsed=statement(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected statement");

		if (tokens[ix]->type==TT_WHILE) ++ix;
		else error_node_token(node, tokens[ix], "Expected \"while\" keyword");

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected left paranthesis");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected expression");

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected right parantheses");

		if (tokens[ix]->type==TT_SEMICOLON_OP) ++ix;
		else error_node_token(node, tokens[ix], "Expected semicolon");
	} else if (tokens[ix]->type==TT_FOR) {
		++ix;

		if (tokens[ix]->type==TT_LEFT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected left paranthesis");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else if ((parsed=declaration(node, tokens+ix))) ix+=parsed;

		if (tokens[ix]->type==TT_SEMICOLON_OP) ++ix;
		else error_node_token(node, tokens[ix], "Expected semicolon");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;

		if (tokens[ix]->type==TT_SEMICOLON_OP) ++ix;
		else error_node_token(node, tokens[ix], "Expected semicolon");

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;

		if (tokens[ix]->type==TT_RIGHT_PARANTHESIS) ++ix;
		else error_node_token(node, tokens[ix], "Expected right paranthesis");

		if ((parsed=statement(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected statement");
	} else return free_node(node);

	return add_node(node), ix;
}

static size_t jump_statement(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, JUMP_STATEMENT, tokens[0]);

	if (tokens[ix]->type==TT_GOTO) {
		if (tokens[++ix]->type==TT_TEXTUAL && tokens[++ix]->type==TT_SEMICOLON_OP) return add_node(node), ++ix;
		else error_node_token(node, tokens[ix], "Expected identifier and semicolon after goto");
	} else if (tokens[ix]->type==TT_CONTINUE || tokens[ix]->type==TT_BREAK) {
		if (tokens[++ix]->type==TT_SEMICOLON_OP) return add_node(node), ++ix;
		else error_node_token(node, tokens[ix], "Expected semicolon after continue/break");
	} else if (tokens[ix]->type==TT_RETURN) {
		++ix;

		if ((parsed=expression(node, tokens+ix))) ix+=parsed;
		else error_node_token(node, tokens[ix], "Expected expression after return");

		if (tokens[ix]->type==TT_SEMICOLON_OP) return add_node(node), ++ix;
		else error_node_token(node, tokens[ix], "Expected semicolon after return expression");
	}

	return free_node(node);
}

static size_t statement(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, STATEMENT, tokens[0]);

	if ((parsed=labeled_statement(node, tokens+ix))
		|| (parsed=compound_statement(node, tokens+ix))
		|| (parsed=optional_postfix_token(node, tokens+ix, EXPRESSION_STATEMENT, expression, TT_SEMICOLON_OP))
		|| (parsed=selection_statement(node, tokens+ix))
		|| (parsed=iteration_statement(node, tokens+ix))
		|| (parsed=jump_statement(node, tokens+ix))) return add_node(node), parsed;
	else return free_node(node);
}

static size_t block_item(struct node_s *parent, struct token_s **tokens)
{
	/* Node-less parse rule */
	size_t parsed;

	if ((parsed=declaration(parent, tokens)) || (parsed=statement(parent, tokens)))
		return parsed;
	else return 0u;
}

static size_t compound_statement(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, COMPOUND_STATEMENT, tokens[0]);

	if (tokens[ix]->type==TT_LEFT_CURLY) ++ix;
	else return free_node(node);

	if ((parsed=many(node, tokens+ix, BLOCK_ITEM_LIST, block_item, 1))) ix+=parsed;

	if (tokens[ix]->type==TT_RIGHT_CURLY) ++ix;
	else error_node_token(node, tokens[ix], "Missing right curly brace - } at end of compound statement");

	return add_node(node), ix;
}

static size_t function_definition(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, FUNCTION_DEFINITION, tokens[0]);

	if ((parsed=declaration_specifiers(node, tokens+ix))) ix+=parsed;

	if ((parsed=declarator(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	if ((parsed=many(node, tokens+ix, DECLARATION_LIST, declaration, 0))) ix+=parsed;

	if ((parsed=compound_statement(node, tokens+ix))) ix+=parsed;
	else return free_node(node);

	return add_node(node), ix;
}

static size_t translation_unit(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed, ix=0u;
	struct node_s *node=create_node(parent, TRANSLATION_UNIT, tokens[0]);

	add_typealias_text(node, "__builtin_va_list");

	while ((parsed=any_of_2(node, tokens+ix, EXTERNAL_DECLARATION, function_definition, declaration, 1))) ix+=parsed;

	return add_node(node), ix;
}

size_t parse(struct node_s *parent, struct token_s **tokens)
{
	size_t parsed;
	struct node_s *node;

	g_root_node=node=create_node(parent, NT_UNIT, tokens[0]);

	if ((parsed=translation_unit(node, tokens))) return add_node(node), parsed;
	else return free_node(node);
}


