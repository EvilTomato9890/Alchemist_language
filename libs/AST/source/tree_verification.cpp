#include "tree_verification.h"
#include "logger.h"
#include "asserts.h"
#include "tree_operations.h"
#include "tree_file_io.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef VERIFY_DEBUG

error_code tree_verify(const tree_t* tree,
                       tree_ver_info_t ver_info,
                       tree_dump_mode_t mode,
                       const char* fmt, ...) {
    (void)tree; (void)ver_info; (void)mode; (void)fmt;
    return ERROR_NO;
}

error_code tree_dump(const tree_t* tree,
                     tree_ver_info_t ver_info,
                     bool is_visual,
                     const char* fmt, ...) {
    (void)tree; (void)ver_info; (void)is_visual; (void)fmt;
    return ERROR_NO;
}

#else

//================================================================================

#define BUFFER_SIZE_TIME  64
#define BUFFER_SIZE_PATH  256
#define BUFFER_SIZE_CMD   512
#define MAX_NODES_COLLECT 4096

//================================================================================

#define LIGHT1_BLUE   "#6b8fd6"
#define LIGHT2_BLUE   "#e9f2ff"
#define LIGHT1_RED    "#ffecec"
#define LIGHT3_RED    "#d66d66"
#define LIGHT1_GRAY   "#999999"
#define DARK_BLACK    "#0b0e14"
#define LIGHT2_BROWN  "#e6e1cf"
#define LIGHT_CYAN    "#8be9fd"
#define RED           "#ff4d4f"
#define CYAN          "#5aa99b"
#define PURPLE        "#8e6bd6"
#define LIGHT_CIAN    "#e9fbf5"
#define LIGHT_PURPLE  "#f1eafd"
#define LIGHT1_GREEN  "#c9fbdbff"
#define LIGHT2_GREEN  "#e8f9e5ff"
#define LIGHT2_GRAY   "#ffffffe0"

#define HTML_BACKGROUND DARK_BLACK
#define HTML_TEXT       LIGHT2_BROWN
#define HTML_TIME       LIGHT_CYAN
#define HTML_REASON     RED
#define HTML_BORDER     LIGHT1_BLUE
#define NODE_COLOR      PURPLE
#define NODE_FILL       LIGHT_PURPLE
#define ROOT_COLOR      CYAN
#define ROOT_FILL       LIGHT_CIAN
#define EDGE_COLOR      LIGHT1_GRAY
#define HEAD_COLOR      LIGHT1_BLUE
#define HEAD_FILL       LIGHT2_BLUE
#define LEAF_COLOR      LIGHT1_GREEN
#define LEAF_FILL       LIGHT2_GREEN

//================================================================================

static const int MAX_STRLEN_VALUE = 128;

enum graph_node_type_t {
    NODE_BASIC,
    NODE_ROOT,
    NODE_HEAD,
    NODE_LEAF
};

static void vfmt(char* buf, size_t cap, const char* fmt, va_list ap) {
    if (!buf || cap == 0) return;
    if (!fmt) { buf[0] = '\0'; return; }
    vsnprintf(buf, cap, fmt, ap);
}

static int run_dot_to_svg(const char *dot_path, const char *svg_path) {
    char cmd[BUFFER_SIZE_CMD] = {};
    snprintf(cmd, sizeof(cmd), "dot -Tsvg \"%s\" -o \"%s\"", dot_path, svg_path);
    return system(cmd) == 0;
}

static error_code collect_recursive(const tree_node_t* node, const tree_node_t** arr, size_t* n) {
    if (node == nullptr) return ERROR_NO;
    
    arr[(*n)++] = node;
    error_code error = ERROR_NO;
    error |= collect_recursive(node->left, arr, n);
    error |= collect_recursive(node->right, arr, n);
    return error;
}

static error_code collect_nodes_dynamic(const tree_node_t* root, const tree_node_t*** nodes_out, size_t* count_out) {
    if (root == nullptr) {
        *nodes_out = nullptr;
        *count_out = 0;
        return ERROR_NO;
    }
    
    size_t count = count_nodes_recursive(root);
    if (count > MAX_NODES_COLLECT) {
        LOGGER_ERROR("collect_nodes_dynamic: too many nodes (%zu > %d)", count, MAX_NODES_COLLECT);
        return ERROR_BIG_SIZE;
    }
    
    const tree_node_t** nodes = (const tree_node_t**)calloc(count, sizeof(const tree_node_t*));
    if (nodes == nullptr) {
        LOGGER_ERROR("collect_nodes_dynamic: failed to allocate memory");
        return ERROR_MEM_ALLOC;
    }
    
    size_t n = 0;
    error_code error = collect_recursive(root, nodes, &n);
    
    *nodes_out = nodes;
    *count_out = n;
    return error;
}

static const char* node_val_to_str(const tree_t* tree, const tree_node_t* node,
                                   char* buf, size_t buf_size)
{
    HARD_ASSERT(node != nullptr, "node_val_to_str: node is nullptr");
    HARD_ASSERT(buf  != nullptr, "node_val_to_str: buff is nullptr");

    if (node == nullptr) return "<null>";

    switch (node->type) {
        case CONSTANT:
            snprintf(buf, buf_size, "%lf", node->value.constant);
            return buf;
        case VARIABLE: {
            c_string_t curr_string = tree->ident_stack->data[node->value.var_idx].str;
            if(buf_size < curr_string.len) LOGGER_WARNING("string len is bigger than buf_size. Printed %ld symbols, srt_len: %ld", buf_size, curr_string.len);
            
            
            snprintf(buf, buf_size, "%.*s", 
                     (int)curr_string.len, curr_string.ptr);
            return buf;
            }
        case FUNCTION:
            return get_func_name_by_type(node->value.func);
        default:
            return "???";
    }
}

static void print_node_label(const tree_t* tree, const tree_node_t* node, FILE* file, 
                             graph_node_type_t graph_node_type) {
    HARD_ASSERT(file != nullptr, "file pointer is nullptr");
    HARD_ASSERT(node != nullptr, "node pointer is nullptr");


    const tree_node_t* self  = node;
    const void* left  = (node ? node->left  : nullptr);
    const void* right = (node ? node->right : nullptr);

    #define NODE_PRINT(node_color, node_fill)                                           \
        do {                                                                            \
            char val_buf[MAX_STRLEN_VALUE];                                                           \
            const char* val_str = node_val_to_str(tree, self, val_buf, sizeof val_buf);       \
            fprintf(file,                                                               \
                    "  node_%p[shape=record,"                                           \
                    "label=\"{ {ptr: %p} | {val: %s | len: %zu} | { prev: %p | next: %p } }\","    \
                    "color=\"" node_color "\",fillcolor=\"" node_fill "\"];\n",         \
                    self, self, val_str, self->subtree_tex_len,                                              \
                    left, right);                                                       \
        } while (0)

    switch(graph_node_type) {
        case NODE_BASIC:
            NODE_PRINT(NODE_COLOR, NODE_FILL);
            return;
        case NODE_HEAD:
            NODE_PRINT(HEAD_COLOR, HEAD_FILL);
            return;
        case NODE_ROOT:
            NODE_PRINT(ROOT_COLOR, ROOT_FILL);
            return;
        case NODE_LEAF:
            NODE_PRINT(LEAF_COLOR, LEAF_FILL);
            return;
        default:
            LOGGER_ERROR("print_node_labes: Unknown node_type");
            return;
    }

    #undef NODE_PRINT
}



static int dump_make_graphviz_svg(const tree_t* tree, const char* base) {
    if (!tree || !tree->root) return 0;
    char dot_path[BUFFER_SIZE_PATH] = {};
    char svg_path[BUFFER_SIZE_PATH] = {};
    snprintf(dot_path, sizeof(dot_path), "%s.dot", base);
    snprintf(svg_path, sizeof(svg_path), "%s.svg", base);

    FILE* file = fopen(dot_path, "w");
    if (!file) return 0;

    fprintf(file, "digraph T{\n");
    fprintf(file, "  node [fontname=\"Fira Mono\",shape=record,style=\"filled,rounded\",fontsize=12];\n");
    fprintf(file, "  graph [splines=true, nodesep=0.6, ranksep=0.6];\n");

    const tree_node_t** nodes = nullptr;
    size_t n = 0;
    error_code error = collect_nodes_dynamic(tree->root, &nodes, &n);
    if (error != ERROR_NO || nodes == nullptr) {
        fclose(file);
        return 0;
    }

    size_t i = 0;
    for (i = 0; i < n; ++i) {
        const tree_node_t* node = nodes[i];
        if(node == tree->root)               print_node_label(tree, node, file, NODE_ROOT);
        else if(!node->left && !node->right) print_node_label(tree, node, file, NODE_LEAF);
        else                                 print_node_label(tree, node, file, NODE_BASIC);
    }
    for (i = 0; i < n; ++i) {
        const tree_node_t* node = nodes[i];
        if (node->left)  fprintf(file, "  node_%p -> node_%p [color=\"%s\"];\n", node, node->left,  EDGE_COLOR);
        if (node->right) fprintf(file, "  node_%p -> node_%p [color=\"%s\"];\n", node, node->right, EDGE_COLOR);
    }
    fprintf(file, "}\n");
    fclose(file);
    
    int result = run_dot_to_svg(dot_path, svg_path);
    free(nodes);
    return result;
}

error_code tree_verify(const tree_t* tree,
                       tree_ver_info_t ver_info,
                       tree_dump_mode_t mode,
                       const char* fmt, ...) {
    (void)fmt;
    error_code error = ERROR_NO;
    if (!tree) return ERROR_NULL_ARG;
    if (tree->root && tree->size == 0) error |= ERROR_INVALID_STRUCTURE;
    if (error != ERROR_NO && mode != TREE_DUMP_NO) {
        tree_dump(tree, ver_info, true, "verify fail");
    }
    return error;
}
static const char* node_type_to_string(node_type_t type) {
    switch (type) {
        case FUNCTION: return "FUNCTION";
        case CONSTANT: return "CONSTANT";
        case VARIABLE: return "VARIABLE";
        default: return "UNKNOWN";
    }
}
static error_code write_html(const tree_t* tree,
                             tree_ver_info_t ver_info_called,
                             int idx, const char* comment,
                             const char* svg_path, int is_visual) {
    if (!tree || !tree->dump_file || !*tree->dump_file) {
        LOGGER_ERROR("write_html: null args");
        return ERROR_NULL_ARG;
    }
    
    FILE* html = *tree->dump_file;
    
    time_t t = time(nullptr);
    char ts[BUFFER_SIZE_TIME] = {};
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", localtime(&t));

    fprintf(html,
        "<pre style=\"font-family:'Fira Mono', monospace;"
        "font-size:16px; line-height:1.28;"
        "background:%s; color:%s;"
        "padding:12px; border-radius:10px;\">\n",
        HTML_BACKGROUND, HTML_TEXT);
    fprintf(html,
        "<span style=\"color:%s;font-weight:700;\">====================[ TREE DUMP #%d ]====================</span>\n",
        HTML_BORDER, idx);
    fprintf(html, "Timestamp: <span style=\"color:%s;\">%s</span>\n", HTML_TIME, ts);
    if (comment && comment[0]) fprintf(html, "<span style=\"color:%s;font-weight:700;\">%s</span>\n", HTML_REASON, comment);
    fprintf(html,
        "<span style=\"color:%s;font-weight:700;\">===================================================</span>\n",
        HTML_BORDER);
    fprintf(html, "tree ptr  : %p\n", tree);
    fprintf(html, "root ptr  : %p\n", tree->root);
    fprintf(html, "size      : %zu\n", tree->size);
    fprintf(html, "stack ptr : %p\n", tree->ident_stack);
    fprintf(html, "buff.ptr  : %p\n", tree->buff.ptr);
    fprintf(html, "buff.len  : %zu\n", tree->buff.len);
    fprintf(html, "list idx  : %zu\n", tree->list_idx);
    

    fprintf(html, "\n-- Created at (tree ver_info) --\n");
    fprintf(html, "file: %s\n", tree->ver_info.file);
    fprintf(html, "func: %s\n", tree->ver_info.func);
    fprintf(html, "line: %d\n", tree->ver_info.line);

    fprintf(html, "\n-- Called at (passed ver_info) --\n");
    fprintf(html, "file: %s\n", ver_info_called.file);
    fprintf(html, "func: %s\n", ver_info_called.func);
    fprintf(html, "line: %d\n", ver_info_called.line);

    fflush(html);

    fprintf(html, "\nIDX   NODE PTR          TYPE          LEFT PTR        RIGHT PTR           VALUE\n");
    fprintf(html, "----  --------------  --------------  --------------  --------------  --------------------\n");

    const tree_node_t** nodes = nullptr;
    size_t n = 0;
    error_code error = collect_nodes_dynamic(tree->root, &nodes, &n);
    if (error != ERROR_NO) {
        fprintf(html, "Error collecting nodes\n");
        return error;
    }
    
    size_t i = 0;
    for (i = 0; i < n; ++i) {
        const tree_node_t* node = nodes[i];
        char  str_buf[MAX_STRLEN_VALUE] = {};
        const char* value = node_val_to_str(tree, node, str_buf, MAX_STRLEN_VALUE);
        const char* type_str = node ? node_type_to_string(node->type) : "NULL";
        fprintf(html, "%-4zu  %-14p  %-14s  %-14p  %-14p  %s\n",
                i, node,
                type_str,
                (node ? node->left   : nullptr),
                (node ? node->right  : nullptr),
                value);
    }

    free(nodes);
    fprintf(html, "\nSVG: %s\n", svg_path ? svg_path : "");
    fprintf(html, "</pre>\n");
    if (svg_path && svg_path[0] && is_visual) {
        fprintf(html, "<img src=\"%s\" style=\"max-width:2250px;\"/>\n", svg_path);
    } else {
        fprintf(html, "<!-- Image not inserted: svg_path='%s', is_visual=%d -->\n", 
                svg_path ? svg_path : "(null)", is_visual);
    }
    fprintf(html, "\n<hr>\n");
    fflush(html);

    return ERROR_NO;
}

error_code tree_dump(const tree_t* tree,
                     tree_ver_info_t ver_info,
                     bool is_visual,
                     const char* fmt, ...) {
    LOGGER_DEBUG("Dump started");
    if (!tree) return ERROR_NULL_ARG;
    static int dump_idx = 0;
    system("mkdir -p dumps");

    char comment[BUFFER_SIZE_CMD] = {};
    va_list ap = {};
    va_start(ap, fmt);
    vfmt(comment, sizeof(comment), fmt, ap);
    va_end(ap);

    char base[BUFFER_SIZE_PATH] = {};
    snprintf(base, sizeof base, "dumps/tree_dump_%03d", dump_idx);
    char svg_path[BUFFER_SIZE_PATH + 5] = {};
    if (is_visual && tree->root) {
        if (dump_make_graphviz_svg(tree, base)) {
            snprintf(svg_path, sizeof svg_path, "%s.svg", base);
            LOGGER_DEBUG("tree_dump: SVG generated successfully: %s", svg_path);
        } else {
            LOGGER_WARNING("tree_dump: failed to generate SVG");
        }
    } else {
        LOGGER_DEBUG("tree_dump: SVG not generated: is_visual=%d, root=%p", is_visual, tree->root);
    }

    error_code error = write_html(tree, ver_info, dump_idx, comment, svg_path, is_visual);
    if(!error) {
        LOGGER_INFO("Dump #%d written%s%s", dump_idx,
                    svg_path[0] ? " with SVG: " : "",
                    svg_path[0] ? svg_path : "");
    } else {
        LOGGER_INFO("Dump #%d failed to be written", dump_idx);
    }
    dump_idx++;
    return error;
}

#endif 