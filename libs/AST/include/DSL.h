#ifndef LIBS_AST_INCLUDE_DSL_H_NCLUDED
#define LIBS_AST_INCLUDE_DSL_H_NCLUDED

#include "libs/AST/include/tree_info.h"
#include "libs/AST/include/tree_operations.h"
#include "common/keywords/include/keywords.h"

#define cpy(node) subtree_deep_copy(node, nullptr ON_DUMP_CREATION_DEBUG(, tree))

#define c(val) \
    init_node(CONSTANT, make_union_const(val), nullptr, nullptr)

#define v(var_name) \
    init_node(IDENT, make_union_var(get_or_add_ident_idx({var_name, strlen(var_name)}, tree->ident_stack, nullptr)), nullptr, nullptr)

#define FUNC_TEMPLATE(op_code, left, right) \
    init_node(FUNCTION, make_union_func(op_code), left, right)

//================================================================================

#define PLUS_(left, right)  FUNC_TEMPLATE(OP_PLUS,     left, right)
#define MINUS_(left, right) FUNC_TEMPLATE(OP_MINUS,     left, right)
#define MUL_(left, right)   FUNC_TEMPLATE(OP_MUL,     left, right)
#define DIV_(left, right)   FUNC_TEMPLATE(OP_DIV,     left, right)

#define POW_(left, right)   FUNC_TEMPLATE(OP_POW,     left, right)
#define LOG_(left, right)   FUNC_TEMPLATE(OP_LOG,     left, right)

//================================================================================

#endif /* LIBS_AST_INCLUDE_DSL_H_NCLUDED */
