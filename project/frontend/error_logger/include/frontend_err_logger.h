#ifndef PROJECT_FRONTEND_INCLUDE_FRONTEND_ERR_LOGGER_H_NCLUDED
#define PROJECT_FRONTEND_INCLUDE_FRONTEND_ERR_LOGGER_H_NCLUDED

#include <stdio.h>

#include "libs/My_string/include/my_string.h"
#include "libs/Vector/include/vector.h"

const int MAX_ERROR_MESSAGE_LENGTH = 512;

enum error_source_t {
    LEXER_ERROR,
    PARSER_ERROR,
};

enum diag_code_t {
    DIAG_LEX_UNKNOWN_SYMBOL = 1,
    DIAG_LEX_BAD_NUMBER     = 2,
    DIAG_LEX_UNTERMINATED_COMMENT = 3,
};

struct diag_log_t {
    error_source_t  source;
    diag_code_t code;

    size_t position;
    size_t length;
    
    size_t line;
    size_t column;

    char message[MAX_ERROR_MESSAGE_LENGTH]; 
};

void print_diags(FILE* stream, c_string_t buffer, 
                 const char* file_name, const vector_t* diags);

diag_log_t diag_log_init(error_source_t source, diag_code_t code,
                         size_t position, size_t length,
                         size_t line,     size_t column,
                         const char* message, ...);

#endif /* PROJECT_FRONTEND_INCLUDE_FRONTEND_ERR_LOGGER_H_NCLUDED */
