#include "vector.h"

#include "common/asserts/include/asserts.h"
#include "common/logger/include/logger.h"

#include <string.h>

//================================================================================

static const size_t INITIAL_CAPACITY      = 32;

static const float  GROWTH_FACTOR     = 2.0f;   
static const float  SHRINK_FACTOR     = 0.5f;   
static const float  MIN_LOAD_FACTOR   = 0.25f;  

//================================================================================

static inline unsigned char* vector_ptr(const vector_t* vector, size_t index) {
    return (unsigned char*)vector->data + index * vector->elem_size;
}

static inline const unsigned char* vector_ptr_const(const vector_t* vector, size_t index) {
    return (const unsigned char*)vector->data + index * vector->elem_size;
}

size_t vector_required_bytes(size_t capacity, size_t elem_size) {
    if (elem_size == 0) return 0;
    return capacity * elem_size;
}

//================================================================================

static vector_error_t vector_realloc(vector_t* vector, size_t new_capacity) {
    HARD_ASSERT(vector != nullptr, "vector is nullptr");

    if (vector->is_static) return VEC_ERR_FULL;

    if (new_capacity < INITIAL_CAPACITY) new_capacity = INITIAL_CAPACITY;
    if (new_capacity == vector->capacity)   return VEC_ERR_OK;

    const size_t old_bytes = vector->capacity * vector->elem_size;
    const size_t new_bytes = new_capacity  * vector->elem_size;

    void* new_data = realloc(vector->data, new_bytes);
    if (new_data == nullptr) return VEC_ERR_MEM_ALLOC;

    if (new_bytes > old_bytes) {
        memset((unsigned char*)new_data + old_bytes, 0, new_bytes - old_bytes);
    }

    vector->data     = new_data;
    vector->capacity = new_capacity;

    if (vector->size > vector->capacity) vector->size = vector->capacity;

    return VEC_ERR_OK;
}

static vector_error_t normalize_for_grow(vector_t* vector, size_t need_elems) {
    HARD_ASSERT(vector != nullptr, "vector is nullptr");

    if (need_elems <= vector->capacity) return VEC_ERR_OK;
    if (vector->is_static)              return VEC_ERR_FULL;

    size_t new_capacity = vector->capacity;
    if (new_capacity < INITIAL_CAPACITY) new_capacity = INITIAL_CAPACITY;

    while (new_capacity < need_elems) {
        size_t next = (size_t)((double)new_capacity * (double)GROWTH_FACTOR);
        if (next <= new_capacity) return VEC_ERR_BAD_ARG; 
        new_capacity = next;
    }

    return vector_realloc(vector, new_capacity);
}


static vector_error_t normalize_for_shrink(vector_t* vector) {
    HARD_ASSERT(vector != nullptr, "vector is nullptr");

    if (vector->is_static)                    return VEC_ERR_OK;
    if (vector->capacity <= INITIAL_CAPACITY) return VEC_ERR_OK;

    size_t new_capacity = vector->capacity;

    while (new_capacity > INITIAL_CAPACITY) {
        const double load = (new_capacity == 0) ? 1.0 : (double)vector->size / (double)new_capacity;

        if (load >= MIN_LOAD_FACTOR) break;
        size_t candidate = (size_t)((double)new_capacity * (double)SHRINK_FACTOR);
        if (candidate < INITIAL_CAPACITY) candidate = INITIAL_CAPACITY;
        if (candidate < vector->size)        candidate = vector->size;
        if (candidate >= new_capacity) break; 
        new_capacity = candidate;
    }

    if (new_capacity == vector->capacity) return VEC_ERR_OK;
    return vector_realloc(vector, new_capacity);
}


//================================================================================
//                      Конструкторы / Деструкторы / Копировальзики
//================================================================================

vector_error_t vector_init(vector_t* vector, size_t capacity, size_t elem_size) {
    HARD_ASSERT(vector != nullptr, "vector is nullptr");
    HARD_ASSERT(elem_size > 0, "elem_size must be > 0");

    LOGGER_DEBUG("vector_init started");

    if (capacity < INITIAL_CAPACITY) capacity = INITIAL_CAPACITY;

    void* data = calloc(capacity, elem_size);
    if (data == nullptr) return VEC_ERR_MEM_ALLOC;

    vector->data      = data;
    vector->size      = 0;
    vector->capacity  = capacity;
    vector->elem_size = elem_size;
    vector->is_static = false;

    LOGGER_DEBUG("vector_init finished");
    return VEC_ERR_OK;
}

vector_error_t vector_static_init(vector_t* vector, void* data, size_t capacity, size_t elem_size) {
    HARD_ASSERT(vector != nullptr, "vector is nullptr");
    HARD_ASSERT(data != nullptr, "data is nullptr");
    HARD_ASSERT(elem_size > 0, "elem_size must be > 0");

    LOGGER_DEBUG("vector_static_init started");

    if (capacity == 0) return VEC_ERR_BAD_ARG;

    memset(data, 0, capacity * elem_size);

    vector->data      = data;
    vector->size      = 0;
    vector->capacity  = capacity;
    vector->elem_size = elem_size;
    vector->is_static = true;

    LOGGER_DEBUG("vector_static_init finished");
    return VEC_ERR_OK;
}

vector_error_t vector_destroy(vector_t* vector) {
    HARD_ASSERT(vector != nullptr, "vector is nullptr");

    LOGGER_DEBUG("vector_destroy started");

    if (!vector->is_static && vector->data != nullptr) free(vector->data);
    memset(vector, 0, sizeof(*vector));

    LOGGER_DEBUG("vector_destroy finished");
    return VEC_ERR_OK;
}

//================================================================================
//                              Базовые функции
//================================================================================

size_t vector_size(const vector_t* vector) {
    HARD_ASSERT(vector != nullptr, "vector is nullptr");
    return vector->size;
}

size_t vector_capacity(const vector_t* vector) {
    HARD_ASSERT(vector != nullptr, "vector is nullptr");
    return vector->capacity;
}

void* vector_get(const vector_t* vector, size_t index) {
    HARD_ASSERT(vector != nullptr, "vector is nullptr");
    if (index >= vector->size) return nullptr;
    return (void*)vector_ptr(vector, index);
}

const void* vector_get_const(const vector_t* vector, size_t index) {
    HARD_ASSERT(vector != nullptr, "vector is nullptr");
    if (index >= vector->size) return nullptr;
    return (const void*)vector_ptr_const(vector, index);
}

void vector_clear(vector_t* vector) {
    HARD_ASSERT(vector != nullptr, "vector is nullptr");

    vector->size = 0;
    normalize_for_shrink(vector);
}

//================================================================================
//                           Основные функции
//================================================================================

vector_error_t vector_push_back(vector_t* vector, const void* elem) {
    HARD_ASSERT(vector  != nullptr, "vector is nullptr");
    HARD_ASSERT(elem != nullptr, "elem is nullptr");

    vector_error_t err = normalize_for_grow(vector, vector->size + 1);
    vector_RETURN_IF_ERROR(err);

    memcpy(vector_ptr(vector, vector->size), elem, vector->elem_size);
    vector->size++;
    return VEC_ERR_OK;
}

vector_error_t vector_pop_back(vector_t* vector, void* elem_out) {
    HARD_ASSERT(vector != nullptr, "vector is nullptr");

    if (vector->size == 0) return VEC_ERR_NOT_FOUND;
    const size_t last = vector->size - 1;
    if (elem_out != nullptr) 
        memcpy(elem_out, vector_ptr(vector, last), vector->elem_size);

    vector->size--;
    vector_error_t err = normalize_for_shrink(vector);
    vector_RETURN_IF_ERROR(err);

    return VEC_ERR_OK;
}

vector_error_t vector_insert(vector_t* vector, size_t index, const void* elem) {
    HARD_ASSERT(vector  != nullptr, "vector is nullptr");
    HARD_ASSERT(elem != nullptr, "elem is nullptr");

    if (index > vector->size) return VEC_ERR_BAD_ARG;

    vector_error_t err = normalize_for_grow(vector, vector->size + 1);
    vector_RETURN_IF_ERROR(err);

    if (index != vector->size) {
        void*  dst   = vector_ptr(vector, index + 1);
        void*  src   = vector_ptr(vector, index);
        size_t bytes = (vector->size - index) * vector->elem_size;
        memmove(dst, src, bytes);
    }
    memcpy(vector_ptr(vector, index), elem, vector->elem_size);
    vector->size++;

    return VEC_ERR_OK;
}

vector_error_t vector_erase(vector_t* vector, size_t index, void* elem_out) {
    HARD_ASSERT(vector != nullptr, "vector is nullptr");

    if (index >= vector->size) return VEC_ERR_NOT_FOUND;

    if (elem_out != nullptr) 
        memcpy(elem_out, vector_ptr(vector, index), vector->elem_size);
    
    if (index + 1 != vector->size) {
        void*  dst   = vector_ptr(vector, index);
        void*  src   = vector_ptr(vector, index + 1);
        size_t bytes = (vector->size - index - 1) * vector->elem_size;
        memmove(dst, src, bytes);
    }

    vector->size--;

    vector_error_t err = normalize_for_shrink(vector);
    vector_RETURN_IF_ERROR(err);

    return VEC_ERR_OK;
}
