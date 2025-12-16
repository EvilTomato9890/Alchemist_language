#ifndef FOREST_OPERATIONS_H_INCLUDED
#define FOREST_OPERATIONS_H_INCLUDED

#include "forest_info.h"
#include "tree_info.h"
#include "error_handler.h"
#include "debug_meta.h"

error_code forest_init(forest_t* forest ON_DEBUG(, ver_info_t ver_info));
    
ON_DEBUG(
error_code forest_open_dump_file(forest_t* forest, const char* dump_file_name);

error_code forest_close_dump_file(forest_t* forest);

error_code forest_open_tex_file(forest_t* forest, const char* tech_file_name);

error_code forest_close_tex_file(forest_t* forest);
)
tree_t* forest_add_tree(forest_t* forest, error_code* error_ptr);

error_code forest_delete_tree(forest_t* forest, tree_t* tree);

tree_t* forest_include_tree(forest_t* forest, tree_t* tree, error_code* error_ptr);

error_code forest_exclude_tree(forest_t* forest, tree_t* tree);

error_code forest_dest(forest_t* forest);

#endif