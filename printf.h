#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

/*
 * Format string and place result into `buf`, trimming if the result's length
 * is greater than `len`.
 *
 * IMPORTANT: this assumes that the buffer is zero-filled.
 */
void snprintf(char *const buf, size_t cnt, const char *fmt, ...);

