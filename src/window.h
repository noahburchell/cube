#ifndef WINDOW_H
#define WINDOW_H

typedef struct window {
        int width, height;

        char *grid;
        char *prev;
        char *out;

        bool dirty;
} window;

const char *win_error(void);

bool win_guessed(void);

int init_win(window *win);

void destroy_win(window *win);

int draw_win(window *win);

void clear_win(const window *win);

int watch_resize(void);

int resize_pending(void);

void mark_resize(void);

int resize_win(window *win);

#endif
