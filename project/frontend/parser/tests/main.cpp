#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/asserts/include/asserts.h"

#include "frontend_parser.h"
#include "lexer/include/lexer_tokenizer.h"
#include "error_logger/include/frontend_err_logger.h"

#include "common/keywords/include/keywords.h"

#include "libs/AST/include/tree_operations.h"
#include "libs/AST/include/tree_verification.h"
#include "libs/AST/include/tree_file_io.h"
#include "libs/Stack/include/ident_stack.h"

static const char* token_kind_name(lexer_token_kind_t kind) {
    switch (kind) {
        case LEX_TK_EOF:     return "EOF";
        case LEX_TK_NUMBER:  return "NUMBER";
        case LEX_TK_IDENT:   return "IDENT";
        case LEX_TK_LPAREN:  return "LPAREN";
        case LEX_TK_RPAREN:  return "RPAREN";
        case LEX_TK_RBRACE:  return "RBRACE";
        case LEX_TK_KEYWORD: return "KEYWORD";
        default:             return "UNKNOWN";
    }
}

static void dump_tokens(const vector_t* tokens, size_t limit) {
    size_t count = vector_size(tokens);
    if (count < limit) limit = count;

    for (size_t index = 0; index < limit; ++index) {
        const lexer_token_t* token =
            (const lexer_token_t*)vector_get_const(tokens, index);

        printf("[%zu] kind=%s opcode=%d lex='%.*s'\n",
               index,
               token_kind_name(token->kind),
               (int)token->op_code,
               (int)token->lexeme.len, token->lexeme.ptr);
    }
}


static bool read_file_all(const char* filepath,
                          char** data_out,
                          size_t* size_out) {
    HARD_ASSERT(filepath != nullptr, "filepath is nullptr");
    HARD_ASSERT(data_out  != nullptr, "data_out is nullptr");
    HARD_ASSERT(size_out  != nullptr, "size_out is nullptr");

    *data_out = nullptr;
    *size_out = 0;

    LOGGER_DEBUG("Reading file: %s", filepath);
    FILE* file = fopen(filepath, "r");
    if (file == nullptr) return false;
    LOGGER_DEBUG("File opened: %s", filepath);
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return false;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }

    LOGGER_DEBUG("File size: %ld bytes", file_size);
    char* buffer_data = (char*)calloc((size_t)file_size + 1, 1);
    if (buffer_data == nullptr) {
        fclose(file);
        return false;
    }

    size_t read_size = fread(buffer_data, 1, (size_t)file_size, file);
    fclose(file);

    if (read_size != (size_t)file_size) {
        free(buffer_data);
        return false;
    }

    *data_out = buffer_data;
    *size_out = read_size;
    return true;
}

static c_string_t make_cstr(const char* data_ptr, size_t data_len) {
    c_string_t result = {};
    result.ptr = data_ptr;
    result.len = data_len;
    return result;
}

int main(int argc, char** argv) {
    logger_initialize_stream(stderr);
    const char* filename = (argc >= 2) ? argv[1] : "inline_test.alc";

    const char* inline_src =
        "func main(a, b) {"
        "    x = a + b;"
        "    if (x) { print(x); };"
        "    return x;"
        "};";

    char*  file_data = nullptr;
    size_t file_size = 0;

    if (argc >= 2) {
        if (!read_file_all(filename, &file_data, &file_size)) {
            fprintf(stderr, "cannot read file: %s\n", filename);
            return 1;
        }
    } else {
        file_data = (char*)inline_src;
        file_size = strlen(inline_src);
    }

    c_string_t buffer = make_cstr(file_data, file_size);

    vector_t token_vec = {};
    vector_t diag_vec  = {};
    SIMPLE_VECTOR_INIT(&token_vec, 64, lexer_token_t);
    SIMPLE_VECTOR_INIT(&diag_vec,  32, diag_log_t);

    lexer_config_t lexer_cfg = {};
    lexer_cfg.filename            = filename;
    lexer_cfg.keywords            = KEYWORDS;
    lexer_cfg.keywords_count      = KEYWORDS_COUNT;
    lexer_cfg.ignored_words       = IGNORED_KEYWORDS;
    lexer_cfg.ignored_words_count = IGNORED_KEYWORDS_COUNT;

    lexer_error_t lex_error = lexer_tokenize(buffer, &lexer_cfg,
                                             &token_vec, &diag_vec);
    dump_tokens(&token_vec, 100);
    if (vector_size(&diag_vec) != 0) {
        print_diags(stderr, buffer, filename, &diag_vec);
    }

    if (lex_error != LEX_ERR_OK) {
        if (argc >= 2) free(file_data);
        vector_destroy(&token_vec);
        vector_destroy(&diag_vec);
        return 1;
    }

    u_map_t func_table = {};
    SIMPLE_U_MAP_INIT(&func_table, 128,
                      size_t, func_decl_info_t,
                      parser_hash_size_t, parser_key_cmp_size_t);


    tree_t tree = {};

    tree.buff = buffer;

    error_code tree_error = tree_init(&tree ON_TREE_DEBUG(, TREE_VER_INIT));
    (void)tree_error;
    ON_TREE_DEBUG(
        tree_open_dump_file(&tree, "parser_test_tree_dump.txt");
    )
    LOGGER_DEBUG("Starting frontend_parse_ast");
    frontend_parse_ast(&tree, &token_vec, &func_table, &diag_vec);

    if (vector_size(&diag_vec) != 0) {
        print_diags(stderr, buffer, filename, &diag_vec);
    }
    tree_dump(&tree, TREE_VER_INIT, true, "Final AST after parsing");
    tree_write_to_file(&tree, "tree_write");
    ON_TREE_DEBUG(
        tree_close_dump_file(&tree);
    )
    tree_destroy(&tree);
    
    u_map_destroy(&func_table);
    vector_destroy(&token_vec);
    vector_destroy(&diag_vec);

    if (argc >= 2) free(file_data);
    return 0;
}
