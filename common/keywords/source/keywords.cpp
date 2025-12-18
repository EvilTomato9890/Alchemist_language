#include "keywords.h"

//================================================================================

const keyword_def_t KEYWORDS[] = {
    {OP_EQ,        "==",            "EQ"},
    {OP_NEQ,       "!=",            "NEQ"},
    {OP_LE,        "<=",            "LE"},
    {OP_GE,        ">=",            "GE"},
    {OP_LT,        "<",             "LT"},
    {OP_GT,        ">",             "GT"},
    {OP_AND,       "&&",            "AND"},
    {OP_OR,        "||",            "OR"},
                 
    {OP_PLUS,      "+",             "ADD"},
    {OP_MINUS,     "-",             "SUB"},
    {OP_MUL,       "*",             "MUL"},
    {OP_DIV,       "/",             "DIV"},
                                             
    {OP_ASSIGN,    "=",             "ASSIGN"},
                                     
    {OP_VIS_START, "{",             "VIS_START"},
    {OP_LCAT,      ";",             "LCAT"},
    {OP_ENUM_SEP,  ",",             "ENUM_SEP"},
               
    {OP_IF,         "if",           "IF"},

    {OP_WHILE,      "while",        "WHILE"},
    {OP_BREAK,      "break",        "BREAK"},
    {OP_CONTINUE,   "continue",     "CONTINUE"},

    {OP_FINISH,     "finish",       "FINISH"},
    {OP_RETURN,     "return",       "RETURN"},

    {OP_FUNC_DECL,  "func",         "FUNC_DECL"},
    {OP_PROC_DECL,  "proc",         "PROC_DECL"},

    {OP_CALL,       "call",         "CALL"},

    {OP_PRINT,      "print",        "PRINT"},
    {OP_INPUT,      "input",        "INPUT"},

    {OP_FUNC_INFO,  "NOT_FOR_CODE", "FUNC_INFO"}
    
};

const size_t KEYWORDS_COUNT =
    sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);

//================================================================================

const keyword_def_t IGNORED_KEYWORDS[] = {
    {OP_NONE, "NOT_FOR_CODE", nullptr},
    {OP_NONE, "and", nullptr},
    {OP_NONE, "or",  nullptr},
};

const size_t IGNORED_KEYWORDS_COUNT =
    sizeof(IGNORED_KEYWORDS) / sizeof(IGNORED_KEYWORDS[0]);
