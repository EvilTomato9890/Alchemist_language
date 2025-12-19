#include "assembler.h"

reg_t available_regs[RegsCount] = {{"RAX"}, {"RBX"}, {"RCX"}, {"RDX"}, {"REX"}, {"RFX"}, {"RGX"}, {"RHX"}};

CodeError_t assembler(assembler_t* assem);
CodeError_t PassingCode(assembler_t* assem, int pass_num);
CodeError_t ReadCodeFile(assembler_t* assem);
CodeError_t WriteToExFile(assembler_t* assem, const char* buf);
CodeError_t ZeroOper(char* operation);
CodeError_t ParseOper(assembler_t* assem, const operation_t* operation, int pass_num);
CodeError_t ParseNumber(assembler_t* assem, const int* value);
CodeError_t ParseLabel(assembler_t* assem, const char* label);
CodeError_t ParseMem(assembler_t* assem, const char* reg);
CodeError_t ParseString(assembler_t* assem, const char* str, int pass_num);
CodeError_t PrintNumber(assembler_t* assem, const int value);
CodeError_t AddLabel(assembler_t* assem, const char* str, const int ic);

int CheckHash(assembler_t *assem, size_t hash);
int GetLabel(assembler_t* assem, const char* str);
size_t GetLabelHash(label_t* label);
int GetLabelIc(label_t* label);
bool CheckLabelsHash(assembler_t* assem);

int RegComp(const void* param1, const void* param2);
int LabelComp(const void* param1, const void* param2);
CodeError_t CalcRegsHash();
bool CheckReg(const char* reg);

CodeError_t assembler(assembler_t* assem) {
    my_assert(assem, NULLPTR, NULLPTR);

    CodeError_t error_code = ReadCodeFile(assem);
    my_assert(error_code == NOTHING, error_code, error_code);

    error_code = CalcRegsHash();
    my_assert(error_code == NOTHING, error_code, error_code);

    assem->ex_ptr = (char*)calloc(assem->program->file_size, sizeof(char));
    my_assert(assem->ex_ptr, NULLPTR, NULLPTR);

    char operation[MaxOperationSize + 1] = {};

    char* exec_file = assem->ex_ptr;

    error_code = PassingCode(assem, FirstPass);

    label_t lab = assem->labels[0];

    if (!CheckLabelsHash(assem)) {
        printerr(RED_COLOR "Labels hash was not sorting after first passing\n" RESET_COLOR);
        qsort(assem->labels, assem->label_cnt, sizeof(label_t), &LabelComp);
    }

    fclose(assem->listing);

    if (error_code != NOTHING) 
        return error_code;

    assem->buf = assem->program->buf;
    assem->ex_ptr = exec_file;

    error_code = PassingCode(assem, SecondPass);
    
    if (error_code != NOTHING) 
        return error_code;

    fclose(assem->listing);

    WriteToExFile(assem, exec_file);

    return error_code;
}

CodeError_t PassingCode(assembler_t* assem, int pass_num) {
    char operation[MaxOperationSize] = {0};

    CodeError_t error_code = NOTHING;
    assem->ic = 0;
    bool is_end = false;
    int last_ic = 0;

    assem->listing = fopen(assem->listing_file, "w");

    fprintf(assem->listing, "IC    |  Operation         |  Code\n");
    fprintf(assem->listing, "------------------------------------\n");

    while (ZeroOper(operation) == NOTHING && ParseString(assem, operation, pass_num) == NOTHING) {
        if (operation[0] == ':')
            continue;
        
        fprintf(assem->listing, "%04x  |  %-5s  ", last_ic, operation);
        
        size_t operation_hash = StringHash(operation);
        my_assert(operation_hash != -1, VALUE_ERR, VALUE_ERR);
        
        int operation_code = CheckHash(assem, operation_hash);
        
        if (operation_code < 0 || operation_code >= OPER_COUNT) {
            printerr("%d\n", operation_hash);
            printerr(RED_COLOR "Unknown operation\n" RESET_COLOR);
            return OPERATION_ERR;
        }

        if (operation_code == HLT) {
            fprintf(assem->listing, "\n");
            is_end = true;
            break;
        }
        
        error_code = ParseOper(assem, &operations[operation_code], pass_num);

        fprintf(assem->listing, "\n");
        last_ic = assem->ic;
    }

    fprintf(assem->listing, "------------------------------------\n");

    if (pass_num == SecondPass) my_assert(is_end, TERM_ERR, TERM_ERR);

    return NOTHING;
}

int CheckHash(assembler_t *assem, size_t hash) {
    my_assert(assem, NULLPTR, -1);

    for (size_t i = 0; i < OPER_COUNT; ++i)
        if (operations[i].hash == hash)
            return i;

    return -1;
}

CodeError_t ReadCodeFile(assembler_t* assem) {
    my_assert(assem, NULLPTR, NULLPTR);

    assem->program = TextCtor(assem->text_file);
    my_assert(assem->program, FILE_ERR, NULLPTR);

    assem->buf = assem->program->buf;

    assem->labels = (label_t*)calloc(LabelCount, sizeof(label_t));
    my_assert(assem->labels, CALLOC_ERR, CALLOC_ERR);

    CodeError_t error_code = ReadFile(assem->program);
    my_assert(error_code == NOTHING, error_code, NULLPTR);

    return NOTHING;
}

CodeError_t WriteToExFile(assembler_t* assem, const char* buf) {
    my_assert(assem, NULLPTR, NULLPTR);
    my_assert(buf, NULLPTR, NULLPTR);

    FILE* ex_file = fopen(assem->commands_file, "w");
    my_assert(ex_file, FILE_ERR, FILE_ERR);

    fwrite(buf, sizeof(char), assem->ex_ptr - buf, ex_file);
    fclose(ex_file);

    return NOTHING;
}

CodeError_t ZeroOper(char* operation) {
    my_assert(operation, NULLPTR, NULLPTR);

    for (size_t i = 0 ; i < MaxOperationSize; ++i)
        operation[i] = 0;

    return NOTHING;
}

CodeError_t ParseOper(assembler_t* assem, const operation_t* operation, int pass_num) {
    my_assert(assem, NULLPTR, NULLPTR);

    int oper_args = operation->args;

    CodeError_t error_code = NOTHING;
    StackElem_t value = 0;
    char new_ic[100] = {0};
    char reg_type[5] = {0};

    if (oper_args & Number)
        error_code = ParseNumber(assem, &value);

    if (oper_args & Label)
        error_code = ParseLabel(assem, new_ic);

    if (oper_args & Reg) {
        error_code = ParseString(assem, reg_type, pass_num);
        my_assert(error_code == NOTHING, error_code, error_code);
        if (!CheckReg(reg_type)) return HASH_ERR;

        fprintf(assem->listing, "%-9s", reg_type);
    }

    if (oper_args & Mem) {
        error_code = ParseMem(assem, reg_type);
        my_assert(error_code == NOTHING, error_code, error_code);
        reg_type[3] = '\0';
        if (!CheckReg(reg_type)) return HASH_ERR;
    }

    if (oper_args == 0)
        fprintf(assem->listing, "%*s", 9, "");

    fprintf(assem->listing, "  |  ");

    PrintNumber(assem, operation->code);

    if (oper_args & Number)
        error_code = PrintNumber(assem, value);
    
    if (oper_args & Label)
        error_code = PrintNumber(assem, GetLabel(assem, new_ic));
    
    if (oper_args & Reg)
        error_code = PrintNumber(assem, reg_type[1] - 'A');
    
    if (oper_args & Mem)
        error_code = PrintNumber(assem, reg_type[1] - 'A');

    return error_code;
}

CodeError_t ParseNumber(assembler_t* assem, const int* value) {
    my_assert(value, NULLPTR, NULLPTR);
    my_assert(assem, NULLPTR, NULLPTR);

    int read_symbols = 0;
    int correct = sscanf(assem->buf, "%d%n", value, &read_symbols);
    my_assert(correct == 1, OPERATION_ERR, OPERATION_ERR);
    assem->buf += read_symbols;
    ++assem->ic;

    fprintf(assem->listing, "%-9d", *value);

    return NOTHING;
}

CodeError_t ParseLabel(assembler_t* assem, const char* label) {
    my_assert(label, NULLPTR, NULLPTR);
    my_assert(assem, NULLPTR, NULLPTR);

    int read_symbols;
    int correct = sscanf(assem->buf, " :%s%n", label, &read_symbols);
    my_assert(correct == 1, OPERATION_ERR, OPERATION_ERR);
    assem->buf += read_symbols;
    ++assem->ic;

    fprintf(assem->listing, ":%-8s", label);
    
    return NOTHING;
}

CodeError_t ParseMem(assembler_t* assem, const char* reg) {
    my_assert(reg, NULLPTR, NULLPTR);
    my_assert(assem, NULLPTR, NULLPTR);

    int read_symbols = 0;
    int correct = sscanf(assem->buf, " [%s%n", reg, &read_symbols);
    my_assert(correct == 1, OPERATION_ERR, OPERATION_ERR);
    assem->buf += read_symbols;
    ++assem->ic;

    fprintf(assem->listing, "[%-8s", reg);

    return NOTHING;
}

CodeError_t ParseString(assembler_t* assem, const char* str, int pass_num) {
    my_assert(str, NULLPTR, NULLPTR);
    my_assert(assem, NULLPTR, NULLPTR);

    int read_symbols = 0;
    int correct = sscanf(assem->buf, "%s%n", str, &read_symbols);
    my_assert(correct == 1, OPERATION_ERR, OPERATION_ERR);

    if (pass_num == FirstPass && str[0] == ':')
        AddLabel(assem, str + 1, assem->ic);

    if (str[0] == ':')
        assem->ic--;

    assem->buf += read_symbols;
    ++assem->ic;

    return NOTHING;
}

CodeError_t PrintNumber(assembler_t* assem, const int value) {
    my_assert(assem, NULLPTR, NULLPTR);

    sprintf(assem->ex_ptr, "%d ", value);
    while (*assem->ex_ptr != '\0')
        ++assem->ex_ptr;

    fprintf(assem->listing, "%-4x", value);

    return NOTHING;
}

CodeError_t AddLabel(assembler_t* assem, const char* str, const int ic) {
    my_assert(assem, NULLPTR, NULLPTR);

    label_t label = {};
    label.ic = ic;
    label.hash = StringHash(str);

    assem->labels[assem->label_cnt++] = label;
    
    for (int i = assem->label_cnt - 1; i > 0; --i) {
        if (assem->labels[i].hash > assem->labels[i - 1].hash)
            break;
        
        label_t prev_label = assem->labels[i - 1];
        assem->labels[i - 1] = assem->labels[i];
        assem->labels[i] = prev_label;
    }

    return NOTHING;
}

int GetLabel(assembler_t* assem, const char* str) {
    my_assert(assem, NULLPTR, -1);

    int hash = StringHash(str);

    int l = 0, r = assem->label_cnt;

    while (r - l > 1) {                                // bin_search
        int m = (l + r) / 2;

        if (assem->labels[m].hash <= hash)
            l = m;
        else 
            r = m;
    }

    if (GetLabelHash(&assem->labels[l]) == hash)
        return GetLabelIc(&assem->labels[l]);

    return -1;
}

size_t GetLabelHash(label_t* label) {
    my_assert(label, NULLPTR, -1);

    return label->hash;
}

int GetLabelIc(label_t* label) {
    my_assert(label, NULLPTR, -1);

    return label->ic;
}

bool CheckLabelsHash(assembler_t* assem) {
    my_assert(assem, NULLPTR, false);

    for (int i = 1; i < assem->label_cnt; ++i)
        if (GetLabelHash(&assem->labels[i]) <= GetLabelHash(&assem->labels[i - 1]))
            return false;

    return true;
}

int LabelComp(const void* param1, const void* param2) { // connect to common HashComp 
    my_assert(param1 && param2, NULLPTR, ABOVE);

    label_t* label1 = (label_t*)param1;
    label_t* label2 = (label_t*)param2;

    return label1->hash - label2->hash;
}

int RegComp(const void* param1, const void* param2) {
    my_assert(param1 && param2, NULLPTR, ABOVE);

    reg_t* reg1 = (reg_t*)param1;
    reg_t* reg2 = (reg_t*)param2;

    return reg1->hash - reg2->hash;
}

CodeError_t CalcRegsHash() {
    for (int i = 0; i < RegsCount; ++i)
        available_regs[i].hash = StringHash(available_regs[i].name);
    
    qsort(available_regs, RegsCount, sizeof(reg_t), &RegComp);

    for (int i = 1; i < RegsCount; ++i) {
        if (available_regs[i].hash == available_regs[i - 1].hash) {
            printerr(RED_COLOR "Two registers have one hash\n" RESET_COLOR);
            return HASH_ERR;
        }
    }

    return NOTHING;
}

bool CheckReg(const char* reg) {
    my_assert(reg, NULLPTR, false);

    int hash = StringHash(reg);

    int l = 0, r = RegsCount;

    while (r - l > 1) {                                         // bin_search
        int m = (l + r) / 2;

        if (available_regs[m].hash <= hash)
            l = m;
        else
            r = m;
    }

    return (available_regs[l].hash == hash);
}