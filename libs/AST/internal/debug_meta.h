#ifndef LIBS_AST_INTERNAL_DEBUG_META_H_NCLUDED
#define LIBS_AST_INTERNAL_DEBUG_META_H_NCLUDED

#include <stddef.h>

#ifdef TREE_VERIFY_DEBUG
    #define ON_TREE_DEBUG(...) __VA_ARGS__
#else
    #define ON_TREE_DEBUG(...)
#endif

#ifdef TREE_DUMP_CREATION_DEBUG
    #define ON_TREE_DUMP_CREATION_DEBUG(...) __VA_ARGS__
#else
    #define ON_TREE_DUMP_CREATION_DEBUG(...)
#endif

#ifdef TREE_TEX_CREATION_DEBUG
    #define ON_TREE_TEX_CREATION_DEBUG(...) __VA_ARGS__
#else
    #define ON_TREE_TEX_CREATION_DEBUG(...)
#endif

#ifdef TREE_TEX_SQUASH
    #define ON_TREE_TEX_SQUASH(...) __VA_ARGS__
#else
    #define ON_TREE_TEX_SQUASH(...)
#endif

struct tree_ver_info_t {
    const char* file;
    const char* func;
    int         line;
};

#define TREE_VER_INIT tree_ver_info_t{__FILE__, __func__, __LINE__}

#endif /* LIBS_AST_INTERNAL_DEBUG_META_H_NCLUDED */

