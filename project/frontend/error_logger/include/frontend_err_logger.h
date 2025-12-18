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
    DIAG_LEX_UNKNOWN_SYMBOL,
    DIAG_LEX_BAD_NUMBER,
    DIAG_LEX_UNTERMINATED_COMMENT,
    DIAG_PARSE_EXPECTED,
    DIAG_PARSE_UNDEF_FUNCTION,
    DIAG_PARSE_REDEF_FUNCTION,
    DIAG_PARSE_NESTED_DECL,
    DIAG_PARSE_VOID_IN_EXPR,
    DIAG_PARSE_RETURN_IN_PROC,
    DIAG_PARSE_FINISH_IN_FUNC,
    DIAG_PARSE_BREAK_OUTSIDE,
    DIAG_PARSE_ARGC_MISMATCH,
    DIAG_PARSE_UNDEF_VARIABLE,
    DIAG_PARSE_TOPLEVEL_STMT,
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
