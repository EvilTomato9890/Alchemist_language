#include "lexer_tokenizer.h"

#include "common/asserts/include/asserts.h"
#include "common/logger/include/logger.h"
#include "error_logger/include/frontend_err_logger.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

//================================================================================

struct lexer_state_t {
    c_string_t buffer;
    size_t     position;

    size_t     line;
    size_t     column;

    const lexer_config_t* config;

    vector_t* tokens_out;
    vector_t* diags_out;
};

static bool is_space_char(unsigned char ch) {
    return isspace(ch) != 0;
}

static bool is_gap_char(unsigned char ch) {
    return (ch == ' ') || (ch == '\t') || (ch == '\r');
}

static bool is_ident_char(unsigned char ch) {
    return (isalnum(ch) != 0) || (ch == '_');
}

static bool is_ident_start(unsigned char ch) {
    return (isalpha(ch) != 0) || (ch == '_');
}
//================================================================================

static lexer_error_t lexer_push_diag(lexer_state_t* state,
                                     const diag_log_t* diag);
                                     
//================================================================================

static void lexer_advance(lexer_state_t* state, size_t count) {
    HARD_ASSERT(state != nullptr, "state is nullptr");

    size_t end_pos = state->position + count;
    if (end_pos > state->buffer.len) end_pos = state->buffer.len;

    while (state->position < end_pos) {
        unsigned char ch = (unsigned char)state->buffer.ptr[state->position];

        state->position++;
        if (ch == '\n') {
            state->line++;
            state->column = 1;
        } else {
            state->column++;
        }
    }
}

static void lexer_skip_spaces(lexer_state_t* state) {
    HARD_ASSERT(state != nullptr, "state is nullptr");

    while (state->position < state->buffer.len) {
        unsigned char ch = (unsigned char)state->buffer.ptr[state->position];
        if (!is_space_char(ch)) break;
        lexer_advance(state, 1);
    }
}

static bool check_2_chars(c_string_t buffer, size_t position,
                          char first, char second) {
    if (position + 1 >= buffer.len) return false;
    return buffer.ptr[position] == first && buffer.ptr[position + 1] == second;
}

static void skip_line_comment(lexer_state_t* state) {
    HARD_ASSERT(state != nullptr, "state is nullptr");

    lexer_advance(state, 2);

    while (state->position < state->buffer.len) {
        if (state->buffer.ptr[state->position] == '\n') break;
        lexer_advance(state, 1);
    }
}

static lexer_error_t skip_block_comment(lexer_state_t* state) {
    HARD_ASSERT(state != nullptr, "state is nullptr");

    size_t start_pos = state->position;
    size_t start_line = state->line;
    size_t start_col = state->column;

    lexer_advance(state, 2);

    while (state->position + 1 < state->buffer.len) {
        if (check_2_chars(state->buffer, state->position, '*', '/')) {
            lexer_advance(state, 2);
            return LEX_ERR_OK;
        }
        lexer_advance(state, 1);
    }

    diag_log_t diag = diag_log_init(LEXER_ERROR, DIAG_LEX_UNTERMINATED_COMMENT,
                                    start_pos, 2,
                                    start_line, start_col,
                                    "unterminated block comment");

    lexer_error_t err = lexer_push_diag(state, &diag);
    if (err != LEX_ERR_OK) return err;

    state->position = state->buffer.len;
    return LEX_ERR_OK;
}

static lexer_error_t lexer_skip_trivia(lexer_state_t* state) {
    while (true) {
        size_t before = state->position;

        lexer_skip_spaces(state);

        if (check_2_chars(state->buffer, state->position, '/', '/')) {
            skip_line_comment(state);
        } else if (check_2_chars(state->buffer, state->position, '/', '*')) {
            lexer_error_t err = skip_block_comment(state);
            if (err != LEX_ERR_OK) return err;
        }

        if (state->position == before) break;
    }

    return LEX_ERR_OK;
}

//================================================================================

static lexer_error_t lexer_push_token(lexer_state_t* state,
                                      const lexer_token_t* token) {
    HARD_ASSERT(state != nullptr, "state is nullptr");
    HARD_ASSERT(token != nullptr, "token is nullptr");

    vector_error_t err = vector_push_back(state->tokens_out, token);
    return (err == 0) ? LEX_ERR_OK : LEX_ERR_VEC_FAIL;
}

static lexer_error_t lexer_push_diag(lexer_state_t* state,
                                     const diag_log_t* diag) {
    HARD_ASSERT(state != nullptr, "state is nullptr");
    HARD_ASSERT(diag != nullptr, "diag is nullptr");

    vector_error_t err = vector_push_back(state->diags_out, diag);
    return (err == 0) ? LEX_ERR_OK : LEX_ERR_VEC_FAIL;
}

static void lexer_make_lexeme(c_string_t* out, c_string_t buffer,
                              size_t position, size_t length) {
    HARD_ASSERT(out != nullptr, "out is nullptr");

    if (position > buffer.len)          position = buffer.len;
    if (position + length > buffer.len) length = buffer.len - position;

    *out = { buffer.ptr + position, length };
}

static bool pattern_is_wordlike(const char* pattern) {
    if (pattern == nullptr || pattern[0] == '\0') return false;

    size_t first = 0;
    while (pattern[first] != '\0' && is_gap_char((unsigned char)pattern[first]))
        first++;

    if (pattern[first] == '\0') return false;
    size_t last = strlen(pattern);
    while (last > 0 && is_gap_char((unsigned char)pattern[last - 1]))
        last--;
    if (last == 0) return false;

    unsigned char beg = (unsigned char)pattern[first];
    unsigned char end = (unsigned char)pattern[last - 1];

    return is_ident_char(beg) && is_ident_char(end);
}

static bool match_pattern_at(c_string_t buffer, size_t position,
                             const char* pattern, size_t* advance_out) {
    HARD_ASSERT(pattern != nullptr, "pattern is nullptr");
    HARD_ASSERT(advance_out != nullptr, "advance_out is nullptr");

    size_t buf_pos = position;
    size_t pat_pos = 0;

    while (pattern[pat_pos] != '\0') {
        if (buf_pos >= buffer.len) return false;

        unsigned char pat_ch = (unsigned char)pattern[pat_pos];
        if (is_gap_char(pat_ch)) {
            if (!is_gap_char((unsigned char)buffer.ptr[buf_pos])) return false;

            while (buf_pos < buffer.len     && is_gap_char((unsigned char)buffer.ptr[buf_pos]))
                buf_pos++;
            while (pattern[pat_pos] != '\0' && is_gap_char((unsigned char)pattern[pat_pos]))
                pat_pos++;

            continue;
        }

        if ((unsigned char)buffer.ptr[buf_pos] != pat_ch) return false;

        buf_pos++;
        pat_pos++;
    }

    *advance_out = buf_pos - position;
    return true;
}

static bool check_word_boundaries(c_string_t buffer, size_t position, size_t advance) {
    if (position > 0) {
        unsigned char left = (unsigned char)buffer.ptr[position - 1];
        if (is_ident_char(left)) return false;
    }

    size_t end_pos = position + advance;
    if (end_pos < buffer.len) {
        unsigned char right = (unsigned char)buffer.ptr[end_pos];
        if (is_ident_char(right)) return false;
    }

    return true;
}

static bool find_best_match(const keyword_def_t* table, size_t count,
                            c_string_t buffer, size_t position,
                            const keyword_def_t** best_out,
                            size_t* advance_out) {
    HARD_ASSERT(best_out != nullptr, "best_out is nullptr");
    HARD_ASSERT(advance_out != nullptr, "advance_out is nullptr");

    const keyword_def_t* best = nullptr;
    size_t best_advance = 0;
    size_t best_len = 0;

    for (size_t i = 0; i < count; ++i) {
        const char* pattern = table[i].lang_name;
        if (pattern == nullptr || pattern[0] == '\0') continue;

        size_t advance = 0;
        if (!match_pattern_at(buffer, position, pattern, &advance)) continue;

        if (pattern_is_wordlike(pattern) &&
            !check_word_boundaries(buffer, position, advance))
            continue;

        size_t pattern_len = strlen(pattern);
        if (advance > best_advance ||
            (advance == best_advance && pattern_len > best_len)) {
            best = &table[i];
            best_advance = advance;
            best_len = pattern_len;
        }
    }

    if (best == nullptr) return false;

    *best_out = best;
    *advance_out = best_advance;
    return true;
}

static bool number_can_start(c_string_t buffer, size_t position) {
    if (position >= buffer.len) return false;

    unsigned char ch = (unsigned char)buffer.ptr[position];
    if (isdigit(ch)) return true;

    if (ch == '.' && (position + 1) < buffer.len)
        return isdigit((unsigned char)buffer.ptr[position + 1]) != 0;

    return false;
}


static size_t scan_number_len(c_string_t buffer, size_t position) {
    size_t pos = position;

    bool has_digits = false;
    while (pos < buffer.len && isdigit((unsigned char)buffer.ptr[pos])) {
        pos++;
        has_digits = true;
    }

    if (pos < buffer.len && buffer.ptr[pos] == '.') {
        pos++;
        while (pos < buffer.len && isdigit((unsigned char)buffer.ptr[pos])) {
            pos++;
            has_digits = true;
        }
    }

    if (!has_digits) return 0;

    if (pos < buffer.len) {
        char ch = buffer.ptr[pos];
        if (ch == 'e' || ch == 'E') {
            size_t exp_pos = pos;
            pos++;

            if (pos < buffer.len) {
                char s = buffer.ptr[pos];
                if (s == '+' || s == '-') pos++;
            }

            size_t dig_pos = pos;
            while (pos < buffer.len && isdigit((unsigned char)buffer.ptr[pos]))
                pos++;

            if (dig_pos == pos) pos = exp_pos;
        }
    }

    return pos - position;
}

static lexer_error_t parse_number_slice(c_string_t slice, double* out_value) {
    HARD_ASSERT(out_value != nullptr, "out_value is nullptr");

    char* tmp = (char*)calloc(slice.len + 1, 1);
    if (tmp == nullptr) return LEX_ERR_NO_MEM;

    memcpy(tmp, slice.ptr, slice.len);
    tmp[slice.len] = '\0';

    char* endp = nullptr;
    double val = strtod(tmp, &endp);

    bool ok = (endp != nullptr) && ((size_t)(endp - tmp) == slice.len);
    free(tmp);

    if (!ok) return LEX_ERR_BAD_ARG;

    *out_value = val;
    return LEX_ERR_OK;
}

static size_t scan_ident_len(c_string_t buffer, size_t position) {
    if (position >= buffer.len) return 0;
    if (!is_ident_start((unsigned char)buffer.ptr[position])) return 0;

    size_t pos = position + 1;
    while (pos < buffer.len && is_ident_char((unsigned char)buffer.ptr[pos]))
        pos++;

    return pos - position;
}

//================================================================================

static lexer_error_t lex_emit_eof(lexer_state_t* state) {
    HARD_ASSERT(state != nullptr, "state is nullptr");

    lexer_token_t token = {};

    token.kind = LEX_TK_EOF;
    token.position = state->position;
    token.line = state->line;
    token.column = state->column;

    lexer_make_lexeme(&token.lexeme, state->buffer, state->position, 0);
    return lexer_push_token(state, &token);
}

static lexer_error_t lex_unknown_symbol(lexer_state_t* state, unsigned char got) {
    HARD_ASSERT(state != nullptr, "state is nullptr");

    diag_log_t diag = diag_log_init(LEXER_ERROR, DIAG_LEX_UNKNOWN_SYMBOL,
                                    state->position, 1,
                                    state->line, state->column,
                                    isprint(got) ? "unknown symbol '%c'" : "unknown symbol (0x%02X)",
                                    isprint(got) ? (char)got : (unsigned)got);

    lexer_error_t err = lexer_push_diag(state, &diag);
    if (err != LEX_ERR_OK) return err;

    lexer_advance(state, 1);
    return LEX_ERR_OK;
}

static lexer_error_t lex_bad_number(lexer_state_t* state, size_t length) {
    HARD_ASSERT(state != nullptr, "state is nullptr");

    diag_log_t diag = diag_log_init(LEXER_ERROR, DIAG_LEX_BAD_NUMBER,
                                    state->position, (length > 0) ? length : 1,
                                    state->line, state->column,
                                    "bad number literal");

    lexer_error_t err = lexer_push_diag(state, &diag);
    if (err != LEX_ERR_OK) return err;

    lexer_advance(state, diag.length);
    return LEX_ERR_OK;
}


static lexer_error_t lex_try_ignore_word(lexer_state_t* state, bool* ignored_out) {
    HARD_ASSERT(ignored_out != nullptr, "ignored_out is nullptr");
    *ignored_out = false;

    const lexer_config_t* cfg = state->config;
    if (cfg->ignored_words == nullptr || cfg->ignored_words_count == 0)
        return LEX_ERR_OK;

    const keyword_def_t* best = nullptr;
    size_t advance = 0;

    bool found = find_best_match(cfg->ignored_words, cfg->ignored_words_count,
                                 state->buffer, state->position,
                                 &best, &advance);
    if (!found) return LEX_ERR_OK;

    lexer_advance(state, advance);
    *ignored_out = true;
    return LEX_ERR_OK;
}

static lexer_error_t lex_try_parens(lexer_state_t* state, bool* matched_out) {
    HARD_ASSERT(matched_out != nullptr, "matched_out is nullptr");
    *matched_out = false;

    if (state->position >= state->buffer.len) return LEX_ERR_OK;

    char ch = state->buffer.ptr[state->position];
    if (ch != '(' && ch != ')') return LEX_ERR_OK;

    lexer_token_t token = {};

    token.kind     = (ch == '(') ? LEX_TK_LPAREN : LEX_TK_RPAREN;
    token.position = state->position;
    token.line     = state->line;
    token.column   = state->column;

    lexer_make_lexeme(&token.lexeme, state->buffer, state->position, 1);

    lexer_error_t err = lexer_push_token(state, &token);
    if (err != LEX_ERR_OK) return err;

    lexer_advance(state, 1);
    *matched_out = true;
    return LEX_ERR_OK;
}

static lexer_error_t lex_try_number(lexer_state_t* state, bool* matched_out) {
    HARD_ASSERT(matched_out != nullptr, "matched_out is nullptr");
    *matched_out = false;

    if (!number_can_start(state->buffer, state->position)) return LEX_ERR_OK;

    size_t len = scan_number_len(state->buffer, state->position);
    if (len == 0) return LEX_ERR_OK;

    c_string_t slice = {};
    lexer_make_lexeme(&slice, state->buffer, state->position, len);

    double value = 0.0;
    lexer_error_t parse_err = parse_number_slice(slice, &value);
    if (parse_err == LEX_ERR_NO_MEM) return LEX_ERR_NO_MEM;
    if (parse_err != LEX_ERR_OK) return lex_bad_number(state, len);

    lexer_token_t token = {};

    token.kind     = LEX_TK_NUMBER;
    token.position = state->position;
    token.line     = state->line;
    token.column   = state->column;
    token.number   = value;
    token.lexeme   = slice;

    lexer_error_t err = lexer_push_token(state, &token);
    if (err != LEX_ERR_OK) return err;

    lexer_advance(state, len);
    *matched_out = true;
    return LEX_ERR_OK;
}

static lexer_error_t lex_try_keyword(lexer_state_t* state, bool* matched_out) {
    HARD_ASSERT(matched_out != nullptr, "matched_out is nullptr");
    *matched_out = false;

    const lexer_config_t* cfg = state->config;
    if (cfg->keywords == nullptr || cfg->keywords_count == 0)
        return LEX_ERR_OK;

    const keyword_def_t* best = nullptr;
    size_t advance = 0;

    bool found = find_best_match(cfg->keywords, cfg->keywords_count,
                                 state->buffer, state->position,
                                 &best, &advance);
    if (!found) return LEX_ERR_OK;

    lexer_token_t token = {};

    token.kind     = LEX_TK_KEYWORD;
    token.position = state->position;
    token.line     = state->line;
    token.column   = state->column;
    token.opcode   = best->opcode;

    lexer_make_lexeme(&token.lexeme, state->buffer, state->position, advance);

    lexer_error_t err = lexer_push_token(state, &token);
    if (err != LEX_ERR_OK) return err;

    lexer_advance(state, advance);
    *matched_out = true;
    return LEX_ERR_OK;
}

static lexer_error_t lex_try_ident(lexer_state_t* state, bool* matched_out) {
    HARD_ASSERT(matched_out != nullptr, "matched_out is nullptr");
    *matched_out = false;

    size_t len = scan_ident_len(state->buffer, state->position);
    if (len == 0) return LEX_ERR_OK;

    lexer_token_t token = {};

    token.kind     = LEX_TK_IDENT;
    token.position = state->position;
    token.line     = state->line;
    token.column   = state->column;

    lexer_make_lexeme(&token.lexeme, state->buffer, state->position, len);

    lexer_error_t err = lexer_push_token(state, &token);
    if (err != LEX_ERR_OK) return err;

    lexer_advance(state, len);
    *matched_out = true;
    return LEX_ERR_OK;
}

//================================================================================

lexer_error_t lexer_tokenize(c_string_t buffer, const lexer_config_t* config,
                             vector_t* tokens_out, vector_t* diags_out) {
    HARD_ASSERT(config != nullptr, "config is nullptr");
    HARD_ASSERT(tokens_out != nullptr, "tokens_out is nullptr");
    HARD_ASSERT(diags_out != nullptr, "diags_out is nullptr");

    lexer_state_t state = {};
    state.buffer = buffer;
    state.position = 0;
    state.line = 1;
    state.column = 1;
    state.config = config;
    state.tokens_out = tokens_out;
    state.diags_out = diags_out;

    while (state.position < state.buffer.len) {
        lexer_error_t skip_err = lexer_skip_trivia(&state);
        if (skip_err != LEX_ERR_OK) return skip_err;
        if (state.position >= state.buffer.len) break;

        bool ignored = false;
        lexer_error_t err = lex_try_ignore_word(&state, &ignored);
        if (err != LEX_ERR_OK) return err;
        if (ignored) continue;

        bool matched = false;

        err = lex_try_parens(&state, &matched);
        if (err != LEX_ERR_OK) return err;
        if (matched) continue;

        err = lex_try_number(&state, &matched);
        if (err != LEX_ERR_OK) return err;
        if (matched) continue;

        err = lex_try_keyword(&state, &matched);
        if (err != LEX_ERR_OK) return err;
        if (matched) continue;

        err = lex_try_ident(&state, &matched);
        if (err != LEX_ERR_OK) return err;
        if (matched) continue;

        err = lex_unknown_symbol(&state,
                                 (unsigned char)state.buffer.ptr[state.position]);
        if (err != LEX_ERR_OK) return err;
    }

    return lex_emit_eof(&state);
}
