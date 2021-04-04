#include "printf.h"

void snprintf(char *const buf, size_t cnt, const char *fmt, ...) {
    size_t i = 0; // index in `fmt`
    size_t size = 0; // growing size of result
    while(fmt[i] && size < cnt) {
        buf[i] = fmt[i];
        ++i;
        ++size;
    }
}
