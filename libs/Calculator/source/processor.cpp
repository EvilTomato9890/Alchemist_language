#include "processor.h"

CodeError_t execution(processor_t *proc) {
    my_assert(proc, NULLPTR, NULLPTR);

    int operation = 0, a = 0, b = 0;

    for (proc->ic = 0; proc->ic < proc->cmd_cnt;) {
        operation = proc->bytecode[proc->ic++];
        CodeError_t error_code = NOTHING;

        if (operation == HLT) {
            ON_DEBUG(procdump(proc));
            return NOTHING;
        }

        if (operation < 0 || operation >= OPER_COUNT)
            return OPERATION_ERR;
        
        error_code = operations[operation].func(proc);
        
        if (error_code != NOTHING) {
            ON_DEBUG(procdump(proc));
            return error_code;
        }
    }

    return NOTHING;
}

CodeError_t LoadFile(processor_t *proc) {
    my_assert(proc, NULLPTR, NULLPTR);

    CodeError_t error_code = ReadFile(proc->code);
    my_assert(error_code == NOTHING, error_code, error_code);

    error_code = ParsingFile(proc);
    my_assert(error_code == NOTHING, error_code, error_code);

    return NOTHING;
}

CodeError_t ParsingFile(processor_t *proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    int cmd_ind = 0;
    char* start = proc->code->buf;
    
    for (; start < proc->code->buf + proc->code->file_size; ) {
        int value = 0;
        int n = 0;
        sscanf(start, "%d %n", &value, &n);
        start += n;
        proc->bytecode[cmd_ind++] = value;
    }

    proc->cmd_cnt = cmd_ind;

    return NOTHING;
}

processor_t* ProcCtor(const char* code_file ON_DEBUG(, VarInfo varinfo)) {
    ON_DEBUG(PrintVarInfo(varinfo));
    my_assert(code_file, NULLPTR, NULL);

    processor_t* proc = (processor_t*)calloc(1, sizeof(processor_t));
    my_assert(proc, NULLPTR, NULL);

    proc->stack = make_stack(BaseStackSize);
    my_assert(proc->stack, NULLPTR, NULL);

    proc->stack_ret = make_stack(BaseStackSize);
    my_assert(proc->stack_ret, NULLPTR, NULL);

    proc->code = TextCtor(code_file);
    my_assert(proc->code, NULLPTR, NULL);

    proc->bytecode = (int*)calloc(proc->code->file_size, sizeof(int));
    my_assert(proc->bytecode, NULLPTR, NULL);

    proc->cmd_cnt = 0;
    proc->ic = 0;

    return proc;
}

CodeError_t ProcDtor(processor_t* proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    StackDtor(proc->stack);
    free(proc->code);
    proc->code = NULL;
    free(proc);
    proc = NULL;

    return NOTHING;
}

CodeError_t ProcVerify(processor_t* proc) {
    my_assert(proc, NULLPTR, NULLPTR);

    stackverify(proc->stack);
    my_assert(code_error == NOTHING, code_error, code_error);
    
    my_assert(proc->stack, STACK_ERR, STACK_ERR);
    
    my_assert(proc->code, NULLPTR, NULLPTR);

    my_assert(0 <= proc->ic && proc->ic <= proc->cmd_cnt, CMD_IND_ERR, CMD_IND_ERR);

    return NOTHING;
}

void ProcDump(processor_t* proc, VarInfo varinfo) {
    printerr("\nProcDump called from %s: in function %s, line %d\n", varinfo.file_name, varinfo.func_name, varinfo.line_ind);
    printerr("%s ", varinfo.object_name);

    printerr("[");
    if (proc == NULL) {
        printerr(RED_COLOR "NULL" RESET_COLOR);
        printerr("] ");
        printerr(RED_COLOR "(!!!! BAD !!!!)\n" RESET_COLOR);
    }
    else {
        printerr(GREEN_COLOR "%x" RESET_COLOR, proc);
        printerr("]\n");
    }

    printerr("\t{\n");

    if (proc) {
        printerr("\tstack[");
        if (proc->stack == NULL) {
            printerr(RED_COLOR "NULL" RESET_COLOR);
            printerr("] ");
            printerr(RED_COLOR "(!!!! BAD !!!!)\n" RESET_COLOR);
        }
        else {
            printerr(GREEN_COLOR "%x" RESET_COLOR, proc->stack);
            printerr("] = { ");
            stackverify(proc->stack);
            if (code_error == NOTHING) {
                for (size_t ind = 0; ind < proc->stack->size; ++ind)
                    printerr(GREEN_COLOR "%d " RESET_COLOR, proc->stack->data[ind]);
            }
            else {
                printerr(RED_COLOR " Some problem with stack of processor " RESET_COLOR);
            }
            printerr("};\n");
        }

        printerr("\n\tcode[");
        if (proc->code == NULL) {
            printerr(RED_COLOR "NULL" RESET_COLOR);
            printerr("]; ");
            printerr(RED_COLOR "(!!!! BAD !!!!)\n" RESET_COLOR);
        }
        else {
            printerr(GREEN_COLOR "%x" RESET_COLOR, proc->stack);
            printerr("];\n");
        }

        printerr("\n\tCount of commands = %d;\n", proc->cmd_cnt);

        printerr("\n\tIndex of command = %d;\n", proc->ic);

        printerr("\n\tregister[");
        if (proc->regs == NULL) {
            printerr(RED_COLOR "NULL" RESET_COLOR);
            printerr("] ");
            printerr(RED_COLOR "(!!!! BAD !!!!)\n" RESET_COLOR);
        }
        else {
            printerr(GREEN_COLOR "%x" RESET_COLOR, proc->stack);
            printerr("] = { ");
            for (size_t ind = 0; ind < RegsCount; ++ind)
                printerr(GREEN_COLOR "%d " RESET_COLOR, proc->regs[ind]);

            printerr("};\n");
        }
    }

    printerr("\t}\n");
}

size_t StringHash(const char* str) {
    my_assert(str, NULLPTR, 0);

    size_t str_code = 0;
    for (size_t c = 0; c < MaxOperationSize; c++) {
        if (str[c] == '\0' || str[c] == '\n')
            break;

        str_code = ((str_code * 33) + str[c]) % 100000;
    }

    return str_code;
}

CodeError_t ProcStackPush(processor_t* proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);
    
    StackElem_t value = proc->bytecode[proc->ic++];

    CodeError_t error_code = StackPush(proc->stack, value);

    return error_code;
}

CodeError_t ProcStackPop(processor_t* proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    StackElem_t last = StackPop(proc->stack);
    my_assert(last != POIZON_VALUE, VALUE_ERR, VALUE_ERR);

    return NOTHING;
}

CodeError_t ProcCall(processor_t* proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    CodeError_t error_code = StackPush(proc->stack_ret, proc->ic + 1);
    proc->ic = proc->bytecode[proc->ic];

    return error_code;
}

CodeError_t ProcRet(processor_t* proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    if (proc->stack_ret->size)
        proc->ic = StackPop(proc->stack_ret);

    return NOTHING;
}

CodeError_t ProcPushReg(processor_t* proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    int regs_ind = proc->bytecode[proc->ic++];
    my_assert(0 <= regs_ind && regs_ind <= 7, REG_IND_ERR, REG_IND_ERR);

    CodeError_t error_code = StackPush(proc->stack, proc->regs[regs_ind]);

    return NOTHING;
}

CodeError_t ProcPopReg(processor_t* proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    int regs_ind = proc->bytecode[proc->ic++];
    my_assert(0 <= regs_ind && regs_ind <= 7, REG_IND_ERR, REG_IND_ERR);

    StackElem_t last = StackPop(proc->stack);

    if (last == POIZON_VALUE) {
        printerr(RED_COLOR "Last number from stack cannot be written in a register\n" RESET_COLOR);
        return STACK_ERR;
    }

    proc->regs[regs_ind] = last;

    return NOTHING;
}

CodeError_t ProcPushRam(processor_t* proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    int ram_ind = proc->regs[proc->bytecode[proc->ic++]];

    code_error = StackPush(proc->stack, proc->ram[ram_ind]);

    return code_error;
}

CodeError_t ProcPopRam(processor_t* proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    int ram_ind = proc->regs[proc->bytecode[proc->ic++]];
    StackElem_t value = StackPop(proc->stack);

    proc->ram[ram_ind] = value;

    return code_error;
}

CodeError_t ProcJmp(processor_t* proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    proc->ic = proc->bytecode[proc->ic];

    return NOTHING;
}

CodeError_t ProcCmpJump(processor_t* proc) {
    CodeError_t error_code = ProcVerify(proc);
    my_assert(error_code == NOTHING, error_code, error_code);
    
    int operation = proc->bytecode[proc->ic - 1];
    int a = StackPop(proc->stack);
    int b = StackPop(proc->stack);
    my_assert(a != POIZON_VALUE && b != POIZON_VALUE, VALUE_ERR, VALUE_ERR);

    bool result = false;

    switch (operation) {
        case JB:    result = (b < a);       break;
        case JA:    result = (b > a);       break;
        case JBE:   result = (b <= a);      break;
        case JAE:   result = (b >= a);      break;
        case JE:    result = (b == a);      break;
        case JNE:   result = (b != a);      break;
    }

    if (result)
        proc->ic = proc->bytecode[proc->ic];
    else
        ++proc->ic;

    return error_code;
}

CodeError_t ProcDraw(processor_t* proc) {
    CodeError_t error_code = ProcVerify(proc);
    my_assert(error_code == NOTHING, error_code, error_code);

    char circle[RamSize] = {0};

    char* circle_print = circle;

    for (int i = 0; i < RamSize; ++i) {
        if (proc->ram[i] == 1)
            sprintf(circle_print++, "0");
        else
            sprintf(circle_print++, " ");

        if (i % 200 == 199) {
            sprintf(circle_print++, "\n");
        }
    }

    printf(circle);

    return NOTHING;
}
