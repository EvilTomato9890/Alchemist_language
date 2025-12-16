#ifndef LIBS_STACK_INCLUDE_STACK_H_NCLUDED
#define LIBS_STACK_INCLUDE_STACK_H_NCLUDED

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>

#include "libs/Stack/include/debug_meta.h"
#include "libs/Stack/include/error_handler.h"

typedef double st_type;

struct stack_t {
	ON_STACK_CANARY_DEBUG(
		int canary_begin;
	)

	st_type* data;
	st_type* original_ptr;
	ON_STACK_DEBUG(
		st_ver_info_t ver_info;
		bool is_constructed = true;
	)
	size_t size;
	size_t capacity;

	ON_STACK_CANARY_DEBUG(
		int canary_end;
	)
};

error_code stack_init(stack_t* stack_return, size_t capacity ON_STACK_DEBUG(, ver_info_t ver_info));

error_code stack_destroy(stack_t* stack);

error_code stack_verify(const stack_t* const stack);

error_code stack_push(stack_t* stack, st_type elem);

st_type stack_pop(stack_t* stack, error_code* error_return); //Лучше возвращать ошибку или значение?

error_code stack_clone(const stack_t* source, stack_t* dest ON_STACK_DEBUG(, ver_info_t ver_info));

#endif /* LIBS_STACK_INCLUDE_STACK_H_NCLUDED */
