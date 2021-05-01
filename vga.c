#include "vga.h"

// pointer to the memory-mapped VGA buffer
static volatile uint16_t* vga_buffer = (volatile uint16_t*) 0xb8000; 
// pointer to the (for now) statically-allocated terminal backbuffer
static uint16_t term_backbuffer[2*VGA_HEIGHT*VGA_WIDTH];

static void vga_fill(uint16_t vga_entry) {
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            size_t const index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry;
        }
    }
}

static inline uint8_t vga_entry_color(enum vga_color const fg, enum vga_color const bg) {
    return fg | bg << 4;
}
static inline uint16_t vga_entry(unsigned char const c, uint8_t const color) {
    return (uint16_t) c | (uint16_t) color << 8;
}
static inline void vga_clear() {
    vga_fill(vga_entry(' ', vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK)));
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
static void term_put_lf(struct term *term) {
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
    // `row_screen` is an index and therefore should always be strictly smaller than
    // `row_screen_n`, hence the `-1`
    if (term->row_screen < term->row_screen_n - 1) {
        // no need to scroll, just write below
        ++term->row_screen;
    } else {
        // scroll screen down
        ++term->row_shift;
        term->row_shift %= term->row_n;
    }
}

/*
 * Write a character at position (x, y) relative to the current screen.
 * This will do nothing if the entry is a non-printable ASCII character.
 * Those should be handled separately by the caller.
 */
static void term_put_entry_at(struct term *term, char const c, size_t const x, size_t const y) {
    // discard non-printable ASCII characters
    if (c >= 32 && c != 127) {
        size_t const index = 
            ((term->row_shift + y) % term->row_n) * term->column_n + x;
        term->buff[index] = vga_entry(c, term->color);
    }
}

static void term_write(struct term *term, char const *const data, size_t const size) {
    for (size_t i = 0; i < size; ++i) {
        term_put_char(term, data[i]);
    }
}

/*
 * Write `str` to `term`.
 */
static void term_put_str(struct term *term, char const *const str) {
    term_write(term, str, strlen(str));
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
        term_put_entry_at(term, c, term->column, term->row_screen);
        ++term->column;
    }
    if (term->column >= term->column_n) {
        term_put_lf(term);
    }
}

/*
 * Copy the contents of the current screen of the backbuffer to the VGA
 * buffer for display.
 */
static void vga_refresh_all(struct term const *const term) {
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

void term_init(struct term *term) {
    term->row = 0;
    term->column = 0;
    term->row_n = TERM_BACKBUFF_SIZE; 
    term->column_n = VGA_WIDTH;
    term->row_shift = 0;
    term->row_screen = 0;
    term->row_screen_n = VGA_HEIGHT;
    term->color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    term->buff = term_backbuffer;
    // make sure the buffer is zero-filled
    for (size_t i = 0; i < term->row_n * term->column_n; ++i) {
        term->buff[i] = 0;
    }
}

void term_clear(struct term *term) {
    for (size_t i = 0; i < term->column_n * term->row_n; ++i) {
        term->buff[i] = 0;
    }
    vga_refresh_all(term);
}

/*
 * Set the color that the terminal will write characters in.
 */
void term_set_color(struct term *term, enum vga_color const fg, enum vga_color const bg) {
    term->color = vga_entry_color(fg, bg);
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
    vga_refresh_all(term);
}

