// backend.cpp
#include "backend.h"

#include "common/asserts/include/asserts.h"
#include "common/logger/include/logger.h"

#include "libs/AST/include/tree_operations.h"
#include "libs/Vector/include/vector.h"
#include "libs/Vector/include/error_handler.h"
#include "libs/My_string/include/my_string.h"
#include "libs/Unordered_map/include/unordered_map.h"
#include "libs/Unordered_map/include/error_handler.h"

#include <stdint.h>
#include <string.h>

//================================================================================

static size_t hash_size_t_key(const void* key_ptr) {
    HARD_ASSERT(key_ptr != nullptr, "key_ptr is nullptr");
    return *(const size_t*)key_ptr;
}

static bool cmp_size_t_key(const void* left_ptr, const void* right_ptr) {
    HARD_ASSERT(left_ptr  != nullptr, "left_ptr is nullptr");
    HARD_ASSERT(right_ptr != nullptr, "right_ptr is nullptr");
    return *(const size_t*)left_ptr == *(const size_t*)right_ptr;
}

//================================================================================
//                               Регистры
//================================================================================

enum backend_reg_t {
    REG_RAX = 0, // return
    REG_RBX = 1, // mem_free
    REG_RCX = 2, // addr
    REG_RDX = 3, // base
};

static const char* reg_full(backend_reg_t reg_id) {
    switch (reg_id) {
        case REG_RAX: return "RAX";
        case REG_RBX: return "RBX";
        case REG_RCX: return "RCX";
        case REG_RDX: return "RDX";
        default:      return "R??";
    }
}

static const char* reg_mem(backend_reg_t reg_id) {
    switch (reg_id) {
        case REG_RAX: return "AX";
        case REG_RBX: return "BX";
        case REG_RCX: return "CX";
        case REG_RDX: return "DX";
        default:      return "??";
    }
}

//================================================================================

struct backend_scope_t {
    u_map_t var_map;      
    size_t  saved_offset;  
};

static hm_error_t scope_init(backend_scope_t* scop_ptr) {
    HARD_ASSERT(scop_ptr != nullptr, "scop_ptr is nullptr");
    scop_ptr->saved_offset = 0;

    hm_error_t erro_code = SIMPLE_U_MAP_INIT(&scop_ptr->var_map, 32,
                                             size_t, size_t,
                                             hash_size_t_key, cmp_size_t_key);
    return erro_code;
}

static void scope_destroy(backend_scope_t* scop_ptr) {
    if (scop_ptr == nullptr) return;
    (void)u_map_destroy(&scop_ptr->var_map);
}

static backend_scope_t* scope_top(vector_t* scop_stack) {
    HARD_ASSERT(scop_stack != nullptr, "scop_stack is nullptr");
    if (vector_size(scop_stack) == 0) return nullptr;
    return (backend_scope_t*)vector_get(scop_stack, vector_size(scop_stack) - 1);
}

static const backend_scope_t* scope_top_const(const vector_t* scop_stack) {
    HARD_ASSERT(scop_stack != nullptr, "scop_stack is nullptr");
    if (vector_size(scop_stack) == 0) return nullptr;
    return (const backend_scope_t*)vector_get_const(scop_stack, vector_size(scop_stack) - 1);
}

//================================================================================

struct backend_ctx_t {
    const tree_t* tree_ptr;
    FILE*         file_ptr;
    u_map_t*      func_table;

    vector_t      scop_stack; 
    size_t        label_next;

    size_t        offc_curr;   
    size_t        scop_depth;  
};

//================================================================================

static void emit_line(backend_ctx_t* ctx_ptr, const char* text_ptr) {
    HARD_ASSERT(ctx_ptr  != nullptr, "ctx_ptr is nullptr");
    HARD_ASSERT(text_ptr != nullptr, "text_ptr is nullptr");
    fputs(text_ptr, ctx_ptr->file_ptr);
}

static void emit_fmt1(backend_ctx_t* ctx_ptr, const char* fmt_ptr, const char* arg_ptr) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    HARD_ASSERT(fmt_ptr != nullptr, "fmt_ptr is nullptr");
    HARD_ASSERT(arg_ptr != nullptr, "arg_ptr is nullptr");
    fprintf(ctx_ptr->file_ptr, fmt_ptr, arg_ptr);
}

static void emit_push_double(backend_ctx_t* ctx_ptr, double valu_data) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    fprintf(ctx_ptr->file_ptr, "PUSH %.17g\n", valu_data);
}

static void emit_push_size(backend_ctx_t* ctx_ptr, size_t valu_data) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    fprintf(ctx_ptr->file_ptr, "PUSH %zu\n", valu_data);
}

static void emit_pushr(backend_ctx_t* ctx_ptr, backend_reg_t reg_id) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    fprintf(ctx_ptr->file_ptr, "PUSHR %s\n", reg_full(reg_id));
}

static void emit_popr(backend_ctx_t* ctx_ptr, backend_reg_t reg_id) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    fprintf(ctx_ptr->file_ptr, "POPR %s\n", reg_full(reg_id));
}

static void emit_pushm(backend_ctx_t* ctx_ptr, backend_reg_t reg_id) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    fprintf(ctx_ptr->file_ptr, "PUSHM [%s]\n", reg_mem(reg_id));
}

static void emit_popm(backend_ctx_t* ctx_ptr, backend_reg_t reg_id) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    fprintf(ctx_ptr->file_ptr, "POPM [%s]\n", reg_mem(reg_id));
}

static size_t new_label_id(backend_ctx_t* ctx_ptr) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    return ctx_ptr->label_next++;
}

static void emit_label_id(backend_ctx_t* ctx_ptr, const char* pref_ptr, size_t labl_id) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    HARD_ASSERT(pref_ptr != nullptr, "pref_ptr is nullptr");
    fprintf(ctx_ptr->file_ptr, ":%s_%zu\n", pref_ptr, labl_id);
}

static void emit_jump_id(backend_ctx_t* ctx_ptr, const char* inst_ptr,
                         const char* pref_ptr, size_t labl_id) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    HARD_ASSERT(inst_ptr != nullptr, "inst_ptr is nullptr");
    HARD_ASSERT(pref_ptr != nullptr, "pref_ptr is nullptr");
    fprintf(ctx_ptr->file_ptr, "%s :%s_%zu\n", inst_ptr, pref_ptr, labl_id);
}

static void emit_add_reg_imm(backend_ctx_t* ctx_ptr, backend_reg_t reg_id, size_t imm_data) {
    emit_pushr(ctx_ptr, reg_id);
    emit_push_size(ctx_ptr, imm_data);
    emit_line(ctx_ptr, "ADD\n");
    emit_popr(ctx_ptr, reg_id);
}

static void emit_move_reg_reg(backend_ctx_t* ctx_ptr, backend_reg_t dst_id, backend_reg_t src_id) {
    emit_pushr(ctx_ptr, src_id);
    emit_popr(ctx_ptr, dst_id);
}

static void emit_addr_from_base(backend_ctx_t* ctx_ptr, size_t offc_data) {
    emit_pushr(ctx_ptr, REG_RDX);
    emit_push_size(ctx_ptr, offc_data);
    emit_line(ctx_ptr, "ADD\n");
    emit_popr(ctx_ptr, REG_RCX);
}

static void emit_unwind_scopes(backend_ctx_t* ctx_ptr, size_t scop_count) {
    for (size_t idx_i = 0; idx_i < scop_count; ++idx_i) {
        emit_popr(ctx_ptr, REG_RBX);
    }
}

//================================================================================

static bool is_func_node(const tree_node_t* node_ptr, op_code_t op_code) {
    return node_ptr != nullptr &&
           node_ptr->type == FUNCTION &&
           node_ptr->value.func == op_code;
}

static c_string_t ident_name_by_idx(const tree_t* tree_ptr, size_t idnt_idx) {
    tree_node_t temp_node = {};
    temp_node.type = IDENT;
    temp_node.value.ident_idx = idnt_idx;
    return get_var_name(tree_ptr, &temp_node);
}

static bool node_is_ident(const tree_node_t* node_ptr) {
    return node_ptr != nullptr && node_ptr->type == IDENT;
}

static size_t require_ident_idx(const tree_node_t* node_ptr) {
    HARD_ASSERT(node_is_ident(node_ptr), "expected IDENT node");
    return node_ptr->value.ident_idx;
}

//================================================================================

static void collect_param_idents(const tree_node_t* node_ptr, vector_t* list_ptr) {
    if (node_ptr == nullptr) return;

    if (is_func_node(node_ptr, OP_ENUM_SEP)) {
        collect_param_idents(node_ptr->left,  list_ptr);
        collect_param_idents(node_ptr->right, list_ptr);
        return;
    }

    if (node_is_ident(node_ptr)) {
        size_t idnt_idx = node_ptr->value.ident_idx;
        (void)vector_push_back(list_ptr, &idnt_idx);
        return;
    }
}

static void collect_call_args(const tree_node_t* node_ptr, vector_t* list_ptr) {
    if (node_ptr == nullptr) return;

    if (is_func_node(node_ptr, OP_ENUM_SEP)) {
        collect_call_args(node_ptr->left,  list_ptr);
        collect_call_args(node_ptr->right, list_ptr);
        return;
    }

    const tree_node_t* expr_ptr = node_ptr;
    (void)vector_push_back(list_ptr, &expr_ptr);
}

//================================================================================

static hm_error_t scopes_reset(backend_ctx_t* ctx_ptr) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");

    while (vector_size(&ctx_ptr->scop_stack) != 0) {
        backend_scope_t scop_last = {};
        (void)vector_pop_back(&ctx_ptr->scop_stack, &scop_last);
        scope_destroy(&scop_last);
    }

    ctx_ptr->offc_curr  = 0;
    ctx_ptr->scop_depth = 0;

    backend_scope_t scop_new = {};
    hm_error_t error_code = scope_init(&scop_new);
    if (error_code != HM_ERR_OK) return error_code;

    vector_error_t vec_error_code = vector_push_back(&ctx_ptr->scop_stack, &scop_new);
    if (vec_error_code != VEC_ERR_OK) return HM_ERR_MEM_ALLOC;

    return HM_ERR_OK;
}

static bool scopes_find_offc(const backend_ctx_t* ctx_ptr, size_t idnt_idx, size_t* offc_out) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    HARD_ASSERT(offc_out != nullptr, "offc_out is nullptr");

    for (size_t idx_i = vector_size(&ctx_ptr->scop_stack); idx_i > 0; --idx_i) {
        const backend_scope_t* scop_ptr = (const backend_scope_t*)
            vector_get_const(&ctx_ptr->scop_stack, idx_i - 1);

        size_t offc_val = 0;
        if (u_map_get_elem(&scop_ptr->var_map, &idnt_idx, &offc_val)) {
            *offc_out = offc_val;
            return true;
        }
    }
    return false;
}

static hm_error_t scopes_insert_local(backend_ctx_t* ctx_ptr, size_t idnt_idx, size_t offc_val) {
    backend_scope_t* scop_ptr = scope_top(&ctx_ptr->scop_stack);
    HARD_ASSERT(scop_ptr != nullptr, "scop_ptr is nullptr");
    return u_map_insert_elem(&scop_ptr->var_map, &idnt_idx, &offc_val);
}

static hm_error_t ensure_var_slot(backend_ctx_t* ctx_ptr, size_t idnt_idx, size_t* offc_out) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    HARD_ASSERT(offc_out != nullptr, "offc_out is nullptr");

    if (scopes_find_offc(ctx_ptr, idnt_idx, offc_out)) return HM_ERR_OK;

    size_t offc_new = ctx_ptr->offc_curr++;
    hm_error_t erro_code = scopes_insert_local(ctx_ptr, idnt_idx, offc_new);
    if (erro_code != HM_ERR_OK) return erro_code;

    emit_add_reg_imm(ctx_ptr, REG_RBX, 1);

    emit_push_size(ctx_ptr, 0);
    emit_addr_from_base(ctx_ptr, offc_new);
    emit_popm(ctx_ptr, REG_RCX);

    *offc_out = offc_new;
    return HM_ERR_OK;
}

static hm_error_t enter_scope(backend_ctx_t* ctx_ptr) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");

    backend_scope_t scop_new = {};
    hm_error_t erro_code = scope_init(&scop_new);
    if (erro_code != HM_ERR_OK) return erro_code;

    scop_new.saved_offset = ctx_ptr->offc_curr;

    vector_error_t vec_error = vector_push_back(&ctx_ptr->scop_stack, &scop_new);
    if (vec_error != VEC_ERR_OK) {
        scope_destroy(&scop_new);
        return HM_ERR_MEM_ALLOC;
    }

    emit_pushr(ctx_ptr, REG_RBX);
    ctx_ptr->scop_depth++;
    return HM_ERR_OK;
}

static void leave_scope(backend_ctx_t* ctx_ptr) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");

    backend_scope_t scop_old = {};
    (void)vector_pop_back(&ctx_ptr->scop_stack, &scop_old);

    ctx_ptr->offc_curr = scop_old.saved_offset;

    emit_popr(ctx_ptr, REG_RBX);
    HARD_ASSERT(ctx_ptr->scop_depth > 0, "scope depth underflow");
    ctx_ptr->scop_depth--;

    scope_destroy(&scop_old);
}

//================================================================================

static inline void* u_map_key_ptr(const u_map_t* map_ptr, size_t indx) {
    return (void*)((unsigned char*)map_ptr->data_keys + indx * map_ptr->key_stride);
}

static inline void* u_map_val_ptr(const u_map_t* map_ptr, size_t indx) {
    return (void*)((unsigned char*)map_ptr->data_values + indx * map_ptr->value_stride);
}

static bool func_table_find_main(const backend_ctx_t* ctx_ptr, size_t* idnt_out) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    HARD_ASSERT(idnt_out != nullptr, "idnt_out is nullptr");

    const u_map_t* tabl_ptr = ctx_ptr->func_table;
    for (size_t idx_i = 0; idx_i < tabl_ptr->capacity; ++idx_i) {
        if (tabl_ptr->data_states[idx_i] != USED) continue;

        size_t idnt_idx = *(size_t*)u_map_key_ptr(tabl_ptr, idx_i);
        c_string_t name_ptr = ident_name_by_idx(ctx_ptr->tree_ptr, idnt_idx);

        if (my_scstrcmp(name_ptr, "main") == 0) {
            *idnt_out = idnt_idx;
            return true;
        }
    }
    return false;
}

static bool func_table_find_first(const u_map_t* tabl_ptr, size_t* idnt_out) {
    for (size_t idx_i = 0; idx_i < tabl_ptr->capacity; ++idx_i) {
        if (tabl_ptr->data_states[idx_i] != USED) continue;
        *idnt_out = *(size_t*)u_map_key_ptr(tabl_ptr, idx_i);
        return true;
    }
    return false;
}

static bool func_table_get_symbol(const backend_ctx_t* ctx_ptr, size_t idnt_idx,
                                 backend_func_symbol_t* symb_out) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    HARD_ASSERT(symb_out != nullptr, "symb_out is nullptr");
    return u_map_get_elem(ctx_ptr->func_table, &idnt_idx, symb_out);
}

//================================================================================
//                                   Pass1
//================================================================================

static const tree_node_t* get_decl_info(const tree_node_t* decl_ptr) {
    return decl_ptr != nullptr ? decl_ptr->left : nullptr;
}

static const tree_node_t* get_info_name(const tree_node_t* info_ptr) {
    return info_ptr != nullptr ? info_ptr->right : nullptr;
}

static const tree_node_t* get_info_args(const tree_node_t* info_ptr) {
    return info_ptr != nullptr ? info_ptr->left : nullptr;
}

static hm_error_t collect_one_decl(backend_ctx_t* ctx_ptr, const tree_node_t* decl_ptr) {
    HARD_ASSERT(ctx_ptr != nullptr, "ctx_ptr is nullptr");
    HARD_ASSERT(decl_ptr != nullptr, "decl_ptr is nullptr");

    bool is_proc = is_func_node(decl_ptr, OP_PROC_DECL);
    const tree_node_t* info_ptr = get_decl_info(decl_ptr);
    HARD_ASSERT(is_func_node(info_ptr, OP_FUNC_INFO), "bad func_info node");

    const tree_node_t* name_ptr = get_info_name(info_ptr);
    HARD_ASSERT(node_is_ident(name_ptr), "func name must be IDENT");

    vector_t parm_list = {};
    (void)SIMPLE_VECTOR_INIT(&parm_list, 8, size_t);
    collect_param_idents(get_info_args(info_ptr), &parm_list);

    size_t idnt_idx = require_ident_idx(name_ptr);

    backend_func_symbol_t symb_val = {};
    symb_val.label_id     = ctx_ptr->label_next++;
    symb_val.param_count  = vector_size(&parm_list);
    symb_val.is_proc      = is_proc;

    vector_destroy(&parm_list);

    hm_error_t erro_code = u_map_insert_elem(ctx_ptr->func_table, &idnt_idx, &symb_val);
    return erro_code;
}

static hm_error_t pass1_collect(backend_ctx_t* ctx_ptr, const tree_node_t* node_ptr) {
    if (node_ptr == nullptr) return HM_ERR_OK;

    if (is_func_node(node_ptr, OP_FUNC_DECL) || is_func_node(node_ptr, OP_PROC_DECL)) {
        hm_error_t erro_code = collect_one_decl(ctx_ptr, node_ptr);
        if (erro_code != HM_ERR_OK) return erro_code;
    }

    hm_error_t left_code = pass1_collect(ctx_ptr, node_ptr->left);
    if (left_code != HM_ERR_OK) return left_code;

    hm_error_t rght_code = pass1_collect(ctx_ptr, node_ptr->right);
    return rght_code;
}

//================================================================================

static hm_error_t emit_stmt(backend_ctx_t* ctx_ptr, const tree_node_t* node_ptr);
static hm_error_t emit_expr(backend_ctx_t* ctx_ptr, const tree_node_t* node_ptr);

//================================================================================

static hm_error_t emit_load_ident(backend_ctx_t* ctx_ptr, size_t idnt_idx) {
    size_t offc_val = 0;
    hm_error_t erro_code = ensure_var_slot(ctx_ptr, idnt_idx, &offc_val);
    if (erro_code != HM_ERR_OK) return erro_code;

    emit_addr_from_base(ctx_ptr, offc_val);
    emit_pushm(ctx_ptr, REG_RCX);
    return HM_ERR_OK;
}

static hm_error_t emit_store_ident(backend_ctx_t* ctx_ptr, size_t idnt_idx) {
    size_t offc_val = 0;
    hm_error_t erro_code = ensure_var_slot(ctx_ptr, idnt_idx, &offc_val);
    if (erro_code != HM_ERR_OK) return erro_code;

    emit_addr_from_base(ctx_ptr, offc_val);
    emit_popm(ctx_ptr, REG_RCX);
    return HM_ERR_OK;
}

//================================================================================

static void emit_call_label(backend_ctx_t* ctx_ptr, size_t idnt_idx) {
    c_string_t name_ptr = ident_name_by_idx(ctx_ptr->tree_ptr, idnt_idx);
    fprintf(ctx_ptr->file_ptr, "CALL :%.*s\n", (int)name_ptr.len, name_ptr.ptr);
}

static void emit_decl_label(backend_ctx_t* ctx_ptr, size_t idnt_idx) {
    c_string_t name_ptr = ident_name_by_idx(ctx_ptr->tree_ptr, idnt_idx);
    fprintf(ctx_ptr->file_ptr, ":%.*s\n", (int)name_ptr.len, name_ptr.ptr);
}

static hm_error_t emit_call_common(backend_ctx_t* ctx_ptr, const tree_node_t* call_ptr, bool need_value) {
    HARD_ASSERT(is_func_node(call_ptr, OP_CALL), "expected OP_CALL");

    const tree_node_t* name_ptr = call_ptr->right;
    HARD_ASSERT(node_is_ident(name_ptr), "call name must be IDENT");
    size_t idnt_idx = require_ident_idx(name_ptr);

    backend_func_symbol_t symb_val = {};
    if (!func_table_get_symbol(ctx_ptr, idnt_idx, &symb_val)) {
        LOGGER_ERROR("call to unknown function/proc (ident_idx=%zu)", idnt_idx);
        return HM_ERR_NOT_FOUND;
    }

    vector_t args_list = {};
    (void)SIMPLE_VECTOR_INIT(&args_list, 8, const tree_node_t*);
    collect_call_args(call_ptr->left, &args_list);

    for (size_t idx_i = 0; idx_i < vector_size(&args_list); ++idx_i) {
        const tree_node_t* expr_ptr = *(const tree_node_t**)vector_get_const(&args_list, idx_i);
        hm_error_t erro_code = emit_expr(ctx_ptr, expr_ptr);
        if (erro_code != HM_ERR_OK) {
            vector_destroy(&args_list);
            return erro_code;
        }
    }

    emit_pushr(ctx_ptr, REG_RBX);
    emit_call_label(ctx_ptr, idnt_idx);
    emit_popr(ctx_ptr, REG_RBX);

    if (need_value) {
        if (symb_val.is_proc) {
            emit_push_size(ctx_ptr, 0);
        } else {
            emit_pushr(ctx_ptr, REG_RAX);
        }
    }

    vector_destroy(&args_list);
    return HM_ERR_OK;
}

//================================================================================

static hm_error_t emit_binary_math(backend_ctx_t* ctx_ptr, const tree_node_t* node_ptr, const char* inst_ptr) {
    hm_error_t erro_code = emit_expr(ctx_ptr, node_ptr->left);
    if (erro_code != HM_ERR_OK) return erro_code;

    erro_code = emit_expr(ctx_ptr, node_ptr->right);
    if (erro_code != HM_ERR_OK) return erro_code;

    emit_line(ctx_ptr, inst_ptr);
    emit_line(ctx_ptr, "\n");
    return HM_ERR_OK;
}

static hm_error_t emit_assign_expr(backend_ctx_t* ctx_ptr, const tree_node_t* node_ptr) {
    HARD_ASSERT(is_func_node(node_ptr, OP_ASSIGN), "expected OP_ASSIGN");
    HARD_ASSERT(node_is_ident(node_ptr->left), "lvalue must be IDENT");

    size_t idnt_idx = require_ident_idx(node_ptr->left);

    hm_error_t erro_code = emit_expr(ctx_ptr, node_ptr->right);
    if (erro_code != HM_ERR_OK) return erro_code;

    erro_code = emit_store_ident(ctx_ptr, idnt_idx);
    if (erro_code != HM_ERR_OK) return erro_code;

    erro_code = emit_load_ident(ctx_ptr, idnt_idx);
    return erro_code;
}

static hm_error_t emit_expr(backend_ctx_t* ctx_ptr, const tree_node_t* node_ptr) {
    if (node_ptr == nullptr) {
        emit_push_size(ctx_ptr, 0);
        return HM_ERR_OK;
    }

    if (node_ptr->type == CONSTANT) {
        emit_push_double(ctx_ptr, node_ptr->value.constant);
        return HM_ERR_OK;
    }

    if (node_ptr->type == IDENT) {
        return emit_load_ident(ctx_ptr, node_ptr->value.ident_idx);
    }

    HARD_ASSERT(node_ptr->type == FUNCTION, "unexpected node type in expr");

    op_code_t op_code = node_ptr->value.func;

    if (op_code == OP_CALL)    return emit_call_common(ctx_ptr, node_ptr, true);
    if (op_code == OP_ASSIGN)  return emit_assign_expr(ctx_ptr, node_ptr);
    if (op_code == OP_INPUT)   { emit_line(ctx_ptr, "IN\n"); return HM_ERR_OK; }

    if (op_code == OP_PLUS)  return emit_binary_math(ctx_ptr, node_ptr, "ADD");
    if (op_code == OP_MINUS) return emit_binary_math(ctx_ptr, node_ptr, "SUB");
    if (op_code == OP_MUL)   return emit_binary_math(ctx_ptr, node_ptr, "MULT");
    if (op_code == OP_DIV)   return emit_binary_math(ctx_ptr, node_ptr, "DEL");
    if (op_code == OP_POW)   return emit_binary_math(ctx_ptr, node_ptr, "POW");

    LOGGER_WARNING("expr op not implemented: %d", (int)op_code);
    emit_push_size(ctx_ptr, 0);
    return HM_ERR_OK;
}

//================================================================================

static void emit_truthy_jump_end(backend_ctx_t* ctx_ptr, size_t labl_end) {
    emit_push_size(ctx_ptr, 0);
    emit_jump_id(ctx_ptr, "JE", "ifend", labl_end);
}

static hm_error_t emit_if_stmt(backend_ctx_t* ctx_ptr, const tree_node_t* node_ptr) {
    HARD_ASSERT(is_func_node(node_ptr, OP_IF), "expected OP_IF");

    const tree_node_t* cond_ptr = node_ptr->left;
    const tree_node_t* body_ptr = node_ptr->right;

    size_t labl_true = new_label_id(ctx_ptr);
    size_t labl_end  = new_label_id(ctx_ptr);

    if (cond_ptr != nullptr && cond_ptr->type == FUNCTION) {
        op_code_t op_code = cond_ptr->value.func;

        if (op_code == OP_EQ || op_code == OP_LT || op_code == OP_GT ||
            op_code == OP_LE || op_code == OP_GE || op_code == OP_NEQ) {

            hm_error_t erro_code = emit_expr(ctx_ptr, cond_ptr->left);
            if (erro_code != HM_ERR_OK) return erro_code;

            erro_code = emit_expr(ctx_ptr, cond_ptr->right);
            if (erro_code != HM_ERR_OK) return erro_code;

            if (op_code == OP_EQ) {
                emit_jump_id(ctx_ptr, "JE", "iftrue", labl_true);
                emit_jump_id(ctx_ptr, "JUMP", "ifend", labl_end);
                emit_label_id(ctx_ptr, "iftrue", labl_true);
                erro_code = emit_stmt(ctx_ptr, body_ptr);
                if (erro_code != HM_ERR_OK) return erro_code;
                emit_label_id(ctx_ptr, "ifend", labl_end);
                return HM_ERR_OK;
            }

            if (op_code == OP_NEQ) {
                emit_jump_id(ctx_ptr, "JE", "ifend", labl_end);
                erro_code = emit_stmt(ctx_ptr, body_ptr);
                if (erro_code != HM_ERR_OK) return erro_code;
                emit_label_id(ctx_ptr, "ifend", labl_end);
                return HM_ERR_OK;
            }

            if (op_code == OP_LT) {
                emit_jump_id(ctx_ptr, "JB", "iftrue", labl_true);
            } else if (op_code == OP_GT) {
                emit_jump_id(ctx_ptr, "JA", "iftrue", labl_true);
            } else if (op_code == OP_LE) {
                emit_jump_id(ctx_ptr, "JA", "ifend", labl_end);
                erro_code = emit_stmt(ctx_ptr, body_ptr);
                if (erro_code != HM_ERR_OK) return erro_code;
                emit_label_id(ctx_ptr, "ifend", labl_end);
                return HM_ERR_OK;
            } else if (op_code == OP_GE) {
                emit_jump_id(ctx_ptr, "JB", "ifend", labl_end);
                erro_code = emit_stmt(ctx_ptr, body_ptr);
                if (erro_code != HM_ERR_OK) return erro_code;
                emit_label_id(ctx_ptr, "ifend", labl_end);
                return HM_ERR_OK;
            }

            emit_jump_id(ctx_ptr, "JUMP", "ifend", labl_end);
            emit_label_id(ctx_ptr, "iftrue", labl_true);
            erro_code = emit_stmt(ctx_ptr, body_ptr);
            if (erro_code != HM_ERR_OK) return erro_code;
            emit_label_id(ctx_ptr, "ifend", labl_end);
            return HM_ERR_OK;
        }
    }

    hm_error_t erro_code = emit_expr(ctx_ptr, cond_ptr);
    if (erro_code != HM_ERR_OK) return erro_code;

    emit_truthy_jump_end(ctx_ptr, labl_end);

    erro_code = emit_stmt(ctx_ptr, body_ptr);
    if (erro_code != HM_ERR_OK) return erro_code;

    emit_label_id(ctx_ptr, "ifend", labl_end);
    return HM_ERR_OK;
}

//================================================================================

static hm_error_t emit_print_stmt(backend_ctx_t* ctx_ptr, const tree_node_t* node_ptr) {
    HARD_ASSERT(is_func_node(node_ptr, OP_PRINT), "expected OP_PRINT");
    hm_error_t erro_code = emit_expr(ctx_ptr, node_ptr->right);
    if (erro_code != HM_ERR_OK) return erro_code;
    emit_line(ctx_ptr, "OUT\n");
    return HM_ERR_OK;
}

static hm_error_t emit_return_stmt(backend_ctx_t* ctx_ptr, const tree_node_t* node_ptr) {
    HARD_ASSERT(is_func_node(node_ptr, OP_RETURN), "expected OP_RETURN");

    hm_error_t erro_code = emit_expr(ctx_ptr, node_ptr->right);
    if (erro_code != HM_ERR_OK) return erro_code;

    emit_popr(ctx_ptr, REG_RAX);
    emit_unwind_scopes(ctx_ptr, ctx_ptr->scop_depth);
    emit_line(ctx_ptr, "RET\n");
    return HM_ERR_OK;
}

static hm_error_t emit_finish_stmt(backend_ctx_t* ctx_ptr, const tree_node_t* node_ptr) {
    HARD_ASSERT(is_func_node(node_ptr, OP_FINISH), "expected OP_FINISH");
    (void)node_ptr;

    emit_unwind_scopes(ctx_ptr, ctx_ptr->scop_depth);
    emit_line(ctx_ptr, "RET\n");
    return HM_ERR_OK;
}

static hm_error_t emit_scope_stmt(backend_ctx_t* ctx_ptr, const tree_node_t* node_ptr) {
    HARD_ASSERT(is_func_node(node_ptr, OP_VIS_START), "expected OP_VIS_START");

    hm_error_t erro_code = enter_scope(ctx_ptr);
    if (erro_code != HM_ERR_OK) return erro_code;

    erro_code = emit_stmt(ctx_ptr, node_ptr->right);
    leave_scope(ctx_ptr);
    return erro_code;
}

static hm_error_t emit_stmt(backend_ctx_t* ctx_ptr, const tree_node_t* node_ptr) {
    if (node_ptr == nullptr) return HM_ERR_OK;

    if (node_ptr->type != FUNCTION) {
        hm_error_t erro_code = emit_expr(ctx_ptr, node_ptr);
        if (erro_code != HM_ERR_OK) return erro_code;
        emit_line(ctx_ptr, "POP\n");
        return HM_ERR_OK;
    }

    op_code_t op_code = node_ptr->value.func;

    if (op_code == OP_LCAT) {
        hm_error_t left_code = emit_stmt(ctx_ptr, node_ptr->left);
        if (left_code != HM_ERR_OK) return left_code;
        return emit_stmt(ctx_ptr, node_ptr->right);
    }

    if (op_code == OP_VIS_START) return emit_scope_stmt(ctx_ptr, node_ptr);
    if (op_code == OP_PRINT)     return emit_print_stmt(ctx_ptr, node_ptr);
    if (op_code == OP_RETURN)    return emit_return_stmt(ctx_ptr, node_ptr);
    if (op_code == OP_FINISH)    return emit_finish_stmt(ctx_ptr, node_ptr);
    if (op_code == OP_IF)        return emit_if_stmt(ctx_ptr, node_ptr);

    if (op_code == OP_CALL) {
        return emit_call_common(ctx_ptr, node_ptr, false);
    }

    if (op_code == OP_ASSIGN) {
        hm_error_t erro_code = emit_assign_expr(ctx_ptr, node_ptr);
        if (erro_code != HM_ERR_OK) return erro_code;
        emit_line(ctx_ptr, "POP\n");
        return HM_ERR_OK;
    }

    hm_error_t erro_code = emit_expr(ctx_ptr, node_ptr);
    if (erro_code != HM_ERR_OK) return erro_code;
    emit_line(ctx_ptr, "POP\n");
    return HM_ERR_OK;
}

//================================================================================

static hm_error_t emit_func_body(backend_ctx_t* ctx_ptr, const tree_node_t* decl_ptr) {
    const tree_node_t* info_ptr = get_decl_info(decl_ptr);
    HARD_ASSERT(is_func_node(info_ptr, OP_FUNC_INFO), "bad func_info");

    const tree_node_t* name_ptr = get_info_name(info_ptr);
    HARD_ASSERT(node_is_ident(name_ptr), "func name must be IDENT");
    size_t idnt_idx = require_ident_idx(name_ptr);

    backend_func_symbol_t symb_val = {};
    HARD_ASSERT(func_table_get_symbol(ctx_ptr, idnt_idx, &symb_val), "symbol missing in table");

    emit_decl_label(ctx_ptr, idnt_idx);

    hm_error_t erro_code = scopes_reset(ctx_ptr);
    if (erro_code != HM_ERR_OK) return erro_code;

    emit_move_reg_reg(ctx_ptr, REG_RDX, REG_RBX);

    vector_t parm_list = {};
    (void)SIMPLE_VECTOR_INIT(&parm_list, 8, size_t);
    collect_param_idents(get_info_args(info_ptr), &parm_list);

    for (size_t idx_i = 0; idx_i < vector_size(&parm_list); ++idx_i) {
        size_t parm_idx = *(size_t*)vector_get_const(&parm_list, idx_i);
        size_t offc_val = ctx_ptr->offc_curr++;
        erro_code = scopes_insert_local(ctx_ptr, parm_idx, offc_val);
        if (erro_code != HM_ERR_OK) { vector_destroy(&parm_list); return erro_code; }
        emit_add_reg_imm(ctx_ptr, REG_RBX, 1);
    }

    for (size_t idx_i = vector_size(&parm_list); idx_i > 0; --idx_i) {
        size_t offc_val = idx_i - 1;
        emit_addr_from_base(ctx_ptr, offc_val);
        emit_popm(ctx_ptr, REG_RCX);
    }

    vector_destroy(&parm_list);

    erro_code = emit_stmt(ctx_ptr, decl_ptr->right);
    if (erro_code != HM_ERR_OK) return erro_code;

    if (!symb_val.is_proc) {
        emit_push_size(ctx_ptr, 0);
        emit_popr(ctx_ptr, REG_RAX);
    }

    emit_unwind_scopes(ctx_ptr, ctx_ptr->scop_depth);
    emit_line(ctx_ptr, "RET\n");
    return HM_ERR_OK;
}

static hm_error_t pass2_emit_funcs(backend_ctx_t* ctx_ptr, const tree_node_t* node_ptr) {
    if (node_ptr == nullptr) return HM_ERR_OK;

    if (is_func_node(node_ptr, OP_FUNC_DECL) || is_func_node(node_ptr, OP_PROC_DECL)) {
        return emit_func_body(ctx_ptr, node_ptr);
    }

    hm_error_t left_code = pass2_emit_funcs(ctx_ptr, node_ptr->left);
    if (left_code != HM_ERR_OK) return left_code;

    return pass2_emit_funcs(ctx_ptr, node_ptr->right);
}

//================================================================================

static void emit_entry_prelude(backend_ctx_t* ctx_ptr) {
    emit_push_size(ctx_ptr, 0);
    emit_popr(ctx_ptr, REG_RBX);

    size_t entry_idx = 0;
    if (func_table_find_main(ctx_ptr, &entry_idx) ||
        func_table_find_first(ctx_ptr->func_table, &entry_idx)) {
        emit_call_label(ctx_ptr, entry_idx);
    }

    emit_line(ctx_ptr, "HLT\n\n");
}

//================================================================================

hm_error_t backend_emit_asm(const tree_t* tree,
                           FILE* asm_file,
                           u_map_t* func_table) {
    HARD_ASSERT(tree      != nullptr, "tree is nullptr");
    HARD_ASSERT(asm_file  != nullptr, "asm_file is nullptr");
    HARD_ASSERT(func_table != nullptr, "func_table is nullptr");

    backend_ctx_t ctx_data = {};
    ctx_data.tree_ptr    = tree;
    ctx_data.file_ptr    = asm_file;
    ctx_data.func_table  = func_table;
    ctx_data.label_next  = 1;
    ctx_data.offc_curr   = 0;
    ctx_data.scop_depth  = 0;

    vector_error_t vec_error = SIMPLE_VECTOR_INIT(&ctx_data.scop_stack, 16, backend_scope_t);
    if (vec_error != VEC_ERR_OK) return HM_ERR_MEM_ALLOC;

    hm_error_t erro_code = pass1_collect(&ctx_data, tree->root);
    if (erro_code != HM_ERR_OK) {
        vector_destroy(&ctx_data.scop_stack);
        return erro_code;
    }

    emit_entry_prelude(&ctx_data);

    erro_code = pass2_emit_funcs(&ctx_data, tree->root);

    while (vector_size(&ctx_data.scop_stack) != 0) {
        backend_scope_t scop_old = {};
        (void)vector_pop_back(&ctx_data.scop_stack, &scop_old);
        scope_destroy(&scop_old);
    }
    vector_destroy(&ctx_data.scop_stack);

    return erro_code;
}
