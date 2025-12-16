#include "../include/my_string.h"

int my_ssstrcmp(c_string_t str1, c_string_t str2) {
    if(str1.len > str2.len) return strncmp(str1.ptr, str2.ptr, str1.len);
    return strncmp(str1.ptr, str2.ptr, str1.len);
}

int my_scstrcmp(c_string_t str1, const char* str2) {
    size_t str2_len = strlen(str2);
    if(str1.len > str2_len) return strncmp(str1.ptr, str2, str1.len);
    return strncmp(str1.ptr, str2, str1.len);
}
