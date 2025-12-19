#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "asserts.h"
#include "common/logger/include/logger.h"
#include "tree_operations.h"
#include "tree_verification.h"
#include "error_handler.h"
#include "tree_info.h"
#include "tree_file_io.h"
#include "libs/Stack/include/stack.h"
#include "libs/My_string/include/my_string.h"
#include "common/file_operations/include/file_operations.h"
#include "common/keywords/include/keywords.h"

//================================================================================

static op_code_t get_op_code(c_string_t func_str, error_code* error) {
    HARD_ASSERT(func_str.ptr != nullptr, "func_name nullptr");
    HARD_ASSERT(error        != nullptr, "error nullptr");

    for (size_t i = 0; i < KEYWORDS_COUNT; i++) {
        if (my_scstrcmp(func_str, KEYWORDS[i].tree_name) == 0) {
            return KEYWORDS[i].op_code;
        }
    }

    LOGGER_ERROR("unknown function token");
    *error |= ERROR_UNKNOWN_FUNC;
    return OP_PLUS;
}

static bool is_token_delim(char ch) {
    return ch == '\0' || isspace((unsigned char)ch) || ch == '(' || ch == ')' || ch == ',';
}

static int parse_double_span(const char* begin_ptr, const char* end_ptr, double* out_val) {
    HARD_ASSERT(begin_ptr != nullptr, "begin_ptr nullptr");
    HARD_ASSERT(end_ptr   != nullptr, "end_ptr nullptr");
    HARD_ASSERT(out_val   != nullptr, "out_val nullptr");

    if (end_ptr <= begin_ptr) return 0;

    const size_t len = (size_t)(end_ptr - begin_ptr);

    char  stack_buf[128] = {};
    char* tmp_buf = stack_buf;

    if (len + 1 > sizeof(stack_buf)) {
        tmp_buf = (char*)calloc(len + 1, sizeof(char));
        if (tmp_buf == nullptr) return 0;
    }

    memcpy(tmp_buf, begin_ptr, len);
    tmp_buf[len] = '\0';

    errno = 0;
    char* end_num = nullptr;
    const double val = strtod(tmp_buf, &end_num);

    const int ok = (errno == 0) && (end_num == tmp_buf + (ptrdiff_t)len);

    if (tmp_buf != stack_buf) free(tmp_buf);

    if (!ok) return 0;

    *out_val = val;
    return 1;
}

static const char* skip_whitespace(const char* buff) {
    HARD_ASSERT(buff != nullptr, "buff nullptr");

    const char* cur = buff;
    while (*cur != '\0' && isspace((unsigned char)*cur)) {
        cur++;
    }
    return cur;
}

static int peek_paren_nil(const char* cur) {
    cur = skip_whitespace(cur);
    if (*cur != '(') return 0;

    cur++;
    cur = skip_whitespace(cur);
    return (*cur == ')');
}

static void cleanup_failed_node(tree_t* tree, tree_node_t* failed_node, tree_node_t* parent) {
    HARD_ASSERT(tree != nullptr, "tree nullptr");

    if (failed_node == nullptr) return;

    size_t removed = 0;
    destroy_node_recursive(failed_node, &removed);

    if (parent == nullptr) {
        tree->root = nullptr;
    } else if (parent->left == failed_node) {
        parent->left = nullptr;
    } else if (parent->right == failed_node) {
        parent->right = nullptr;
    }
}

static void attach_node_to_parent(tree_t* tree, tree_node_t* new_node, tree_node_t* parent) {
    HARD_ASSERT(tree     != nullptr, "tree nullptr");
    HARD_ASSERT(new_node != nullptr, "new_node nullptr");

    if (parent == nullptr) {
        tree->root = new_node;
    } else if (parent->left == nullptr) {
        parent->left = new_node;
    } else {
        parent->right = new_node;
    }
}

static error_code debug_print_buffer_remainder(tree_t* tree, const char* value_start,
                                               const char* value_end, c_string_t buff_str) {
    HARD_ASSERT(tree        != nullptr, "tree nullptr");
    HARD_ASSERT(value_start != nullptr, "value_start nullptr");

    error_code error = ERROR_NO;

    ON_TREE_DEBUG({
        if (tree->dump_file == nullptr) {
            LOGGER_WARNING("dump_file is nullptr");
            return ERROR_NO;
        }

        int tok_len = 0;
        if (value_end != nullptr && value_end >= value_start) {
            tok_len = (int)(value_end - value_start);
        }

        if (value_end != nullptr &&
            buff_str.ptr != nullptr &&
            value_end >= buff_str.ptr &&
            value_end < buff_str.ptr + buff_str.len) {

            size_t remainder_len = buff_str.len - (size_t)(value_end - buff_str.ptr);
            if (remainder_len > 200) remainder_len = 200;

            char* remainder_copy = (char*)calloc(remainder_len + 1, sizeof(char));
            if (remainder_copy != nullptr) {
                memcpy(remainder_copy, value_end, remainder_len);
                remainder_copy[remainder_len] = '\0';

                error = tree_dump(tree, TREE_VER_INIT, true,
                                  "After reading node: %.*str \nBuffer remainder: %s",
                                  tok_len, value_start, remainder_copy);

                free(remainder_copy);
            } else {
                error = tree_dump(tree, TREE_VER_INIT, true,
                                  "After reading node: %.*str",
                                  tok_len, value_start);
            }
        } else {
            error = tree_dump(tree, TREE_VER_INIT, true,
                              "After reading node: %.*str",
                              tok_len, value_start);
        }
    })

    return error;
}

//================================================================================

static int try_parse_node_value(tree_t* tree_ptr, node_type_t* node_type_ptr, value_t* node_value_ptr,
                                const char** value_start_ptr_ref, const char** value_end_ptr_ref,
                                const char** current_ptr_ref, error_code* error) {
    HARD_ASSERT(tree_ptr            != nullptr, "tree_ptr nullptr");
    HARD_ASSERT(node_type_ptr       != nullptr, "node_type_ptr nullptr");
    HARD_ASSERT(node_value_ptr      != nullptr, "node_value_ptr nullptr");
    HARD_ASSERT(value_start_ptr_ref != nullptr, "value_start_ptr_ref nullptr");
    HARD_ASSERT(value_end_ptr_ref   != nullptr, "value_end_ptr_ref nullptr");
    HARD_ASSERT(current_ptr_ref     != nullptr, "current_ptr_ref nullptr");
    HARD_ASSERT(error               != nullptr, "error nullptr");

    const char* cur = *current_ptr_ref;

    const char* value_start_ptr = nullptr;
    const char* value_end_ptr   = nullptr;

    if (*cur == '\"') {
        cur++;
        value_start_ptr = cur;

        const char* scan = cur;
        while (*scan != '\0' && *scan != '\"') {
            scan++;
        }

        if (*scan != '\"') {
            LOGGER_ERROR("read_node: missing closing '\"' for IDENT");
            *error |= ERROR_READ_FILE;
            return 0;
        }

        value_end_ptr = scan;

        *node_type_ptr = IDENT;
        node_value_ptr->ident_idx =
            get_or_add_ident_idx({value_start_ptr, (unsigned long)(value_end_ptr - value_start_ptr)},
                                 tree_ptr->ident_stack, error);

        if (*error != ERROR_NO) {
            LOGGER_ERROR("get_or_add_ident_idx failed");
            return 0;
        }

        cur = scan + 1;
    } else {
        value_start_ptr = cur;

        const char* scan = cur;
        while (!is_token_delim(*scan)) {
            scan++;
        }

        value_end_ptr = scan;

        if (value_end_ptr == value_start_ptr) {
            LOGGER_ERROR("read_node: empty token");
            *error |= ERROR_READ_FILE;
            return 0;
        }

        double num_val = 0.0;
        if (parse_double_span(value_start_ptr, value_end_ptr, &num_val)) {
            *node_type_ptr = CONSTANT;
            node_value_ptr->constant = (const_val_type)num_val;
        } else {
            *node_type_ptr = FUNCTION;
            node_value_ptr->func =
                get_op_code({value_start_ptr, (unsigned long)(value_end_ptr - value_start_ptr)}, error);
        }

        cur = value_end_ptr;
    }

    *current_ptr_ref     = cur;
    *value_start_ptr_ref = value_start_ptr;
    *value_end_ptr_ref   = value_end_ptr;

    return 1;
}

//================================================================================

static int take_char(const char** cur_ref, char need) {
    HARD_ASSERT(cur_ref != nullptr, "cur_ref nullptr");

    const char* cur = skip_whitespace(*cur_ref);
    if (*cur != need) return 0;

    *cur_ref = cur + 1;
    return 1;
}

static int expect_char(const char** cur_ref, char need, error_code* err, const char* msg) {
    HARD_ASSERT(cur_ref != nullptr, "cur_ref nullptr");
    HARD_ASSERT(err     != nullptr, "err nullptr");
    HARD_ASSERT(msg     != nullptr, "msg nullptr");

    if (take_char(cur_ref, need)) return 1;

    LOGGER_ERROR("%s", msg);
    *err |= ERROR_READ_FILE;
    return 0;
}

static int try_read_paren_nil(const char** cur_ref) {
    HARD_ASSERT(cur_ref != nullptr, "cur_ref nullptr");

    const char* cur = skip_whitespace(*cur_ref);
    if (*cur != ')') return 0;

    *cur_ref = cur + 1;
    return 1;
}

static int read_value_span(tree_t* tree_ptr, node_type_t* type_ptr, value_t* val_ptr,
                           const char** cur_ref, const char** val_begin_ref,
                           const char** val_end_ref, error_code* err) {
    HARD_ASSERT(tree_ptr      != nullptr, "tree_ptr nullptr");
    HARD_ASSERT(type_ptr      != nullptr, "type_ptr nullptr");
    HARD_ASSERT(val_ptr       != nullptr, "val_ptr nullptr");
    HARD_ASSERT(cur_ref       != nullptr, "cur_ref nullptr");
    HARD_ASSERT(val_begin_ref != nullptr, "val_begin_ref nullptr");
    HARD_ASSERT(val_end_ref   != nullptr, "val_end_ref nullptr");
    HARD_ASSERT(err           != nullptr, "err nullptr");

    const char* cur = skip_whitespace(*cur_ref);

    const char* vbegin = nullptr;
    const char* vend   = nullptr;

    if (!try_parse_node_value(tree_ptr, type_ptr, val_ptr, &vbegin, &vend, &cur, err)) {
        return 0;
    }

    *cur_ref = cur;
    *val_begin_ref = vbegin;
    *val_end_ref   = vend;

    return 1;
}

static tree_node_t* alloc_attach_node(tree_t* tree_ptr, tree_node_t* parent_ptr,
                                      node_type_t type, value_t value, error_code* err) {
    HARD_ASSERT(tree_ptr != nullptr, "tree_ptr nullptr");
    HARD_ASSERT(err      != nullptr, "err nullptr");

    tree_node_t* node = init_node(type, value, nullptr, nullptr);
    if (node == nullptr) {
        *err |= ERROR_READ_FILE;
        return nullptr;
    }

    attach_node_to_parent(tree_ptr, node, parent_ptr);
    return node;
}

static void debug_after_value(tree_t* tree_ptr, const char* vbegin,
                              const char* vend, c_string_t buff_str) {
    HARD_ASSERT(tree_ptr != nullptr, "tree_ptr nullptr");

    if (vbegin == nullptr) return;

    debug_print_buffer_remainder(tree_ptr, vbegin, vend, buff_str);
}

static tree_node_t* read_node_impl(tree_t* tree_ptr, tree_node_t* parent_ptr,
                                   error_code* err, const char** cur_ref,
                                   c_string_t buff_str);

static int read_children(tree_t* tree_ptr, tree_node_t* node_ptr,
                         error_code* err, const char** cur_ref,
                         c_string_t buff_str) {
    HARD_ASSERT(tree_ptr != nullptr, "tree_ptr nullptr");
    HARD_ASSERT(node_ptr != nullptr, "node_ptr nullptr");
    HARD_ASSERT(err      != nullptr, "err nullptr");
    HARD_ASSERT(cur_ref  != nullptr, "cur_ref nullptr");

    node_ptr->left = read_node_impl(tree_ptr, node_ptr, err, cur_ref, buff_str);
    if (*err) return 0;

    const int left_nil = (node_ptr->left == nullptr);

    const char* cur = skip_whitespace(*cur_ref);

    int comma_seen = 0;
    if (*cur == ',') {
        comma_seen = 1;
        cur++;
        *cur_ref = cur;
    }

    cur = skip_whitespace(*cur_ref);
    const int right_nil = peek_paren_nil(cur);

    if (comma_seen) {
        if (left_nil || right_nil) {
            LOGGER_ERROR("read_node: ',' forbidden when at least one child is ()");
            *err |= ERROR_READ_FILE;
            return 0;
        }
    } else {
        if (!left_nil && !right_nil) {
            LOGGER_ERROR("read_node: expected ',' between non-empty children");
            *err |= ERROR_READ_FILE;
            return 0;
        }
    }

    node_ptr->right = read_node_impl(tree_ptr, node_ptr, err, cur_ref, buff_str);
    if (*err) return 0;

    return 1;
}


static tree_node_t* read_paren_node(tree_t* tree_ptr, tree_node_t* parent_ptr,
                                    error_code* err, const char** cur_ref,
                                    c_string_t buff_str) {
    HARD_ASSERT(tree_ptr != nullptr, "tree_ptr nullptr");
    HARD_ASSERT(err      != nullptr, "err nullptr");
    HARD_ASSERT(cur_ref  != nullptr, "cur_ref nullptr");

    if (!expect_char(cur_ref, '(', err, "read_node: expected '('")) {
        return nullptr;
    }

    if (try_read_paren_nil(cur_ref)) {
        return nullptr;
    }

    const char* chk = skip_whitespace(*cur_ref);
    if (*chk == '(' || *chk == ')' || *chk == ',' || *chk == '\0') {
        LOGGER_ERROR("read_node: expected node value after '('");
        *err |= ERROR_READ_FILE;
        return nullptr;
    }

    node_type_t type  = {};
    value_t     value = {};

    const char* vbegin = nullptr;
    const char* vend   = nullptr;

    if (!read_value_span(tree_ptr, &type, &value, cur_ref, &vbegin, &vend, err)) {
        return nullptr;
    }

    tree_node_t* node = alloc_attach_node(tree_ptr, parent_ptr, type, value, err);
    if (node == nullptr) return nullptr;

    debug_after_value(tree_ptr, vbegin, vend, buff_str);

    if (!read_children(tree_ptr, node, err, cur_ref, buff_str)) {
        cleanup_failed_node(tree_ptr, node, parent_ptr);
        return nullptr;
    }

    if (!expect_char(cur_ref, ')', err, "read_node: expected ')' after children")) {
        cleanup_failed_node(tree_ptr, node, parent_ptr);
        return nullptr;
    }

    return node;
}

static tree_node_t* read_node_impl(tree_t* tree_ptr, tree_node_t* parent_ptr,
                                   error_code* err, const char** cur_ref,
                                   c_string_t buff_str) {
    HARD_ASSERT(tree_ptr != nullptr, "tree_ptr nullptr");
    HARD_ASSERT(err      != nullptr, "err nullptr");
    HARD_ASSERT(cur_ref  != nullptr, "cur_ref nullptr");

    const char* cur = skip_whitespace(*cur_ref);

    if (*cur != '(') {
        LOGGER_ERROR("read_node: expected '('");
        *err |= ERROR_READ_FILE;
        return nullptr;
    }

    *cur_ref = cur;
    return read_paren_node(tree_ptr, parent_ptr, err, cur_ref, buff_str);
}

// проектная сигнатура
static tree_node_t* read_node(tree_t* tree_ptr, tree_node_t* parent_node_ptr,
                              error_code* error, const char** current_ptr_ref,
                              c_string_t buff_str) {
    return read_node_impl(tree_ptr, parent_node_ptr, error, current_ptr_ref, buff_str);
}

//================================================================================

error_code tree_parse_from_buffer(tree_t* tree) {
    HARD_ASSERT(tree           != nullptr, "tree nullptr");
    HARD_ASSERT(tree->buff.ptr != nullptr, "buffer nullptr");

    LOGGER_DEBUG("tree_parse_from_buffer: started");

    const char* cur = skip_whitespace(tree->buff.ptr);

    if (*cur == '\0') {
        tree->root = nullptr;
        tree->size = 0;
        return ERROR_NO;
    }

    error_code parse_error = 0;

    tree_node_t* root = read_node(tree, nullptr, &parse_error, &cur, tree->buff);
    if (parse_error) {
        LOGGER_ERROR("tree_parse_from_buffer: parse failed");
        return ERROR_INVALID_STRUCTURE;
    }

    tree->root = root;
    tree->size = (root == nullptr) ? 0 : count_nodes_recursive(root);

    ON_TREE_DEBUG(fflush(tree->dump_file);)

    return ERROR_NO;
}

error_code tree_read_from_file(tree_t* tree, const char* filename, string_t* buffer_out) {
    HARD_ASSERT(tree       != nullptr, "tree nullptr");
    HARD_ASSERT(filename   != nullptr, "filename nullptr");
    HARD_ASSERT(buffer_out != nullptr, "buffer_out nullptr");

    FILE* file = fopen(filename, "r");
    if (file == nullptr) {
        LOGGER_ERROR("error opening file");
        errno = 0;
        return ERROR_OPEN_FILE;
    }

    string_t buff_str = {};
    error_code err = read_file_to_buffer(file, &buff_str);

    fclose(file);

    if (err != ERROR_NO) {
        return err;
    }

    tree->buff.ptr = buff_str.ptr;
    tree->buff.len = buff_str.len;

    err = tree_parse_from_buffer(tree);
    if (err != ERROR_NO) {
        free(buff_str.ptr);
        return err;
    }

    *buffer_out = buff_str;
    return ERROR_NO;
}

//================================================================================


const char* get_func_name_by_type(op_code_t func_type_value) {
    for (size_t i = 0; i < KEYWORDS_COUNT; ++i) {
        if (KEYWORDS[i].op_code == func_type_value) {
            return KEYWORDS[i].tree_name;
        }
    }

    LOGGER_ERROR("write_node: unknown func_type %d", (int)func_type_value);
    return "";
}

static error_code write_ch(FILE* file, char ch) {
    HARD_ASSERT(file != nullptr, "file nullptr");
    return (fputc(ch, file) == EOF) ? ERROR_OPEN_FILE : ERROR_NO;
}

static error_code write_str(FILE* file, const char* str) {
    HARD_ASSERT(file != nullptr, "file nullptr");
    HARD_ASSERT(str != nullptr, "str nullptr");
    return (fputs(str, file) == EOF) ? ERROR_OPEN_FILE : ERROR_NO;
}

static error_code write_nil(FILE* file) {
    error_code error = write_ch(file, '(');
    TREE_RETURN_IF_ERROR(error);
    return write_ch(file, ')');
}

static error_code write_ident(const tree_t* tree, FILE* file, const tree_node_t* node) {
    HARD_ASSERT(tree != nullptr, "tree nullptr");
    HARD_ASSERT(file != nullptr, "file nullptr");
    HARD_ASSERT(node != nullptr, "node nullptr");

    c_string_t str = tree->ident_stack->data[node->value.ident_idx];
    if (fprintf(file, "\"%.*str\"", (int)str.len, str.ptr) < 0) return ERROR_OPEN_FILE;

    return ERROR_NO;
}

static error_code write_const(FILE* file, const tree_node_t* node) {
    HARD_ASSERT(file != nullptr, "file nullptr");
    HARD_ASSERT(node != nullptr, "node nullptr");

    if (fprintf(file, "%.17g", (double)node->value.constant) < 0) return ERROR_OPEN_FILE;
    return ERROR_NO;
}

static error_code write_func(FILE* file, const tree_node_t* node) {
    HARD_ASSERT(file != nullptr, "file nullptr");
    HARD_ASSERT(node != nullptr, "node nullptr");

    const char* name = get_func_name_by_type(node->value.func);
    if (fprintf(file, "%s", name) < 0) return ERROR_OPEN_FILE;

    return ERROR_NO;
}

static error_code write_value(const tree_t* tree, FILE* file, const tree_node_t* node) {
    HARD_ASSERT(file != nullptr, "file nullptr");
    HARD_ASSERT(node != nullptr, "node nullptr");

    if (node->type == IDENT)    return write_ident(tree, file, node);
    if (node->type == CONSTANT) return write_const(file, node);
    if (node->type == FUNCTION) return write_func(file, node);

    LOGGER_ERROR("write_node: unknown node type %d", (int)node->type);
    return ERROR_INVALID_STRUCTURE;
}

static error_code write_node_impl(const tree_t* tree, FILE* file, const tree_node_t* node) {
    HARD_ASSERT(file != nullptr, "file nullptr");

    if (node == nullptr) return write_nil(file);

    error_code error = write_ch(file, '(');
    TREE_RETURN_IF_ERROR(error);

    error = write_value(tree, file, node);
    TREE_RETURN_IF_ERROR(error);

    error = write_ch(file, ' ');
    TREE_RETURN_IF_ERROR(error);

    error = write_node_impl(tree, file, node->left);
    TREE_RETURN_IF_ERROR(error);

    if (node->left != nullptr && node->right != nullptr) {
        error = write_str(file, ", ");
        TREE_RETURN_IF_ERROR(error);
    } else {
        error = write_str(file, " ");
        TREE_RETURN_IF_ERROR(error);
    }  

    error = write_node_impl(tree, file, node->right);
    TREE_RETURN_IF_ERROR(error);

    return write_ch(file, ')');
}

static error_code write_node(const tree_t* tree_ptr, FILE* file_ptr, tree_node_t* node_ptr) {
    return write_node_impl(tree_ptr, file_ptr, node_ptr);
}

//================================================================================

error_code tree_write_to_file(const tree_t* tree, const char* filename) {
    HARD_ASSERT(tree     != nullptr, "tree nullptr");
    HARD_ASSERT(filename != nullptr, "filename nullptr");

    FILE* file = fopen(filename, "w");
    if (file == nullptr) {
        LOGGER_ERROR("tree_write_to_file: failed to open file '%s'", filename);
        errno = 0;
        return ERROR_OPEN_FILE;
    }

    error_code err = ERROR_NO;

    if (tree->root != nullptr) {
        err = write_node(tree, file, tree->root);
    } else {
        err = write_node(tree, file, nullptr);
    }

    if (fclose(file) != 0) {
        LOGGER_ERROR("tree_write_to_file: failed to close file");
        if (err == ERROR_NO) err = ERROR_OPEN_FILE;
    }

    return err;
}
