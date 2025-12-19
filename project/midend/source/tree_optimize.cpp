#include <math.h>

#include "common/asserts/include/asserts.h"
#include "common/logger/include/logger.h"
#include "libs/AST/include/tree_info.h"
#include "libs/AST/include/error_handler.h"
#include "libs/AST/include/tree_operations.h"
#include "common/keywords/include/keywords.h"
#include "tree_optimize.h"

static const double CMP_PRECISION = 1e-9;

static int double_cmp(double a, double b) {
    if(fabs(a - b) < CMP_PRECISION) return 0;
    if(a - b       > CMP_PRECISION)  return 1;
    return -1;
}

static bool node_is_constant(const tree_node_t* node) {
    return node != nullptr && node->type == CONSTANT;
}

#define HANDLE_FUNC(op_code, impl_func, arg_cnt, ...)           \
    static const_val_type op_code##_func(const_val_type a, const_val_type b) {  \
        HARD_ASSERT(!isnan(a), "a is nan");                               \
        if(arg_cnt == 2) HARD_ASSERT(!isnan(b), "b is nan");              \
        return impl_func;                                                 \
    }

#include "copy_past_file"

#undef HANDLE_FUNC

//--------------------------------------------------------------------------------

static error_code handle_fixed_elem(tree_node_t* node, const_val_type constant_value) {
    if (node == nullptr) return ERROR_NO;

    error_code error = ERROR_NO;

    if (node->left) {
        error |= destroy_node_recursive(node->left, nullptr);
        node->left = nullptr;
    }
    if (node->right) {
        error |= destroy_node_recursive(node->right, nullptr);
        node->right = nullptr;
    }

    node->type           = CONSTANT;
    node->value.constant = constant_value;

    if (error != ERROR_NO) {
        LOGGER_ERROR("handle_fixed_elem: destroy_node_recursive failed");
    }
    return error;
}

//--------------------------------------------------------------------------------

static error_code handle_neutral_elem(tree_node_t* node,
                                      tree_node_t* child_keep_ptr,
                                      tree_node_t* child_drop_ptr) {
    HARD_ASSERT(node           != nullptr,        "handle_neutral_elem: node is nullptr");
    HARD_ASSERT(child_keep_ptr != nullptr, "handle_neutral_elem: child_keep_ptr is nullptr");

    error_code error = ERROR_NO;

    if (child_drop_ptr != nullptr) {
        error |= destroy_node_recursive(child_drop_ptr, nullptr);
    }

    tree_node_t child_copy = *child_keep_ptr;
    *node = child_copy;

    free(child_keep_ptr);

    if (error != ERROR_NO) {
        LOGGER_ERROR("handle_neutral_elem: destroy_node_recursive failed");
    }
    return error;
}

//--------------------------------------------------------------------------------

static const_val_type eval_function_constant(op_code_t      func_type_value,
                                             const_val_type left_value,
                                             const_val_type right_value) {

    #define HANDLE_FUNC(func_name, ...) \
        case func_name:                 \
            return func_name##_func(left_value, right_value);

    switch (func_type_value) {
        #include "copy_past_file"
        default:
            LOGGER_ERROR("eval_function_constant: Unknown func op_code %d",
                         (int)func_type_value);
            return NAN;
    }

    #undef HANDLE_FUNC
}

//--------------------------------------------------------------------------------

static bool get_is_calculatable(op_code_t op_code) { //TODO: change
    for (size_t i = 0; i < KEYWORDS_COUNT; i++) {
        if (op_code == KEYWORDS[i].op_code) return KEYWORDS[i].is_calculatable;
    }
    LOGGER_ERROR("Unknown op_code");
    return false;
}

static error_code fold_constants_in_node(tree_node_t* node) {
    HARD_ASSERT(node != nullptr, "fold_constants_in_node: node is nullptr");

    if (node->type != FUNCTION)                 return ERROR_NO;
    if (!get_is_calculatable(node->value.func)) return ERROR_NO;
    tree_node_t* left_ptr  = node->left;
    tree_node_t* right_ptr = node->right;

    const bool is_left_constant  = node_is_constant(left_ptr);
    const bool is_right_constant = node_is_constant(right_ptr);

    const bool is_unary = (right_ptr == nullptr);

    if (is_unary) {
        if (!is_left_constant) {
            return ERROR_NO;
        }
    } else {
        if (!is_left_constant || !is_right_constant) {
            return ERROR_NO;
        }
    }

    const_val_type  left_value =  left_ptr ?  left_ptr->value.constant : 0.0;
    const_val_type right_value = right_ptr ? right_ptr->value.constant : 0.0;

    const_val_type result_value = eval_function_constant(node->value.func, left_value, right_value);
    if (isnan(result_value)) {
        LOGGER_ERROR("fold_constants_in_node: eval_function_constant failed");
        return ERROR_UNKNOWN_FUNC;
    }

    error_code error = ERROR_NO;

    if (left_ptr != nullptr) {
        error |= destroy_node_recursive(left_ptr, nullptr);
    }
    if (right_ptr != nullptr) {
        error |= destroy_node_recursive(right_ptr, nullptr);
    }

    node->left           = nullptr;
    node->right          = nullptr;
    node->type           = CONSTANT;
    node->value.constant = result_value;

    if (error != ERROR_NO) {
        LOGGER_ERROR("fold_constants_in_node: destroy_node_recursive failed");
    }

    return error;
}

//--------------------------------------------------------------------------------

struct arg_rule_t {
    const_val_type neutral_value;
    const_val_type fixed_value;
    const_val_type fixed_result;
};

struct op_rule_t {
    op_code_t  func_type_value;
    size_t     arg_count;
    arg_rule_t args[MAX_NEUTRAL_ARGS];
};

#define ARG_RULE(neu, fix, res)  { (neu), (fix), (res) }

static const op_rule_t OP_SIMPLIFY_RULES[] = {
    // u + 0 = u, 0 + v = v
    { OP_PLUS, 2, {
        ARG_RULE(0.0, NAN, 0.0),  
        ARG_RULE(0.0, NAN, 0.0)   
    }},

    // u - 0 = u
    { OP_MINUS, 2, {
        ARG_RULE(NAN, NAN, 0.0),
        ARG_RULE(0.0, NAN, 0.0)  
    }},

    // 1 * v = v, u * 1 = u
    // 0 * v = 0, u * 0 = 0
    { OP_MUL, 2, {
        ARG_RULE(1.0, 0.0, 0.0),  
        ARG_RULE(1.0, 0.0, 0.0)  
    }},

    // 0 / v = 0   u / 1 = u
    { OP_DIV, 2, {
        ARG_RULE(NAN, 0.0, 0.0), 
        ARG_RULE(1.0, NAN, 0.0)  
    }},

    // 1^v = 1  u^0 = 1  u^1 = u      
    { OP_POW, 2, {
        ARG_RULE(NAN, 1.0, 1.0),  
        ARG_RULE(1.0, 0.0, 1.0)   
    }},

    // log_u(1) = 0 
    { OP_LOG, 2, {
        ARG_RULE(NAN, NAN, 0.0),  
        ARG_RULE(NAN, 1.0, 0.0)   
    }},
};

#undef ARG_RULE

static const op_rule_t* find_op_rule(op_code_t func_type_value) {
    const size_t rules_count = sizeof(OP_SIMPLIFY_RULES) / sizeof(OP_SIMPLIFY_RULES[0]);
    for (size_t i = 0; i < rules_count; ++i) {
        if (OP_SIMPLIFY_RULES[i].func_type_value == func_type_value) {
            return &OP_SIMPLIFY_RULES[i];
        }
    }
    return nullptr;
}

//--------------------------------------------------------------------------------
//TODO: поправить для разных фиксированных элементов аргументов
static error_code simplify_fixed_elements(tree_node_t* node,
                                          const op_rule_t* rule) {
    HARD_ASSERT(node != nullptr, "simplify_fixed_elements: node is nullptr");
    HARD_ASSERT(rule != nullptr, "simplify_fixed_elements: rule is nullptr");

    tree_node_t* args[MAX_NEUTRAL_ARGS] = { node->left, node->right };
    error_code error = ERROR_NO;

    for (size_t i = 0; i < rule->arg_count; ++i) {
        const arg_rule_t& arg_rule = rule->args[i];
        tree_node_t*      arg_node = args[i];

        if (isnan(arg_rule.fixed_value))                                     continue;
        if (!node_is_constant(arg_node))                                     continue;
        if (double_cmp(arg_node->value.constant, arg_rule.fixed_value) != 0) continue;

        error |= handle_fixed_elem(node, arg_rule.fixed_result);
        return error;
    }

    return error;
}


static error_code simplify_neutral_elements(tree_node_t* node,
                                            const op_rule_t* rule) {
    HARD_ASSERT(node != nullptr, "simplify_neutral_elements: node is nullptr");
    HARD_ASSERT(rule != nullptr, "simplify_neutral_elements: rule is nullptr");

    tree_node_t* args[MAX_NEUTRAL_ARGS] = { node->left, node->right };
    error_code error = ERROR_NO;

    for (size_t i = 0; i < rule->arg_count; ++i) {
        const arg_rule_t& arg_rule = rule->args[i];
        tree_node_t*      arg_node = args[i];

        if (isnan(arg_rule.neutral_value))                                  continue;
        if (!node_is_constant(arg_node))                                    continue;
        if (double_cmp(arg_node->value.constant,  arg_rule.neutral_value))  continue;

        size_t other_index = (i == 0 ? 1 : 0);
        tree_node_t* keep_ptr = args[other_index];
        tree_node_t* drop_ptr = arg_node;

        if (keep_ptr != nullptr) {
            error |= handle_neutral_elem(node, keep_ptr, drop_ptr);
        }
        return error;
    }

    return error;
}

//--------------------------------------------------------------------------------

static error_code simplify_neutral_and_constant_elements(tree_node_t* node) {
    HARD_ASSERT(node != nullptr,
                "simplify_neutral_and_constant_elements: node is nullptr");

    if (node->type != FUNCTION) return ERROR_NO;

    const op_rule_t* rule = find_op_rule(node->value.func);
    if (rule == nullptr) {
        return ERROR_NO;
    }

    error_code error = ERROR_NO;

    error |= simplify_fixed_elements(node, rule);
    if (error != ERROR_NO) {
        return error;
    }

    if (node->type != FUNCTION) {
        return ERROR_NO;
    }

    error |= simplify_neutral_elements(node, rule);
    return error;
}

//--------------------------------------------------------------------------------
//TODO: 0^0
tree_node_t* optimize_subtree_recursive(tree_node_t* node, error_code* error_ptr) {
    HARD_ASSERT(error_ptr != nullptr, "optimize_subtree_recursive: error_ptr is nullptr");

    if (node == nullptr) {
        return nullptr;
    }

    if (*error_ptr != ERROR_NO) {
        return node;
    }

    if (node->type == FUNCTION) {
        node->left  = optimize_subtree_recursive(node->left,  error_ptr);
        node->right = optimize_subtree_recursive(node->right, error_ptr);

        *error_ptr |= fold_constants_in_node(node);
        *error_ptr |= simplify_neutral_and_constant_elements(node);
    }

    return node;
}

//================================================================================

error_code tree_optimize(tree_t* tree) {
    HARD_ASSERT(tree != nullptr, "tree_optimize: tree is nullptr");

    LOGGER_DEBUG("tree_optimize: started");
    if (tree->root == nullptr) {
        return ERROR_NO;
    }

    error_code error_value = ERROR_NO;
    tree->root = optimize_subtree_recursive(tree->root, &error_value);

    if (error_value != ERROR_NO) {
        LOGGER_ERROR("tree_optimize: optimize_subtree_recursive failed");
        return error_value;
    }

    tree->size = count_nodes_recursive(tree->root);
    return ERROR_NO;
}