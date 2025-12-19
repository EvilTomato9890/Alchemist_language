#ifndef _CALCULATOR_H_
#define _CALCULATOR_H_

#include "errors.h"

struct processor_t;

CodeError_t ProcAdd(processor_t *proc);

CodeError_t ProcSub(processor_t *proc);

CodeError_t ProcMul(processor_t *proc);

CodeError_t ProcDiv(processor_t *proc);

CodeError_t ProcSqrt(processor_t *proc);

CodeError_t ProcPow(processor_t *proc);

CodeError_t ProcOut(processor_t *proc);

CodeError_t ProcTop(processor_t *proc);

CodeError_t ProcIn(processor_t *proc);

#endif