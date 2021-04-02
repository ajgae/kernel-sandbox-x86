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

static inline uint8_t vga_entry_color(enum vga_color const fg, enum vga_color const bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char const uc, uint8_t const color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

size_t str_len(char const *const str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

static size_t const VGA_WIDTH = 80;
static size_t const VGA_HEIGHT = 25;

// defines the characteristics of the entries added to the terminal buffer
// row and column are display-relative
struct term_ctx {
    size_t row;
    size_t column;
    uint8_t color;
};

struct term_buff {
    size_t row_n; // number of rwos
    size_t row_shift; // first row that is displayed
    uint16_t *buff;
};

struct term {
    struct term_ctx* ctx;
    struct term_buff* buff;
};

void term_ctx_init(struct term_ctx *ctx) {
    ctx->column = 0;
    ctx->row = 0;
    ctx->color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void term_buff_init(struct term_buff *buff) {
    buff->row_n = VGA_HEIGHT; // FIXME when handling backbuffer larger than VGA buffer
    buff->row_shift = 0;
    for (size_t i = 0; i < buff->row_n * VGA_WIDTH; ++i) {
        buff->buff[i] = 0;
    }
}

// pointer to the memory-mapped VGA buffer
volatile uint16_t* vga_buffer; 

void term_init(struct term *term) {
    term_ctx_init(term->ctx);
    term_buff_init(term->buff);
}

void vga_fill(uint16_t vga_entry) {
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            size_t const index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry;
        }
    }
}

void vga_init(struct term *term) {
    vga_buffer = (volatile uint16_t*) 0xb8000;
    vga_fill(vga_entry(' ', term->ctx->color));
}

void term_ctx_set_color(struct term_ctx *ctx, uint8_t const color) {
    ctx->color = color;
}

void term_put_entry_at(char const c, struct term *term, size_t const x, size_t const y) {
    size_t const index_y = (term->buff->row_shift + y) % term->buff->row_n;
    size_t const index_x = VGA_WIDTH + x;
    size_t const index = index_y + index_x;
    term->buff->buff[index] = vga_entry(c, term->ctx->color);
}

void term_put_lf(struct term *term) {
    ++term->ctx->row;
    if (term->ctx->row >= VGA_HEIGHT) {
        term->ctx->row = 0;
    }
    term->ctx->column = 0;
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
        term_put_entry_at(c, term, term->ctx->column, term->ctx->row);
        ++term->ctx->column;
    }
    if (term->ctx->column >= VGA_WIDTH) {
        term->ctx->column = 0;
        // FIXME handle scrolling n whatnot, this only works because backbuffer is
        // the same height as the VGA buffer
        ++term->ctx->row;
        if (term->ctx->row >= VGA_HEIGHT) {
            term->ctx->row = 0;
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
    struct term_buff buff;
    struct term_ctx ctx;
    struct term terminal;
    terminal.buff = &buff;
    terminal.ctx = &ctx;
    term_init(&terminal);
    vga_init(&terminal);
    term_putstr(&terminal, "Hello, kernel world!\n");
    vga_buffer[0] = vga_entry('x', vga_entry_color(VGA_COLOR_BLUE, VGA_COLOR_BLACK));
}
