#ifndef LIBS_AST_INCLUDE_TREE_OPERATIONS_H_NCLUDED
#define LIBS_AST_INCLUDE_TREE_OPERATIONS_H_NCLUDED

#include "libs/AST/include/tree_info.h"
#include "libs/AST/include/error_handler.h"
#include "libs/AST/internal/asserts.h"
#include "libs/Stack/include/ident_stack.h"
#include "libs/AST/internal/debug_meta.h"
#include "common/keywords/include/keywords.h"

error_code tree_init(tree_t* tree ON_TREE_DEBUG(, tree_ver_info_t ver_info));

tree_node_t* init_node(node_type_t node_type, value_t value, tree_node_t* left, tree_node_t* right);
tree_node_t* init_node_with_dump(node_type_t node_type, value_t value, tree_node_t* left, tree_node_t* right, const tree_t* tree);

error_code tree_destroy(tree_t* tree);

bool tree_is_empty(const tree_t* tree);
bool is_subree_const(const tree_node_t* node);

size_t count_nodes_recursive(const tree_node_t* node);

error_code   tree_change_root(tree_t* tree, tree_node_t* node);
tree_node_t* tree_init_root(tree_t* tree, node_type_t node_type, value_t value);
tree_node_t* tree_insert_left(tree_t* tree, node_type_t node_type, value_t value, tree_node_t* parent);
tree_node_t* tree_insert_right(tree_t* tree, node_type_t node_type, value_t value, tree_node_t* parent);

error_code tree_replace_value(tree_node_t* node, node_type_t node_type, value_t value);
error_code tree_replace_subtree(tree_node_t** target_node, tree_node_t* source_node, size_t* new_subtree_size);
error_code tree_replace_root(tree_t* tree, tree_node_t* source_node);

error_code destroy_node_recursive(tree_node_t* node, size_t* removed_out);

value_t make_union_const(const_val_type constant);
value_t make_union_var(size_t ident_idx);
value_t make_union_func(op_code_t func);
value_t make_union_universal(node_type_t type, ...);

ssize_t      get_ident_idx       (c_string_t ident, const ident_stack_t* ident_stack);
size_t       add_ident           (c_string_t str, ident_stack_t* ident_stack, error_code* error);
size_t       get_or_add_ident_idx(c_string_t str, ident_stack_t* ident_stack, error_code* error);
c_string_t   get_var_name        (const tree_t* tree, const tree_node_t* node);

inline tree_node_t* clone_node(const tree_node_t* node) {
    HARD_ASSERT(node != nullptr, "node is nullptr");
    return init_node(node->type, node->value, node->left, node->right);
}

tree_node_t* subtree_deep_copy(const tree_node_t* node, error_code* error ON_TREE_DUMP_CREATION_DEBUG(, const tree_t* tree));
tree_node_t* clone_child_subtree(tree_node_t* node, error_code* error ON_TREE_DUMP_CREATION_DEBUG(, const tree_t* tree));

#endif /* LIBS_AST_INCLUDE_TREE_OPERATIONS_H_NCLUDED */
