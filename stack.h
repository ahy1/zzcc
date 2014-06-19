
#ifndef STACK_H_INCLUDED
#define STACK_H_INCLUDED

#include <stdlib.h>

typedef struct {
	void **addr;
	size_t ix;
	size_t capacity;
} STACK;

STACK *stackalloc(size_t capacity);
STACK *stackrealloc(STACK *stack, size_t capacity);
int stackfree(STACK *stack);
int stackpush(STACK *stack, void *data);
void *stacktop(STACK *stack);
void *stackpop(STACK *stack);

#endif

