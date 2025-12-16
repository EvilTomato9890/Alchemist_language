#ifndef FOREST_INFO_H_INCLUDED
#define FOREST_INFO_H_INCLUDED

#include "debug_meta.h"
#include "../libs/List/include/list_info.h"
#include "../libs/StackDead-main/stack.h"

struct forest_t {
    list_t*    tree_list;
    c_string_t buff;
    stack_t*   var_stack;
    ON_DEBUG(
        ver_info_t ver_info;
        FILE* dump_file;
    )
    FILE* tex_file;
    
};

#endif
