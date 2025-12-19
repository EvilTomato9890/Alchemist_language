#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/logger/include/logger.h"
#include "common/keywords/include/keywords.h"

#include "libs/AST/include/tree_operations.h"
#include "libs/AST/include/tree_info.h"
#include "libs/AST/include/tree_verification.h"
#include "libs/AST/include/tree_file_io.h"
#include "libs/Vector/include/vector.h"
#include "libs/My_string/include/my_string.h"
#include "libs/Unordered_map/include/unordered_map.h"
#include "libs/Stack/include/ident_stack.h"

#include "project/frontend/lexer/include/lexer_tokenizer.h"
#include "project/frontend/parser/include/frontend_parser.h"
#include "project/frontend/error_logger/include/frontend_err_logger.h"
#include "project/midend/include/tree_optimize.h"
#include "project/backend/include/backend.h"

//================================================================================
//                          Вспомогательные функции
//================================================================================
//TODO
static c_string_t make_cstr(const char* data_ptr, size_t data_len) {
    c_string_t result = {};
    result.ptr = data_ptr;
    result.len = data_len;
    return result;
}

static bool read_file_all(const char* filepath,
                          char** data_out,
                          size_t* size_out) {
    FILE* file = fopen(filepath, "r");
    if (file == nullptr) {
        fprintf(stderr, "Ошибка: не удалось открыть файл: %s\n", filepath);
        return false;
    }

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

static const char* op_code_tree_name(op_code_t op_code) {
    for (size_t i = 0; i < KEYWORDS_COUNT; ++i) {
        if (KEYWORDS[i].op_code == op_code)
            return KEYWORDS[i].tree_name;
    }
    return nullptr;
}

static void print_token(const lexer_token_t* token) {
    if (token == nullptr) return;

    printf("%zu:%zu  %-8s  ",
           token->line, token->column,
           token_kind_name(token->kind));

    if (token->kind == LEX_TK_NUMBER) {
        printf("val=%g  lex='", token->number);
        printf("%.*s", (int)token->lexeme.len, token->lexeme.ptr);
        printf("'\n");
        return;
    }

    if (token->kind == LEX_TK_KEYWORD) {
        const char* tree = op_code_tree_name(token->op_code);
        printf("op=%d", (int)token->op_code);
        if (tree != nullptr) printf("(%s)", tree);
        printf("  lex='");
        printf("%.*s", (int)token->lexeme.len, token->lexeme.ptr);
        printf("'\n");
        return;
    }

    printf("lex='");
    printf("%.*s", (int)token->lexeme.len, token->lexeme.ptr);
    printf("'\n");
}

//================================================================================
//                                  main
//================================================================================

int main(int argc, char** argv) {
    // Инициализация логгера
    logger_initialize_stream(stderr);

    // Аргументы:
    //   main.exe <input.alc> [output.asm] [frontend.ast] [midend.ast] [--keep-temps]
    const char* input_filename  = nullptr;
    const char* output_filename = "output.asm";
    const char* ast_frontend    = "frontend.ast";
    const char* ast_midend      = "midend.ast";
    bool keep_temps = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--keep-temps") == 0) {
            keep_temps = true;
        }
    }

    // Определение источника кода
    const char* filename = nullptr;
    char* file_data = nullptr;
    size_t file_size = 0;
    bool need_free = false;

    if (argc >= 2 && argv[1] != nullptr && argv[1][0] != '\0' && strcmp(argv[1], "--keep-temps") != 0) {
        input_filename = argv[1];
    }

    if (argc >= 3 && argv[2] != nullptr && argv[2][0] != '\0' && strcmp(argv[2], "--keep-temps") != 0) {
        output_filename = argv[2];
    }

    if (argc >= 4 && argv[3] != nullptr && argv[3][0] != '\0' && strcmp(argv[3], "--keep-temps") != 0) {
        ast_frontend = argv[3];
    }

    if (argc >= 5 && argv[4] != nullptr && argv[4][0] != '\0' && strcmp(argv[4], "--keep-temps") != 0) {
        ast_midend = argv[4];
    }

    if (input_filename != nullptr) {
        filename = input_filename;
        if (!read_file_all(filename, &file_data, &file_size)) {
            fprintf(stderr, "Ошибка: не удалось прочитать файл: %s\n", filename);
            return 1;
        }
        need_free = true;
    } else {
        // Используем тестовое выражение по умолчанию
        const char* default_code =
            "func main(a, b) {"
            "    x = a + b;"
            "    if (x) { print(x); };"
            "    return x;"
            "};";
        file_data = (char*)default_code;
        file_size = strlen(default_code);
        filename = "inline_code.alc";
    }

    c_string_t buffer = make_cstr(file_data, file_size);

    //================================================================================
    //                          Frontend: Lexer
    //================================================================================

    vector_t token_vec = {};
    vector_t diag_vec = {};
    SIMPLE_VECTOR_INIT(&token_vec, 64, lexer_token_t);
    SIMPLE_VECTOR_INIT(&diag_vec, 32, diag_log_t);

    lexer_config_t lexer_cfg = {};
    lexer_cfg.filename = filename;
    lexer_cfg.keywords = KEYWORDS;
    lexer_cfg.keywords_count = KEYWORDS_COUNT;
    lexer_cfg.ignored_words = IGNORED_KEYWORDS;
    lexer_cfg.ignored_words_count = IGNORED_KEYWORDS_COUNT;

    LOGGER_DEBUG("Начало токенизации");
    lexer_error_t lex_error = lexer_tokenize(buffer, &lexer_cfg, &token_vec, &diag_vec);

    if (vector_size(&diag_vec) != 0) {
        fprintf(stderr, "Ошибки лексера:\n");
        print_diags(stderr, buffer, filename, &diag_vec);
    }

    
    if (lex_error != LEX_ERR_OK) {
        fprintf(stderr, "Ошибка токенизации\n");
        if (need_free) free(file_data);
        vector_destroy(&token_vec);
        vector_destroy(&diag_vec);
        return 1;
    }
    for (size_t i = 0; i < vector_size(&token_vec); ++i) {
        const lexer_token_t* token =
            (const lexer_token_t*)vector_get_const(&token_vec, i);
        print_token(token);
    }

    LOGGER_DEBUG("Токенизация завершена успешно, токенов: %zu", vector_size(&token_vec));

    //================================================================================
    //                          Frontend: Parser
    //================================================================================

    u_map_t parser_func_table = {};
    SIMPLE_U_MAP_INIT(&parser_func_table, 128,
                      size_t, func_decl_info_t,
                      parser_hash_size_t, parser_key_cmp_size_t);

    tree_t tree = {};
    tree.buff = buffer;

    error_code tree_error = tree_init(&tree ON_TREE_DEBUG(, TREE_VER_INIT));
    if (tree_error != ERROR_NO) {
        fprintf(stderr, "Ошибка инициализации дерева\n");
        if (need_free) free(file_data);
        vector_destroy(&token_vec);
        vector_destroy(&diag_vec);
        u_map_destroy(&parser_func_table);
        return 1;
    }
    tree_open_dump_file(&tree, "TEST0.html");
    LOGGER_DEBUG("Начало парсинга AST");
    frontend_parse_ast(&tree, &token_vec, &parser_func_table, &diag_vec);

    if (vector_size(&diag_vec) != 0) {
        fprintf(stderr, "Ошибки парсера:\n");
        print_diags(stderr, buffer, filename, &diag_vec);
    }

    LOGGER_DEBUG("Парсинг завершен, размер дерева: %zu", tree.size);

    // Если были диагностические сообщения — считаем, что компиляцию продолжать нельзя.
    if (vector_size(&diag_vec) != 0) {
        fprintf(stderr, "Компиляция остановлена из-за ошибок frontend.\n");
        vector_destroy(&token_vec);
        u_map_destroy(&parser_func_table);
        tree_destroy(&tree);
        vector_destroy(&diag_vec);
        if (need_free) free(file_data);
        return 1;
    }

    //================================================================================
    //                          Frontend: AST -> файл
    //================================================================================
    tree_dump(&tree, TREE_VER_INIT, true, "First");
    LOGGER_DEBUG("Запись AST (frontend) в файл: %s", ast_frontend);
    error_code write_front_err = tree_write_to_file(&tree, ast_frontend);
    if (write_front_err != ERROR_NO) {
        fprintf(stderr, "Ошибка: не удалось записать AST в файл: %s\n", ast_frontend);
        tree_destroy(&tree);
        vector_destroy(&diag_vec);
        if (need_free) free(file_data);
        return 1;
    }

    // Очистка ресурсов frontend (дерево после записи больше не нужно)
    tree_close_dump_file(&tree);
    vector_destroy(&token_vec);
    u_map_destroy(&parser_func_table);
    tree_destroy(&tree);
    vector_destroy(&diag_vec);

    // Важно: буфер исходного кода должен жить как минимум до tree_write_to_file()
    if (need_free) free(file_data);

    //================================================================================
    //                          Middleend: файл -> AST -> оптимизация -> файл
    //================================================================================

    tree_t mid_tree = {};
    error_code mid_init_err = tree_init(&mid_tree ON_TREE_DEBUG(, TREE_VER_INIT));
    if (mid_init_err != ERROR_NO) {
        fprintf(stderr, "Ошибка: tree_init (midend)\n");
        return 1;
    }
    tree_open_dump_file(&mid_tree, "TEST.html");
    string_t mid_buffer = {nullptr, 0};
    error_code read_front_err = tree_read_from_file(&mid_tree, ast_frontend, &mid_buffer);
    tree_dump(&mid_tree, TREE_VER_INIT, true, "aaaa");
    if (read_front_err != ERROR_NO) {
        fprintf(stderr, "Ошибка: не удалось прочитать AST из файла: %s\n", ast_frontend);
        tree_destroy(&mid_tree);
        if (mid_buffer.ptr) free(mid_buffer.ptr);
        return 1;
    }

    LOGGER_DEBUG("Начало оптимизации дерева (midend)");
    tree_dump(&mid_tree, TREE_VER_INIT, true, "aaaa");

    error_code opt_error = tree_optimize(&mid_tree);
    if (opt_error != ERROR_NO) {
        fprintf(stderr, "Ошибка оптимизации дерева\n");
        tree_destroy(&mid_tree);
        free(mid_buffer.ptr);
        return 1;
    }
    LOGGER_DEBUG("Оптимизация завершена (midend)");
    tree_dump(&mid_tree, TREE_VER_INIT, true, "aaaa");
    LOGGER_DEBUG("Запись AST (midend) в файл: %s", ast_midend);
    error_code write_mid_err = tree_write_to_file(&mid_tree, ast_midend);
    if (write_mid_err != ERROR_NO) {
        fprintf(stderr, "Ошибка: не удалось записать AST в файл: %s\n", ast_midend);
        tree_destroy(&mid_tree);
        free(mid_buffer.ptr);
        return 1;
    }

    // Важно: mid_buffer.ptr должен жить до tree_destroy()
    tree_close_dump_file(&mid_tree);
    tree_destroy(&mid_tree);
    free(mid_buffer.ptr);

    //================================================================================
    //                          Backend: файл -> AST -> ASM
    //================================================================================

    tree_t back_tree = {};
    error_code back_init_err = tree_init(&back_tree ON_TREE_DEBUG(, TREE_VER_INIT));
    if (back_init_err != ERROR_NO) {
        fprintf(stderr, "Ошибка: tree_init (backend)\n");
        return 1;
    }

    string_t back_buffer = {nullptr, 0};
    error_code read_mid_err = tree_read_from_file(&back_tree, ast_midend, &back_buffer);
    if (read_mid_err != ERROR_NO) {
        fprintf(stderr, "Ошибка: не удалось прочитать AST из файла: %s\n", ast_midend);
        tree_destroy(&back_tree);
        if (back_buffer.ptr) free(back_buffer.ptr);
        return 1;
    }

    // Создаем func_table для backend
    u_map_t backend_func_table = {};
    SIMPLE_U_MAP_INIT(&backend_func_table, 128,
                      size_t, backend_func_symbol_t,
                      parser_hash_size_t, parser_key_cmp_size_t);

    FILE* asm_file = fopen(output_filename, "w");
    if (asm_file == nullptr) {
        fprintf(stderr, "Ошибка: не удалось открыть файл для записи: %s\n", output_filename);
        tree_destroy(&back_tree);
        free(back_buffer.ptr);
        u_map_destroy(&backend_func_table);
        return 1;
    }

    LOGGER_DEBUG("Начало генерации ассемблера (backend)");
    hm_error_t backend_error = backend_emit_asm(&back_tree, asm_file, &backend_func_table);
    fclose(asm_file);

    if (backend_error != HM_ERR_OK) {
        fprintf(stderr, "Ошибка генерации ассемблера\n");
        tree_destroy(&back_tree);
        free(back_buffer.ptr);
        u_map_destroy(&backend_func_table);
        return 1;
    }

    LOGGER_DEBUG("Генерация ассемблера завершена, файл: %s", output_filename);
    printf("Успешно! Ассемблер сохранен в файл: %s\n", output_filename);

    tree_destroy(&back_tree);
    free(back_buffer.ptr);
    u_map_destroy(&backend_func_table);

    if (!keep_temps) {
        remove(ast_frontend);
        remove(ast_midend);
    }

    return 0;
}

