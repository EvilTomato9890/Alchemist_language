#include "frontend_err_logger.h"
#include "common/console_colors/include/colors.h"
#include "common/logger/include/logger.h"

#include <ctype.h>
#include <string.h>

//================================================================================

static void find_line_bounds(c_string_t buffer, size_t position,
                             size_t* start_out, size_t* end_out) {
    size_t start = position;
    if (start > buffer.len) start = buffer.len;

    while (start > 0) {
        if (buffer.ptr[start - 1] == '\n') break;
        start--;
    }

    size_t end = position;
    if (end > buffer.len) end = buffer.len;

    while (end < buffer.len) {
        if (buffer.ptr[end] == '\n') break;
        end++;
    }

    *start_out = start;
    *end_out = end;
}

static void print_diag_message(FILE* stream, const lexer_diag_t* diag) {
    switch (diag->code) {
        case LEX_DIAG_UNKNOWN_SYMBOL:
            if (isprint(diag->got))
                fprintf(stream, "unknown symbol '%c'", (char)diag->got);
            else
                fprintf(stream, "unknown symbol (0x%02X)", (unsigned)diag->got);
            break;

        case LEX_DIAG_BAD_NUMBER:
            fprintf(stream, "bad number literal");
            break;

        case LEX_DIAG_UNTERMINATED_COMMENT:
            fprintf(stream, "unterminated block comment");
            break;
            
        default:
            fprintf(stream, "lexer error");
            break;
    }
}

static void print_caret_line(FILE* stream, c_string_t line,
                             size_t column_1based, size_t length) {
    if (column_1based == 0) column_1based = 1;
    if (length == 0) length = 1;

    size_t col = 1;
    for (size_t i = 0; i < line.len && col < column_1based; ++i) {
        unsigned char ch = (unsigned char)line.ptr[i];

        if (ch == '\t') fputc('\t', stream);
        else            fputc(' ', stream);

        col++;
    }

    fputc('^', stream);
    for (size_t i = 1; i < length; ++i) fputc('~', stream);
    fputc('\n', stream);
}

//================================================================================

void print_diags(FILE* stream, c_string_t buffer, error_source_t source,
                 const char* filename, const vector_t* diags) {

    if (stream == nullptr) stream = stderr;
    if (diags == nullptr) return;

    size_t diag_count = vector_size(diags);
    for (size_t i = 0; i < diag_count; ++i) {
        lexer_diag_t* diag = (lexer_diag_t*)vector_get_const(diags, i);
        if (diag == nullptr) continue;

        if (source == LEXER_ERROR) {
            fprintf(stream, "%s:%zu:%zu:" ORANGE_CONSOLE " lexer_error: " RESET_CONSOLE,
                    filename != nullptr ? filename : "<missed file>", diag->line, diag->column);
        } else if (source == PARSER_ERROR) {
            fprintf(stream, "%s:%zu:%zu:" RED_CONSOLE " error: " RESET_CONSOLE,
                    filename != nullptr ? filename : "<missed file>", diag->line, diag->column);
        } else {
            LOGGER_ERROR("Wrong source type");
            fprintf(stream, "%s:%zu:%zu:" RED_CONSOLE " unkown source error: " RESET_CONSOLE,
                    filename != nullptr ? filename : "<missed file>", diag->line, diag->column);
        }
        print_diag_message(stream, diag);
        fputc('\n', stream);

        size_t line_start = 0;
        size_t line_end = 0;
        find_line_bounds(buffer, diag->position, &line_start, &line_end);

        c_string_t line = { buffer.ptr + line_start, line_end - line_start };

        fwrite(line.ptr, 1, line.len, stream);
        fputc('\n', stream);
        print_caret_line(stream, line, diag->column, diag->length);
    }
}
