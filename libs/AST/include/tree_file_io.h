#ifndef LIBS_AST_INCLUDE_TREE_FILE_IO_H_NCLUDED
#define LIBS_AST_INCLUDE_TREE_FILE_IO_H_NCLUDED

#include "libs/AST/include/error_handler.h"
#include "libs/AST/include/tree_info.h"

error_code tree_read_from_file(tree_t* tree, const char* filename, string_t* buffer_out);
error_code tree_write_to_file(const tree_t* tree, const char* filename);

const char* get_func_name_by_type(func_code_t func_type_value);

error_code tree_parse_from_buffer(tree_t* tree);

#endif /* LIBS_AST_INCLUDE_TREE_FILE_IO_H_NCLUDED */
