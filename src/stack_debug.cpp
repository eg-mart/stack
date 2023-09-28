#include "logger.h"
#include "stack.h"
#include "stack_debug.h"

#ifdef HASH_PROTECTION
unsigned long gnu_hash(void *data_ptr, size_t size);
#endif

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


#ifdef CANARY_PROTECTION
	if (stk->data)
		if (((canary_t*) stk->data)[-1] != DEFAULT_CANARY) {
			err->left_data_canary_bad = 1;
			return_status = STACK_FAILED;
		}
#endif

#ifdef HASH_PROTECTION
	unsigned long old_hash = stk->hash;
	unsigned long old_data_hash = stk->data_hash;
	stk->hash = 0;
	stk->data_hash = 0;
	if (old_hash != gnu_hash(stk, sizeof(Stack))) {
		err->wrong_hash = 1;
		return STACK_FAILED;
	} else if (old_data_hash != gnu_hash(stk->data, stk->capacity)) {
		err->wrong_data_hash = 1;
		return_status = STACK_FAILED;
	}
	stk->hash = old_hash;
	stk->data_hash = old_data_hash;
#endif

#ifdef CANARY_PROTECTION
	if (stk->left_canary != DEFAULT_CANARY) {
		err->left_canary_bad = 1;
		return STACK_FAILED;
	}

	if (stk->right_canary != DEFAULT_CANARY) {
		err->right_canary_bad = 1;
		return STACK_FAILED;
	}

	if (stk->data)
		if (*((canary_t*) (stk->data + stk->capacity)) != DEFAULT_CANARY) {
			err->left_data_canary_bad = 1;
			return_status = STACK_FAILED;
		}
#endif

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
	log_string(DEBUG, "\t\tleft canary = 0x%llX\n"
			   "\t\tright canary = 0x%llX\n"
			   "\t\tdefault canary = 0x%llX\n",
			   stk->left_canary, stk->right_canary, DEFAULT_CANARY);
#endif

#ifdef HASH_PROTECTION
	unsigned long old_hash = stk->hash;
	unsigned long old_data_hash = stk->data_hash;
	stk->hash = 0;
	stk->data_hash = 0;
	unsigned long new_hash = gnu_hash(stk, sizeof(Stack));
	log_string(DEBUG, "\t\thash = 0x%lX\n"
			   "\t\tactual hash = 0x%lX\n",
			   old_hash, new_hash);

	if (old_hash == new_hash)
		log_string(DEBUG, "\t\tdata hash = 0x%lX\n"
				   "\t\tactual data hash = 0x%lX\n",
				   old_data_hash, gnu_hash(stk->data, stk->capacity));

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
	log_string(DEBUG, "\t\t\tleft canary = 0x%lX\n", ((canary_t*) stk->data)[-1]);
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

#ifdef CANARY_PROTECTION
	log_string(DEBUG, "\t\t\tright canary = 0x%lX\n", 
			   *((canary_t*) (stk->data + stk->capacity)));
#endif

	log_string(DEBUG, "\t\t}\n\t}\n");
}

void stack_report_fail(struct Stack *stk, struct StackFailure err,
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
	if (err.double_ctor) log_message(ERROR, "A constructor was called twice!\n");

#ifdef CANARY_PROTECTION
	if (err.left_canary_bad) log_message(ERROR, "Stack's left canary is bad!\n");
	if (err.right_canary_bad) log_message(ERROR, "Stack's right canary is bad!\n");
	if (err.left_data_canary_bad) log_message(ERROR, "Stack's data left canary is bad!\n");
	if (err.right_data_canary_bad) log_message(ERROR, "Stack's data right canary is bad!\n");
#endif

#ifdef HASH_PROTECTION
	if (err.wrong_hash) log_message(ERROR, "Stack's hash doesn't match!\n");
	if (err.wrong_data_hash) log_message(ERROR, "Stack's data hash doesn't match!\n");
#endif

	log_message(DEBUG, "stack_dump called from %s (%d) %s()\n",
				filename, line, func_name);

	stack_dump(stk);
}

#ifdef HASH_PROTECTION
unsigned long gnu_hash(void *data_ptr, size_t size)
{
	char *data = (char*) data_ptr;
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