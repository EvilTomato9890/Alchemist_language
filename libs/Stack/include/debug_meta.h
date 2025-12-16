#ifndef LIBS_STACK_INCLUDE_DEBUG_META_H_NCLUDED
#define LIBS_STACK_INCLUDE_DEBUG_META_H_NCLUDED

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

#endif /* LIBS_STACK_INCLUDE_DEBUG_META_H_NCLUDED */


/*
используются внешние типы и опережающие обхявления. К примеру struct tree_node_t дает возможность объявлять указатели, но нельзя пользоваться. Что-то вроеде прототипа. 
в stack.h убирается полное определение стктуры. Добавляется прототип. Позволяет указатели на stack. Который уже инклюдится

если поля струкутуры не меняет, то можно прямо в cpp

закрытые (непрозрачный) тип
известно только назавние и функции

стандартный ппример FILE*. Нигде не написано что он  содержит. Работа напрямую неккоректна, только через функции. 


для функций либо систему оберток, чтобы не вызывать напрямую функции, которые зависят от ON_DEBUG

ON_DEBUG лучше общий, чтобы работал ов овсем поекте
с параметрами номера мрдуля того, кооый подвергается оталдке
ON_DEBUG(id модуля)
ON_DEBUG(STACK_MODULE, ...)

просто для отладки полезно

аодозрения формируются на опыте


обратный frontend

сделайть README для языка
можно сделать зимой.
Допилить процессор
дифференциатор


обычно с теми файлами, которые имеют одинаоквые имена делается генерализация, тоесть общиц config или если нужно stack_config или внутренний*/