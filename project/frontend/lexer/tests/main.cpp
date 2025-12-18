#include "lexer_tokenizer.h"

#include "error_logger/include/frontend_err_logger.h"
#include "common/keywords/include/keywords.h"

#include <stdio.h>

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

int main() {
    const char* text =
        "function   decl foo(x, 10); //SOME SHIT\n"
        "$1.3){ /* TEST */ 123a <= .5 and or }\n"
        "aaaa  bbbb @@@@@@@@@@@@@\n"
        "AAB */ bbbb\n"
        "call foo;\n"
        "x = -5\n"
        "NOT_FOR_CODE return 42;\n";

    c_string_t buffer = { text, 0 };
    while (text[buffer.len] != '\0') buffer.len++;

    lexer_config_t config = {};
    config.filename = "lexer_test.txt";
    config.keywords = KEYWORDS;
    config.keywords_count = KEYWORDS_COUNT;
    config.ignored_words = IGNORED_KEYWORDS;
    config.ignored_words_count = IGNORED_KEYWORDS_COUNT;

    vector_t tokens = {};
    vector_t diags  = {};

    if (SIMPLE_VECTOR_INIT(&tokens, 64, lexer_token_t) != 0) return 1;
    if (SIMPLE_VECTOR_INIT(&diags,  32, diag_log_t)  != 0) return 1;

    lexer_error_t lex_err =
        lexer_tokenize(buffer, &config, &tokens, &diags);

    printf("lexer_tokenize returned: %d\n", (int)lex_err);

    size_t token_count = vector_size(&tokens);
    for (size_t i = 0; i < token_count; ++i) {
        const lexer_token_t* token =
            (const lexer_token_t*)vector_get_const(&tokens, i);
        print_token(token);
    }

    if (vector_size(&diags) != 0) {
        printf("\nDiagnostics:\n");
        print_diags(stderr, buffer, config.filename, &diags);
    }

    vector_destroy(&diags);
    vector_destroy(&tokens);
    return 0;
}
