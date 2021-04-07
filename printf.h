#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

void vsnprintf(char *const buf, size_t cnt, const char *fmt, va_list ap);

