
#include "expression.h"

#include "tokenclass.h"
#include "stack.h"

enum {ASSOC_NONE=0, ASSOC_LEFT, ASSOC_RIGHT};
enum {
	ARITY_NONE=0, 
	ARITY_LEFT_UNARY, 
	ARITY_RIGHT_UNARY, 
	ARITY_BINARY, 
	ARITY_TRINARY, 
	ARITY_GROUPING, 
	ARITY_GROUPING_END
};

struct op_s {
	int tt, sec_tt;		/* sec_tt is token type for second operator in trinary operator */
	int prec;		/* High number means higher precedence */
	int assoc;
	int arity;
} ops[]={
	{.tt=TT_PLUSPLUS_OP, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_RIGHT_UNARY},
	{.tt=TT_MINUSMINUS_OP, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_RIGHT_UNARY},
	{.tt=TT_LEFT_PARANTHESIS, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_GROUPING},	/* Function call */
	{.tt=TT_LEFT_SQUARE, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_GROUPING},	/* Array subscripting */
	{.tt=TT_RIGHT_PARANTHESIS, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_GROUPING_END},	/* Function call */
	{.tt=TT_RIGHT_SQUARE, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_GROUPING_END},	/* Array subscripting */
	{.tt=TT_DOT_OP, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_ARROW_OP, .prec=100, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_PLUSPLUS_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_MINUSMINUS_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_PLUS_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_MINUS_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_NEGATION_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_COMPLEMENT_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	/* Type cast (how to handle this) */
	{.tt=TT_STAR_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_BIT_AND_OP, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},
	{.tt=TT_SIZEOF, .prec=95, .assoc=ASSOC_RIGHT, .arity=ARITY_LEFT_UNARY},

	{.tt=TT_STAR_OP, .prec=90, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_SLASH_OP, .prec=90, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_PERCENT_OP, .prec=90, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_PLUS_OP, .prec=85, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_MINUS_OP, .prec=85, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_LEFTSHIFT_OP, .prec=80, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_RIGHTSHIFT_OP, .prec=80, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_LESSTHAN_OP, .prec=75, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_LESSTHAN_EQUAL_OP, .prec=75, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_GREATERTHAN_OP, .prec=75, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_GREATERTHAN_EQUAL_OP, .prec=75, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_EQUAL_OP, .prec=70, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},
	{.tt=TT_NOT_EQUAL_OP, .prec=70, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_BIT_AND_OP, .prec=65, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_BIT_XOR_OP, .prec=60, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_BIT_OR_OP, .prec=55, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_AND_OP, .prec=50, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_OR_OP, .prec=45, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

	{.tt=TT_QUESTION_OP, .sec_tt=TT_COLON_OP, .prec=40, .assoc=ASSOC_RIGHT, .arity=ARITY_TRINARY},

	{.tt=TT_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_PLUS_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_MINUS_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_STAR_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_SLASH_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_PERCENT_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_LEFTSHIFT_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_RIGHTSHIFT_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_BIT_AND_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_BIT_XOR_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},
	{.tt=TT_BIT_OR_ASSIGNMENT_OP, .prec=35, .assoc=ASSOC_RIGHT, .arity=ARITY_BINARY},

	{.tt=TT_COMMA_OP, .prec=30, .assoc=ASSOC_LEFT, .arity=ARITY_BINARY},

};


/* Expression parser: */
static struct op_s *getop(int tt, int arity)
{
	size_t i;

	for (i=0; i<sizeof ops/sizeof ops[0]; ++i) {
		if (tt==ops[i].tt && arity==ops[i].arity) return &ops[i];
	}

	return NULL;
}

void PUSH_RPN(struct token_s *token)
{
	/* Do nothing for now */
}

size_t Xexpression(struct node_s *parent, struct token_s **tokens)
{
	size_t /*parsed, */ix=0u;
	struct node_s *node=create_node(parent, EXPRESSION, tokens[0]);
	int done=0;
	struct op_s value={.tt=0, .prec=1000, .assoc=ASSOC_NONE, .arity=ARITY_GROUPING_END};
	struct op_s *op;
	STACK *op_stack;

	op_stack=stackalloc(16u);

	while (!done) {
		if (ix==0u || (isop(tokens[ix-1u]) && tokens[ix-1u]->type!=TT_RIGHT_PARANTHESIS)) {
			op=getop(tokens[ix]->type, ARITY_LEFT_UNARY);
			if (!op) op=getop(tokens[ix]->type, ARITY_GROUPING);
		} else {
			op=getop(tokens[ix]->type, ARITY_BINARY);
			if (!op) op=getop(tokens[ix]->type, ARITY_RIGHT_UNARY);
			if (!op) op=getop(tokens[ix]->type, ARITY_GROUPING_END);
		}
		
		if(op) {	/* Operator found */
			if (op->assoc==ASSOC_RIGHT) {
				while (stacksize(op_stack) > 0u
						&& op->prec < ((struct op_s *)stacktop(op_stack))->prec)
					PUSH_RPN(stackpop(op_stack));
			} else {
				while (stacksize(op_stack) > 0u
						&& op->prec <= ((struct op_s *)stacktop(op_stack))->prec)
					PUSH_RPN(stackpop(op_stack));
			}
			stackpush(op_stack, op);

		} else {					/* No operator found */
			op=&value;
		}
	}

	stackfree(op_stack);

	return free_node(node);
}

