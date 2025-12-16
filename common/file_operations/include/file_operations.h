#ifndef COMMON_FILE_OPERATIONS_INCLUDE_FILE_OPERATIONS_H_NCLUDED
#define COMMON_FILE_OPERATIONS_INCLUDE_FILE_OPERATIONS_H_NCLUDED

#include <stdlib.h>
#include <stdio.h>

#include "libs/My_string/include/my_string.h"

int read_file_to_buffer_by_name(string_t* buff, const char* filename);

int read_file_to_buffer(FILE* filename, string_t* buff_str);

#endif /* COMMON_FILE_OPERATIONS_INCLUDE_FILE_OPERATIONS_H_NCLUDED */
