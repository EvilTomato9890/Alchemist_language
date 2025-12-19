#include "vars.h"

VarInfo VarInfoCtor(const char* object_name, const char* file_name, const char* func_name, const int line_ind) {
    VarInfo varinfo = {};

    varinfo.object_name = object_name;
    varinfo.file_name  = file_name;
    varinfo.func_name  = func_name;
    varinfo.line_ind   = line_ind;

    return varinfo;
}

CodeError_t PrintVarInfo(const VarInfo varinfo) {
    printf("\"%s\" %s, %s: %d\n", varinfo.object_name, varinfo.file_name, varinfo.func_name, varinfo.line_ind);
    return NOTHING;
}