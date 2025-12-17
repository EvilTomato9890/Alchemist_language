#ifndef LIBS_VECTOR_INCLUDE_VECTOR_H_NCLUDED
#define LIBS_VECTOR_INCLUDE_VECTOR_H_NCLUDED

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#include "error_handler.h"

//================================================================================

struct vector_t {
    void*  data;

    size_t size;
    size_t capacity;

    size_t elem_size;

    bool   is_static;
};

//================================================================================
//                      Конструкторы / Деструкторы / Копировальщики
//================================================================================

vector_error_t vector_init(vector_t* vec, size_t capacity, size_t elem_size);

vector_error_t vector_static_init(vector_t* vec, void* data, size_t capacity, size_t elem_size);

vector_error_t vector_destroy(vector_t* vec);

//================================================================================
//                              Базовые функции
//================================================================================

size_t vector_size    (const vector_t* vec);
size_t vector_capacity(const vector_t* vec);

void*       vector_get      (const vector_t* vec, size_t index);
const void* vector_get_const(const vector_t* vec, size_t index);

void vector_clear(vector_t* vec);

//================================================================================
//                           Модифицирующие операции
//================================================================================

vector_error_t vector_push_back(vector_t* vec, const void* elem);
vector_error_t vector_pop_back (vector_t* vec, void* elem_out);

vector_error_t vector_insert(vector_t* vec, size_t index, const void* elem); 
vector_error_t vector_erase (vector_t* vec, size_t index, void* elem_out);   

//================================================================================
//                          Вспоогательное
//================================================================================

size_t vector_required_bytes(size_t capacity, size_t elem_size);

//================================================================================
//                        Макросы-обертки
//================================================================================

#define SIMPLE_VECTOR_INIT(vector_, capacity_, elem_type_) \
    vector_init((vector_), (capacity_), sizeof(elem_type_))

#define SIMPLE_VECTOR_STATIC_INIT(vector_, data_, capacity_, elem_type_) \
    vector_static_init((vector_), (data_), (capacity_), sizeof(elem_type_))


#endif /* LIBS_VECTOR_INCLUDE_VECTOR_H_NCLUDED */
