#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

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

struct term_ctx {
    size_t row;
    size_t column;
    uint8_t color;
};

void term_ctx_init(struct term_ctx *ctx) {
    ctx->column = 0;
    ctx->row = 0;
    ctx->color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

uint16_t* term_buffer;

void term_init(struct term_ctx *ctx) {
    term_ctx_init(ctx);
    term_buffer = (uint16_t*) 0xb8000;
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            size_t const index = y * VGA_WIDTH + x;
            term_buffer[index] = vga_entry(' ', ctx->color);
        }
    }
}

void term_ctx_set_color(struct term_ctx *ctx, uint8_t const color) {
    ctx->color = color;
}

void term_put_entry_at(char const c, uint8_t const color, size_t const x, size_t const y) {
    size_t const index = y * VGA_WIDTH + x;
    term_buffer[index] = vga_entry(c, color);
}

void term_put_lf(struct term_ctx *ctx) {
    ++ctx->row;
    if (ctx->row >= VGA_HEIGHT) {
        ctx->row = 0;
    }
    ctx->column = 0;
}

void term_put_char(struct term_ctx *ctx, char c) {
    switch (c) { // handle non-print chars
    case '\n': 
        term_put_lf(ctx);
        break;
    default:
        term_put_entry_at(c, ctx->color, ctx->column, ctx->row);
        ++ctx->column;
    }
    if (ctx->column >= VGA_WIDTH) {
        ctx->column = 0;
        ++ctx->row;
        if (ctx->row >= VGA_HEIGHT) {
            ctx->row = 0;
        }
    }
}

void term_write(struct term_ctx *ctx, char const *const data, size_t const size) {
    for (size_t i = 0; i < size; ++i) {
        term_put_char(ctx, data[i]);
    }
}

void term_writestring(struct term_ctx *ctx, char const *const str) {
    term_write(ctx, str, str_len(str));
}

void kernel_main(void) {
    struct term_ctx ctx;
    term_init(&ctx);
    term_writestring(&ctx, "Hello,\nkernel world!\n");
}
