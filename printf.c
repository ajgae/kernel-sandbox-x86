#include "printf.h"

#include "strings.h"

// NOTE: for a 64-bit number, 16 characters are enough for a hexadecimal
// representation.
// NOTE: make sure to leave space for the null-terminator, because `ntoa`
// treats the temporary buffers as strings (it has to)
#define PRINTF_NTOA_BUFFER_SIZE 32u

/*
 * Returns true iff `c` is a valid decimal digit.
 */
static inline bool is_digit(char c) {
    return (c >= '0') && (c <= '9');
}

/*
 * Returns true iff `c` is a valid hexadecimal digit.
 */
static inline bool is_hex(char c) {
    return is_digit(c) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F'));
}

/*
 * Writes reversed `src` string into `buf`. Assumes a zero-filled
 * `buf` of size >= to `src`'s size, and that a valid null-terminated
 * string is in `src`, otherwise behaviour is undefined.
 */
void out_rev(char *const buf, char const *const src) {
    size_t len = strlen(src);
    for (size_t i = 0; i < len; ++i) {
        buf[i] = src[len - 1 - i];
    }
}

/*
 * Appends the string representation of `num` in base `base` to `buf`,
 * stopping mid-way (with the number only partly represented) if `i_buf`,
 * the index in `buf` at which the representation should be written`,
 * reaches `cnt`, that is, if we reach `cnt` (the max amount of character
 * to write to `buf`) while writing the string representation of `num` to
 * `buf`.
 */ 
void ntoa(char *const buf, size_t *i_buf, size_t cnt, long num, size_t base) {
    char digits[PRINTF_NTOA_BUFFER_SIZE] = {0};
    char tmp[PRINTF_NTOA_BUFFER_SIZE] = {0};
    size_t i_digits = 0;
    
    // convert `num` to (reversed) string representation
    do {
        char const digit = (char)(num % base);
        digits[i_digits] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        i_digits++;
        num /= base;
    } while (num);
    
    // reverse digit order
    out_rev(tmp, digits);

    // actually append digits to `buf`
    for (size_t i = 0; i < strlen(tmp) && *i_buf < cnt; ++i) {
        buf[*i_buf] = tmp[i];
        (*i_buf)++;
    }
}

/*
 * Format a string and place result into `buf`, stopping at `cnt` if it
 * is reached.
 *
 * IMPORTANT: this assumes that `buf` is zero-filled. Otherwise,
 * behaviour is undefined.
 *
 * IMPORTANT (FIXME?): make sure to account for the null-terminator when
 * specifying `cnt`, i.e. `cnt` should be at most `sizeof(buf)-1`, not
 * `sizeof(buf)`!
 *
 * TODO types??? rn we're just casting all variadic args to long
 */
void vsnprintf(char *const buf, size_t cnt, const char *fmt, va_list ap) {
    size_t i_fmt = 0; // index in `fmt`
    size_t i_buf = 0; // index in `buf`
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
            case '%': // escaped '%' char, treat normally
            {
                buf[i_buf] = '%';
                i_buf++;
                break;
            }
            case 'x':
            case 'X': 
            case 'd':
            {
                long value = (long)va_arg(ap, size_t);
                size_t base;
                if (convspec == 'x' || convspec == 'X') {
                    base = 16;
                } else {
                    base = 10;
                }
                ntoa(buf, &i_buf, cnt, value, base);
                break;
            }
        }
        ++i_fmt;
    }
    va_end(ap);
}

