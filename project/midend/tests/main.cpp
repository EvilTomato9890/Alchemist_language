#include "libs/AST/include/DSL.h"
#include "libs/AST/include/node_info.h"
#include "tree_optimize.h"

int main() {
    tree_t tree_main = {};
    tree_t* tree = &tree_main;
    tree_init(tree ON_TREE_DEBUG(, TREE_VER_INIT));
    tree_node_t* new_root = init_node(FUNCTION, make_union_func(OP_BREAK), nullptr, v("x"));
    tree_change_root(tree, new_root);

    tree_optimize(tree);
    if(tree->root->type != FUNCTION && tree->root->value.func != OP_BREAK) printf("\nFailed\n");
    else                                                                   printf("\nPAssed\n");

    tree_destroy(tree);
    return 0;
}