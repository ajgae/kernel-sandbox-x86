#include "printf.h"

static inline bool is_digit(char c) {
    return (c >= '0') && (c <= '9');
}

static inline bool is_hex(char c) {
    return is_digit(c) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F'));
}

void ntoa(char *const buf, size_t cnt, long num) {

}

void snprintf(char *const buf, size_t cnt, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    size_t i_fmt = 0u; // index in `fmt`
    size_t i_buf = 0u; // index in `buf`
    while(fmt[i_fmt] && i_buf < cnt) {
        if (fmt[i_fmt] != '%') {
            // normal character
            buf[i_buf] = fmt[i_fmt];
            ++i_buf;
            ++i_fmt;
            continue;
        }
        ++i_fmt;
        char convspec = fmt[i_fmt];
        switch (convspec) {
            case '%':
            {
                buf[i_buf] = '%';
                i_buf++;
                break;
            }
            case 'x':
            case 'X': 
            case 'd':
            // FIXME digits are in reverse order! fugg
            {
                long value = va_arg(ap, long);
                size_t base;
                if (convspec == 'x' || convspec == 'X') {
                    base = 16;
                } else {
                    base = 10;
                }
                do {
                    char const digit = (char)(value % base);
                    buf[i_buf] = digit < 10 ? '0' + digit : (convspec == 'X' ? 'A' : 'a') + digit - 10;
                    i_buf++;
                    value /= base;
                } while (value && i_buf < cnt);
                break;
            }
        }
        ++i_fmt;
    }
    va_end(ap);
}

