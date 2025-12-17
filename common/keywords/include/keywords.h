#ifndef COMMON_KEYWORDS_INCLUDE_KEYWORDS_H_NCLUDED
#define COMMON_KEYWORDS_INCLUDE_KEYWORDS_H_NCLUDED

#include <stddef.h>

//================================================================================

enum opcode_t {
    LEX_OP_NONE = 0,

    LEX_OP_PLUS,
    LEX_OP_MINUS,
    LEX_OP_MUL,
    LEX_OP_DIV,

    LEX_OP_EQ,
    LEX_OP_NEQ,
    LEX_OP_LE,
    LEX_OP_GE,
    LEX_OP_LT,
    LEX_OP_GT,

    LEX_OP_ASSIGN,

    LEX_OP_LBRACE,
    LEX_OP_RBRACE,
    LEX_OP_SEMI,
    LEX_OP_COMMA,

    LEX_OP_FUNC_DECL,
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
