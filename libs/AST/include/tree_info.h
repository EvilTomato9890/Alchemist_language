#ifndef TREE_INFO_H_INCLUDED
#define TREE_INFO_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>

#include "debug_meta.h"
#include "node_info.h"
#include "../libs/StackDead-main/stack.h"
#include "my_string.h"

const int MAX_FOREST_CAP = 10;

struct tex_squash_binding_t {
    char         letter;
    tree_node_t* expr_subtree;
};

struct tree_t {
    tree_node_t*   root;
    size_t         size;
    stack_t*       var_stack;
    c_string_t     buff;
    size_t         list_idx;
    ON_DEBUG(
        ver_info_t ver_info;
        FILE* const * dump_file;
    )
    FILE* const * tex_file;
    ON_TEX_SQUASH(
        tree_node_t*           squash_root;
        tex_squash_binding_t*  squash_bindings;
        size_t                 squash_count;
    )
};

#endif
