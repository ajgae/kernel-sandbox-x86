#pragma once

#include <stddef.h>
#include <stdint.h>

#include "printf.h"
#include "strings.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define TERM_BACKBUFF_SIZE 2*VGA_HEIGHT
#define TERM_PRINTF_BUFFER_SIZE 256

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

/*
 * The `term` struct provides a nicer abstraction for writing to the VGA
 * buffer. 
 *
 * The internal buffer is circular: when writing to it, if the results of
 * what we write goes beyond the last row, the results are instead written
 * at the start of the buffer, overwriting whatever was there.
 *
 * The buffer's rows map directly to the VGA buffer's rows. That is,
 * the backbuffer doesn't just contain a stream of characters. Instead, it
 * contains actual VGA screenspace, including e.g. empty space at the end of an
 * LF-terminated line.
 *
 * A subset of the buffer's rows, called a "screen", are copied to the VGA
 * buffer on refresh and are therefore displayed. The start of the screen starts
 * moving down when the bottom of the screen has been reached, effectively
 * implementing scrolling.
 *
 * IMPORTANT: for now, `column_n` should match the width of the actual VGA
 * buffer.
 */
struct term {
    size_t row; // current row (x)
    size_t column; // current column (y)
    size_t row_n; // total number of rows in backbuffer
    size_t column_n; // total number of columns in backbuffer
    size_t row_shift; // first row of the current screen (first row that is displayed)
    size_t row_screen; // current row relative to the current screen
    size_t row_screen_n; // height of a screen in rows
    uint8_t color; // current color
    uint16_t *buff; // backbuffer
};

void term_init(struct term *term);
void term_clear(struct term *term);
void term_set_color(struct term *term, uint8_t const color);
void term_put_char(struct term *term, char c);
void term_printf(struct term *const term, char const *const fmt, ...);
