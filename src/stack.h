#ifndef STACK
#define STACK

#include <limits.h>

#define STACK_CTOR(stk) stack_ctor((stk), #stk, __LINE__, __FILE__, __func__)

#define FMT "%d"
typedef int elem_t;
const elem_t POISON = INT_MAX;
const int INIT_CAPACITY = 2;

#ifdef CANARY_PROTECTION
typedef unsigned long long canary_t;
#endif

struct Stack {
#ifdef CANARY_PROTECTION
	canary_t left_canary;
#endif

#ifdef HASH_PROTECTION
	unsigned long hash;
	unsigned long data_hash;
#endif

	size_t capacity;
	size_t size;
	elem_t *data;
	const char *varname;
	const char *filename;
	const char *funcname;
	int line;

#ifdef CANARY_PROTECTION
	canary_t right_canary;
#endif
};

enum StackError {
	ERR_STACK_EMPTY = -3,
	STACK_FAILED = -2,
	ERR_NO_MEM = -1,
	STACK_NO_ERR = 0
};

enum StackError stack_ctor(struct Stack *stk, const char *varname, int line, 
						   const char *filename, const char *funcname);
enum StackError stack_dtor(struct Stack *stk);
enum StackError stack_push(struct Stack *stk, elem_t value);
enum StackError stack_pop(struct Stack *stk, elem_t *value);

#endif