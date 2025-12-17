#include "keywords.h"

//================================================================================

const keyword_def_t KEYWORDS[] = {
    {LEX_OP_EQ,        "==",            "EQ"},
    {LEX_OP_NEQ,       "!=",            "NEQ"},
    {LEX_OP_LE,        "<=",            "LE"},
    {LEX_OP_GE,        ">=",            "GE"},
    {LEX_OP_LT,        "<",             "LT"},
    {LEX_OP_GT,        ">",             "GT"},

    {LEX_OP_PLUS,      "+",             "ADD"},
    {LEX_OP_MINUS,     "-",             "SUB"},
    {LEX_OP_MUL,       "*",             "MUL"},
    {LEX_OP_DIV,       "/",             "DIV"},

    {LEX_OP_ASSIGN,    "=",             "ASSIGN"},

    {LEX_OP_LBRACE,    "{",             "LBRACE"},
    {LEX_OP_RBRACE,    "}",             "RBRACE"},
    {LEX_OP_SEMI,      ";",             "SEMI"},
    {LEX_OP_COMMA,     ",",             "COMMA"},

    {LEX_OP_FUNC_DECL, "function decl", "FUNC_DECL"},
};

const size_t KEYWORDS_COUNT =
    sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);

//================================================================================

const keyword_def_t IGNORED_KEYWORDS[] = {
    {LEX_OP_NONE, "and", nullptr},
    {LEX_OP_NONE, "or",  nullptr},
};

const size_t IGNORED_KEYWORDS_COUNT =
    sizeof(IGNORED_KEYWORDS) / sizeof(IGNORED_KEYWORDS[0]);
