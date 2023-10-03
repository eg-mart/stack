#include <string.h>
#include <stdlib.h>

#include "logger.h"
#include "stack.h"
#include "stack_debug.h"
#include "colors.h"

#ifdef HASH_PROTECTION
unsigned long gnu_hash(void *data_ptr, size_t size);
#endif

print_func PRINT_ELEM = NULL;

const char *STACK_FAILURE_MSG[] = {
	"Stack pointer is NULL!\n",
	"Stack capacity is less then initial!\n",
	"Data pointer in a stack is NULL!\n",
	"A poison value is in stack!\n",
	"A non-poison value is in stack's unused memory!\n",
	"Stack size is greater then capacity!\n",
	"A constructor was called twice!\n",

#ifdef CANARY_PROTECTION
	"Stack's left canary is bad!\n",
	"Stack's right canary is bad!\n",
	"Stack's data left canary is bad!\n",
	"Stack's data right canary is bad!\n",
#endif

#ifdef HASH_PROTECTION
	"Stack's hash doesn't match!\n",
	"Stack's data hash doesn't match!\n",
#endif
};

enum StackError validate_stack(struct Stack *stk, int *err)
{
	*err = 0;

	if (!stk) {
		*err |= 1 << NULL_STACK_POINTER;
		return STACK_FAILED;
	}

	if (!stk->data)
		*err |= 1 << NULL_DATA_POINTER;

	if (stk->size > stk->capacity)
		*err |= 1 << CAPACITY_OVERFLOW;

	if (stk->capacity < INIT_CAPACITY)
		*err |= 1 << SMALL_CAPACITY;


#ifdef CANARY_PROTECTION
	if (stk->data && ((canary_t*) stk->data)[-1] != DEFAULT_CANARY)
		*err |= 1 << LEFT_DATA_CANARY_BAD;
#endif

#ifdef HASH_PROTECTION
	unsigned long old_hash = stk->hash;
	unsigned long old_data_hash = stk->data_hash;
	stk->hash = 0;
	stk->data_hash = 0;
	if (old_hash != gnu_hash(stk, sizeof(Stack)))
		*err |= 1 << WRONG_HASH;
	stk->hash = old_hash;
	stk->data_hash = old_data_hash;
#endif

#ifdef CANARY_PROTECTION
	if (stk->left_canary != DEFAULT_CANARY)
		*err |= 1 << LEFT_CANARY_BAD;

	if (stk->right_canary != DEFAULT_CANARY)
		*err |= 1 << RIGHT_CANARY_BAD;

	if (*err & 1 << RIGHT_CANARY_BAD || *err & 1 << LEFT_CANARY_BAD)
		return STACK_FAILED;
#endif

#ifdef HASH_PROTECTION
	if (*err & 1 << WRONG_HASH) {
		return STACK_FAILED;
	}
	stk->hash = 0;
	stk->data_hash = 0;
	if (old_data_hash != gnu_hash(stk->data, stk->capacity))
		*err |= 1 << WRONG_DATA_HASH;
	stk->hash = old_hash;
	stk->data_hash = old_data_hash;
#endif

#ifdef STACK_PROTECTION
	if (stk->data && *((canary_t*) (stk->data + stk->capacity)) != DEFAULT_CANARY)
		*err |= 1 << RIGHT_DATA_CANARY_BAD;
#endif

	if (stk->data) {
		unsigned char tester[sizeof(elem_t)] = {};
		memset(tester, POISON, sizeof(elem_t));

		for (size_t i = 0; i < stk->size; i++)
			if (memcmp(stk->data + i, tester, sizeof(elem_t)) == 0)
				*err |= 1 << POISONED_VALUE;

		for (size_t i = stk->size; i < stk->capacity; i++)
			if (memcmp(stk->data + i, tester, sizeof(elem_t)) != 0)
				*err |= 1 << UNPOISONED_VALUE;
	}

	if (*err != 0)
		return STACK_FAILED;
	return STACK_NO_ERR;
}

void stack_dump(struct Stack *stk)
{
	const int POISONED_MAX = 20;
	log_message(DEBUG, "Stack [%p]\n", stk);

	if (!stk) return;

	log_string(DEBUG, "\t\"%s\" from %s (%d) %s()\n", stk->varname, stk->filename, 
			   stk->line, stk->funcname);
	log_string(DEBUG, "\t{\n\t\tsize = %lu\n"
			   "\t\tcapacity = %lu\n",
			   stk->size, stk->capacity);

#ifdef CANARY_PROTECTION
	if (stk->left_canary == DEFAULT_CANARY)
		log_string(DEBUG, "%s\t\tleft canary = 0x%llX\n%s",
				   GREEN, stk->left_canary, RESET_COLOR);
	else
		log_string(DEBUG, "%s\t\tleft canary = 0x%llX\n%s",
				   RED, stk->left_canary, RESET_COLOR);

	if (stk->right_canary == DEFAULT_CANARY)
		log_string(DEBUG, "%s\t\tright canary = 0x%llX\n%s",
				   GREEN, stk->right_canary, RESET_COLOR);
	else
		log_string(DEBUG, "%s\t\tright canary = 0x%llX\n%s",
				   RED, stk->right_canary, RESET_COLOR);

	log_string(DEBUG, "%s\t\tdefault canary = 0x%llX\n%s",
			   BLUE, DEFAULT_CANARY, RESET_COLOR);
#endif

#ifdef HASH_PROTECTION
	unsigned long old_hash = stk->hash;
	unsigned long old_data_hash = stk->data_hash;
	stk->hash = 0;
	stk->data_hash = 0;
	unsigned long new_hash = gnu_hash(stk, sizeof(Stack));

	if (old_hash == new_hash)
		log_string(DEBUG, "%s\t\thash = 0x%lX\n%s",
				   GREEN, old_hash, RESET_COLOR);
	else
		log_string(DEBUG, "%s\t\thash = 0x%lX\n%s",
				   RED, old_hash, RESET_COLOR);
   log_string(DEBUG, "%s\t\tactual hash = 0x%lX\n%s",
		      BLUE, new_hash, RESET_COLOR);

	if (old_hash == new_hash) {
		unsigned long new_data_hash = gnu_hash(stk->data, stk->capacity);

		if (old_data_hash == new_data_hash)
			log_string(DEBUG, "%s\t\tdata hash = 0x%lX\n%s",
					   GREEN, old_data_hash, RESET_COLOR);
		else
			log_string(DEBUG, "%s\t\tdata hash = 0x%lX\n%s",
					   RED, old_data_hash, RESET_COLOR);
		log_string(DEBUG, "%s\t\tactual data hash = 0x%lX\n%s",
				   BLUE, new_data_hash, RESET_COLOR);
	}

	stk->hash = old_hash;
	stk->data_hash = old_data_hash;
#endif

	log_string(DEBUG, "\t\tdata [%p]\n", stk->data);

	if (!stk->data) {
		log_string(DEBUG, "\t}\n");
		return;
	}

	log_string(DEBUG, "\t\t{\n");

#ifdef CANARY_PROTECTION
	if (((canary_t*) stk->data)[-1] == DEFAULT_CANARY)
		log_string(DEBUG, "%s\t\t\tleft canary = 0x%lX\n%s", 
				   GREEN, ((canary_t*) stk->data)[-1], RESET_COLOR);
	else
		log_string(DEBUG, "%s\t\t\tleft canary = 0x%lX\n%s", 
				   RED, ((canary_t*) stk->data)[-1], RESET_COLOR);

	if (stk->left_canary != DEFAULT_CANARY || stk->right_canary != DEFAULT_CANARY) {
		log_string(DEBUG, "\t\t}\n\t}\n");
		return;
	}
#endif

#ifdef HASH_PROTECTION
	if (old_hash != new_hash) {
		log_string(DEBUG, "\t\t}\n\t}\n");
		return;
	}
#endif

	const size_t BUFF_SIZE = 1024;
	char buffer[BUFF_SIZE] = {};
	unsigned char tester[sizeof(elem_t)] = {};
	memset(tester, POISON, sizeof(elem_t));
	size_t i = 0;
	for (; i < stk->size && i < stk->capacity; i++) {
		PRINT_ELEM(buffer, stk->data[i], BUFF_SIZE);
		log_string(DEBUG, "\t\t\t*[%lu] = ", i);
		log_string(DEBUG, "%s", buffer);
		if (memcmp(stk->data + i, tester, sizeof(elem_t)) == 0)
			log_string(DEBUG, " (poison)");
		log_string(DEBUG, "\n");
	}
	for (; i < stk->capacity && i < stk->size + POISONED_MAX; i++) {
		PRINT_ELEM(buffer, stk->data[i], BUFF_SIZE);
		log_string(DEBUG, "\t\t\t[%lu] = ", i);
		log_string(DEBUG, "%s", buffer);
		if (memcmp(stk->data + i, tester, sizeof(elem_t)) == 0)
			log_string(DEBUG, " (poison)");
		log_string(DEBUG, "\n");
	}

#ifdef CANARY_PROTECTION
	if (*((canary_t*) (stk->data + stk->capacity)) == DEFAULT_CANARY)
		log_string(DEBUG, "%s\t\t\tright canary = 0x%lX\n%s",
				   GREEN, *((canary_t*) (stk->data + stk->capacity)), RESET_COLOR);
	else
		log_string(DEBUG, "%s\t\t\tright canary = 0x%lX\n%s",
				   RED, *((canary_t*) (stk->data + stk->capacity)), RESET_COLOR);
#endif

	log_string(DEBUG, "\t\t}\n\t}\n");
}

void stack_report_fail(struct Stack *stk, int err,
					   const char *filename, int line, const char *func_name)
{
	size_t stack_failure_num = sizeof(STACK_FAILURE_MSG) / sizeof(STACK_FAILURE_MSG[0]);
	for (size_t i = 0; i < stack_failure_num; i++)
		if (err & 1 << i)
			log_message(ERROR, STACK_FAILURE_MSG[i]);

	log_message(DEBUG, "stack_dump called from %s (%d) %s()\n",
				filename, line, func_name);

	stack_dump(stk);
}

#ifdef HASH_PROTECTION
unsigned long gnu_hash(void *data_ptr, size_t size)
{
	unsigned char *data = (unsigned char*) data_ptr;
	unsigned long hash = 5381;

	for (size_t i = 0; i < size; i++)
		hash = ((hash << 5) + hash) + (long unsigned int) data[i];

	return hash;
}

void update_hash(struct Stack *stk)
{
	stk->hash = 0;
	stk->data_hash = 0;
	stk->hash = gnu_hash(stk, sizeof(Stack));
	stk->data_hash = gnu_hash(stk->data, stk->capacity);
}
#endif