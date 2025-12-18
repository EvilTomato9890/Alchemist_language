#include "keywords.h"

//================================================================================

const keyword_def_t KEYWORDS[] = {
    {OP_EQ,        "==",            "EQ",                false },
    {OP_NEQ,       "!=",            "NEQ",               false },
    {OP_LE,        "<=",            "LE",                false },
    {OP_GE,        ">=",            "GE",                false },
    {OP_LT,        "<",             "LT",                false },
    {OP_GT,        ">",             "GT",                false },
    {OP_AND,       "&&",            "AND",               false },
    {OP_OR,        "||",            "OR",                false },
                 
    {OP_PLUS,      "+",             "ADD",               false },
    {OP_MINUS,     "-",             "SUB",               false },
    {OP_MUL,       "*",             "MUL",               false },
    {OP_DIV,       "/",             "DIV",               false },
                                             
    {OP_ASSIGN,    "=",             "ASSIGN",            false },
                                     
    {OP_VIS_START, "{",             "VIS_START",         false },
    {OP_LCAT,      ";",             "LCAT",              false },
    {OP_ENUM_SEP,  ",",             "ENUM_SEP",          false },
               
    {OP_IF,         "if",           "IF",                false },

    {OP_WHILE,      "while",        "WHILE",             false },
    {OP_BREAK,      "break",        "BREAK",             false },
    {OP_CONTINUE,   "continue",     "CONTINUE",          false },

    {OP_FINISH,     "finish",       "FINISH",            false },
    {OP_RETURN,     "return",       "RETURN",            false },

    {OP_FUNC_DECL,  "func",         "FUNC_DECL",         false },
    {OP_PROC_DECL,  "proc",         "PROC_DECL",         false },

    {OP_CALL,       "call",         "CALL",              false },

    {OP_PRINT,      "print",        "PRINT",             true  },
    {OP_INPUT,      "input",        "INPUT",             true  },

    {OP_FUNC_INFO,  "NOT_FOR_CODE", "FUNC_INFO",         false }
    
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
