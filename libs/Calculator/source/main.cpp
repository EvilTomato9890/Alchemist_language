#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <strings.h>

#include "errors.h"
#include "stack.h"
#include "vars.h"
#include "calculator.h"
#include "assembler.h"
#include "processor.h"

int main(int argc, char* argv[]) {
    const char* start = "../start.txt";
    if (argc >= 2)
        start = argv[1];

    const char* ex_file = "../ex_file.txt";
    if (argc >= 3)
        ex_file = argv[2];

    const char* list_file = "../listing.txt";
    if (argc >= 4)
        list_file = argv[3];
    
    assembler_t assem = {};
    assem.text_file = start;
    assem.commands_file = ex_file;
    assem.listing_file = list_file;

    CodeError_t error_code = assembler(&assem);

    if (error_code != NOTHING)
        printerr("(((\n");

    processor_t *proc = make_processor(ex_file);
    my_assert(proc, NULLPTR, 0);

    LoadFile(proc);

    execution(proc);
}