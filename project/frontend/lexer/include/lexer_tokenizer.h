#ifndef TOKENIZER_H_INCLUDED
#define TOKENIZER_H_INCLUDED

#include <stddef.h>
#include <stdbool.h>

#include "libs/Vector/include/vector.h"
#include "libs/My_string/include/my_string.h"
#include "common/keywords/include/keywords.h"

//================================================================================

enum lexer_token_kind_t {
    LEX_TK_EOF     = 0,
    LEX_TK_NUMBER  = 1,
    LEX_TK_IDENT   = 2,
    LEX_TK_LPAREN  = 3,
    LEX_TK_RPAREN  = 4,
    LEX_TK_KEYWORD = 5,
};

enum lexer_diag_code_t {
    LEX_DIAG_UNKNOWN_SYMBOL = 1,
    LEX_DIAG_BAD_NUMBER     = 2,
    LEX_DIAG_UNTERMINATED_COMMENT = 3,
};

enum lexer_error_t {
    LEX_ERR_OK       = 0,
    LEX_ERR_BAD_ARG  = 1,
    LEX_ERR_NO_MEM   = 2,
    LEX_ERR_VEC_FAIL = 3,
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
    opcode_t   opcode;
};

struct lexer_diag_t {
    lexer_diag_code_t code;

    size_t position;
    size_t length;

    size_t line;
    size_t column;

    unsigned char got;
};

//================================================================================

lexer_error_t lexer_tokenize(c_string_t buffer, const lexer_config_t* config,
                             vector_t* tokens_out, vector_t* diags_out);

//================================================================================
#endif // TOKENIZER_H_INCLUDED
