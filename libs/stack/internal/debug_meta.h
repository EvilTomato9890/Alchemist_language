#ifndef LIBS_STACK_INTERNAL_DEBUG_META_H_NCLUDED
#define LIBS_STACK_INTERNAL_DEBUG_META_H_NCLUDED

#include <stddef.h>

#ifdef STACK_VERIFY_DEBUG
    #define ON_STACK_DEBUG(...) __VA_ARGS__
#else
    #define ON_STACK_DEBUG(...)
#endif

#ifdef STACK_HASH_DEBUG
	#define STACK_VERIFY_DEBUG
	#define ON_STACK_HASH_DEBUG(...) __VA_ARGS__
#else
	#define ON_STACK_HASH_DEBUG(...)
#endif	


#ifdef STACK_CANARY_DEBUG
	#define STACK_VERIFY_DEBUG
	#define ON_STACK_CANARY_DEBUG(...) __VA_ARGS__
#else
	#define ON_STACK_CANARY_DEBUG(...)
#endif

struct st_ver_info_t {
    const char* file;
    const char* func;
    int         line;
};

#define STACK_VER_INIT st_ver_info_t{__FILE__, __func__, __LINE__}

#endif /* LIBS_STACK_INTERNAL_DEBUG_META_H_NCLUDED */

