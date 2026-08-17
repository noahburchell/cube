#ifndef WINDOW_H
#define WINDOW_H

#include <stddef.h>

typedef struct cell_t {
        char c;
        /* color later potentially */
} cell_t;

constexpr cell_t CELL_BLANK = { .c = ' ' }; // constexpr instead of const for when color is added if i need to to switch on a glyph or size an array by one 

typedef struct window {
        int width, height;
        cell_t *grid;

        char *out;
        size_t out_len;
} window;

int init_win_auto(window *win);
int init_win(window *win, int w, int h);

void destroy_win(window *win);

int draw_win(window *win);

int clear_win(window *win);

int watch_resize(void);

int resize_pending(void);

int resize_win(window *win);

#endif
