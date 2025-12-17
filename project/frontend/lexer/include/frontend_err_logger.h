#ifndef PROJECT_FRONTEND_INCLUDE_FRONTEND_ERR_LOGGER_H_NCLUDED
#define PROJECT_FRONTEND_INCLUDE_FRONTEND_ERR_LOGGER_H_NCLUDED

#include <stdio.h>

#include "lexer_tokenizer.h"

enum error_source_t {
    LEXER_ERROR,
    PARSER_ERROR,
};

void print_diags(FILE* stream, c_string_t buffer, error_source_t source,
                 const char* file_name, const vector_t* diags);


#endif /* PROJECT_FRONTEND_INCLUDE_FRONTEND_ERR_LOGGER_H_NCLUDED */
