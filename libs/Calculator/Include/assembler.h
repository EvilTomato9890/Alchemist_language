#ifndef __ASSEMBLER_H_
#define __ASSEMBLER_H_

#include <strings.h>

#include "stack.h"
#include "calculator.h"
#include "processor.h"

const int NumberCnt = 10;
const int AlphaCnt = 26;
const int FirstPass = 1;
const int SecondPass = 2;
const int LabelCount = 200;

struct label_t {
    int ic = -1;

    size_t hash = 0;
};

struct reg_t {
    const char* name;

    size_t hash;
};

struct assembler_t {
    const char* text_file = NULL;
    const char* commands_file = NULL;

    const char* listing_file = NULL;
    FILE* listing = NULL;

    Text* program = NULL;
    char* buf = NULL;
    char* ex_ptr = NULL;

    label_t* labels = NULL;
    int label_cnt = 0;

    int ic = 0;
};

enum Comparison {
    LESS  = -1,
    EQUAL =  0,
    ABOVE =  1,
};

CodeError_t assembler(assembler_t* assem);

#endif