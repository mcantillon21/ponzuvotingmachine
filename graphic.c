#include "graphic.h"
#include "gl.h"
#include "gpio.h"
#include "keyboard.h"
#include "printf.h"
#include "strings.h"
#include "malloc.h"
#include <stdbool.h>
#include <stdarg.h>

static color_t c_foreground = 0xFFFFFFFF;
static color_t c_background = 0xFF0e380e;
static unsigned int width = 1200;
static unsigned int height = 1000;

#define em(x) x * 10

unsigned int NUM_BUTTONS = 2;

unsigned int CREATE_VOTE = 0;
unsigned int NEW_VOTE = 1;

void main(void)
{
    gpio_init();
    keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);

    init_voting();
}



void init_voting(void) {
    gl_init(width, height, GL_DOUBLEBUFFER); 
    gl_clear(c_background);
 
    gl_draw_rect(em(5), em(5), em(110), em(11), GL_WHITE);
    
    gl_draw_string(em(7), em(7), "Ponzu Voting Machine", GL_BLACK);
    gl_draw_string(em(7), em(10), "Welcome in.", GL_BLACK);

    gl_draw_string(em(7), em(13), "Use the options below to cast your vote", GL_BLACK);
    gl_swap_buffer();
}
