#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif
*/

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
// pointer to the statically-allocated terminal backbuffer
static uint16_t term_backbuffer[VGA_HEIGHT*VGA_WIDTH] = {0};

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

size_t str_len(char const *const str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

/*
 * The `term` struct provides a nicer abstraction for writing to the VGA
 * buffer. TODO make the buffer better
 *
 * The buffer is circular: when writing to it, if the results of what we write
 * goes beyond the last row, the results are instead written at the start of
 * the buffer, overwriting (TODO: make it clear before writing) whatever was
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
    uint16_t *buff; // backbuffer, >= in size to the VGA buffer
};

void term_init(struct term *term, uint16_t *buff) {
    term->row = 0;
    term->column = 0;
    term->color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    // FIXME when handling backbuffer larger than VGA buffer
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
    size_t const index_y = (term->row_shift + y) % term->row_n;
    size_t const index_x = VGA_WIDTH + x;
    size_t const index = index_y + index_x;
    term->buff[index] = vga_entry(c, term->color);
}

void term_put_lf(struct term *term) {
    ++term->row;
    if (term->row >= VGA_HEIGHT) {
        term->row = 0;
    }
    term->column = 0;
    /* FIXME row_shift is modulo row_n so this conditional doesn't work,
     * ok for now since the backbuffer is the same height as the VGA buffer
    // if we have reached the bottom of the screen, start scrolling
    if (term->buff->row_shift >= VGA_HEIGHT) {
        ++term->buff->row_shift;
        if (term->buff->row_shift >= term->buff->row_n) {
            term->buff->row_shift = 0;
        }
    }
    // */
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

void term_putstr(struct term *term, char const *const str) {
    term_write(term, str, str_len(str));
}

/* TODO when copying from backbuffer to VGA buffer
void vga_refresh_line(struct term_buff *buff) {

}

void vga_refresh_all(struct term_buff *buff) {
    for (size_t i = 0; i < VGA_HEIGHT; ++i) {

    }
}
*/

void kernel_main(void) {
    struct term terminal;
    term_init(&terminal, term_backbuffer);
    vga_fill(vga_entry(' ', vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK)));
    term_putstr(&terminal, "Hello, kernel world!\n");
    vga_buffer[0] = vga_entry('x', vga_entry_color(VGA_COLOR_BLUE, VGA_COLOR_BLACK));
}
