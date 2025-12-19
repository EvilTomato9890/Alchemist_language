#include "keywords.h"

//================================================================================

const keyword_def_t KEYWORDS[] = { //TODO: "!"
    {OP_NONE,      "NOT_FOR_CODE",  "NONE",              false, false },
                                                                                                                                                                                                                    
    {OP_EQ,        "==",            "EQ",                false, true  },
    {OP_NEQ,       "!=",            "NEQ",               false, true  },
    {OP_LE,        "<=",            "LE",                false, true  },
    {OP_GE,        ">=",            "GE",                false, true  },
    {OP_LT,        "<",             "LT",                false, true  },
    {OP_GT,        ">",             "GT",                false, true  },
    {OP_AND,       "&&",            "AND",               false, true  },
    {OP_OR,        "||",            "OR",                false, true  },
                                                                                                                                     
    {OP_PLUS,      "+",             "ADD",               false, true  },
    {OP_MINUS,     "-",             "SUB",               false, true  },
    {OP_MUL,       "*",             "MUL",               false, true  },
    {OP_DIV,       "/",             "DIV",               false, true  },
    {OP_POW,       "pow",           "POW",               true,  true  },
    {OP_LOG,       "log",           "LOG",               true,  true  },
                                                                                                                                                                         
    {OP_ASSIGN,    "=",             "ASSIGN",            false, false},
                                                                                                                                                 
    {OP_VIS_START, "{",             "VIS_START",         false, false },
    {OP_LCAT,      ";",             "LCAT",              false, false },
    {OP_ENUM_SEP,  ",",             "ENUM_SEP",          false, false },
                                                                                                         
    {OP_IF,         "if",           "IF",                false, false },
                                                                                                                 
    {OP_WHILE,      "while",        "WHILE",             false, false },
    {OP_BREAK,      "break",        "BREAK",             false, false },
    {OP_CONTINUE,   "continue",     "CONTINUE",          false, false },
                                                                                                                             
    {OP_FINISH,     "finish",       "FINISH",            false, false },
    {OP_RETURN,     "return",       "RETURN",            false, false },
                                                                                                                     
    {OP_FUNC_DECL,  "func",         "FUNC_DECL",         false, false },
    {OP_PROC_DECL,  "proc",         "PROC_DECL",         false, false },
                                                                                                                 
    {OP_CALL,       "call",         "CALL",              false, false },
                                                                                                                                         
    {OP_PRINT,      "print",        "PRINT",             true,  false },
    {OP_INPUT,      "input",        "INPUT",             true,  false },
                                                                                                             
    {OP_FUNC_INFO,  "NOT_FOR_CODE", "FUNC_INFO",         false, false }
    
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
