

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "libs/AST/include/tree_operations.h"
#include "libs/AST/include/tree_file_io.h"
#include "common/logger/include/logger.h"

//------------------------------------------------------------------------------
// Мини-фреймворк

static int g_failed = 0;

#define TEST_CASE(name) static void name()

#define CHECK_TRUE(cond) do { \
    if (!(cond)) { \
        ++g_failed; \
        std::fprintf(stderr, "[FAIL] %s:%d: CHECK_TRUE(%s)\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

#define CHECK_EQ_U64(a, b) do { \
    unsigned long long _va = (unsigned long long)(a); \
    unsigned long long _vb = (unsigned long long)(b); \
    if (_va != _vb) { \
        ++g_failed; \
        std::fprintf(stderr, "[FAIL] %s:%d: CHECK_EQ_U64(%s, %s) got %llu vs %llu\n", \
                     __FILE__, __LINE__, #a, #b, _va, _vb); \
    } \
} while (0)

#define CHECK_EQ_INT(a, b) do { \
    int _va = (int)(a); \
    int _vb = (int)(b); \
    if (_va != _vb) { \
        ++g_failed; \
        std::fprintf(stderr, "[FAIL] %s:%d: CHECK_EQ_INT(%s, %s) got %d vs %d\n", \
                     __FILE__, __LINE__, #a, #b, _va, _vb); \
    } \
} while (0)

#define CHECK_NE_PTR(a, b) do { \
    const void* _pa = (const void*)(a); \
    const void* _pb = (const void*)(b); \
    if (_pa == _pb) { \
        ++g_failed; \
        std::fprintf(stderr, "[FAIL] %s:%d: CHECK_NE_PTR(%s, %s) both=%p\n", __FILE__, __LINE__, #a, #b, _pa); \
    } \
} while (0)

#define CHECK_DBL_NEAR(a, b, eps) do { \
    double _da = (double)(a); \
    double _db = (double)(b); \
    double _de = (double)(eps); \
    if (std::fabs(_da - _db) > _de) { \
        ++g_failed; \
        std::fprintf(stderr, "[FAIL] %s:%d: CHECK_DBL_NEAR(%s,%s,%.3g) got %.17g vs %.17g\n", \
                     __FILE__, __LINE__, #a, #b, _de, _da, _db); \
    } \
} while (0)

//------------------------------------------------------------------------------
// Хелперы

static tree_t make_empty_tree() {
    tree_t tree = {};

    // ВАЖНО: tree_destroy() делает var_stack_destroy() и free(tree->var_stack),
    // поэтому стек выделяем на куче.
    var_stack_t* vs = (var_stack_t*)std::calloc(1, sizeof(var_stack_t));
    CHECK_TRUE(vs != nullptr);
    var_stack_init(vs, 10, STACK_VER_INIT);
    error_code err = tree_init(&tree, vs ON_TREE_DEBUG(, TREE_VER_INIT));
    CHECK_EQ_INT(err, ERROR_NO);

    return tree;
}

static void destroy_tree(tree_t* tree) {
    if (!tree) return;
    (void)tree_destroy(tree);
}

static tree_node_t* mk_const(double x) {
    return init_node(CONSTANT, make_union_const(x), nullptr, nullptr);
}

static tree_node_t* mk_func(func_type_t f, tree_node_t* l, tree_node_t* r) {
    return init_node(FUNCTION, make_union_func(f), l, r);
}

// Структурное сравнение (для VARIABLE сравниваем индекс; если хочешь — поменяй на имя)
static bool nodes_equal(const tree_node_t* a, const tree_node_t* b) {
    if (a == nullptr || b == nullptr) return a == b;
    if (a->type != b->type) return false;

    switch (a->type) {
        case CONSTANT:
            if (a->value.constant != b->value.constant) return false;
            break;
        case FUNCTION:
            if (a->value.func != b->value.func) return false;
            break;
        case VARIABLE:
            if (a->value.var_idx != b->value.var_idx) return false;
            break;
        default:
            return false;
    }

    return nodes_equal(a->left, b->left) && nodes_equal(a->right, b->right);
}

static bool nodes_equal_by_var_name(const tree_t* ta, const tree_node_t* a,
                                   const tree_t* tb, const tree_node_t* b) {
    if (a == nullptr || b == nullptr) return a == b;
    if (a->type != b->type) return false;

    switch (a->type) {
        case CONSTANT:
            if (a->value.constant != b->value.constant) return false;
            break;
        case FUNCTION:
            if (a->value.func != b->value.func) return false;
            break;
        case VARIABLE: {
            c_string_t na = get_var_name(ta, a);
            c_string_t nb = get_var_name(tb, b);
            if (na.len != nb.len) return false;
            if (na.len != 0 && std::memcmp(na.ptr, nb.ptr, na.len) != 0) return false;
            break;
        }
        default:
            return false;
    }

    return nodes_equal_by_var_name(ta, a->left, tb, b->left) &&
           nodes_equal_by_var_name(ta, a->right, tb, b->right);
}

//------------------------------------------------------------------------------
// Тесты

TEST_CASE(test_init_empty) {
    tree_t t = make_empty_tree();

    CHECK_TRUE(tree_is_empty(&t));
    CHECK_TRUE(t.root == nullptr);
    CHECK_EQ_U64(t.size, 0);

    destroy_tree(&t);
}

TEST_CASE(test_insert_and_count) {
    tree_t t = make_empty_tree();

    tree_node_t* root = tree_init_root(&t, CONSTANT, make_union_const(1.0));
    CHECK_TRUE(root != nullptr);

    tree_node_t* l = tree_insert_left (&t, CONSTANT, make_union_const(2.0), root);
    tree_node_t* r = tree_insert_right(&t, CONSTANT, make_union_const(3.0), root);

    CHECK_TRUE(l != nullptr);
    CHECK_TRUE(r != nullptr);
    CHECK_EQ_U64(t.size, 3);
    CHECK_EQ_U64(count_nodes_recursive(t.root), 3);
    CHECK_TRUE(!tree_is_empty(&t));

    CHECK_DBL_NEAR(t.root->value.constant, 1.0, 1e-12);
    CHECK_DBL_NEAR(t.root->left->value.constant, 2.0, 1e-12);
    CHECK_DBL_NEAR(t.root->right->value.constant, 3.0, 1e-12);

    destroy_tree(&t);
}

TEST_CASE(test_replace_value) {
    tree_t t = make_empty_tree();

    tree_node_t* root = tree_init_root(&t, CONSTANT, make_union_const(10.0));
    CHECK_TRUE(root != nullptr);

    error_code err = tree_replace_value(root, CONSTANT, make_union_const(123.0));
    CHECK_EQ_INT(err, ERROR_NO);
    CHECK_DBL_NEAR(root->value.constant, 123.0, 1e-12);

    destroy_tree(&t);
}

TEST_CASE(test_subtree_deep_copy) {
    tree_t t = make_empty_tree();

    // (ADD 7 9)
    tree_node_t* root = mk_func(ADD, mk_const(7.0), mk_const(9.0));
    CHECK_TRUE(root != nullptr);

    (void)tree_change_root(&t, root);
    CHECK_EQ_U64(t.size, 3);

    error_code err = ERROR_NO;
    tree_node_t* copy = subtree_deep_copy(t.root, &err ON_TREE_DUMP_CREATION_DEBUG(, &t));
    CHECK_EQ_INT(err, ERROR_NO);
    CHECK_TRUE(copy != nullptr);

    CHECK_TRUE(nodes_equal(t.root, copy));
    CHECK_NE_PTR(t.root, copy);
    CHECK_NE_PTR(t.root->left, copy->left);
    CHECK_NE_PTR(t.root->right, copy->right);

    size_t removed = 0;
    (void)destroy_node_recursive(copy, &removed);
    CHECK_EQ_U64(removed, 3);

    destroy_tree(&t);
}

TEST_CASE(test_parse_empty_and_nil) {
    tree_t t = make_empty_tree();

    // Пустой буфер => пустое дерево
    {
        const char* s = "   \n\t  ";
        t.buff = {(char*)s, (unsigned long)std::strlen(s)};
        error_code err = tree_parse_from_buffer(&t);
        CHECK_EQ_INT(err, ERROR_NO);
        CHECK_TRUE(t.root == nullptr);
        CHECK_EQ_U64(t.size, 0);
    }

    // "nil" => пустое дерево
    {
        const char* s = "nil";
        t.buff = {(char*)s, (unsigned long)std::strlen(s)};
        error_code err = tree_parse_from_buffer(&t);
        CHECK_EQ_INT(err, ERROR_NO);
        CHECK_TRUE(t.root == nullptr);
        CHECK_EQ_U64(t.size, 0);
    }

    destroy_tree(&t);
}

TEST_CASE(test_parse_invalid) {
    tree_t t = make_empty_tree();

    // missing closing ')'
    const char* s = "( 1 nil nil";
    t.buff = {(char*)s, (unsigned long)std::strlen(s)};

    error_code err = tree_parse_from_buffer(&t);
    CHECK_TRUE(err != ERROR_NO);

    // После ошибки дерево должно быть в безопасном состоянии
    CHECK_TRUE(t.root == nullptr || count_nodes_recursive(t.root) == t.size);

    destroy_tree(&t);
}

static void write_text_file(const char* filename, const char* text) {
    std::FILE* f = std::fopen(filename, "wb");
    CHECK_TRUE(f != nullptr);
    if (!f) return;
    std::fwrite(text, 1, std::strlen(text), f);
    std::fclose(f);
}

TEST_CASE(test_write_read_roundtrip_function) {
    const char* fname = "tree_test_tmp.ast";

    // Сборка дерева руками и запись
    {
        tree_t t = make_empty_tree();

        // (ADD 10 20)
        tree_node_t* root = mk_func(ADD, mk_const(10.0), mk_const(20.0));
        (void)tree_change_root(&t, root);

        error_code err = tree_write_to_file(&t, fname);
        CHECK_EQ_INT(err, ERROR_NO);

        destroy_tree(&t);
    }

    // Чтение и проверка структуры
    {
        tree_t t = make_empty_tree();
        string_t buff = {};
        error_code err = tree_read_from_file(&t, fname, &buff);
        CHECK_EQ_INT(err, ERROR_NO);

        CHECK_TRUE(t.root != nullptr);
        CHECK_EQ_INT(t.root->type, FUNCTION);
        CHECK_EQ_INT((int)t.root->value.func, (int)ADD);

        CHECK_TRUE(t.root->left && t.root->right);
        CHECK_EQ_INT(t.root->left->type, CONSTANT);
        CHECK_EQ_INT(t.root->right->type, CONSTANT);
        CHECK_DBL_NEAR(t.root->left->value.constant, 10.0, 1e-12);
        CHECK_DBL_NEAR(t.root->right->value.constant, 20.0, 1e-12);

        free(buff.ptr);
        destroy_tree(&t);
    }

    std::remove(fname);
}

TEST_CASE(test_read_write_with_variable) {
    const char* fname = "tree_test_tmp_var.ast";

    // Пытаемся прогнать VARIABLE-ветку (если var_stack_push работает без явного init — ок).
    {
        tree_t t = make_empty_tree();

        error_code err = ERROR_NO;
        c_string_t x = {(char*)"x", 1};
        size_t idx = get_or_add_var_idx(x, 0.0, t.var_stack, &err);
        CHECK_EQ_INT(err, ERROR_NO);

        tree_node_t* xnode = init_node(VARIABLE, make_union_var(idx), nullptr, nullptr);
        tree_node_t* cnode = mk_const(5.0);
        tree_node_t* root  = mk_func(ADD, xnode, cnode);

        (void)tree_change_root(&t, root);

        err = tree_write_to_file(&t, fname);
        CHECK_EQ_INT(err, ERROR_NO);

        destroy_tree(&t);
    }

    {
        tree_t t = make_empty_tree();
        string_t buff = {};
        error_code err = tree_read_from_file(&t, fname, &buff);
        CHECK_EQ_INT(err, ERROR_NO);

        CHECK_TRUE(t.root != nullptr);
        CHECK_EQ_INT(t.root->type, FUNCTION);

        CHECK_TRUE(t.root->left != nullptr);
        CHECK_EQ_INT(t.root->left->type, VARIABLE);

        c_string_t name = get_var_name(&t, t.root->left);
        CHECK_EQ_U64(name.len, 1);
        if (name.ptr && name.len == 1) {
            CHECK_TRUE(name.ptr[0] == 'x');
        }
        free(buff.ptr);
        destroy_tree(&t);
    }

    std::remove(fname);
}

//------------------------------------------------------------------------------

int main() {
    logger_initialize_stream(nullptr);
    LOGGER_DEBUG("Start");

    test_init_empty();
    test_insert_and_count();
    test_replace_value();
    test_subtree_deep_copy();
    test_parse_empty_and_nil();
    test_parse_invalid();
    test_write_read_roundtrip_function();
    test_read_write_with_variable();

    if (g_failed == 0) {
        std::printf("OK\n");
        return 0;
    }

    std::fprintf(stderr, "FAILED: %d\n", g_failed);
    return 1;
}
