#ifndef STACK_DEBUG
#define STACK_DEBUG

#include "stack.h"

#define STACK_REPORT_FAIL(stk, err) stack_report_fail((stk), (err), __FILE__,	\
													  __LINE__, __func__)

#define VALIDATE_STACK(stk) int err = 0;										\
							if (validate_stack(stk, &err) == STACK_FAILED) {	\
								STACK_REPORT_FAIL((stk), err);					\
								abort();										\
							}

#ifdef CANARY_PROTECTION
const canary_t DEFAULT_CANARY = 0xDECAFBAD;
#endif

extern print_func PRINT_ELEM;

enum StackFailure {
	NULL_STACK_POINTER	  = 0,
	CAPACITY_OVERFLOW	  = 1,
	NULL_DATA_POINTER	  = 2,
	POISONED_VALUE		  = 3,
	UNPOISONED_VALUE	  = 4,
	SMALL_CAPACITY		  = 5,
	DOUBLE_CTOR			  = 6,

#ifdef CANARY_PROTECTION
	LEFT_CANARY_BAD		  = 7,
	RIGHT_CANARY_BAD	  = 8,
	RIGHT_DATA_CANARY_BAD = 9,
	LEFT_DATA_CANARY_BAD  = 10,
#endif

#ifdef HASH_PROTECTION
	WRONG_HASH			  = 11,
	WRONG_DATA_HASH		  = 12,
#endif
};

enum StackError validate_stack(struct Stack *stk, int *err);
void stack_dump(struct Stack *stk);
void stack_report_fail(struct Stack *stk, int err,
					   const char *filename, int line, const char *func_name);

#ifdef HASH_PROTECTION
void update_hash(struct Stack *stk);
#endif

#endif