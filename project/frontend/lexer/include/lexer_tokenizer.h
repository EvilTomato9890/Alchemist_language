#ifndef PROJECT_FRONTEND_LEXER_INCLUDE_LEXER_TOKENIZER_H_NCLUDED
#define PROJECT_FRONTEND_LEXER_INCLUDE_LEXER_TOKENIZER_H_NCLUDED

#include <stddef.h>
#include <stdbool.h>

#include "libs/Vector/include/vector.h"
#include "libs/My_string/include/my_string.h"
#include "common/keywords/include/keywords.h"

//================================================================================

enum lexer_token_kind_t {
    LEX_TK_EOF,     
    LEX_TK_NUMBER,  
    LEX_TK_IDENT,   
    LEX_TK_LPAREN, 
    LEX_TK_RPAREN, 
    LEX_TK_RBRACE,  
    LEX_TK_KEYWORD, 
};

enum lexer_error_t {
    LEX_ERR_OK,       
    LEX_ERR_BAD_ARG,  
    LEX_ERR_NO_MEM,   
    LEX_ERR_VEC_FAIL, 
};

//================================================================================

struct lexer_config_t {
    const char* filename;

    const keyword_def_t* keywords;
    size_t               keywords_count;

    const keyword_def_t* ignored_words;
    size_t               ignored_words_count;
};

struct lexer_token_t {
    lexer_token_kind_t kind;

    c_string_t lexeme;
    size_t     position;

    size_t     line;
    size_t     column;

    double     number;
    op_code_t  op_code;
};


//================================================================================

lexer_error_t lexer_tokenize(c_string_t buffer, const lexer_config_t* config,
                             vector_t* tokens_out, vector_t* diags_out);

//================================================================================
#endif /* PROJECT_FRONTEND_LEXER_INCLUDE_LEXER_TOKENIZER_H_NCLUDED */
