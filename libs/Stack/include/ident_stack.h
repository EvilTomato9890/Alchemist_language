#ifndef LIBS_STACK_INCLUDE_ident_stack_H_NCLUDED
#define LIBS_STACK_INCLUDE_ident_stack_H_NCLUDED

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>

#include "libs/Stack/include/debug_meta.h"
#include "libs/Stack/include/error_handler.h"
#include "libs/AST/include/node_info.h"

typedef c_string_t var_st_type;

struct ident_stack_t {
	ON_STACK_CANARY_DEBUG(
		int canary_begin;
	)

	var_st_type* data;
	var_st_type* original_ptr;
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

error_code ident_stack_init(ident_stack_t* stack_return, size_t capacity ON_STACK_DEBUG(, st_ver_info_t ver_info));

error_code ident_stack_destroy(ident_stack_t* stack);

error_code ident_stack_verify(const ident_stack_t* const stack);

error_code ident_stack_push(ident_stack_t* stack, var_st_type elem);

var_st_type ident_stack_pop(ident_stack_t* stack, error_code* error_return); //Лучше возвращать ошибку или значение?

error_code ident_stack_clone(const ident_stack_t* source, ident_stack_t* dest ON_STACK_DEBUG(, st_ver_info_t ver_info));

#endif /* LIBS_STACK_INCLUDE_ident_stack_H_NCLUDED */
