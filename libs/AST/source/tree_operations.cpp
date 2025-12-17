#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "debug_meta.h"
#include "asserts.h"
#include "common/logger/include/logger.h"
#include "tree_operations.h"
#include "tree_verification.h"
#include "tree_info.h"
#include "error_handler.h"
#include "libs/Stack/include/ident_stack.h"
#include "libs/My_string/include/my_string.h"

//================================================================================

value_t make_union_const(const_val_type constant) {
    value_t val = {.constant = constant};
    return val;
}

value_t make_union_var(size_t var_idx) {
    value_t val = {.var_idx = var_idx};
    return val;
}

value_t make_union_func(op_code_t func) {
    value_t val = {.func = func};
    return val;
}

value_t make_union_universal(node_type_t type, ...) {
    value_t val = {};

    va_list ap = {};
    va_start(ap, type);

    switch (type) {
        case CONSTANT: {
            const_val_type constant = va_arg(ap, const_val_type);
            val.constant = constant;
            break;
        }
        case VARIABLE: {
            int var = va_arg(ap, int);
            val.var_idx = var;
            break;
        }
        case FUNCTION: {
            int func = va_arg(ap, int);
            val.func = (op_code_t)func;
            break;
        }
        default:
            LOGGER_ERROR("make_union: unknown node_type_t %d", type);
            break;
    }
    va_end(ap);
    return val;
}

tree_node_t* init_node(node_type_t node_type, value_t value, tree_node_t* left, tree_node_t* right) {
    tree_node_t* node = (tree_node_t*)calloc(1, sizeof(tree_node_t));
    if (node == nullptr) {
        LOGGER_ERROR("allocate_node: calloc failed");
        return nullptr;
    }
    node->type   = node_type;
    node->value  = value;
    node->left   = left;
    node->right  = right;
    return node;
}

tree_node_t* init_node_with_dump(node_type_t node_type, value_t value, tree_node_t* left, tree_node_t* right, const tree_t* tree) {
    HARD_ASSERT(tree  != nullptr, "tree is nullptr");

    tree_node_t* node = init_node(node_type, value, left, right);
    tree_t tree_clone = {};
    tree_clone = *tree;
    tree_change_root(&tree_clone, node);
    tree_dump(&tree_clone, TREE_VER_INIT, true, "DSL dumps");
    
    return node;
}

error_code destroy_node_recursive(tree_node_t* node, size_t* removed_out) {
    error_code error = ERROR_NO;
    size_t removed_local = 0;
    if (node == nullptr) {
        if (removed_out != nullptr) *removed_out = 0;
        return error;
    }
    size_t left_removed = 0;
    error |= destroy_node_recursive(node->left, &left_removed);

    size_t right_removed = 0;
    error |= destroy_node_recursive(node->right, &right_removed);

    free(node);
    removed_local = 1 + left_removed + right_removed;
    if (removed_out != nullptr) *removed_out = removed_local;
    return error;
}

error_code tree_init(tree_t* tree, ident_stack_t* stack ON_TREE_DEBUG(, tree_ver_info_t ver_info)) {
    HARD_ASSERT(tree != nullptr, "tree pointer is nullptr");

    LOGGER_DEBUG("tree_init: started");

    error_code error = ERROR_NO;

    tree->root = nullptr;
    tree->size = 0;
    tree->buff = {nullptr, 0};

    //error = stack_init(stack, 10 ON_TREE_DEBUG(, TREE_VER_INIT));
    tree->ident_stack = stack;

    ON_TREE_DEBUG({
        tree->ver_info  = ver_info;
        tree->dump_file = nullptr;
    })

    return error;
}

error_code tree_destroy(tree_t* tree) {
    HARD_ASSERT(tree != nullptr, "tree pointer is nullptr");

    LOGGER_DEBUG("tree_dest: started");

    size_t removed = 0;
    error_code error = ERROR_NO;
    error |= destroy_node_recursive(tree->root, &removed);
    tree->root = nullptr;
    tree->size = 0;
    error |= ident_stack_destroy(tree->ident_stack);
    free(tree->ident_stack);
    tree->ident_stack = nullptr;
    return error;
}
//TODO очистка squashes

tree_node_t* subtree_deep_copy(const tree_node_t* node, error_code* error ON_TREE_DUMP_CREATION_DEBUG(, const tree_t* tree)) {

    if (error != nullptr && *error != ERROR_NO) return nullptr;
    if (node == nullptr) return nullptr;

    tree_node_t* left_copy  = subtree_deep_copy(node->left,  error ON_TREE_DUMP_CREATION_DEBUG(, tree));
    tree_node_t* right_copy = subtree_deep_copy(node->right, error ON_TREE_DUMP_CREATION_DEBUG(, tree));

    if (error != nullptr && *error != ERROR_NO) {
        return nullptr;
    }
    #ifdef CREATION_DEBUG
        tree_node_t* copy = init_node_with_dump(node->type, node->value, left_copy, right_copy, tree);
    #else 
        tree_node_t* copy = init_node(node->type, node->value, left_copy, right_copy);
    #endif

    if (!copy) {
        LOGGER_ERROR("clone_subtree: init_node failed");
        if(error != nullptr) *error |= ERROR_MEM_ALLOC;
        return nullptr;
    }

    return copy;
}

bool tree_is_empty(const tree_t* tree) {
    HARD_ASSERT(tree != nullptr,      "tree pointer is nullptr");
    return tree->root == nullptr;
}

bool is_subree_const(const tree_node_t* node) {
    if (!node) return false;
    if (node->type == CONSTANT) return true;
    if (node->type == VARIABLE) return false;
    return is_subree_const(node->left) && is_subree_const(node->right);
}

size_t count_nodes_recursive(const tree_node_t* node) {
    if (node == nullptr) return 0;
    return 1 + count_nodes_recursive(node->left) + count_nodes_recursive(node->right);
}

//================================================================================

tree_node_t* tree_init_root(tree_t* tree, node_type_t node_type, value_t value) {
    HARD_ASSERT(tree   != nullptr,    "tree pointer is nullptr");

    LOGGER_DEBUG("tree_init_roo: started");

    if (tree->root != nullptr) {
        LOGGER_ERROR("tree_init_root: root already exists");
        return nullptr;
    }
    tree_node_t* node = init_node(node_type, value, nullptr, nullptr);
    if (node == nullptr) return nullptr;

    tree_change_root(tree, node);

    return node;
}

error_code tree_change_root(tree_t* tree, tree_node_t* node) {
    HARD_ASSERT(tree != nullptr, "Tree is nullptr");
    if(!node) {LOGGER_WARNING("New root is nullptr");}

    error_code error = ERROR_NO;

    tree->root = node;
    tree->size = count_nodes_recursive(node);

    return error;
}

error_code tree_replace_root(tree_t* tree, tree_node_t* source_node) {
    HARD_ASSERT(tree        != nullptr, "Tree is nullptr");
    HARD_ASSERT(source_node != nullptr, "Source_node is nullptr");

    return tree_replace_subtree(&tree->root, source_node, &tree->size);
}

error_code tree_replace_subtree(tree_node_t** target_node, tree_node_t* source_node, size_t* new_subtree_size) {
    HARD_ASSERT(target_node      != nullptr, "Node_ptr is nullptr");
    HARD_ASSERT(new_subtree_size != nullptr, "New_subtree_size is nullptr");

    LOGGER_DEBUG("Tree_replace_subtree: started");
    error_code error = ERROR_NO;

    size_t removed_elems_cnt = 0;
    error |= destroy_node_recursive(*target_node, &removed_elems_cnt);
    *target_node = source_node;

    size_t added_elems_cnt = count_nodes_recursive(*target_node);
    *new_subtree_size = added_elems_cnt - removed_elems_cnt;
    return error;
}

tree_node_t* tree_insert_left(tree_t* tree, node_type_t node_type, value_t value, tree_node_t* parent) {
    HARD_ASSERT(tree   != nullptr,    "tree pointer is nullptr");
    HARD_ASSERT(parent != nullptr,    "parent pointer is nullptr");

    if (parent->left != nullptr) {
        LOGGER_ERROR("tree_insert_left: left child already exists");
        return nullptr;
    }
    tree_node_t* node = init_node(node_type, value, nullptr, nullptr);
    if (node == nullptr) return nullptr;
    parent->left = node;
    tree->size += 1;
    return node;
}

tree_node_t* tree_insert_right(tree_t* tree, node_type_t node_type, value_t value, tree_node_t* parent) {
    HARD_ASSERT(tree   != nullptr,    "tree pointer is nullptr");
    HARD_ASSERT(parent != nullptr,    "parent pointer is nullptr");

    if (parent->right != nullptr) {
        LOGGER_ERROR("tree_insert_right: right child already exists");
        return nullptr;
    }
    tree_node_t* node = init_node(node_type, value, nullptr, nullptr);
    if (node == nullptr) return nullptr;
    parent->right = node;
    tree->size += 1;

    return node;
}

error_code tree_replace_value(tree_node_t* node, node_type_t node_type, value_t value) {
    HARD_ASSERT(node     != nullptr,  "node pointer is nullptr");
    
    LOGGER_DEBUG("tree_replace_value: started");
    
    node->type  = node_type;
    node->value = value;
    return ERROR_NO;
}

//================================================================================

ssize_t get_ident_idx(c_string_t ident, const ident_stack_t* ident_stack) {
    HARD_ASSERT(ident_stack != nullptr, "ident_stack is nullptr");

    for(size_t i = 0; i < ident_stack->size; i++) {
        if(my_ssstrcmp(ident_stack->data[i], ident) == 0) {
            return i;
        }
    }
    return -1;
}

size_t add_ident(c_string_t ident, ident_stack_t* ident_stack, error_code* error) {
    HARD_ASSERT(ident_stack != nullptr, "ident_stack is nulltpr");

    if(error == nullptr) ident_stack_push(ident_stack, ident);
    else        *error = ident_stack_push(ident_stack, ident);
    return ident_stack->size - 1;
}

size_t get_or_add_ident_idx(c_string_t ident, ident_stack_t* ident_stack, error_code* error) {
    HARD_ASSERT(ident_stack != nullptr, "ident_stack is nullptr");

    ssize_t idx = get_ident_idx(ident, ident_stack);
    if(idx == -1) {
        return add_ident(ident, ident_stack, error);
    } 
    return (size_t)idx;
}

c_string_t get_var_name(const tree_t* tree, const tree_node_t* node) {  //REVIEW - Стоит ли node или var_idx
    HARD_ASSERT(tree != nullptr, "tree is nullptr");
    HARD_ASSERT(node != nullptr, "Node is nullptr");
    if(node->type != VARIABLE) return {nullptr, 0};

    return tree->ident_stack->data[node->value.var_idx];
}   

//--------------------------------------------------------------------------------

static void clear_input_buff() {
    int ch = 0;
    while ((ch = getchar()) != '\n' && ch != EOF) {}
}
