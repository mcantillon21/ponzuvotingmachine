#include "gl.h"
#include "font.h"
#include "strings.h"

void gl_init(unsigned int width, unsigned int height, gl_mode_t mode)
{
    fb_init(width, height, 4, mode);    // use 32-bit depth always for graphics library
}

void gl_swap_buffer(void)
{
    fb_swap_buffer();
}

unsigned int gl_get_width(void)
{
    return fb_get_width();
}

unsigned int gl_get_height(void)
{
    return fb_get_height();
}

// returns 0xFFrrggbb.
color_t gl_color(unsigned char r, unsigned char g, unsigned char b)
{
    return (color_t) 0xFF000000 | 
           ((color_t) r << 16)  |
           ((color_t) g << 8)   |
           ((color_t) b);
}

void gl_clear(color_t c)
{
    unsigned int per_row = fb_get_pitch() / fb_get_depth();
    unsigned int (*im)[per_row] = fb_get_draw_buffer();
    
    for (int y = 0; y < gl_get_height(); y++) {
        for (int x = 0; x < gl_get_width(); x++) {
            im[y][x] = c;
        }
    }
}

void gl_draw_pixel(int x, int y, color_t c)
{
    // Check bounds
    if (x >= gl_get_width() || y >= gl_get_height() || x < 0 || y < 0) return;

    unsigned int per_row = fb_get_pitch() / fb_get_depth();
    unsigned int (*im)[per_row] = fb_get_draw_buffer();
    im[y][x] = c;
}

color_t gl_read_pixel(int x, int y)
{
    // Check bounds
    if (x >= gl_get_width() || y >= gl_get_height() || x < 0 || y < 0) return 0;
    
    unsigned int per_row = fb_get_pitch() / fb_get_depth();
    unsigned int (*im)[per_row] = fb_get_draw_buffer();
    return im[y][x];
}

void gl_draw_rect(int x, int y, int w, int h, color_t c)
{
    for (int dy = y; dy < y + h; dy++) {
        for (int dx = x; dx < x + w; dx++) {
            gl_draw_pixel(dx, dy, c);
        }
    } 
}

void gl_draw_char(int x, int y, char ch, color_t c)
{
    // Load glyph to char_map.
    unsigned int glyph_size = font_get_glyph_size();
    unsigned char buf[glyph_size];
    if (!font_get_glyph(ch, buf, glyph_size)) return;
    unsigned char (*char_map)[font_get_glyph_width()] = (void *) buf;

    // Copy char_map to frame buffer, with position x, y being upper left corner
    // of the char_map.
    for (int dy = 0; dy < font_get_glyph_height(); dy++) {
        for (int dx = 0; dx < font_get_glyph_width(); dx++) {
            if (char_map[dy][dx] == 0xff) gl_draw_pixel(x + dx, y + dy, c | 0xff000000);
        }
    }
}

void gl_draw_string(int x, int y, const char* str, color_t c)
{
    int xoffset = 0;
    int fontwidth = font_get_glyph_width();
    // Draw all chars in str starting from position x, y 
    for (int i = 0; i < strlen(str); i++) {
        gl_draw_char(x + xoffset, y, str[i], c);
        xoffset += fontwidth; 
    }
}

unsigned int gl_get_char_height(void)
{
    return font_get_glyph_height();
}

unsigned int gl_get_char_width(void)
{
    return font_get_glyph_width(); 
}
