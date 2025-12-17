#ifndef COMMON_KEYWORDS_INCLUDE_KEYWORDS_H_NCLUDED
#define COMMON_KEYWORDS_INCLUDE_KEYWORDS_H_NCLUDED

#include <stddef.h>

//================================================================================

enum opcode_t {
    OP_NONE = 0,
                                                                                                                           
    OP_PLUS,
    OP_MINUS,
    OP_MUL,
    OP_DIV,
                                                                               
    OP_EQ,
    OP_NEQ,
    OP_LE,
    OP_GE,
    OP_LT,
    OP_GT,      
                                                                                           
    OP_ASSIGN,
                                                                                                                       
    OP_LBRACE,
    OP_RBRACE,
    OP_SEMI,
    OP_COMMA,
                                                                                                     
    OP_FUNC_DECL,
};

//================================================================================

struct keyword_def_t {
    opcode_t       opcode;
    const char*    lang_name; // имя в языке (может содержать пробелы)
    const char*    tree_name; // имя в дереве (AST)
};

//================================================================================

extern const keyword_def_t KEYWORDS[];
extern const size_t        KEYWORDS_COUNT;

extern const keyword_def_t IGNORED_KEYWORDS[];
extern const size_t        IGNORED_KEYWORDS_COUNT;

#endif /* COMMON_KEYWORDS_INCLUDE_KEYWORDS_H_NCLUDED */
