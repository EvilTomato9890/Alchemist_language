#include "my_string.h"

#include <string.h>

int my_ssstrcmp(c_string_t str1, c_string_t str2) {
    size_t min_len = (str1.len < str2.len) ? str1.len : str2.len;

    int cmp = memcmp(str1.ptr, str2.ptr, min_len);
    if (cmp != 0) return cmp;

    if (str1.len == str2.len) return 0;
    return (str1.len < str2.len) ? -1 : 1;
}


int my_scstrcmp(c_string_t str1, const char* str2) {
    size_t str2_len = strlen(str2);
    size_t min_len = (str1.len < str2_len) ? str1.len : str2_len;

    int cmp = memcmp(str1.ptr, str2, min_len);
    if (cmp != 0) return cmp;

    if (str1.len == str2_len) return 0;
    return (str1.len < str2_len) ? -1 : 1;
}
