#ifndef DEBUG_META_H_INCLUDED
#define DEBUG_META_H_INCLUDED

#include <stddef.h>

#ifdef VERIFY_DEBUG
    #define ON_DEBUG(...) __VA_ARGS__
#else
    #define ON_DEBUG(...)
#endif


#ifdef DUMP_CREATION_DEBUG
    #define ON_DUMP_CREATION_DEBUG(...) __VA_ARGS__
#else
    #define ON_DUMP_CREATION_DEBUG(...)
#endif

#ifdef TEX_CREATION_DEBUG
    #define ON_TEX_CREATION_DEBUG(...) __VA_ARGS__
#else
    #define ON_TEX_CREATION_DEBUG(...)
#endif

#ifdef TEX_SQUASH
    #define ON_TEX_SQUASH(...) __VA_ARGS__
#else
    #define ON_TEX_SQUASH(...)
#endif

#ifdef HASH_DEBUG
	#define VERIFY_DEBUG
	#define ON_HASH_DEBUG(...) __VA_ARGS__
#else
	#define ON_HASH_DEBUG(...)
#endif	


#ifdef CANARY_DEBUG
	#define VERIFY_DEBUG
	#define ON_CANARY_DEBUG(...) __VA_ARGS__
#else
	#define ON_CANARY_DEBUG(...)
#endif


#ifdef LIST_CANARY_DEBUG
	#define ON_LIST_CANARY_DEBUG(...) __VA_ARGS__
#else
	#define ON_LIST_CANARY_DEBUG(...)
#endif

struct ver_info_t {
    const char* file;
    const char* func;
    int         line;
};

#define VER_INIT ver_info_t{__FILE__, __func__, __LINE__}

#endif

