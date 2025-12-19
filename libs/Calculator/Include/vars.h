#ifndef _VARS_H_
#define _VARS_H_

#include <stdio.h>
#include "errors.h"

struct VarInfo {
    const char* object_name = "";

    const char* file_name  = "";
    
    const char* func_name  = "";

    int line_ind     =  0;
};

VarInfo VarInfoCtor(const char* object_name, const char* file_name, const char* func_name, const int line_ind);

CodeError_t PrintVarInfo(const VarInfo varinfo);

#endif