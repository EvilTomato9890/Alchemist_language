#ifndef LIBS_VECTOR_INCLUDE_ERROR_HANDLER_H_NCLUDED
#define LIBS_VECTOR_INCLUDE_ERROR_HANDLER_H_NCLUDED

#include "common/logger/include/logger.h"

enum vector_error_t {
    vector_ERR_OK,
    vector_ERR_MEM_ALLOC,         
    vector_ERR_FULL,        
    vector_ERR_BAD_ARG,     
    vector_ERR_NOT_FOUND,  
    vector_ERR_INTERNAL 
};
 
#define vector_RETURN_IF_ERROR(error_, ...)                       \
    do {                                                   \
        vector_error_t err_ = error_;                             \
        if((int)(err_) != 0) {                             \
        LOGGER_ERROR("ERROR: %d happend", err_);           \
        __VA_ARGS__;                                       \
        return err_;                                       \
        }                                                  \
    } while (0)

#endif /* LIBS_VECTOR_INCLUDE_ERROR_HANDLER_H_NCLUDED */
