#ifndef LIBS_AST_INCLUDE_TREE_VERIFICATION_H_NCLUDED
#define LIBS_AST_INCLUDE_TREE_VERIFICATION_H_NCLUDED

#include <stdarg.h>

#include "libs/AST/include/tree_info.h"
#include "libs/AST/include/error_handler.h"

enum tree_dump_mode_t {
    TREE_DUMP_NO   = 0,
    TREE_DUMP_TEXT = 1,
    TREE_DUMP_IMG  = 2,
};

error_code tree_verify(const tree_t* tree,
                       tree_ver_info_t ver_info,
                       tree_dump_mode_t mode,
                       const char* fmt, ...);

error_code tree_dump(const tree_t* tree,
                     tree_ver_info_t ver_info,
                     bool is_visual,
                     const char* fmt, ...);

error_code tree_open_dump_file(tree_t* tree, const char* dump_file_name);

error_code tree_close_dump_file(tree_t* tree);
#endif /* LIBS_AST_INCLUDE_TREE_VERIFICATION_H_NCLUDED */

