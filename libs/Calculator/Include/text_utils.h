#ifndef __TEXT_UTILS_H_
#define __TEXT_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include "errors.h"

struct Text {
    const char *file_name = "";
    int file_size = 0;
    
    char* buf = 0;
};

Text* TextCtor(const char* file_name);

int GetFileSize(const char* file_name);

CodeError_t ReadFile(Text* text);

#endif