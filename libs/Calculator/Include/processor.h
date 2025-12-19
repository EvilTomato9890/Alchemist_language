#ifndef __EXECUTOR_H_
#define __EXECUTOR_H_

#include <sys/stat.h>

#include "stack.h"
#include "calculator.h"
#include "text_utils.h"
#include "vars.h"

#define Number (1 << 0)
#define Label  (1 << 1)
#define Reg    (1 << 2)
#define Mem    (1 << 3)

#define make_processor(code_file) ProcCtor(code_file ON_DEBUG(, VarInfoCtor("processor", line_info)))

#define procdump(name) ProcDump(name, VarInfoCtor(#name, line_info))

const size_t RegsCount = 8;
const size_t RamSize = 40000;
const int MaxOperationSize = 20;

struct processor_t {
    stack_t *stack = NULL;
    stack_t *stack_ret = NULL;

    Text *code = NULL;

    int *bytecode = {};

    int ic = 0;
    int cmd_cnt = 0;
    
    int ram[RamSize] = {};
    StackElem_t regs[RegsCount] = {0};
};

struct operation_t {
    const char* name;

    size_t hash;

    int code;

    CodeError_t (*func) (processor_t*);

    int args;
};

enum ProcOper {
    PUSH  ,
    POP   ,
    TOP   ,
    IN    ,
    OUT   ,

    ADD   ,
    SUB   ,
    MUL   ,
    DIV   ,
    SQRT  ,
    POW   ,

    JMP   ,
    JB    ,
    JBE   ,
    JA    ,
    JAE   ,
    JE    ,
    JNE   ,
    CALL  ,
    RET   ,
    
    PUSHM ,
    POPM  ,
    DRAW  ,

    PUSHR , 
    POPR  ,
    
    HLT   ,

    OPER_COUNT
};

processor_t* ProcCtor(const char* code_file ON_DEBUG(, VarInfo varinfo));
CodeError_t ProcDtor(processor_t* proc);
CodeError_t ProcVerify(processor_t* proc);
void ProcDump(processor_t* proc, VarInfo varinfo);
size_t StringHash(const char* str);

CodeError_t ProcStackPush(processor_t* proc);
CodeError_t ProcStackPop(processor_t* proc);
CodeError_t ProcStackWork(processor_t* proc);
CodeError_t ProcCall(processor_t* proc);
CodeError_t ProcRet(processor_t* proc);
CodeError_t ProcPushReg(processor_t* proc);
CodeError_t ProcPopReg(processor_t* proc);
CodeError_t ProcPushRam(processor_t* proc);
CodeError_t ProcPopRam(processor_t* proc);
CodeError_t ProcJmp(processor_t* proc);
CodeError_t ProcCmpJump(processor_t* proc);
CodeError_t ProcDraw(processor_t* proc);

CodeError_t ParsingFile(processor_t *proc);
CodeError_t LoadFile(processor_t *proc);
CodeError_t execution(processor_t *proc);

const operation_t operations[] = {
    {  .name = "PUSH"  ,  .hash =  StringHash("PUSH")   ,  .code =  PUSH  ,  .func = ProcStackPush,  .args = 0 + Number  },
    {  .name = "POP"   ,  .hash =  StringHash("POP")    ,  .code =   POP  ,  .func =  ProcStackPop,  .args = 0           },
    {  .name = "TOP"   ,  .hash =  StringHash("TOP")    ,  .code =   TOP  ,  .func =       ProcTop,  .args = 0           },
    {  .name = "IN"    ,  .hash =  StringHash("IN")     ,  .code =    IN  ,  .func =        ProcIn,  .args = 0           },
    {  .name = "OUT"   ,  .hash =  StringHash("OUT")    ,  .code =   OUT  ,  .func =       ProcOut,  .args = 0           },

    {  .name = "ADD"   ,  .hash =  StringHash("ADD")    ,  .code =   ADD  ,  .func =       ProcAdd,  .args = 0           },
    {  .name = "SUB"   ,  .hash =  StringHash("SUB")    ,  .code =   SUB  ,  .func =       ProcSub,  .args = 0           },
    {  .name = "MUL"   ,  .hash =  StringHash("MUL")    ,  .code =   MUL  ,  .func =       ProcMul,  .args = 0           },
    {  .name = "DIV"   ,  .hash =  StringHash("DIV")    ,  .code =   DIV  ,  .func =       ProcDiv,  .args = 0           },
    {  .name = "SQRT"  ,  .hash =  StringHash("SQRT")   ,  .code =  SQRT  ,  .func =      ProcSqrt,  .args = 0           },
    {  .name = "POW"   ,  .hash =  StringHash("POW")    ,  .code =   POW  ,  .func =       ProcPow,  .args = 0           },

    {  .name = "JMP"   ,  .hash =  StringHash("JMP")    ,  .code =   JMP  ,  .func =       ProcJmp,  .args = 0 + Label   },
    {  .name = "JB"    ,  .hash =  StringHash("JB")     ,  .code =    JB  ,  .func =   ProcCmpJump,  .args = 0 + Label   },
    {  .name = "JBE"   ,  .hash =  StringHash("JBE")    ,  .code =   JBE  ,  .func =   ProcCmpJump,  .args = 0 + Label   },
    {  .name = "JA"    ,  .hash =  StringHash("JA")     ,  .code =    JA  ,  .func =   ProcCmpJump,  .args = 0 + Label   },
    {  .name = "JAE"   ,  .hash =  StringHash("JAE")    ,  .code =   JAE  ,  .func =   ProcCmpJump,  .args = 0 + Label   },
    {  .name = "JE"    ,  .hash =  StringHash("JE")     ,  .code =    JE  ,  .func =   ProcCmpJump,  .args = 0 + Label   },
    {  .name = "JNE"   ,  .hash =  StringHash("JNE")    ,  .code =   JNE  ,  .func =   ProcCmpJump,  .args = 0 + Label   },
    {  .name = "CALL"  ,  .hash =  StringHash("CALL")   ,  .code =  CALL  ,  .func =      ProcCall,  .args = 0 + Label   },
    {  .name = "RET"   ,  .hash =  StringHash("RET")    ,  .code =   RET  ,  .func =       ProcRet,  .args = 0           },

    {  .name = "PUSHM" ,  .hash =  StringHash("PUSHM")  ,  .code = PUSHM  ,  .func =   ProcPushRam,  .args = 0 + Mem     },
    {  .name = "POPM"  ,  .hash =  StringHash("POPM")   ,  .code =  POPM  ,  .func =    ProcPopRam,  .args = 0 + Mem     },
    {  .name = "DRAW"  ,  .hash =  StringHash("DRAW")   ,  .code =  DRAW  ,  .func =      ProcDraw,  .args = 0           },

    {  .name = "PUSHR" ,  .hash =  StringHash("PUSHR")  ,  .code = PUSHR  ,  .func =   ProcPushReg,  .args = 0 + Reg     },
    {  .name = "POPR"  ,  .hash =  StringHash("POPR")   ,  .code =  POPR  ,  .func =    ProcPopReg,  .args = 0 + Reg     },

    {  .name = "HLT"   ,  .hash =  StringHash("HLT")    ,  .code =   HLT  ,  .func = ProcStackPush,  .args = 0           }
};

#endif