#include <stdlib.h>

#include "error_handler.h"
#include "stack.h"
#include "asserts.h"
#include "logger.h"
#include "debug_meta.h"

#include <string.h>

static const int MIN_STACK_SIZE      = 128;
static const double POISON_STACK_VALUE = (double)0xEBA1DEDAEBA1DEDA;
static const float REDUCTION_STACK_FACTOR  = 4; // float
static const float GROWTH_FACTOR     = 2; // float

static const int CANARY_NUM          = INT_MAX;

static error_code normalize_size(stack_t* stack);
static error_code stack_recalloc(stack_t* stack, size_t new_capacity);


error_code stack_init(stack_t* stack_return, size_t capacity ON_STACK_DEBUG(, ver_info_t ver_info)) {
	LOGGER_DEBUG("Stack initialize started");	

	HARD_ASSERT(stack_return != nullptr, "Stack_return is nullptr");
	//todo: is_constructed
	//todo: проверка на пустоту 
	error_code error = 0;
	if(capacity < MIN_STACK_SIZE) {
		LOGGER_INFO("Capacity normalized");
		capacity = MIN_STACK_SIZE;
	}

	stack_t stack = {};
	LOGGER_DEBUG("Trying to calloc %lu bytes", (capacity ON_STACK_STACK_CANARY_DEBUG(+ 2)) * sizeof(st_type));
	stack.original_ptr = (st_type*)calloc(capacity ON_STACK_CANARY_DEBUG(+ 2), sizeof(st_type));
	if(stack.original_ptr == nullptr) {
		LOGGER_DEBUG("Memory allocation failed");
		error |= ST_MEM_ALLOC_ERROR;
		return error;
	}
	LOGGER_DEBUG("Allocation complete");

	stack.data = stack.original_ptr;
	ON_STACK_CANARY_DEBUG(
		LOGGER_DEBUG("Canary initialize");
		stack.canary_begin = CANARY_NUM;
		stack.canary_end = CANARY_NUM;
		stack.data[0] = CANARY_NUM;
		stack.data[capacity + 1] = CANARY_NUM;
		stack.data++; 																																					//todo: Можно разные канарейки на начало и конец
	)

	LOGGER_DEBUG("Filling the array");
	for(size_t i = 0; i < capacity; i++) {
		stack.data[i] = POISON_STACK_VALUE;
	}
	stack.capacity = capacity;
	ON_STACK_DEBUG(
		stack.ver_info = ver_info;
	)
	ON_STACK_DEBUG(stack.is_constructed = true;)
	*stack_return = stack;

	ON_STACK_HASH_DEBUG(
		stack_return->ver_info.hash = stack_get_hash(stack_return, &error);
	)
	ON_STACK_DEBUG(
		error = stack_verify(stack_return);
	)
	if(error != 0) {
		LOGGER_ERROR("Error code: %lu", error);
	}
	return error;
}

error_code stack_destroy(stack_t* stack) {
	LOGGER_DEBUG("stack_destroy started");

	HARD_ASSERT(stack != nullptr, "Stack is nullptr");
	ON_STACK_DEBUG(HARD_ASSERT(stack->is_constructed, "Stack is not constructed");)

	free(stack->original_ptr);
	return 0;
}


error_code stack_push(stack_t* stack, st_type elem) {
	LOGGER_DEBUG("Push started, elem = %p", (void*)elem);

	HARD_ASSERT(stack != nullptr, "Stack is nullptr");

	error_code error = 0;
	ON_STACK_DEBUG(
		error = stack_verify(stack);
		RETURN_IF_ERROR(error);
	)

	ON_STACK_DEBUG(HARD_ASSERT(stack->is_constructed, "Stack is not constructed");)

	error = normalize_size(stack);
	STACK_RETURN_IF_ERROR(error);
	
	stack->size++;
	stack->data[stack->size - 1] = elem;
	
	ON_STACK_HASH_DEBUG(
		stack->ver_info.hash = stack_get_hash(stack, &error);
	)
	ON_STACK_DEBUG(
		error = stack_verify(stack);
	)

	return error;
}

static error_code normalize_size(stack_t* stack) {
	LOGGER_DEBUG("Normalize_size started");

	HARD_ASSERT(stack != nullptr, "Stack is nullptr");

	error_code error = 0;
	ON_STACK_DEBUG(
		error = stack_verify(stack);
		STACK_RETURN_IF_ERROR(error);
	)

	if((float)stack->size * REDUCTION_STACK_FACTOR < (float)stack->capacity && stack->capacity > MIN_STACK_SIZE) {
		size_t new_capacity = (size_t)((float)stack->size * (REDUCTION_STACK_FACTOR / GROWTH_FACTOR));
		if (new_capacity < MIN_STACK_SIZE) new_capacity = MIN_STACK_SIZE;
		LOGGER_DEBUG("Trying to stack_recalloc data for %lu elemets", new_capacity);
		error = stack_recalloc(stack, new_capacity); 
		STACK_RETURN_IF_ERROR(error);

		stack->capacity = new_capacity;
	}

	ON_STACK_DEBUG(
		error = stack_verify(stack);
		STACK_RETURN_IF_ERROR(error);
	)


	if(stack->size + 1 > stack->capacity) {
		LOGGER_DEBUG("Old capacity = %lu", stack->capacity);
		size_t new_capacity = (size_t)((float)stack->capacity * GROWTH_FACTOR);
		LOGGER_DEBUG("Trying to realloc stack for %lu elemets", new_capacity);
		error = stack_recalloc(stack, new_capacity); 
		STACK_RETURN_IF_ERROR(error);
		
		LOGGER_DEBUG("Reallocation complete");
		LOGGER_DEBUG("NEW CAPACITY = %lu", stack->capacity);
	}

	ON_STACK_DEBUG(
		error = stack_verify(stack);
	)
	return error;
}

static error_code stack_recalloc(stack_t* stack, size_t new_capacity) {
    LOGGER_DEBUG("Stack_recalloc started");

    HARD_ASSERT(stack != nullptr, "Stack is nullptr");
    HARD_ASSERT(stack->data != nullptr, "Arr is nullptr");

    error_code error = 0;
    ON_STACK_DEBUG(
    	error = stack_verify(stack);
    	RETURN_IF_ERROR(error);
    )
    if(new_capacity == 0) {
		LOGGER_WARNING("Realloc_size is null");
	}

    LOGGER_DEBUG("Trying to realloc %lu bytes", (new_capacity ON_STACK_CANARY_DEBUG(+ 2)) * sizeof(st_type));
    st_type* new_block = (st_type*)realloc(stack->original_ptr, (new_capacity ON_STACK_CANARY_DEBUG(+ 2)) * sizeof(st_type));
    if(new_block == nullptr) {
        LOGGER_ERROR("Reallocation failed");
        error |= ST_MEM_ALLOC_ERROR;
        return error;
    }
    LOGGER_DEBUG("Reallocation complete");
    stack->original_ptr = new_block;
    st_type* new_data = new_block;
    ON_STACK_CANARY_DEBUG(
    	LOGGER_DEBUG("Canary reinitialize started");
    	new_block[0] = CANARY_NUM;
    	new_block[new_capacity + 1] = CANARY_NUM;
    	new_data++;
    )
    LOGGER_DEBUG("Filling the arr");
    for(size_t i = stack->size; i < new_capacity; i++) {
        new_data[i] = POISON_STACK_VALUE;
    }

    stack->data = new_data;
    stack->capacity = new_capacity;

    ON_STACK_HASH_DEBUG(
    	stack->ver_info.hash = stack_get_hash(stack, &error);
    )
    ON_STACK_DEBUG(
    	error = stack_verify(stack);
    )

    return error;
}

st_type stack_pop(stack_t* stack, error_code* error_return) { //Лучше возвращать ошибку или значение? //D c++ разделено на 2 функциии 1 вычитает значение другая возвращает (для души)(какая=то хуйня я не помню)
	LOGGER_DEBUG("Pop stared");

	HARD_ASSERT(stack != nullptr, "Stack is nullptr");
	HARD_ASSERT(error_return != nullptr, "Stack for return is nullptr");
	ON_STACK_DEBUG(HARD_ASSERT(stack->is_constructed, "Stack is not constructed");)

	error_code error = 0;
	ON_STACK_DEBUG(
		error = stack_verify(stack);
		if(error != 0) {
			LOGGER_ERROR("Error code: %lu", error);
			*error_return = error;
			return 0; 
		} else if(stack->size == 0) {
			error |= ST_SMALL_SIZE_ERROR;
			LOGGER_ERROR("Error code: %lu", error);
			*error_return = error;
			return 0;
		}
	)
 	st_type popped_elem = stack->data[stack->size - 1];
	stack->data[stack->size - 1] = POISON_STACK_VALUE;
	stack->size--;

	ON_STACK_HASH_DEBUG(
		stack->ver_info.hash = stack_get_hash(stack, &error);
	)

	error = normalize_size(stack);

	if(error != 0) {
		LOGGER_ERROR("Error code: %lu", error);
		*error_return = error;
		return 0;
	}

	ON_STACK_DEBUG(
		error = stack_verify(stack);
	)
	return popped_elem;
}
//a0 + a1 * a2 + a3 * a4
//Канарейка в структуры
//Tckb 
//Канарейки как uint64_t нужно добавить выравнивание 
//HFpvth rfyfhtqrb rjycnfynysq (закинут в гугл)
//Большой каллок где-то храниться на ку, а структура на стеке

//Почему с большой буквы ищет хрень в онегине
//Почему нельзя ifdef в define СПРОСИТЬ
error_code stack_verify(const stack_t* stack) {
	//LOGGER_DEBUG("stack_verify started");
	error_code error = 0;
	if(stack == nullptr) {
		error |= ST_NULL_ARG_ERROR;
		return error;
	}

	if(stack->size > stack->capacity) 			 			  error |= ST_BIG_SIZE_ERROR;
	// size_t is unsigned, so it can't be < 0 - removed check
	if(stack->capacity  == 0)		              		      error |= ST_ZERO_CAP_ERROR;
	if(stack->data == nullptr)       						  error |= ST_NULL_DATA_ERROR;
	ON_STACK_CANARY_DEBUG(if(stack->data[-1] != CANARY_NUM ||
	   stack->data[stack->capacity] != CANARY_NUM)    		  error |= ST_CANARY_DATA_ERROR;)
	ON_STACK_CANARY_DEBUG(if(stack->canary_begin != CANARY_NUM ||
	   stack->canary_end != CANARY_NUM) 					  error |= ST_CANARY_STRUCT_ERROR;)
	ON_STACK_HASH_DEBUG(if(stack->ver_info.hash != stack_get_hash(stack, &error)) 
															  error |= ST_HASH_ERROR;)
	return error;	
}



