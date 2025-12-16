#ifndef LIBS_AST_INCLUDE_BUILTIN_FUNC_INFO_H_NCLUDED
#define LIBS_AST_INCLUDE_BUILTIN_FUNC_INFO_H_NCLUDED

#include <stdlib.h>

enum func_code_t {
    ADD,
    SUB,
    MUL,
    DIV,

    POW,
    LOG,
    LN,
    EXP,

    SIN,
    COS,
    TAN,
    CTAN,

    ARCSIN,
    ARCCOS,
    ARCTAN,
    ARCCTAN,

    CH,
    SH,

    ARCSH, 
    ARCCH
};

struct builtin_func_t {
    func_code_t    code;
    const char*  name;
};

extern const builtin_func_t builtin_func_table[];
extern const size_t builtin_func_table_size;

#endif /* LIBS_AST_INCLUDE_BUILTIN_FUNC_INFO_H_NCLUDED */
