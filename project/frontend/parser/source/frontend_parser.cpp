#include "parser/include/frontend_parser.h"

#include "common/asserts/include/asserts.h"
#include "libs/AST/include/tree_operations.h"
#include "libs/AST/include/tree_info.h"
#include "libs/Unordered_map/include/unordered_map.h"
#include "common/keywords/include/keywords.h"
#include "error_logger/include/frontend_err_logger.h"
#include "lexer/include/lexer_tokenizer.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

//================================================================================
//                               Диагностика
//================================================================================

static const int DIAG_PARSE_EXPECTED        = 100;
static const int DIAG_PARSE_UNDEF_FUNCTION  = 101;
static const int DIAG_PARSE_REDEF_FUNCTION  = 102;
static const int DIAG_PARSE_NESTED_DECL     = 103;
static const int DIAG_PARSE_VOID_IN_EXPR    = 104;
static const int DIAG_PARSE_RETURN_IN_PROC  = 105;
static const int DIAG_PARSE_FINISH_IN_FUNC  = 106;
static const int DIAG_PARSE_BREAK_OUTSIDE   = 107;
static const int DIAG_PARSE_ARGC_MISMATCH   = 108;

//================================================================================

struct parser_state_t {
    tree_t*          tree;
    const vector_t*  tokens;
    size_t           position;

    u_map_t*         func_table;
    vector_t*        diags;

    op_code_t        current_decl; // OP_FUNC_DECL | OP_PROC_DECL | OP_NONE
    size_t           while_depth;
};

static const lexer_token_t* parser_token_at(const parser_state_t* parser,
                                            size_t index) {
    HARD_ASSERT(parser != nullptr, "parser is nullptr");
    if (parser->tokens == nullptr) return nullptr;
    if (index >= vector_size(parser->tokens)) return nullptr;
    return (const lexer_token_t*)vector_get_const(parser->tokens, index);
}

static const lexer_token_t* parser_peek(const parser_state_t* parser) {
    return parser_token_at(parser, parser->position);
}

static const lexer_token_t* parser_prev(const parser_state_t* parser) {
    if (parser->position == 0) return nullptr;
    return parser_token_at(parser, parser->position - 1);
}

static bool parser_is_eof(const parser_state_t* parser) {
    const lexer_token_t* token = parser_peek(parser);
    return token == nullptr || token->kind == LEX_TK_EOF;
}

static const lexer_token_t* parser_advance(parser_state_t* parser) {
    HARD_ASSERT(parser != nullptr, "parser is nullptr");
    if (!parser_is_eof(parser)) {
        parser->position++;
    }
    return parser_prev(parser);
}

static bool parser_check_kind(const parser_state_t* parser,
                              lexer_token_kind_t kind) {
    const lexer_token_t* token = parser_peek(parser);
    return token != nullptr && token->kind == kind;
}

static bool parser_check_keyword(const parser_state_t* parser,
                                 op_code_t op_code) {
    const lexer_token_t* token = parser_peek(parser);
    return token != nullptr &&
           token->kind == LEX_TK_KEYWORD &&
           token->op_code == op_code;
}

static bool parser_match_kind(parser_state_t* parser,
                              lexer_token_kind_t kind) {
    if (!parser_check_kind(parser, kind)) return false;
    parser_advance(parser);
    return true;
}

static bool parser_match_keyword(parser_state_t* parser,
                                 op_code_t op_code) {
    if (!parser_check_keyword(parser, op_code)) return false;
    parser_advance(parser);
    return true;
}

static void parser_push_diag(parser_state_t* parser, int diag_code,
                             const lexer_token_t* token,
                             const char* format, ...) {
    HARD_ASSERT(parser != nullptr, "parser is nullptr");
    HARD_ASSERT(parser->diags != nullptr, "parser->diags is nullptr");
    HARD_ASSERT(format != nullptr, "format is nullptr");

    const lexer_token_t dummy = {};
    if (token == nullptr) token = parser_peek(parser);
    if (token == nullptr) token = &dummy;

    char message_buf[MAX_ERROR_MESSAGE_LENGTH] = {};

    va_list args;
    va_start(args, format);
    (void)vsnprintf(message_buf, sizeof(message_buf), format, args);
    va_end(args);

    diag_log_t diag = diag_log_init(PARSER_ERROR, (diag_code_t)diag_code,
                                   token->position, token->lexeme.len,
                                   token->line, token->column,
                                   "%s", message_buf);

    (void)vector_push_back(parser->diags, &diag);
}

static void parser_expected(parser_state_t* parser, const char* what) {
    parser_push_diag(parser, DIAG_PARSE_EXPECTED, nullptr,
                     "ожидалось: %s", what);
}

static void parser_sync_to_lcat(parser_state_t* parser) {
    while (!parser_is_eof(parser)) {
        if (parser_check_kind(parser, LEX_TK_RBRACE)) return;
        if (parser_check_keyword(parser, OP_LCAT)) {
            parser_advance(parser);
            return;
        }
        parser_advance(parser);
    }
}

//================================================================================
//                      Таблица функций: size_t -> func_decl_info_t
//================================================================================

static uint64_t splitmix64(uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    value = value ^ (value >> 31);
    return value;
}

size_t parser_hash_size_t(const void* key_ptr) {
    HARD_ASSERT(key_ptr != nullptr, "key_ptr is nullptr");
    uint64_t value = (uint64_t)(*(const size_t*)key_ptr);
    return (size_t)splitmix64(value);
}

bool parser_key_cmp_size_t(const void* left_ptr, const void* right_ptr) {
    HARD_ASSERT(left_ptr  != nullptr, "left_ptr is nullptr");
    HARD_ASSERT(right_ptr != nullptr, "right_ptr is nullptr");
    return (*(const size_t*)left_ptr) == (*(const size_t*)right_ptr);
}

static bool func_table_get(const u_map_t* func_table,
                           size_t name_idx,
                           func_decl_info_t* info_out) {
    return u_map_get_elem(func_table, &name_idx, info_out);
}

static void func_table_put(u_map_t* func_table,
                           size_t name_idx,
                           const func_decl_info_t* info_ptr) {
    (void)u_map_insert_elem(func_table, &name_idx, info_ptr);
}

//================================================================================
//                             Создание узлов AST
//================================================================================

static tree_node_t* ast_func(op_code_t op_code,
                             tree_node_t* left_node,
                             tree_node_t* right_node) {
    value_t value = make_union_func(op_code);
    return init_node(FUNCTION, value, left_node, right_node);
}

static tree_node_t* ast_const(double value) {
    value_t node_val = make_union_const(value);
    return init_node(CONSTANT, node_val, nullptr, nullptr);
}

static tree_node_t* ast_var(size_t var_idx) {
    value_t node_val = make_union_var(var_idx);
    return init_node(VARIABLE, node_val, nullptr, nullptr);
}

static tree_node_t* ast_unary(op_code_t op_code,
                              tree_node_t* child_node) {
    // Унарные: ребенок справа.
    return ast_func(op_code, nullptr, child_node);
}

static tree_node_t* ast_make_list(op_code_t op_code,
                                  tree_node_t* list_root,
                                  tree_node_t* next_node) {
    if (list_root == nullptr) return next_node;
    return ast_func(op_code, list_root, next_node);
}

//================================================================================
//                              Парсинг: forward
//================================================================================

static tree_node_t* parse_toplevel(parser_state_t* parser);

static tree_node_t* parse_decl(parser_state_t* parser);
static tree_node_t* parse_block(parser_state_t* parser);

static tree_node_t* parse_stmt_list(parser_state_t* parser);
static tree_node_t* parse_stmt(parser_state_t* parser);

static tree_node_t* parse_expr_value(parser_state_t* parser);
static tree_node_t* parse_assign(parser_state_t* parser);
static tree_node_t* parse_or(parser_state_t* parser);
static tree_node_t* parse_and(parser_state_t* parser);
static tree_node_t* parse_eq(parser_state_t* parser);
static tree_node_t* parse_rel(parser_state_t* parser);
static tree_node_t* parse_add(parser_state_t* parser);
static tree_node_t* parse_mul(parser_state_t* parser);
static tree_node_t* parse_unary(parser_state_t* parser);
static tree_node_t* parse_primary(parser_state_t* parser);

static tree_node_t* parse_call(parser_state_t* parser,
                               bool value_context);

static size_t parse_ident_idx(parser_state_t* parser,
                              const lexer_token_t* token) {
    HARD_ASSERT(parser != nullptr, "parser is nullptr");
    HARD_ASSERT(parser->tree != nullptr, "parser->tree is nullptr");
    HARD_ASSERT(parser->tree->ident_stack != nullptr, "ident_stack is nullptr");
    HARD_ASSERT(token != nullptr, "token is nullptr");

    error_code error = {};
    size_t name_idx = get_or_add_ident_idx(token->lexeme,
                                           parser->tree->ident_stack,
                                           &error);
    (void)error;
    return name_idx;
}

//================================================================================
//                      Проход 1: собрать таблицу функций
//================================================================================

static void pass1_skip_block(parser_state_t* parser) {
    size_t brace_depth = 0;

    if (parser_match_keyword(parser, OP_VIS_START)) {
        brace_depth = 1;
    } else {
        parser_expected(parser, "'{'");
        parser_sync_to_lcat(parser);
        return;
    }

    while (!parser_is_eof(parser) && brace_depth > 0) {
        if (parser_check_keyword(parser, OP_VIS_START)) {
            parser_advance(parser);
            brace_depth++;
            continue;
        }
        if (parser_check_kind(parser, LEX_TK_RBRACE)) {
            parser_advance(parser);
            brace_depth--;
            continue;
        }
        parser_advance(parser);
    }

    if (brace_depth != 0) {
        parser_push_diag(parser, DIAG_PARSE_EXPECTED, nullptr,
                         "ожидалось: '}' (не закрыт блок)");
    }
}

static size_t pass1_count_params(parser_state_t* parser) {
    size_t argc = 0;

    if (!parser_match_kind(parser, LEX_TK_LPAREN)) {
        parser_expected(parser, "'('");
        return 0;
    }

    if (parser_match_kind(parser, LEX_TK_RPAREN)) {
        return 0;
    }

    while (!parser_is_eof(parser)) {
        if (!parser_check_kind(parser, LEX_TK_IDENT)) {
            parser_expected(parser, "идентификатор параметра");
            parser_sync_to_lcat(parser);
            break;
        }

        const lexer_token_t* token = parser_advance(parser);
        (void)parse_ident_idx(parser, token);
        argc++;

        if (parser_match_keyword(parser, OP_ENUM_SEP)) continue;
        if (parser_match_kind(parser, LEX_TK_RPAREN)) break;

        parser_expected(parser, "')' или ','");
        parser_sync_to_lcat(parser);
        break;
    }

    return argc;
}

static void pass1_collect_one(parser_state_t* parser,
                              op_code_t decl_opcode) {
    parser_advance(parser);

    if (!parser_check_kind(parser, LEX_TK_IDENT)) {
        parser_expected(parser, "имя функции/процедуры");
        parser_sync_to_lcat(parser);
        return;
    }

    const lexer_token_t* name_tok = parser_advance(parser);
    size_t name_idx = parse_ident_idx(parser, name_tok);

    func_decl_info_t exists = {};
    if (func_table_get(parser->func_table, name_idx, &exists)) {
        parser_push_diag(parser, DIAG_PARSE_REDEF_FUNCTION, name_tok,
                         "переопределение функции/процедуры");
    } else {
        func_decl_info_t info = {decl_opcode, 0};
        info.argc = pass1_count_params(parser);
        func_table_put(parser->func_table, name_idx, &info);
    }

    pass1_skip_block(parser);

    if (parser_check_keyword(parser, OP_LCAT)) {
        parser_advance(parser);
    }
}

static void parser_pass1_collect(parser_state_t* parser) {
    while (!parser_is_eof(parser)) {
        if (parser_check_keyword(parser, OP_FUNC_DECL)) {
            pass1_collect_one(parser, OP_FUNC_DECL);
            continue;
        }
        if (parser_check_keyword(parser, OP_PROC_DECL)) {
            pass1_collect_one(parser, OP_PROC_DECL);
            continue;
        }
        parser_advance(parser);
    }
}

//================================================================================
//                      Вспомогательное: списки и ';'
//================================================================================

static bool parser_consume_stmt_end(parser_state_t* parser,
                                   bool allow_trailing,
                                   bool* trailing_out) {
    HARD_ASSERT(parser != nullptr, "parser is nullptr");
    HARD_ASSERT(trailing_out != nullptr, "trailing_out is nullptr");

    if (!parser_match_keyword(parser, OP_LCAT)) {
        parser_expected(parser, "';'");
        parser_sync_to_lcat(parser);
        *trailing_out = false;
        return false;
    }

    bool is_trailing = parser_is_eof(parser) ||
                       parser_check_kind(parser, LEX_TK_RBRACE);
    *trailing_out = is_trailing && allow_trailing;
    return true;
}

//================================================================================
//                      Проход 2: разбор верхнего уровня
//================================================================================

static tree_node_t* parse_toplevel(parser_state_t* parser) {
    tree_node_t* list_root = nullptr;

    while (!parser_is_eof(parser)) {
        LOGGER_DEBUG("a");
        if (!parser_check_keyword(parser, OP_FUNC_DECL) &&
            !parser_check_keyword(parser, OP_PROC_DECL)) {
            parser_expected(parser, "объявление func/proc");
            parser_sync_to_lcat(parser);
            continue;
        }

        tree_node_t* decl_node = parse_decl(parser);
        if (decl_node == nullptr) {
            parser_sync_to_lcat(parser);
            continue;
        }

        bool trailing = false;
        (void)parser_consume_stmt_end(parser, true, &trailing);

        list_root = ast_make_list(OP_LCAT, list_root, decl_node);
        if (trailing) break;
    }

    return ast_unary(OP_VIS_START, list_root);
}

//================================================================================
//                          Разбор объявления func/proc
//================================================================================

static tree_node_t* parse_param_list(parser_state_t* parser,
                                     size_t* argc_out) {
    HARD_ASSERT(argc_out != nullptr, "argc_out is nullptr");
    *argc_out = 0;

    if (!parser_match_kind(parser, LEX_TK_LPAREN)) {
        parser_expected(parser, "'('");
        return nullptr;
    }

    if (parser_match_kind(parser, LEX_TK_RPAREN)) {
        return nullptr;
    }

    tree_node_t* list_root = nullptr;

    while (!parser_is_eof(parser)) {
        if (!parser_check_kind(parser, LEX_TK_IDENT)) {
            parser_expected(parser, "идентификатор параметра");
            break;
        }

        const lexer_token_t* param_tok = parser_advance(parser);
        size_t param_idx = parse_ident_idx(parser, param_tok);
        tree_node_t* param_node = ast_var(param_idx);

        list_root = ast_make_list(OP_ENUM_SEP, list_root, param_node);
        (*argc_out)++;

        if (parser_match_keyword(parser, OP_ENUM_SEP)) continue;
        if (parser_match_kind(parser, LEX_TK_RPAREN)) break;

        parser_expected(parser, "')' или ','");
        break;
    }

    if (!parser_prev(parser) || parser_prev(parser)->kind != LEX_TK_RPAREN) {
        parser_sync_to_lcat(parser);
    }

    return list_root;
}

static tree_node_t* parse_decl(parser_state_t* parser) {
    op_code_t decl_opcode = OP_NONE;

    if (parser_match_keyword(parser, OP_FUNC_DECL)) decl_opcode = OP_FUNC_DECL;
    else if (parser_match_keyword(parser, OP_PROC_DECL)) decl_opcode = OP_PROC_DECL;
    else return nullptr;

    if (!parser_check_kind(parser, LEX_TK_IDENT)) {
        parser_expected(parser, "имя функции/процедуры");
        return nullptr;
    }

    const lexer_token_t* name_tok = parser_advance(parser);
    size_t name_idx = parse_ident_idx(parser, name_tok);

    func_decl_info_t decl_info = {};
    if (!func_table_get(parser->func_table, name_idx, &decl_info)) {
        parser_push_diag(parser, DIAG_PARSE_UNDEF_FUNCTION, name_tok,
                         "объявление не найдено в таблице 1-го прохода");
    }

    if (decl_info.decl_opcode != OP_NONE &&
        decl_info.decl_opcode != decl_opcode) {
        parser_push_diag(parser, DIAG_PARSE_EXPECTED, name_tok,
                         "несовпадение вида (func/proc) с 1-м проходом");
    }

    size_t argc = 0;
    tree_node_t* args_node = parse_param_list(parser, &argc);

    if (decl_info.decl_opcode != OP_NONE && decl_info.argc != argc) {
        parser_push_diag(parser, DIAG_PARSE_EXPECTED, name_tok,
                         "несовпадение числа параметров с 1-м проходом");
    }

    tree_node_t* name_node = ast_var(name_idx);
    tree_node_t* info_node = ast_func(OP_FUNC_INFO, args_node, name_node);

    op_code_t prev_decl = parser->current_decl;
    parser->current_decl = decl_opcode;

    tree_node_t* body_node = parse_block(parser);

    parser->current_decl = prev_decl;

    if (body_node == nullptr) return nullptr;
    return ast_func(decl_opcode, info_node, body_node);
}

//================================================================================
//                              Разбор блока { ... }
//================================================================================

static tree_node_t* parse_block(parser_state_t* parser) {
    if (!parser_match_keyword(parser, OP_VIS_START)) {
        parser_expected(parser, "'{'");
        return nullptr;
    }

    tree_node_t* list_node = parse_stmt_list(parser);

    if (!parser_match_kind(parser, LEX_TK_RBRACE)) {
        parser_expected(parser, "'}'");
        parser_sync_to_lcat(parser);
    }

    return ast_unary(OP_VIS_START, list_node);
}

static tree_node_t* parse_stmt_list(parser_state_t* parser) {
    tree_node_t* list_root = nullptr;

    while (!parser_is_eof(parser) &&
           !parser_check_kind(parser, LEX_TK_RBRACE)) {

        tree_node_t* stmt_node = parse_stmt(parser);
        if (stmt_node == nullptr) {
            parser_sync_to_lcat(parser);
            continue;
        }

        bool trailing = false;
        (void)parser_consume_stmt_end(parser, true, &trailing);

        list_root = ast_make_list(OP_LCAT, list_root, stmt_node);
        if (trailing) break;
    }

    return list_root;
}

//================================================================================
//                              Разбор операторов
//================================================================================

static tree_node_t* parse_if(parser_state_t* parser) {
    parser_advance(parser); // IF

    bool has_paren = parser_match_kind(parser, LEX_TK_LPAREN);
    tree_node_t* cond_node = parse_expr_value(parser);

    if (has_paren && !parser_match_kind(parser, LEX_TK_RPAREN)) {
        parser_expected(parser, "')'");
        parser_sync_to_lcat(parser);
    }

    tree_node_t* one_node = ast_const(1.0);
    tree_node_t* eq_node  = ast_func(OP_EQ, cond_node, one_node);

    tree_node_t* body_node = parse_block(parser);
    return ast_func(OP_IF, eq_node, body_node);
}

static tree_node_t* parse_while(parser_state_t* parser) {
    parser_advance(parser); // WHILE

    bool has_paren = parser_match_kind(parser, LEX_TK_LPAREN);
    tree_node_t* cond_node = parse_expr_value(parser);

    if (has_paren && !parser_match_kind(parser, LEX_TK_RPAREN)) {
        parser_expected(parser, "')'");
        parser_sync_to_lcat(parser);
    }

    parser->while_depth++;
    tree_node_t* body_node = parse_block(parser);
    parser->while_depth--;

    return ast_func(OP_WHILE, cond_node, body_node);
}

static tree_node_t* parse_stmt_expr(parser_state_t* parser) {
    if (parser_check_keyword(parser, OP_CALL)) {
        return parse_call(parser, false);
    }
    return parse_expr_value(parser);
}

static tree_node_t* parse_stmt(parser_state_t* parser) {
    if (parser_check_keyword(parser, OP_FUNC_DECL) ||
        parser_check_keyword(parser, OP_PROC_DECL)) {
        parser_push_diag(parser, DIAG_PARSE_NESTED_DECL, nullptr,
                         "объявления func/proc запрещены внутри тела");
        return nullptr;
    }

    if (parser_check_keyword(parser, OP_IF)) return parse_if(parser);
    if (parser_check_keyword(parser, OP_WHILE)) return parse_while(parser);

    if (parser_match_keyword(parser, OP_BREAK)) {
        if (parser->while_depth == 0) {
            parser_push_diag(parser, DIAG_PARSE_BREAK_OUTSIDE, parser_prev(parser),
                             "break вне while");
        }
        return ast_unary(OP_BREAK, nullptr);
    }

    if (parser_match_keyword(parser, OP_CONTINUE)) {
        if (parser->while_depth == 0) {
            parser_push_diag(parser, DIAG_PARSE_BREAK_OUTSIDE, parser_prev(parser),
                             "continue вне while");
        }
        return ast_unary(OP_CONTINUE, nullptr);
    }

    if (parser_match_keyword(parser, OP_FINISH)) {
        if (parser->current_decl == OP_FUNC_DECL) {
            parser_push_diag(parser, DIAG_PARSE_FINISH_IN_FUNC, parser_prev(parser),
                             "finish запрещен в func");
        }
        return ast_unary(OP_FINISH, nullptr);
    }

    if (parser_match_keyword(parser, OP_RETURN)) {
        if (parser->current_decl == OP_PROC_DECL) {
            parser_push_diag(parser, DIAG_PARSE_RETURN_IN_PROC, parser_prev(parser),
                             "return запрещен в proc");
        }

        if (parser_check_keyword(parser, OP_LCAT) ||
            parser_check_kind(parser, LEX_TK_RBRACE) ||
            parser_is_eof(parser)) {
            parser_expected(parser, "выражение после return");
            return ast_unary(OP_RETURN, nullptr);
        }

        tree_node_t* expr_node = parse_expr_value(parser);
        return ast_unary(OP_RETURN, expr_node);
    }

    if (parser_match_keyword(parser, OP_PRINT)) {
        bool has_paren = parser_match_kind(parser, LEX_TK_LPAREN);
        tree_node_t* expr_node = parse_expr_value(parser);

        if (has_paren && !parser_match_kind(parser, LEX_TK_RPAREN)) {
            parser_expected(parser, "')'");
            parser_sync_to_lcat(parser);
        }
        return ast_unary(OP_PRINT, expr_node);
    }

    if (parser_check_keyword(parser, OP_VIS_START)) return parse_block(parser);
    return parse_stmt_expr(parser);
}

//================================================================================
//                              Разбор выражений
//================================================================================

static tree_node_t* parse_expr_value(parser_state_t* parser) {
    return parse_assign(parser);
}

static tree_node_t* parse_assign(parser_state_t* parser) {
    tree_node_t* left_node = parse_or(parser);

    if (!parser_match_keyword(parser, OP_ASSIGN)) {
        return left_node;
    }

    if (left_node == nullptr || left_node->type != VARIABLE) {
        parser_push_diag(parser, DIAG_PARSE_EXPECTED, parser_prev(parser),
                         "слева от '=' должна быть переменная");
    }

    tree_node_t* right_node = parse_assign(parser);
    return ast_func(OP_ASSIGN, left_node, right_node);
}

static tree_node_t* parse_or(parser_state_t* parser) {
    tree_node_t* node = parse_and(parser);

    while (parser_match_keyword(parser, OP_OR)) {
        tree_node_t* right_node = parse_and(parser);
        node = ast_func(OP_OR, node, right_node);
    }

    return node;
}

static tree_node_t* parse_and(parser_state_t* parser) {
    tree_node_t* node = parse_eq(parser);

    while (parser_match_keyword(parser, OP_AND)) {
        tree_node_t* right_node = parse_eq(parser);
        node = ast_func(OP_AND, node, right_node);
    }

    return node;
}

static tree_node_t* parse_eq(parser_state_t* parser) {
    tree_node_t* node = parse_rel(parser);

    while (true) {
        if (parser_match_keyword(parser, OP_EQ)) {
            tree_node_t* right_node = parse_rel(parser);
            node = ast_func(OP_EQ, node, right_node);
            continue;
        }
        if (parser_match_keyword(parser, OP_NEQ)) {
            tree_node_t* right_node = parse_rel(parser);
            node = ast_func(OP_NEQ, node, right_node);
            continue;
        }
        break;
    }

    return node;
}

static tree_node_t* parse_rel(parser_state_t* parser) {
    tree_node_t* node = parse_add(parser);

    while (true) {
        if (parser_match_keyword(parser, OP_LT)) {
            tree_node_t* right_node = parse_add(parser);
            node = ast_func(OP_LT, node, right_node);
            continue;
        }
        if (parser_match_keyword(parser, OP_LE)) {
            tree_node_t* right_node = parse_add(parser);
            node = ast_func(OP_LE, node, right_node);
            continue;
        }
        if (parser_match_keyword(parser, OP_GT)) {
            tree_node_t* right_node = parse_add(parser);
            node = ast_func(OP_GT, node, right_node);
            continue;
        }
        if (parser_match_keyword(parser, OP_GE)) {
            tree_node_t* right_node = parse_add(parser);
            node = ast_func(OP_GE, node, right_node);
            continue;
        }
        break;
    }

    return node;
}

static tree_node_t* parse_add(parser_state_t* parser) {
    tree_node_t* node = parse_mul(parser);

    while (true) {
        if (parser_match_keyword(parser, OP_PLUS)) {
            tree_node_t* right_node = parse_mul(parser);
            node = ast_func(OP_PLUS, node, right_node);
            continue;
        }
        if (parser_match_keyword(parser, OP_MINUS)) {
            tree_node_t* right_node = parse_mul(parser);
            node = ast_func(OP_MINUS, node, right_node);
            continue;
        }
        break;
    }

    return node;
}

static tree_node_t* parse_mul(parser_state_t* parser) {
    tree_node_t* node = parse_unary(parser);

    while (true) {
        if (parser_match_keyword(parser, OP_MUL)) {
            tree_node_t* right_node = parse_unary(parser);
            node = ast_func(OP_MUL, node, right_node);
            continue;
        }
        if (parser_match_keyword(parser, OP_DIV)) {
            tree_node_t* right_node = parse_unary(parser);
            node = ast_func(OP_DIV, node, right_node);
            continue;
        }
        break;
    }

    return node;
}

static tree_node_t* parse_unary(parser_state_t* parser) {
    if (parser_match_keyword(parser, OP_PLUS)) {
        tree_node_t* right_node = parse_unary(parser);
        return ast_unary(OP_PLUS, right_node);
    }
    if (parser_match_keyword(parser, OP_MINUS)) {
        tree_node_t* right_node = parse_unary(parser);
        return ast_unary(OP_MINUS, right_node);
    }
    return parse_primary(parser);
}

static tree_node_t* parse_primary(parser_state_t* parser) {
    if (parser_match_kind(parser, LEX_TK_NUMBER)) {
        const lexer_token_t* token = parser_prev(parser);
        return ast_const(token->number);
    }

    if (parser_match_keyword(parser, OP_INPUT)) {
        return ast_unary(OP_INPUT, nullptr);
    }

    if (parser_check_keyword(parser, OP_CALL)) {
        return parse_call(parser, true);
    }

    if (parser_match_kind(parser, LEX_TK_IDENT)) {
        const lexer_token_t* token = parser_prev(parser);
        size_t name_idx = parse_ident_idx(parser, token);
        return ast_var(name_idx);
    }

    if (parser_match_kind(parser, LEX_TK_LPAREN)) {
        tree_node_t* node = parse_expr_value(parser);
        if (!parser_match_kind(parser, LEX_TK_RPAREN)) {
            parser_expected(parser, "')'");
            parser_sync_to_lcat(parser);
        }
        return node;
    }

    parser_expected(parser, "выражение");
    return nullptr;
}

//================================================================================
//                                 call <name>(args)
//================================================================================

static tree_node_t* parse_call_args(parser_state_t* parser,
                                   size_t* argc_out) {
    HARD_ASSERT(argc_out != nullptr, "argc_out is nullptr");
    *argc_out = 0;

    if (!parser_match_kind(parser, LEX_TK_LPAREN)) {
        parser_expected(parser, "'('");
        return nullptr;
    }

    if (parser_match_kind(parser, LEX_TK_RPAREN)) {
        return nullptr;
    }

    tree_node_t* list_root = nullptr;

    while (!parser_is_eof(parser)) {
        tree_node_t* arg_node = parse_expr_value(parser);
        list_root = ast_make_list(OP_ENUM_SEP, list_root, arg_node);
        (*argc_out)++;

        if (parser_match_keyword(parser, OP_ENUM_SEP)) continue;
        if (parser_match_kind(parser, LEX_TK_RPAREN)) break;

        parser_expected(parser, "')' или ','");
        parser_sync_to_lcat(parser);
        break;
    }

    return list_root;
}

static tree_node_t* parse_call(parser_state_t* parser,
                               bool value_context) {
    parser_advance(parser); // CALL

    if (!parser_check_kind(parser, LEX_TK_IDENT)) {
        parser_expected(parser, "имя вызываемой функции/процедуры");
        return nullptr;
    }

    const lexer_token_t* name_tok = parser_advance(parser);
    size_t name_idx = parse_ident_idx(parser, name_tok);

    func_decl_info_t decl_info = {};
    bool known = func_table_get(parser->func_table, name_idx, &decl_info);
    if (!known) {
        parser_push_diag(parser, DIAG_PARSE_UNDEF_FUNCTION, name_tok,
                         "вызов неизвестной функции/процедуры");
    }

    size_t argc = 0;
    tree_node_t* args_node = parse_call_args(parser, &argc);

    if (known && decl_info.argc != argc) {
        parser_push_diag(parser, DIAG_PARSE_ARGC_MISMATCH, name_tok,
                         "несовпадение числа аргументов (ожидалось %zu, получено %zu)",
                         decl_info.argc, argc);
    }

    bool is_proc = known && decl_info.decl_opcode == OP_PROC_DECL;
    if (is_proc && value_context) {
        parser_push_diag(parser, DIAG_PARSE_VOID_IN_EXPR, name_tok,
                         "proc нельзя использовать как выражение");
    }

    tree_node_t* name_node = ast_var(name_idx);
    tree_node_t* info_node = ast_func(OP_FUNC_INFO, args_node, name_node);
    return ast_func(OP_CALL, info_node, nullptr);
}

//================================================================================
//                               Публичный API
//================================================================================

void frontend_parse_ast(tree_t* tree, const vector_t* tokens,
                        u_map_t* func_table, vector_t* diags_out) {
    HARD_ASSERT(tree != nullptr, "tree is nullptr");
    HARD_ASSERT(tokens != nullptr, "tokens is nullptr");
    HARD_ASSERT(func_table != nullptr, "func_table is nullptr");
    HARD_ASSERT(diags_out != nullptr, "diags_out is nullptr");

    parser_state_t parser = {};
    parser.tree         = tree;
    parser.tokens       = tokens;
    parser.position     = 0;
    parser.func_table   = func_table;
    parser.diags        = diags_out;
    parser.current_decl = OP_NONE;
    parser.while_depth  = 0;

    parser_pass1_collect(&parser);

    parser.position = 0;

    LOGGER_DEBUG("start parser AST");
    tree_node_t* root = parse_toplevel(&parser);
    tree->root = root;
    LOGGER_DEBUG(" end parser AST");
    tree->size = count_nodes_recursive(root);
}
