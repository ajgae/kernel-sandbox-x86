#include <stddef.h>
#include <stdint.h>

#include "strings.h"
#include "printf.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
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

// pointer to the memory-mapped VGA buffer
volatile uint16_t* vga_buffer = (volatile uint16_t*) 0xb8000; 
// pointer to the (for now) statically-allocated terminal backbuffer
static uint16_t term_backbuffer[2*VGA_HEIGHT*VGA_WIDTH];

static inline uint8_t vga_entry_color(enum vga_color const fg, enum vga_color const bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char const c, uint8_t const color) {
    return (uint16_t) c | (uint16_t) color << 8;
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

void term_init(struct term *term, uint16_t *buff) {
    term->row = 0;
    term->column = 0;
    term->row_n = 2*VGA_HEIGHT; 
    term->column_n = VGA_WIDTH;
    term->row_shift = 0;
    term->row_screen = 0;
    term->row_screen_n = VGA_HEIGHT;
    term->color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    term->buff = buff;
    // make sure the buffer is zero-filled
    for (size_t i = 0; i < term->row_n * term->column_n; ++i) {
        term->buff[i] = 0;
    }
}

/*
 * Set the color that the terminal will write characters in.
 */
void term_set_color(struct term *term, uint8_t const color) {
    term->color = color;
}

/*
 * Write a character at position (x, y) relative to the current screen.
 * This will do nothing if the entry is a non-printable ASCII character.
 * Those should be handled separately by the caller.
 */
void term_put_entry_at(char const c, struct term *term, size_t const x, size_t const y) {
    // discard non-printable ASCII characters
    if (c >= 32 && c != 127) {
        size_t const index = 
            ((term->row_shift + y) % term->row_n) * term->column_n + x;
        term->buff[index] = vga_entry(c, term->color);
    }
}

/*
 * Write a line feed, force-starting a new row.
 *
 * NOTE: a function that would do the equivalent of <C-l> in VT-100
 * should reset `row_screen` to 0 so that it doesn't grow infinitely
 *
 * TODO: 
 * - option to not clear the row? sometimes handy (e.g. curses-like behavior)
 */
void term_put_lf(struct term *term) {
    // always increase `row` (modulo `row_n`), it is absolute (not relative)
    ++term->row;
    term->row %= term->row_n;
    // a new line always starts at column 0
    term->column = 0;

    // clear the new row
    for (size_t i = 0; i < term->column_n; ++i) {
        term->buff[term->row * term->column_n + i] = 
            vga_entry(' ', vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    }

    // update screen-relative position
    if (term->row_screen < term->row_screen_n) {
        // no need to scroll, just write below
        ++term->row_screen;
    } else {
        // scroll screen down
        ++term->row_shift;
        term->row_shift %= term->row_n;
    }
}

/*
 * Write `c` to `term`.
 */
void term_put_char(struct term *term, char c) {
    switch (c) { // handle non-print chars
    case '\n': 
        term_put_lf(term);
        return;
    default:
        term_put_entry_at(c, term, term->column, term->row_screen);
        ++term->column;
    }
    if (term->column >= term->column_n) {
        term_put_lf(term);
    }
}

void term_write(struct term *term, char const *const data, size_t const size) {
    for (size_t i = 0; i < size; ++i) {
        term_put_char(term, data[i]);
    }
}

/*
 * Write `str` to `term`.
 */
void term_put_str(struct term *term, char const *const str) {
    term_write(term, str, strlen(str));
}

/*
 * Copy the contents of the current screen of the backbuffer to the VGA
 * buffer for display.
 */
void vga_refresh_all(struct term const *const term) {
    for (size_t i = 0; i < term->row_screen_n; ++i) {
        for (size_t j = 0; j < term->column_n; ++j) {
            size_t vga_index = i * VGA_WIDTH + j;
            size_t term_index = 
                ((i + term->row_shift) % term->row_n)
                * term->column_n + j;
            vga_buffer[vga_index] = term->buff[term_index];
        }
    }
}

/*
 * Print a formatted string to the VGA terminal, cropping the formatted
 * string to `TERM_PRINTF_BUFFER_SIZE` if it is longer than that.
 */
void term_printf(struct term *const term, char const *const fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[TERM_PRINTF_BUFFER_SIZE] = {0};
    vsnprintf(buf, sizeof(buf)-1, fmt, ap);
    term_put_str(term, buf);
}

void kernel_main(void) {
    vga_clear();
    struct term term;
    term_init(&term, term_backbuffer);
    /*
    snprintf(buf, sizeof(buf)-1, "very long string that is probably longer than 80 characters long at least i hope it is");
    // */
    /*
    snprintf(buf, sizeof(buf)-1, "hello, world!\n\
hex value of %d is %x, which seems to work!\n", 200, 200);
    // */
    /*
    term_put_str(&term, buf);
    // */
    for (size_t i = 0; i < 128; ++i) {
        term_printf(&term, "number %d\n", i);
    }
    // */
    vga_refresh_all(&term);
    // */
}

