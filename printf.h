#pragma once

#include <stdarg.h>
#include <stddef.h>

/*
 * Format string and place result into `buf`, trimming if the result's length
 * is greater than `len`
 */
void snprintf(char *const buf, size_t cnt, const char *fmt, ...);
