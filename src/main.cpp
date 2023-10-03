#include <stdio.h>
#include <limits.h>

#include "stack.h"
#include "stack_debug.h"
#include "logger.h"

int print_int(char *buffer, int x, size_t n);
int print_struct(char *buffer, struct Elem x, size_t n);

int print_int(char *buffer, int x, size_t n)
{
	return snprintf(buffer, n, "%d", x);
}

int print_struct(char *buffer, struct Elem x, size_t n)
{
	return snprintf(buffer, n, "cost: %.2lf; amount: %d", x.cost, x.amount);
}

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
	STACK_CTOR(&stk, print_struct);

	for (int i = 0; i < 10; i++) {
		stack_push(&stk, {i * 10 + 15.75, i + 2});
	}

	stack_dump(&stk);
	stack_dtor(&stk);
//-----------------------------

	logger_dtor();
	fclose(log);
	
	return 0;
}
