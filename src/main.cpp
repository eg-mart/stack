#include <stdio.h>
#include <limits.h>

#include "stack.h"
#include "logger.h"

int main()
{
	logger_ctor();
	add_log_handler({ stderr, DEBUG, true });

	FILE *log = fopen("log.txt", "w");
	if (!log) {
		log_message(WARN, "Unable to open log file\n");
	} else {
		setvbuf(log, NULL, _IONBF, 0);
		add_log_handler({ log, DEBUG, false });
	}

//-------------------------------
	struct Stack stk = {};
	STACK_CTOR(&stk);

	for (int i = 0; i < 10; i++) {
		stack_push(&stk, i);
	}

	stk.capacity = 10000;
	stk.size = 100;
	((canary_t*) stk.data)[-1] = 0;
	enum StackError err = STACK_NO_ERR;
	int elem = 0;
	while (err >= 0) {
		err = stack_pop(&stk, &elem);
	}

	stack_dtor(&stk);
//-----------------------------

	logger_dtor();
	fclose(log);
	
	return 0;
}
