#ifndef STACK
#define STACK

#include <limits.h>

#define STACK_CTOR(stk) stack_ctor((stk), #stk, __LINE__, __FILE__, __func__)

#define FMT "%d"
typedef int elem_t;
const elem_t POISON = INT_MAX;

struct Stack {
	size_t capacity;
	size_t size;
	elem_t *data;
	const char *varname;
	const char *filename;
	const char *funcname;
	int line;
};

enum StackError {
	ERR_STACK_EMPTY = -3,
	STACK_FAILED = -2,
	ERR_NO_MEM = -1,
	STACK_NO_ERR = 0
};

struct StackFailure {
	unsigned int null_stack_pointer : 1;
	unsigned int capacity_overflow : 1;
	unsigned int null_data_pointer : 1;
	unsigned int poisoned_value : 1;
	unsigned int unpoisoned_value : 1;
	unsigned int small_capacity : 1;
};

enum StackError stack_ctor(struct Stack *stk, const char *varname, int line, 
						   const char *filename, const char *funcname);
enum StackError stack_dtor(struct Stack *stk);
enum StackError stack_push(struct Stack *stk, elem_t value);
enum StackError stack_pop(struct Stack *stk, elem_t *value);
void stack_dump(const struct Stack *stk);

#endif
