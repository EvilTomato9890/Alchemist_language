#ifndef _STACK_H_
#define _STACK_H_

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "errors.h"
#include "vars.h"

typedef int StackElem_t;

struct stack_t {
    StackElem_t* data = NULL;

    unsigned long hash = 0;

    ssize_t size      = 0;
    ssize_t capacity  = 0;
};

const StackElem_t POIZON_VALUE = 146988;
const StackElem_t CANARY_LEFT  = 52954; // CEDA
const StackElem_t CANARY_RIGHT = 65242; // FEDA

#ifdef DEBUG
#define ON_DEBUG(...) __VA_ARGS__
#else
#define ON_DEBUG(...)
#endif

#define make_stack(capacity) StackCtor(capacity ON_DEBUG(, VarInfoCtor("stack", line_info)))

#define stackdump(name) StackDump(name, VarInfoCtor(#name, line_info));

#define stackverify(stack)  CodeError_t code_error = StackVerify(stack); \
                            if (code_error != NOTHING) { \
                                ON_DEBUG(stackdump(stack)); \
                                PrintErr(code_error, line_info); \
                            }

const long mod = 998244353;

const ssize_t MaxCapacity = 1000000;
const int MaxPrintedCount = 15;

const int ExpandMn =  2;
const int CheckMn  =  4;
const int NarrowMn =  3;
const int BadSize  = -1;
const int BaseStackSize = 1;

int get_size(stack_t *stack);

stack_t* StackCtor(ssize_t capacity ON_DEBUG(, VarInfo varinfo));

CodeError_t StackDtor(stack_t *stack);

CodeError_t StackPush(stack_t *stack, StackElem_t new_value);

StackElem_t StackPop(stack_t *stack);

CodeError_t StackVerify(stack_t* stack);

void StackDump(stack_t *stack, VarInfo varinfo);

#endif