#include <stdio.h>
#include <stdlib.h>

#include "logger.h"
#include "stack.h"

#define STACK_REPORT_FAIL(stk, err) stack_report_fail((stk), (err), __FILE__,	\
													  __LINE__, __func__)

#define VALIDATE_STACK(stk) struct StackFailure err = {};						\
							if (validate_stack(stk, &err) == STACK_FAILED) {	\
								STACK_REPORT_FAIL((stk), err);					\
								abort();										\
							}

const int INIT_CAPACITY = 2;

enum StackError reallocate_stack(struct Stack *stk, size_t old_size, size_t new_size);
enum StackError validate_stack(struct Stack *stk, struct StackFailure *err);
void stack_report_fail(const struct Stack *stk, struct StackFailure err,
					   const char *filename, int line, const char *func_name);

enum StackError stack_ctor(struct Stack *stk, const char *varname, int line, 
						   const char *filename, const char *funcname)
{
	struct StackFailure err = {};
	if (!stk) {
		err.null_stack_pointer = 1;
		STACK_REPORT_FAIL(stk, err);
		abort();
	}

	stk->size = 0;
	stk->capacity = INIT_CAPACITY;
	stk->data = (elem_t*) calloc(INIT_CAPACITY, sizeof(elem_t));
	if (stk->data == NULL) return ERR_NO_MEM;

	for (size_t i = 0; i < stk->capacity; i++)
		stk->data[i] = POISON;

	stk->filename = filename;
	stk->line = line;
	stk->varname = varname;
	stk->funcname = funcname;
	
	return STACK_NO_ERR;
}

enum StackError stack_dtor(struct Stack *stk)
{
	VALIDATE_STACK(stk);
	
	stk->size = 0;
	stk->capacity = 0;
	free(stk->data);
	stk->data = NULL;

	return STACK_NO_ERR;
}

enum StackError validate_stack(struct Stack *stk, struct StackFailure *err)
{
	if (!stk) {
		err->null_stack_pointer = 1;
		return STACK_FAILED;
	}

	enum StackError return_status = STACK_NO_ERR;
	if (!stk->data) {
		err->null_data_pointer = 1;
		return_status = STACK_FAILED;
	}

	if (stk->size > stk->capacity) {
		err->capacity_overflow = 1;
		return_status = STACK_FAILED;
	}

	if (stk->capacity < INIT_CAPACITY) {
		err->small_capacity = 1;
		return_status = STACK_FAILED;
	}

	if (stk->data) {
		for (size_t i = 0; i < stk->size && i < stk->capacity; i++)
			if (stk->data[i] == POISON) {
				err->poisoned_value = 1;
				return_status = STACK_FAILED;
				break;
			}
	}

	if (stk->data) {
		for (size_t i = stk->size; i < stk->capacity; i++)
			if (stk->data[i] != POISON) {
				err->unpoisoned_value = 1;
				return_status = STACK_FAILED;
				break;
			}
	}

	return return_status;
}

void stack_dump(const struct Stack *stk)
{
	const int POISONED_MAX = 20;
	log_message(DEBUG, "Stack [%p] \"%s\" from %s (%d) %s\n",
				stk, stk->varname, stk->filename, stk->line, stk->funcname);

	if (!stk) return;
	log_string(DEBUG, "\t{\n\t\tsize = %lu\n"
			   "\t\tcapacity = %lu\n"
			   "\t\tdata [%p]\n",
			   stk->size, stk->capacity, stk->data);

	if (!stk->data) {
		log_string(DEBUG, "\t}\n");
		return;
	}
	log_string(DEBUG, "\t\t{\n");
	size_t i = 0;
	for (; i < stk->size && i < stk->capacity; i++) {
		log_string(DEBUG, "\t\t\t*[%lu] = " FMT, i, stk->data[i]);
		if (stk->data[i] == POISON) log_string(DEBUG, " (poison)");
		log_string(DEBUG, "\n");
	}
	for (; i < stk->capacity && i < stk->size + POISONED_MAX; i++) {
		log_string(DEBUG, "\t\t\t[%lu] = " FMT, i, stk->data[i]);
		if (stk->data[i] == POISON) log_string(DEBUG, " (poison)");
		log_string(DEBUG, "\n");
	}

	log_string(DEBUG, "\t\t}\n\t}\n");
}

void stack_report_fail(const struct Stack *stk, struct StackFailure err,
					   const char *filename, int line, const char *func_name)
{

	if (err.null_stack_pointer) log_message(ERROR, "Stack pointer is NULL!\n");
	if (err.null_data_pointer) log_message(ERROR, "Data pointer in a stack is NULL!\n");
	if (err.small_capacity) log_message(ERROR, "Stack capacity is less then initial!\n");
	if (err.capacity_overflow) log_message(ERROR, "Stack size is greater then "
										   "capacity!\n");
	if (err.poisoned_value) log_message(ERROR, "A poison value is in stack!\n");
	if (err.unpoisoned_value) log_message(ERROR, "A non-poison value is in stack's "
										  "unused memory!\n");

	log_message(DEBUG, "stack_dump called from %s (%d) %s()\n",
				filename, line, func_name);

	stack_dump(stk);
}

enum StackError reallocate_stack(struct Stack *stk, size_t old_size, size_t new_size)
{
	VALIDATE_STACK(stk);

	log_message(DEBUG, "reallocate_stack was called with "
				"new_size: %lu and old_size: %lu\n", new_size, old_size);

	stk->data = (elem_t*) realloc(stk->data, new_size * sizeof(elem_t));
	if (!stk->data) return ERR_NO_MEM;
	stk->capacity = new_size;

	if (new_size > old_size)
		for (size_t i = old_size; i < new_size; i++)
			stk->data[i] = POISON;

	return STACK_NO_ERR;
}

enum StackError stack_push(struct Stack *stk, elem_t value)
{
	VALIDATE_STACK(stk);

	if (stk->size == stk->capacity) {
		enum StackError error = reallocate_stack(stk, stk->capacity, stk->capacity * 2);
		if (error < 0) return error;
	}

	stk->data[stk->size++] = value;

	return STACK_NO_ERR;
}

enum StackError stack_pop(struct Stack *stk, elem_t *value)
{
	VALIDATE_STACK(stk);

	if (stk->size < stk->capacity / 4 && stk->capacity > INIT_CAPACITY) {
		enum StackError error = reallocate_stack(stk, stk->capacity, stk->capacity / 2);
		if (error < 0) return error;
	}

	if (stk->size == 0) return ERR_STACK_EMPTY;

	*value = stk->data[--stk->size];
	stk->data[stk->size] = POISON;

	return STACK_NO_ERR;
}
