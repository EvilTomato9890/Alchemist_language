#ifndef MY_STRING_H_INCLUDED
#define MY_STRING_H_INCLUDED

#include <stdlib.h>
#include <string.h>

struct string_t {
    char*  ptr;
    size_t len;
};

struct c_string_t {
    const char* ptr;
    size_t      len;
};

int my_ssstrcmp(c_string_t str1, c_string_t str2);
int my_scstrcmp(c_string_t str1, const char* str2);
#endif