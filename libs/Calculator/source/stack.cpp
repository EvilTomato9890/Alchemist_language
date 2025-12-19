#include "stack.h"

unsigned long calc_hash(stack_t *stack);
int get_size(stack_t *stack);
CodeError_t fill_poizon(stack_t *stack, int left, int right);
static int StackRealloc(stack_t *stack, ssize_t new_size);

unsigned long calc_hash(stack_t *stack) {
    my_assert(stack, NULLPTR, -1);
    
    unsigned long hash = 0;
    for (int i = 0; i < stack->capacity; ++i)
        hash = (hash + (i + 1) * stack->data[i]) % mod;
    
    return hash;
}

int get_size(stack_t *stack) {
    my_assert(stack, NULLPTR, -1);

    return stack->size;
}

CodeError_t fill_poizon(stack_t *stack, int left, int right) {
    my_assert(stack, NULLPTR, NULLPTR);
    my_assert(0 <= left && left <= right && right <= stack->capacity, SIZE_ERR, SIZE_ERR);

    for (int i = left; i < right; ++i) {
        stack->data[i] = POIZON_VALUE;
    }

    return NOTHING;
}

stack_t* StackCtor(ssize_t capacity ON_DEBUG(, VarInfo varinfo)) {
    ON_DEBUG(PrintVarInfo(varinfo));

    stack_t *stack = (stack_t*)calloc(1, sizeof(stack_t));
    my_assert(stack, NULLPTR, NULL);

    my_assert(0 <= capacity && capacity <= MaxCapacity, CAPACITY_ERR, NULL);

    stack->size = 0;
    stack->capacity = capacity;

    stack->data = (StackElem_t*)calloc(stack->capacity + 2, sizeof(StackElem_t));
    my_assert(stack->data, NULLPTR, NULL);

    stack->data[0] = CANARY_LEFT;
    ++stack->data;
    stack->data[stack->capacity] = CANARY_RIGHT;

    CodeError_t error_code = fill_poizon(stack, 0, stack->capacity);
    my_assert(error_code == NOTHING, error_code, NULL);

    stack->hash = calc_hash(stack);

    return stack;
}

CodeError_t StackDtor(stack_t *stack) {
    stackverify(stack);
    my_assert(code_error == NOTHING, code_error, code_error);

    free(stack->data);
    stack->data = NULL;
    free(stack);
    stack = NULL;

    return NOTHING;
}

static int StackRealloc(stack_t *stack, ssize_t new_size) {
    stackverify(stack);
    my_assert(code_error == NOTHING, code_error, BadSize);

    my_assert(new_size >= stack->size, SIZE_ERR, BadSize);

    --stack->data;
    stack->data = (StackElem_t*)realloc(stack->data, (new_size + 2) * sizeof(StackElem_t));

    my_assert(stack->data, REALLOC_ERR, BadSize);

    stack->data[0] = CANARY_LEFT;
    ++stack->data;
    stack->capacity = new_size;

    CodeError_t error_code = fill_poizon(stack, stack->size, stack->capacity);
    my_assert(error_code == NOTHING, error_code, BadSize);
    
    stack->data[stack->capacity] = CANARY_RIGHT;

    stack->hash = calc_hash(stack);         

    return stack->capacity;
}

CodeError_t StackPush(stack_t *stack, StackElem_t new_value) {
    stackverify(stack);
    my_assert(code_error == NOTHING, code_error, code_error);

    if (stack->size == stack->capacity) {
        int new_capacity = StackRealloc(stack, stack->size * ExpandMn);

        my_assert(new_capacity != BadSize, REALLOC_ERR, REALLOC_ERR);
    }

    *(stack->data + stack->size) = new_value;
    ++stack->size;
    
    stack->hash = calc_hash(stack);

    return NOTHING;
}

StackElem_t StackPop(stack_t *stack) {
    stackverify(stack);
    my_assert(code_error == NOTHING, code_error, POIZON_VALUE);

    if (stack->size == 0) {
        ON_DEBUG(stackdump(stack));
        PrintErr(EMPTY_STACK, line_info);
        printerr(RED_COLOR "You cannot pop from empty stack\n\n" RESET_COLOR);
        return POIZON_VALUE;
    }

    --stack->size;
    StackElem_t last = *(stack->data + stack->size);
    *(stack->data + stack->size) = POIZON_VALUE;

    stack->hash = calc_hash(stack);
    
    int new_capacity = stack->capacity;
    if (stack->size * CheckMn == stack->capacity)
        new_capacity = StackRealloc(stack, stack->capacity / NarrowMn);

    stack->hash = calc_hash(stack);
    
    if (new_capacity == BadSize)
        return POIZON_VALUE;

    return last;
}

CodeError_t StackVerify(stack_t* stack) {
    if (!stack) 
        return NULLPTR;

    if (stack->size < 0)
        return SIZE_ERR;
    
    if (stack->capacity < 0)
        return CAPACITY_ERR;

    if (stack->size > stack->capacity)
        return CAP_SIZE_ERR;

    if (stack->data == NULL)
        return NULLPTR;

    if (*(stack->data - 1) != CANARY_LEFT || stack->data[stack->capacity] != CANARY_RIGHT)
        return CANARY_ERR;

    if (calc_hash(stack) != stack->hash)
        return HASH_ERR;

    return NOTHING;
}

void StackDump(stack_t* stack, VarInfo varinfo) {
    printerr("\nStackDump called from %s: in function %s, line %d\n", varinfo.file_name, varinfo.func_name, varinfo.line_ind);
    printerr("%s ", varinfo.object_name);

    printerr("[");
    if (stack == NULL) {
        printerr(RED_COLOR "NULL" RESET_COLOR);
        printerr("]");
        printerr(RED_COLOR "(!!!! BAD !!!!)\n" RESET_COLOR);
    }
    else {
        printerr(GREEN_COLOR "%x" RESET_COLOR, stack);
        printerr("]\n");
    }

    printerr("\t{\n");

    if (stack) {
        if (stack->size > stack->capacity || stack->size < 0) {
            printerr(RED_COLOR "\tsize = %d    (!!! BAD !!!)\n" RESET_COLOR, stack->size); 
        }
        else {
            printerr(GREEN_COLOR "\tsize = %d\n" RESET_COLOR, stack->size);
        }

        if (stack->size > stack->capacity || stack->capacity < 0) {
            printerr(RED_COLOR "\tcapacity = %d (!!! BAD !!!)\n" RESET_COLOR, stack->capacity);
        }
        else {
            printerr(GREEN_COLOR "\tcapacity = %d\n" RESET_COLOR, stack->capacity);
        }

        printerr(BLUE_COLOR "\tHash = %d\n" RESET_COLOR, stack->hash);
        
        printerr("\tdata [");
        if (stack->data == NULL) {
            printerr(RED_COLOR "NULL" RESET_COLOR);
            printerr("]");
            printerr(RED_COLOR "(!!!! BAD !!!!)\n" RESET_COLOR);
            
            printerr("\t\t{\n");
        }
        else {
            printerr(GREEN_COLOR "%x" RESET_COLOR, stack->data);
            printerr("]\n");
            
            printerr("\t\t{\n");

            if (stack->capacity >= 0) {
                printerr((*(stack->data - 1) == CANARY_LEFT) ? GREEN_COLOR : RED_COLOR);
                printerr("\t\t   [%2d] = %10d (CANARY)\n" RESET_COLOR, -1, *(stack->data - 1));

                for (ssize_t i = 0; i < stack->capacity; ++i) {
                    if (i >= MaxPrintedCount) {
                        printerr("\t\t\t...\n\n");
                        break;
                    }

                    if (*(stack->data + i) == POIZON_VALUE) {
                        printerr("\t\t   [%2d] = %10d (POIZON)\n", i, POIZON_VALUE);
                    }
                    else {
                        printerr("\t\t * [%2d] = %10d\n", i, *(stack->data + i));
                    }
                }
                
                printerr((stack->data[stack->capacity] == CANARY_RIGHT) ? GREEN_COLOR : RED_COLOR);
                printerr("\t\t   [%2d] = %10d (CANARY)\n" RESET_COLOR, stack->capacity, stack->data[stack->capacity]);
            }
        }

        printerr("\t\t}\n");
    }

    printerr("\t}\n");
}