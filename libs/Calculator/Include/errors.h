#ifndef _ERRORS_H_
#define _ERRORS_H_

#include <stdio.h>

#define printerr(...) fprintf(stderr, __VA_ARGS__)

#define line_info __FILE__, __FUNCTION__, __LINE__

#define my_assert(comp, code_err, ret_value) \
    if (comp == 0) { \
        PrintErr(code_err, line_info); \
        return ret_value; \
    }

#define RESET_COLOR  "\033[0m"
#define RED_COLOR    "\033[31m"
#define GREEN_COLOR  "\033[32m"
#define YELLOW_COLOR "\033[33m"
#define BLUE_COLOR   "\033[36m" 

enum CodeError_t {
    NOTHING        =    0,
    NULLPTR        =    1,
    SIZE_ERR       =    2,
    EMPTY_STACK    =    3,
    CAPACITY_ERR   =    4,
    CANARY_ERR     =    5,
    REALLOC_ERR    =    6,
    HASH_ERR       =    7,
    CAP_SIZE_ERR   =    8,
    VALUE_ERR      =    9,
    OPERATION_ERR  =   10,  
    FILE_ERR       =   11,  
    STACK_ERR      =   12,
    CODE_ERR       =   13,
    CMD_IND_ERR    =   15,
    INPUT_ERR      =   16,
    REG_IND_ERR    =   17,
    TERM_ERR       =   18,
    LABEL_ERR      =   19,
    CALLOC_ERR     =   20,
    ERROR_COUNTS
};

void PrintErr(CodeError_t error_type, const char* file_name, const char* func_name, const int line_ind);

#endif // _ERRORS_H