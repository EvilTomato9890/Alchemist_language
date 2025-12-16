#ifndef LIBS_STACK_INCLUDE_VAR_STACK_H_NCLUDED
#define LIBS_STACK_INCLUDE_VAR_STACK_H_NCLUDED

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>

#include "debug_meta.h"
#include "error_handler.h"
#include "/home/omglo/alchemist_language/libs/AST/include/node_info.h"

typedef variable_t st_type;

struct var_stack_t {
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

error_code var_stack_init(var_stack_t* stack_return, size_t capacity ON_STACK_DEBUG(, ver_info_t ver_info));

error_code var_stack_destroy(var_stack_t* stack);

error_code var_stack_verify(const var_stack_t* const stack);

error_code var_stack_push(var_stack_t* stack, st_type elem);

st_type var_stack_pop(var_stack_t* stack, error_code* error_return); //Лучше возвращать ошибку или значение?

error_code var_stack_clone(const var_stack_t* source, var_stack_t* dest ON_STACK_DEBUG(, ver_info_t ver_info));

#endif /* LIBS_STACK_INCLUDE_VAR_STACK_H_NCLUDED */
