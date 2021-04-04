#include "strings.h"

size_t strlen(char const *const str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

