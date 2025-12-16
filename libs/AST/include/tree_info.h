#ifndef LIBS_AST_INCLUDE_TREE_INFO_H_NCLUDED
#define LIBS_AST_INCLUDE_TREE_INFO_H_NCLUDED

#include <stdlib.h>
#include <stdio.h>

#include "libs/AST/internal/debug_meta.h"
#include "libs/AST/include/node_info.h"
#include "libs/Stack/include/ident_stack.h"
#include "libs/My_string/include/my_string.h"

const int MAX_FOREST_CAP = 10; //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!porno

struct tex_squash_binding_t {
    char         letter;
    tree_node_t* expr_subtree;
};

struct tree_t {
    tree_node_t*   root;
    size_t         size;
    ident_stack_t*   ident_stack;
    c_string_t     buff;
    ON_TREE_DEBUG(
        tree_ver_info_t ver_info;
        FILE* dump_file;
    )
};

#endif /* LIBS_AST_INCLUDE_TREE_INFO_H_NCLUDED */
