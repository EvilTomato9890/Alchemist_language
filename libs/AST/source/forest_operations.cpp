#include <errno.h>

#include "forest_operations.h"
#include "error_handler.h"
#include "../libs/StackDead-main/stack.h"
#include "tree_info.h"
#include "tree_verification.h"
#include "asserts.h"
#include "logger.h"
#include "../libs/List/include/list_operations.h"
#include "../libs/List/include/list_info.h"
#include "tree_operations.h"
#include "forest_info.h"
#include "file_operations.h"
#include "tex_io.h"

//================================================================================

static error_code tree_full_destroy(list_type val) {
    tree_t* tree = (tree_t*)val;
    if (!tree) return ERROR_NO;

    error_code error = tree_destroy(tree);
    free(tree);
    return error;
}

error_code forest_init(forest_t* forest ON_DEBUG(, ver_info_t ver_info)) {
    HARD_ASSERT(forest != nullptr, "Forest_ptr is nulltpr");

    LOGGER_DEBUG("forest_init: started");

    error_code error = ERROR_NO;


    stack_t* stack = (stack_t*)calloc(1, sizeof(stack_t));
    if(!stack) {
        LOGGER_ERROR("forest_init: calloc for stack failed");
        return ERROR_MEM_ALLOC;
    }
    error |= stack_init(stack, 1 ON_DEBUG(, VER_INIT));
    RETURN_IF_ERROR(error, free(stack););


    list_t*  list  = (list_t*) calloc(1, sizeof(list_t));
    if(!list) {
        LOGGER_ERROR("forest_init: calloc for list failed");
        free(stack);
        return ERROR_MEM_ALLOC;
    }
    error |= list_init(list, 1 ON_DEBUG(, VER_INIT));
    RETURN_IF_ERROR(error, free(stack); free(list););


    ON_DEBUG({
    forest->dump_file = nullptr;
    forest->ver_info  = ver_info;
    })
    forest->var_stack = stack;
    forest->tree_list = list;
    forest->buff = {nullptr, 0};

    return error;
}

error_code forest_dest(forest_t* forest) {
    LOGGER_DEBUG("forest_dest: started");
    if(!forest) {
        LOGGER_WARNING("forest_dest: forest is nullptr");
        return ERROR_NO;
    }
    
    error_code error = 0;

    error |= list_dest(forest->tree_list, &tree_full_destroy);
    free(forest->tree_list);          
    forest->tree_list = nullptr;

    error |= stack_destroy(forest->var_stack);
    free(forest->var_stack);
    forest->var_stack = nullptr;

    forest->buff.ptr = nullptr;
    forest->buff.len = 0;

    return error;
}

tree_t* forest_add_tree(forest_t* forest, error_code* error_ptr) {
    HARD_ASSERT(forest != nullptr, "Forest is nullptr");
    HARD_ASSERT(error_ptr != nullptr, "Error is nullptr");
    HARD_ASSERT(forest->tree_list != nullptr, "Tree list is nullptr");

    LOGGER_DEBUG("Forest_add_tree: started");

    error_code error = ERROR_NO;

    tree_t* tree = (tree_t*)calloc(1, sizeof(tree_t));
    if(!tree) {
        LOGGER_ERROR("forest_Add_tree: mem alloc for tree is failed");
        *error_ptr |= ERROR_MEM_ALLOC;
        return nullptr;
    }

    error |= tree_init(tree, forest->var_stack ON_DEBUG(, VER_INIT));
    if(error != ERROR_NO) {
        LOGGER_ERROR("forest_add_tree: tree_init failed");
        *error_ptr |= ERROR_INSERT_FAIL;
        return nullptr;
    }

    ssize_t idx = list_push_back(forest->tree_list, tree);
    if(idx == -1) {
        LOGGER_ERROR("forest_add_tree: list_push_back failed");
        *error_ptr |= ERROR_INSERT_FAIL;
        return nullptr;
    }

    tree->list_idx = idx; 
    tree->buff     = forest->buff;
    ON_DEBUG(
    tree->dump_file = &forest->dump_file;
    )
    tree->tex_file  = &forest->tex_file;
    
    return tree;
}

error_code forest_delete_tree(forest_t* forest, tree_t* tree) {
    HARD_ASSERT(forest != nullptr, "Forest is nullptr");
    HARD_ASSERT(tree   != nullptr, "tree is nullptr");

    LOGGER_DEBUG("forest_delete_tree: started");

    error_code error = ERROR_NO;

    error |= tree_destroy(tree);
    RETURN_IF_ERROR(error);

    error |= list_remove(forest->tree_list, tree->list_idx);
    RETURN_IF_ERROR(error);

    free(tree);

    return error;

}

tree_t* forest_include_tree(forest_t* forest, tree_t* tree, error_code* error_ptr) {
    HARD_ASSERT(forest    != nullptr, "Forest is nullptr");
    HARD_ASSERT(tree      != nullptr, "Tree is nullptr");
    HARD_ASSERT(error_ptr != nullptr, "Error_ptr is nullptr");

    LOGGER_DEBUG("forest_include_tree: started");

    tree->buff      = forest->buff;
    tree->var_stack = forest->var_stack;

    ssize_t idx = list_push_back(forest->tree_list, tree);
    if(idx == -1) {
        LOGGER_ERROR("forest_add_tree: list_push_back failed");
        *error_ptr = ERROR_INSERT_FAIL;
        return nullptr;
    }

    tree->list_idx = idx; 
    tree->buff     = forest->buff;
    ON_DEBUG(
    tree->dump_file = &forest->dump_file;
    )
    return tree;

}

error_code forest_exclude_tree(forest_t* forest, tree_t* tree) {
    HARD_ASSERT(forest != nullptr, "Forest is nullptr");
    HARD_ASSERT(tree != nullptr, "Tree is nullptr");

    LOGGER_DEBUG("Forest_exclude_tree: started");

    error_code error = ERROR_NO;

    error |= list_remove(forest->tree_list, tree->list_idx);
    RETURN_IF_ERROR(error);

    tree->buff      = {nullptr, 0};
    tree->var_stack = nullptr;
    ON_DEBUG(
    tree->dump_file = nullptr;
    )
    return ERROR_NO;
}

//================================================================================

ON_DEBUG(
error_code forest_open_dump_file(forest_t* forest, const char* dump_file_name) {
    HARD_ASSERT(forest            != nullptr, "Forest_ptr is nullptr");
    HARD_ASSERT(dump_file_name    != nullptr, "Dump_file_name is nullptr");
    HARD_ASSERT(forest->tree_list != nullptr, "List is nullptr");
    LOGGER_DEBUG("Forest_open_dump_file: started");

    FILE* dump_file = fopen(dump_file_name, "w");
    if(!dump_file) {
        LOGGER_ERROR("File open error");
        errno = 0;
        return ERROR_OPEN_FILE;
    }
    forest->dump_file = dump_file;
    return ERROR_NO;
}

error_code forest_close_dump_file(forest_t* forest) {
    HARD_ASSERT(forest != nullptr, "Forest is nullptr");

    LOGGER_DEBUG("Forest_close_dump_file: started");
    if(!forest->dump_file) {
        LOGGER_WARNING("dump_file is nullptr");
        return ERROR_NO;
    }

    int error = fclose(forest->dump_file);
    if(error != 0) {
        LOGGER_ERROR("forest_close_dump_file: Failed to close dump_file");
        return ERROR_CLOSE_FILE;
    }
    return ERROR_NO;
}

error_code forest_open_tex_file(forest_t* forest, const char* tex_file_name) {
    HARD_ASSERT(forest            != nullptr, "Forest_ptr is nullptr");
    HARD_ASSERT(tex_file_name    != nullptr, "Dump_file_name is nullptr");
    HARD_ASSERT(forest->tree_list != nullptr, "List is nullptr");

    LOGGER_DEBUG("Forest_open_tex_file: started");

    FILE* tex_file = fopen(tex_file_name, "w");
    if(!tex_file) {
        LOGGER_ERROR("File open error");
        errno = 0;
        return ERROR_OPEN_FILE;
    }
    forest->tex_file = tex_file;
    print_tex_header(tex_file);
    return ERROR_NO;
}

error_code forest_close_tex_file(forest_t* forest) {
    HARD_ASSERT(forest != nullptr, "Forest is nullptr");

    LOGGER_DEBUG("Forest_close_tex_file: started");
    if(!forest->tex_file) {
        LOGGER_WARNING("tex_file is nullptr");
        return ERROR_NO;
    }

    print_tex_footer(forest->tex_file);
    int error = fclose(forest->tex_file);
    if(error != 0) {
        LOGGER_ERROR("forest_close_tex_file: Failed to close tex_file");
        return ERROR_CLOSE_FILE;
    }
    return ERROR_NO;
}
)
