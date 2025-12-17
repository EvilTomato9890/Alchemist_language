#include "keywords.h"

//================================================================================

const keyword_def_t KEYWORDS[] = {
    {OP_EQ,        "==",            "EQ"},
    {OP_NEQ,       "!=",            "NEQ"},
    {OP_LE,        "<=",            "LE"},
    {OP_GE,        ">=",            "GE"},
    {OP_LT,        "<",             "LT"},
    {OP_GT,        ">",             "GT"},
                 
    {OP_PLUS,      "+",             "ADD"},
    {OP_MINUS,     "-",             "SUB"},
    {OP_MUL,       "*",             "MUL"},
    {OP_DIV,       "/",             "DIV"},
                                             
    {OP_ASSIGN,    "=",             "ASSIGN"},
                                     
    {OP_LBRACE,    "{",             "LBRACE"},
    {OP_RBRACE,    "}",             "RBRACE"},
    {OP_SEMI,      ";",             "SEMI"},
    {OP_COMMA,     ",",             "COMMA"},
               
    {OP_FUNC_DECL, "function decl", "FUNC_DECL"},
};

const size_t KEYWORDS_COUNT =
    sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);

//================================================================================

const keyword_def_t IGNORED_KEYWORDS[] = {
    {OP_NONE, "and", nullptr},
    {OP_NONE, "or",  nullptr},
};

const size_t IGNORED_KEYWORDS_COUNT =
    sizeof(IGNORED_KEYWORDS) / sizeof(IGNORED_KEYWORDS[0]);
