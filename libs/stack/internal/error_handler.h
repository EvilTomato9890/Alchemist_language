#ifndef LIBS_STACK_INTERNAL_ERROR_HANDLER_H_NCLUDED
#define LIBS_STACK_INTERNAL_ERROR_HANDLER_H_NCLUDED

enum st_err {
	ST_NO_ERROR = 0,
	ST_NULL_ARG_ERROR 	    = 1 << 0, //TODO: 1 << 0
	ST_BIG_SIZE_ERROR 	    = 1 << 1,
	ST_SMALL_SIZE_ERROR    = 1 << 2,
	ST_NEG_SIZE_ERROR      = 1 << 3,
	ST_NEG_CAP_ERROR       = 1 << 4,
	ST_ZERO_CAP_ERROR      = 1 << 5,
	ST_NULL_DATA_ERROR     = 1 << 6,
	ST_MEM_ALLOC_ERROR     = 1 << 7,
	ST_CANARY_DATA_ERROR   = 1 << 8,
	ST_CANARY_STRUCT_ERROR = 1 << 9,
	ST_HASH_ERROR          = 1 << 10
};

typedef long error_code;

#define ST_RETURN_IF_ERROR(error_, ...) 						\
	do {											 	\
		if((error_) != 0) { 							\
			LOGGER_ERROR("Error code: %lu", (error_));  \
			 __VA_ARGS__\
			return (error_); 							\
		}												\
	} while(0)

#endif /* LIBS_STACK_INTERNAL_ERROR_HANDLER_H_NCLUDED */
