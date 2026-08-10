
#include "stack.h"

#include <stdio.h>

STACK *stackalloc(size_t capacity)
{
	STACK *stack;

	if (!(stack=(STACK *)malloc(sizeof *stack))) return NULL;
	if (!(stack->addr=(void **)malloc(capacity * sizeof *stack->addr))) {
		free(stack);
		return NULL;
	}
	stack->capacity=capacity;
	stack->ix=0;

	return stack;
}

STACK *stackrealloc(STACK *stack, size_t capacity)
{
	void *oldaddr=stack->addr;

	if (!(stack->addr=(void **)realloc(stack->addr, capacity * sizeof *stack->addr))) {
		stack->addr=oldaddr;
		return NULL;
	}

	return stack;
}

int stackfree(STACK *stack)
{
	if (stack) {
		if (stack->addr) free(stack->addr);
		free(stack);
		return 0;
	} else return -1;
}

static STACK *stackexpand(STACK *stack, size_t needed_capacity)
{
	if (needed_capacity>stack->capacity) {
		if (needed_capacity<stack->capacity*2) stack=stackrealloc(stack, stack->capacity*2);
		else stack=stackrealloc(stack, needed_capacity);
	}

	return stack;
}

int stackpush(STACK *stack, void *data)
{
	size_t needed_capacity=stack->ix+1;

	if (!(stack=stackexpand(stack, needed_capacity))) return -1;

	stack->addr[stack->ix++]=data;

	return 0;
}

void *stacktop(STACK *stack)
{
	return stack->ix>0 ? stack->addr[stack->ix-1] : NULL;
}

void *stackpop(STACK *stack)
{
	if (stack->ix>0) return stack->addr[--stack->ix];
	else return NULL;
}

size_t stacksize(STACK *stack)
{
	return stack->ix;
}
