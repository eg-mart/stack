#ifndef STACK_DEBUG
#define STACK_DEBUG

#include "stack.h"

#define STACK_REPORT_FAIL(stk, err) stack_report_fail((stk), (err), __FILE__,	\
													  __LINE__, __func__)

#define VALIDATE_STACK(stk) struct StackFailure err = {};						\
							if (validate_stack(stk, &err) == STACK_FAILED) {	\
								STACK_REPORT_FAIL((stk), err);					\
								abort();										\
							}

#ifdef CANARY_PROTECTION
const canary_t DEFAULT_CANARY = 0xDECAFBAD;
#endif

struct StackFailure {
	unsigned int null_stack_pointer : 1;
	unsigned int capacity_overflow : 1;
	unsigned int null_data_pointer : 1;
	unsigned int poisoned_value : 1;
	unsigned int unpoisoned_value : 1;
	unsigned int small_capacity : 1;
	unsigned int double_ctor : 1;

#ifdef CANARY_PROTECTION
	unsigned int left_canary_bad : 1;
	unsigned int right_canary_bad : 1;
	unsigned int right_data_canary_bad : 1;
	unsigned int left_data_canary_bad : 1;
#endif

#ifdef HASH_PROTECTION
	unsigned int wrong_hash : 1;
	unsigned int wrong_data_hash : 1;
#endif
};

enum StackError validate_stack(struct Stack *stk, struct StackFailure *err);
void stack_dump(struct Stack *stk);
void stack_report_fail(struct Stack *stk, struct StackFailure err,
					   const char *filename, int line, const char *func_name);

#ifdef HASH_PROTECTION
void update_hash(struct Stack *stk);
#endif

#endif
