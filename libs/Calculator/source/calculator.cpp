#include "calculator.h"
#include "processor.h"
#include "stack.h"

static StackElem_t calc_sqrt(StackElem_t value);
static StackElem_t pow(StackElem_t a, StackElem_t b);

CodeError_t ProcAdd(processor_t *proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    my_assert(get_size(proc->stack) >= 2, SIZE_ERR, SIZE_ERR);

    StackElem_t a = StackPop(proc->stack);
    StackElem_t b = StackPop(proc->stack);

    CodeError_t error_code = StackPush(proc->stack, b + a);

    if (error_code != NOTHING)
        printerr(RED_COLOR "Addition was not completed\n" RESET_COLOR);

    return error_code;
}

CodeError_t ProcSub(processor_t *proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    my_assert(get_size(proc->stack) >= 2, SIZE_ERR, SIZE_ERR);

    StackElem_t a = StackPop(proc->stack);
    StackElem_t b = StackPop(proc->stack);

    CodeError_t error_code = StackPush(proc->stack, b - a);

    if (error_code != NOTHING)
        printerr(RED_COLOR "Subtraction was not completed\n" RESET_COLOR);

    return error_code;
}

CodeError_t ProcMul(processor_t *proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    my_assert(get_size(proc->stack) >= 2, SIZE_ERR, SIZE_ERR);

    StackElem_t a = StackPop(proc->stack);
    StackElem_t b = StackPop(proc->stack);

    CodeError_t error_code = StackPush(proc->stack, b * a);

    if (error_code != NOTHING)
        printerr(RED_COLOR "Multiplication was not completed\n" RESET_COLOR);

    return error_code;
}

CodeError_t ProcDiv(processor_t *proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    my_assert(get_size(proc->stack) >= 2, SIZE_ERR, SIZE_ERR);

    StackElem_t a = StackPop(proc->stack);
    StackElem_t b = StackPop(proc->stack);

    if (a == 0) {
        printerr(RED_COLOR "You cannot divise by zero\n" RESET_COLOR);
        return VALUE_ERR;
    }

    CodeError_t error_code = StackPush(proc->stack, b / a);

    if (error_code != NOTHING)
        printerr(RED_COLOR "Division was not completed\n" RESET_COLOR);

    return error_code;
}

static StackElem_t calc_sqrt(StackElem_t value) {
    my_assert(value >= 0, VALUE_ERR, VALUE_ERR);

    StackElem_t l = 0, r = value + 1;

    while (r - l > 1) {
        StackElem_t m = (l + r) / 2;

        if (m * m > value)
            r = m;
        else
            l = m;
    }

    return l;
}

CodeError_t ProcSqrt(processor_t *proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    my_assert(get_size(proc->stack), EMPTY_STACK, EMPTY_STACK);

    StackElem_t a = StackPop(proc->stack);

    StackElem_t sqrt_a = calc_sqrt(a);

    if (sqrt_a < 0) {
        printerr(RED_COLOR "Squarting was not completed" RESET_COLOR);
        return VALUE_ERR;
    }

    CodeError_t error_code = StackPush(proc->stack, sqrt_a);

    if (error_code != NOTHING)
        printerr(RED_COLOR "Squarting was not completed\n" RESET_COLOR);

    return error_code;
}

static StackElem_t pow(StackElem_t a, StackElem_t b) {
    StackElem_t res = 1;

    for (size_t i = 0; i < b; ++i)
        res *= a;
    
    return res;
}

CodeError_t ProcPow(processor_t *proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    if (get_size(proc->stack) < 2) {
        printerr(RED_COLOR "Not enough numbers for exponentiation\n" RESET_COLOR);
        return SIZE_ERR;
    }

    StackElem_t a = StackPop(proc->stack);
    StackElem_t b = StackPop(proc->stack);

    my_assert(a >= 0, VALUE_ERR, VALUE_ERR);

    CodeError_t error_code = StackPush(proc->stack, pow(b, a));

    if (error_code != NOTHING)
        printerr(RED_COLOR "Exponentation was not completed\n" RESET_COLOR);

    return error_code;
}

CodeError_t ProcOut(processor_t *proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    my_assert(get_size(proc->stack) > 0, EMPTY_STACK, EMPTY_STACK);

    StackElem_t a = StackPop(proc->stack);

    if (a == POIZON_VALUE) {
        printerr(RED_COLOR "Something in Outputing went wrong\n" RESET_COLOR);
        return VALUE_ERR;
    }

    printf("%d\n", a);

    return NOTHING;
}

CodeError_t ProcTop(processor_t *proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    my_assert(get_size(proc->stack) > 0, EMPTY_STACK, EMPTY_STACK);

    StackElem_t a = proc->stack->data[get_size(proc->stack) - 1];

    return NOTHING;
}

CodeError_t ProcIn(processor_t *proc) {
    CodeError_t code_error = ProcVerify(proc);
    my_assert(code_error == NOTHING, code_error, code_error);

    StackElem_t value = 0;
    int scanf_check = scanf("%d", &value);

    my_assert(scanf_check == 1, INPUT_ERR, INPUT_ERR);

    CodeError_t error_code = StackPush(proc->stack, value);

    if (error_code != NOTHING)
        printerr(RED_COLOR "Input was not completed\n" RESET_COLOR);

    return error_code;
}