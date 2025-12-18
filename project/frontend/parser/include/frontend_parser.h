#ifndef PROJECT_FRONTEND_PARSER_INCLUDE_FRONTEND_PARSER_H_NCLUDED
#define PROJECT_FRONTEND_PARSER_INCLUDE_FRONTEND_PARSER_H_NCLUDED

#include <stddef.h>
#include <stdbool.h>

#include "libs/AST/include/tree_info.h"
#include "libs/Vector/include/vector.h"
#include "lexer/include/lexer_tokenizer.h"
#include "common/keywords/include/keywords.h"
#include "libs/Unordered_map/include/unordered_map.h"

// key = ident_idx (size_t), value = func_decl_info_t
struct func_decl_info_t {
    op_code_t decl_opcode; // OP_FUNC_DECL | OP_PROC_DECL
    size_t    argc;
};

size_t parser_hash_size_t(const void* key_ptr);
bool   parser_key_cmp_size_t(const void* left_ptr, const void* right_ptr);

void frontend_parse_ast(tree_t* tree, const vector_t* tokens,
                        u_map_t* func_table, vector_t* diags_out);


#endif /* PROJECT_FRONTEND_PARSER_INCLUDE_FRONTEND_PARSER_H_NCLUDED */
