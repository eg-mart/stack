#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "stack.h"
#include "stack_debug.h"

enum StackError reallocate_stack(struct Stack *stk, size_t old_size, size_t new_size);

enum StackError stack_ctor(struct Stack *stk, print_func print_elem,
						   const char *varname, int line, const char *filename,
						   const char *funcname)
{
	int err = {};
	if (!stk) {
		err |= 1 << NULL_STACK_POINTER;
		STACK_REPORT_FAIL(stk, err);
		abort();
	}

	PRINT_ELEM = print_elem;

	if (stk->data || stk->capacity != 0 || stk->size != 0) {
		err |= 1 << DOUBLE_CTOR;
		STACK_REPORT_FAIL(stk, err);
		abort();
	}

	stk->size = 0;
	stk->capacity = INIT_CAPACITY;
	elem_t *mem = NULL;

#ifdef CANARY_PROTECTION
	stk->capacity += (sizeof(canary_t) - stk->capacity % sizeof(canary_t)) % sizeof(canary_t);
	mem = (elem_t*) calloc(stk->capacity * sizeof(elem_t) + 2 * sizeof(canary_t),
						   sizeof(char));
	if (mem == NULL) return ERR_NO_MEM;
	stk->data = (elem_t*) ((unsigned char*) mem + sizeof(canary_t));
#else
	mem = (elem_t*) calloc(stk->capacity, sizeof(elem_t));
	if (mem == NULL) return ERR_NO_MEM;
	stk->data = mem;
#endif

	memset(stk->data, POISON, stk->capacity * sizeof(elem_t));
	
	stk->filename = filename;
	stk->line = line;
	stk->varname = varname;
	stk->funcname = funcname;

#ifdef CANARY_PROTECTION
	stk->left_canary = DEFAULT_CANARY;
	stk->right_canary = DEFAULT_CANARY;
	((canary_t*) stk->data)[-1] = DEFAULT_CANARY;
	*((canary_t*) (stk->data + stk->capacity)) = DEFAULT_CANARY;
#endif

#ifdef HASH_PROTECTION
	update_hash(stk);
#endif
	
	return STACK_NO_ERR;
}

enum StackError stack_dtor(struct Stack *stk)
{
	VALIDATE_STACK(stk);
	
	stk->size = 0;
	stk->capacity = 0;

#ifdef CANARY_PROTECTION
	stk->right_canary = 0;
	stk->left_canary = 0;
	free((unsigned char*) stk->data - sizeof(canary_t));
#else
	free(stk->data);
#endif

	stk->data = NULL;

#ifdef HASH_PROTECTION
	stk->hash = 0;
	stk->data_hash = 0;
#endif

	return STACK_NO_ERR;
}

enum StackError reallocate_stack(struct Stack *stk, size_t old_size, size_t new_size)
{
	VALIDATE_STACK(stk);

	log_message(DEBUG, "reallocated stack from %lu to %lu\n", old_size, new_size);

	elem_t *mem = NULL;

#ifdef CANARY_PROTECTION
	new_size += (sizeof(canary_t) - new_size % sizeof(canary_t)) % sizeof(canary_t);
	if (new_size == old_size)
		return STACK_NO_ERR;
	
	mem = (elem_t*) realloc((unsigned char*) stk->data - sizeof(canary_t),
							new_size * sizeof(elem_t) + 2 * sizeof(canary_t));
	if (!mem) return ERR_NO_MEM;
	stk->data = (elem_t*) ((unsigned char*) mem + sizeof(canary_t));
#else
	mem = (elem_t*) realloc(stk->data, new_size * sizeof(elem_t));
	if (!mem) return ERR_NO_MEM;
	stk->data = mem;
#endif

	stk->capacity = new_size;
	if (new_size > old_size)
		memset(stk->data + old_size, POISON, (new_size - old_size) * sizeof(elem_t));

#ifdef HASH_PROTECTION
	update_hash(stk);
#endif

#ifdef CANARY_PROTECTION
	*((canary_t*) (stk->data + stk->capacity)) = DEFAULT_CANARY;
#endif

	return STACK_NO_ERR;
}

enum StackError stack_push(struct Stack *stk, elem_t value)
{
	VALIDATE_STACK(stk);

	if (stk->size == stk->capacity) {
		enum StackError error = reallocate_stack(stk, stk->capacity,
												 stk->capacity * MULTIPLIER);
		if (error < 0) return error;
	}

	stk->data[stk->size++] = value;

#ifdef HASH_PROTECTION
	update_hash(stk);
#endif

	return STACK_NO_ERR;
}

enum StackError stack_pop(struct Stack *stk, elem_t *value)
{
	VALIDATE_STACK(stk);

	if (stk->size * SHRINK_COEF <= stk->capacity && stk->capacity > INIT_CAPACITY) {
		enum StackError error = reallocate_stack(stk, stk->capacity, 
												 stk->capacity / MULTIPLIER);
		if (error < 0) return error;
	}

	if (stk->size == 0) return ERR_STACK_EMPTY;

	*value = stk->data[--stk->size];
	memset(stk->data + stk->size, POISON, sizeof(elem_t));

#ifdef HASH_PROTECTION
	update_hash(stk);
#endif

	return STACK_NO_ERR;
}