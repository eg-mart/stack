#ifndef STACK
#define STACK

#include <limits.h>

#define STACK_CTOR(stk, print) stack_ctor((stk), (print), #stk, __LINE__, __FILE__, __func__)

struct Elem {
	double cost;
	int amount;
};

typedef struct Elem elem_t;

const int POISON 			= 0xAE;
const size_t INIT_CAPACITY 	= 2;
const size_t MULTIPLIER 	= 2;
const size_t SHRINK_COEF	= 4;

typedef int (*print_func)(char*, elem_t, size_t);

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
	STACK_FAILED 	= -2,
	ERR_NO_MEM 		= -1,
	STACK_NO_ERR 	= 0
};

enum StackError stack_ctor(struct Stack *stk, print_func print_elem,
						   const char *varname, int line, const char *filename,
						   const char *funcname);
enum StackError stack_dtor(struct Stack *stk);
enum StackError stack_push(struct Stack *stk, elem_t value);
enum StackError stack_pop(struct Stack *stk, elem_t *value);

#endif