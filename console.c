#include "console.h"
#include "gl.h"
#include "printf.h"
#include "strings.h"
#include "malloc.h"
#include <stdbool.h>
#include <stdarg.h>

// Please use this amount of space between console rows
static const int LINE_SPACING = 5;
static int line_height;
static color_t c_foreground;
static color_t c_background;
static unsigned int c_nrows;
static unsigned int c_ncols;
static int xloc;
static int yloc;

// The number of times that the cursor has horizontally wrapped since the last time \n was inserted.
static int line_number;
// Denotes the row in frame that stores the line to be displayed at the top of console.
static int starting_row;
// Buffer to store all characters to be displayed in the shell.
static char* shell_frame;


#define MAX_OUTPUT_LEN 1024

static void process_char(char ch);
void advance_cursor(void);
void retract_cursor(void);

void console_init(unsigned int nrows, unsigned int ncols, color_t foreground, color_t background)
{
    line_height = gl_get_char_height() + LINE_SPACING;
    c_foreground = foreground;
    c_background = background;
    c_nrows = nrows;
    c_ncols = ncols;
    xloc = 0;
    yloc = 0;
    shell_frame = malloc(c_nrows * c_ncols);
    starting_row = 0;

    gl_init(c_ncols * gl_get_char_width(), c_nrows * line_height, GL_DOUBLEBUFFER); 
    console_clear();
}

/*
 * Clears the console without displaying buffer
 */
void _console_clear(void)
{
    gl_clear(c_background);
    starting_row = 0;
    xloc = 0;
    yloc = 0;
    memset(shell_frame, '\0', c_ncols * c_nrows);
}

/*
 * Clears the console and display the cleared console 
 */
void console_clear(void)
{    
    _console_clear();
    gl_swap_buffer();
}

/*
 * Displays the data stored in shell_frame onto console.
 */
void console_display(void) {
    char (*frame)[c_ncols] = (void *) shell_frame;

    gl_clear(c_background);

    int y_offset = 0;
    for (int row = 0; row < c_nrows; row++) {
        gl_draw_string(0, y_offset, &frame[(starting_row + row) % c_nrows][0], c_foreground);
        y_offset += line_height;
    }

    gl_swap_buffer();
}

/*
 * Printf to console
 */
int console_printf(const char *format, ...)
{
    // Get arguments and store formatted string in buf
    char buf[MAX_OUTPUT_LEN];
    va_list args;
    va_start(args, format);
    unsigned int printed_len = vsnprintf(buf, MAX_OUTPUT_LEN, format, args);
    va_end(args);

    // Process each char and display
    size_t bufiter = 0;
    while (buf[bufiter] != '\0') {
        process_char(buf[bufiter]);
        bufiter++;
    }
    console_display();

    return printed_len;
}

/*
 * Handles Verticle Scrolling.
 * Checks if yloc has fully wrapped around back to the starting row. If it is, shift the starting row of
 * Shell frame by 1, and clear the row that yloc is currently at.
 */
void scroll(void) {    
    // If yloc has fully wrapped around the container, append starting row by 1.
    if (starting_row == yloc % c_nrows) {
        starting_row++;
        starting_row %= c_nrows;
        yloc %= c_nrows;

        // Clear the row
        char (*frame)[c_ncols] = (void *) shell_frame;
        memset(frame[yloc], '\0', c_ncols);
    }
}

/*
 * Moves the cursor forward by one character
 */
void advance_cursor(void) {
    xloc++;
    // If xloc has overflown the row, wrap it around.
    if (xloc == c_ncols) {
        xloc = 0;
        yloc++;
        line_number++;
        scroll();
    }
}

/*
 * Move the cursor back by one character
 */
void retract_cursor(void) {
    if (xloc != 0) {
        xloc--;
    } else if (line_number) {
        xloc = c_ncols - 1;
        yloc--;
        line_number--;
    }
}

/*
 * Adds a new line underneath the cursor to console
 */
void new_line(void) {
    xloc = 0;
    yloc++;
    line_number = 0;
    scroll();
}

/*
 * Helper function: Given a char, add it to shell_frame, while ensuring that
 * horizontal wrapping and vertical scrolling is applied.
 */
static void process_char(char ch)
{
    char (*frame)[c_ncols] = (void *) shell_frame;
    switch (ch) {
        case '\b': // BACKSPACE
            retract_cursor();
            frame[yloc][xloc] = '\0';
            break;
        case '\n': // NEWLINE
            new_line();
            break;
        case '\f': // CLEAR
            _console_clear();
            break;
        default: // INSERT 
            frame[yloc][xloc] = ch;
            advance_cursor();
    }
}
