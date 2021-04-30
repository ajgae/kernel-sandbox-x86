#include <stddef.h>
#include <stdint.h>

#include "vga.h"
#include "strings.h"
#include "printf.h"

void kernel_main(void) {
    vga_clear();
    struct term term;
    term_init(&term);
    for (size_t i = 0; i < 192; ++i) {
        term_printf(&term, "number %d\n", i);
    }
}

