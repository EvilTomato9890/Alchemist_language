#ifndef COMMON_KEYWORDS_INCLUDE_KEYWORDS_H_NCLUDED
#define COMMON_KEYWORDS_INCLUDE_KEYWORDS_H_NCLUDED

#include <stddef.h>

//================================================================================

enum op_code_t {
    OP_NONE = 0,

    OP_EQ,                                      
    OP_NEQ,                             
    OP_LE,                             
    OP_GE,                             
    OP_LT,                             
    OP_GT,                              
    OP_AND, 
    OP_OR,

    OP_PLUS,                                                        
    OP_MINUS,                           
    OP_MUL,                             
    OP_DIV,  

    OP_ASSIGN,  

    OP_VIS_START,                                           
    OP_LCAT,                             
    OP_ENUM_SEP,    

    OP_IF,                              

    OP_WHILE,                             
    OP_BREAK,                             
    OP_CONTINUE,     
                           
    OP_FINISH,                             
    OP_RETURN,                             
    OP_FUNC_DECL, 
    OP_PROC_DECL, 
    OP_FUNC_INFO,

    OP_CALL,           
    
    OP_PRINT,                             
    OP_INPUT,

};

//================================================================================

struct keyword_def_t {
    op_code_t      op_code;
    const char*    lang_name; 
    const char*    tree_name; 
    bool           is_func;
};

//================================================================================

extern const keyword_def_t KEYWORDS[];
extern const size_t        KEYWORDS_COUNT;

extern const keyword_def_t IGNORED_KEYWORDS[];
extern const size_t        IGNORED_KEYWORDS_COUNT;

#endif /* COMMON_KEYWORDS_INCLUDE_KEYWORDS_H_NCLUDED */
