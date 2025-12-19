#ifndef PROJECT_BACKEND_INCLUDE_BACKEND_H_NCLUDED
#define PROJECT_BACKEND_INCLUDE_BACKEND_H_NCLUDED

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include "libs/AST/include/tree_info.h"
#include "libs/Unordered_map/include/unordered_map.h"

struct backend_func_symbol_t {
    size_t label_id;
    size_t param_count;
    bool   is_proc;
};

// 2 прохода:
//  1) заполняет func_table (можно вызывать функции, объявленные ниже)
//  2) генерирует asm в asm_file
//
// func_table должен быть инициализирован снаружи (u_map_init / u_map_static_init).
// key   = size_t ident_idx
// value = backend_func_symbol_t
hm_error_t backend_emit_asm(const tree_t* tree,
                            FILE* asm_file,
                            u_map_t* func_table);

#endif /* PROJECT_BACKEND_INCLUDE_BACKEND_H_NCLUDED */
