#ifndef PROJECT_MIDEND_INCLUDE_TREE_OPTIMIZE_H_NCLUDED
#define PROJECT_MIDEND_INCLUDE_TREE_OPTIMIZE_H_NCLUDED

#include "libs/AST/include/tree_info.h"

const int MAX_NEUTRAL_ARGS = 2;

struct args_arr_t {
    size_t* arr;
    size_t  size;
};

error_code tree_optimize(tree_t* tree);

tree_node_t* optimize_subtree_recursive(tree_node_t* node, error_code* error_ptr);

#endif /* PROJECT_MIDEND_INCLUDE_TREE_OPTIMIZE_H_NCLUDED */
