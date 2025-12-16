#ifndef NODE_INFO_H_INCLUDED
#define NODE_INFO_H_INCLUDED

#include "my_string.h"

#define HANDLE_FUNC(name, ...) name,

enum func_type_t {
    #include "../source/copy_past_file"
};

#undef HANDLE_FUNC


typedef double const_val_type;
typedef double var_val_type;

struct variable_t {
    c_string_t   str;
    var_val_type val;
};

enum node_type_t {
    FUNCTION,
    CONSTANT,
    VARIABLE
};


union value_t {
    const_val_type constant;
    size_t         var_idx;
    func_type_t    func;

};

struct tree_node_t {
    node_type_t  type;
    value_t      value;
    tree_node_t* left;
    tree_node_t* right;
};


#endif