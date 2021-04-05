#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "strings.h"
#include "printf.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

enum vga_color {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,
    VGA_COLOR_WHITE         = 15,
};

// pointer to the memory-mapped VGA buffer
volatile uint16_t* vga_buffer = (volatile uint16_t*) 0xb8000; 
// pointer to the (for now) statically-allocated terminal backbuffer
static uint16_t term_backbuffer[VGA_HEIGHT*VGA_WIDTH];

static inline uint8_t vga_entry_color(enum vga_color const fg, enum vga_color const bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char const uc, uint8_t const color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

static void vga_fill(uint16_t vga_entry) {
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            size_t const index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry;
        }
    }
}

static inline void vga_clear() {
    vga_fill(vga_entry(' ', vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK)));
}

/*
 * The `term` struct provides a nicer abstraction for writing to the VGA
 * buffer. 
 *
 * TODO:
 * - REMEMBER: buffer size has to be a multiple of VGA_WIDTH
 * - make it clear each row before to it, with an option to not clear,
 *   sometimes handy (e.g. curses-like behavior)
 *
 * The buffer is circular: when writing to it, if the results of what we write
 * goes beyond the last row, the results are instead written at the start of
 * the buffer, overwriting whatever was
 * there.
 *
 * The buffer's rows map directly to the VGA buffer's rows. That is,
 * the backbuffer doesn't just contain a stream of characters. Instead, it
 * contains actual VGA screenspace, including e.g. empty space at the end of an
 * LF-terminated line.
 *
 * A subset of the buffer's rows are copied to the VGA buffer on refresh, also
 * wrapping around if we reach the last row of the buffer while copying.
 */
struct term {
    size_t row; // current row
    size_t column; // current column
    uint8_t color; // current color
    size_t row_n; // total number of rows in backbuffer
    size_t row_shift; // first row from the backbuffer that is displayed
    uint16_t *buff; // backbuffer, >= in size to the VGA buffer, multiple of VGA_WIDTH
};

void term_init(struct term *term, uint16_t *buff) {
    term->row = 0;
    term->column = 0;
    term->color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    // FIXME when we start handling a backbuffer larger than the VGA buffer
    term->row_n = VGA_HEIGHT; 
    term->row_shift = 0;
    term->buff = buff;
    for (size_t i = 0; i < term->row_n * VGA_WIDTH; ++i) {
        term->buff[i] = 0;
    }
}

void term_set_color(struct term *term, uint8_t const color) {
    term->color = color;
}

void term_put_entry_at(char const c, struct term *term, size_t const x, size_t const y) {
    size_t const index = ((term->row_shift + y) % term->row_n) * VGA_WIDTH + x;
    term->buff[index] = vga_entry(c, term->color);
}

void term_put_lf(struct term *term) {
    ++term->row;
    if (term->row >= VGA_HEIGHT) {
        term->row = 0;
    }
    term->column = 0;
    /* FIXME when we start handling a backbuffer larger than the VGA buffer
     * row_shift is modulo row_n so this conditional doesn't work,
    // if we have reached the bottom of the screen, start scrolling
    if (term->row_shift >= VGA_HEIGHT) {
        ++term->row_shift;
        if (term->row_shift >= term->row_n) {
            term->row_shift = 0;
        }
    }
    */
}

void term_put_char(struct term *term, char c) {
    switch (c) { // handle non-print chars
    case '\n': 
        term_put_lf(term);
        break;
    default:
        term_put_entry_at(c, term, term->column, term->row);
        ++term->column;
    }
    if (term->column >= VGA_WIDTH) {
        term->column = 0;
        // FIXME handle scrolling n whatnot, this only works because backbuffer is
        // the same height as the VGA buffer
        ++term->row;
        if (term->row >= VGA_HEIGHT) {
            term->row = 0;
        }
    }
}

void term_write(struct term *term, char const *const data, size_t const size) {
    for (size_t i = 0; i < size; ++i) {
        term_put_char(term, data[i]);
    }
}

void term_put_str(struct term *term, char const *const str) {
    term_write(term, str, strlen(str));
}

/*
 * Copy the contents of the backbuffer to the VGA buffer for display.
 */
void vga_refresh_all(struct term const *const term) {
    for (size_t i = 0; i < VGA_HEIGHT; ++i) {
        for (size_t j = 0; j < VGA_WIDTH; ++j) {
            size_t vga_index = i * VGA_WIDTH + j;
            // FIXME buffer
            size_t term_index = ((i + term->row_shift) % VGA_HEIGHT) * VGA_WIDTH + j;
            vga_buffer[vga_index] = term->buff[term_index];
        }
    }
}

void kernel_main(void) {
    vga_clear();
    struct term term;
    term_init(&term, term_backbuffer);
    char buf[256] = {0}; // needs to be filled with 0s!!
    snprintf(buf, sizeof(buf), "hello, world!\n\
hex value of %d: %x\n", (long)200, (long)200);
    term_put_str(&term, buf);
    /*
    term_put_str(&term, "\
Hello, kernel world! Sure hope this very long line doesn't get yeeted in a bad way! That would be a shame...\n\
Hello 1;\n\
Hello 2;\n\
Hello 3;\n\
Hello 4;\n\
Hello 5;\n\
Hello 6;\n\
Hello 7;\n\
Hello 8;\n\
Hello 9;\n\
Hello 10;\n\
Hello 11;\n\
Hello 12;\n\
Hello 13;\n\
Hello 14;\n\
Hello 15;\n\
Hello 16;\n\
Hello 17;\n\
Hello 18;\n\
Hello 19;\n\
Hello 20;\n\
Hello 21;\n\
Hello 22;\n\
Hello 23;\n\
Hello 24;\n\
Hello 25;\n\
Hello 26;\n\
Hello 27;\n\
");
    // */
    vga_refresh_all(&term);
}

