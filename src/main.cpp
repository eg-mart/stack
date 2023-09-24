#include <stdio.h>
#include <limits.h>

#include "stack.h"
#include "logger.h"

int main()
{
	logger_ctor();
	add_log_handler({ stderr, DEBUG, true });

	struct Stack stk = {};
	STACK_CTOR(&stk);

	for (int i = 0; i < 10; i++) {
		stack_push(&stk, i);
	}

	stack_push(&stk, INT_MAX);

	enum StackError err = STACK_NO_ERR;
	int elem = 0;
	while (err >= 0) {
		err = stack_pop(&stk, &elem);
	}

	stack_dtor(&stk);
	logger_dtor();
	
	return 0;
}
