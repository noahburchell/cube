#define _DEFAULT_SOURCE

#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <stdckdint.h>

#include "window.h"

#define FALLBACK_COLS 100
#define FALLBACK_ROWS 50

#define HOME     "\033[H"
#define HOME_LEN 3

static volatile sig_atomic_t resized;

static const char *win_err;

const char *win_error(void) {
        return win_err ? win_err : "unknown window error";
}

static int alloc_win(window *win, int w, int h) {
        if (w <= 0 || h <= 0) {
                win_err = "invalid window dimensions";
                return 1;
        }

        size_t n;
        if (ckd_mul(&n, (size_t)w, (size_t)h) || ckd_mul(&n, n, sizeof(cell_t))) {
                win_err = "window dimensions too large";
                return 1;
        }

        size_t body, obytes;
        if (ckd_mul(&body, (size_t)h, (size_t)w + 1) || ckd_add(&obytes, body, HOME_LEN)) {
                win_err = "window dimensions too large";
                return 1;
        }

        win->grid = malloc(n);
        if (!win->grid) {
                win_err = "out of memory";
                return 1;
        }

        win->out = malloc(obytes);
        if (!win->out) {
                win_err = "out of memory";
                free(win->grid);
                win->grid = NULL;
                return 1;
        }
        memcpy(win->out, HOME, HOME_LEN);

        win->width   = w;
        win->height  = h;
        win->out_len = HOME_LEN + body - 1;

        return 0;
}

static int query_size(int *w, int *h) {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0 || ws.ws_row == 0)
                return 1;

        *w = ws.ws_col;
        *h = ws.ws_row;
        return 0;
}

int init_win_auto(window *win) {
        int w, h;
        if (query_size(&w, &h)) {
                fprintf(stderr, "cube: failed to fetch window dimensions, falling back to %dx%d\n",
                        FALLBACK_COLS, FALLBACK_ROWS);
                w = FALLBACK_COLS;
                h = FALLBACK_ROWS;
        }

        return init_win(win, w, h);
}

int init_win(window *win, int w, int h) {
        if (alloc_win(win, w, h))
                return 1;

        clear_win(win);
        return 0;
}

void destroy_win(window *win) {
        free(win->grid);
        free(win->out);
        win->grid = NULL;
        win->out = NULL;
        win->width = 0;
        win->height = 0;
        win->out_len = 0;
}

int draw_win(window *win) {
        const int w = win->width;
        const int h = win->height;

        {
                const cell_t *restrict src = win->grid;
                char *restrict dst = win->out + HOME_LEN;

                for (int y = 0; y < h; y++) {
                        if (sizeof(cell_t) == 1) {
                                memcpy(dst, src, (size_t)w);
                                dst += w;
                        } else {
                                for (int x = 0; x < w; x++)
                                        dst[x] = src[x].c;
                                dst += w;
                        }
                        src += w;

                        *dst = '\n';
                        dst += (y != h - 1);
                }
        }

        const char *p = win->out;
        size_t left = win->out_len;

        for (;;) {
                ssize_t n = write(STDOUT_FILENO, p, left);
                if (n == (ssize_t)left)
                        return 0;

                if (n > 0) {
                        p += n;
                        left -= (size_t)n;
                        continue;
                }
                if (n == 0)
                        return -1;

                if (errno == EINTR)
                        continue;
                if (errno == EAGAIN) {
                        /* nonblocking stdout, park until the tty drains */
                        struct pollfd pfd = { .fd = STDOUT_FILENO, .events = POLLOUT };
                        if (poll(&pfd, 1, -1) < 0 && errno != EINTR)
                                return -1;
                        continue;
                }
                return -1;
        }
}

void clear_win(window *win) {
        size_t n = (size_t)win->width * (size_t)win->height;

        if (sizeof(cell_t) == 1)
                memset(win->grid, CELL_BLANK.c, n);
        else
                for (size_t i = 0; i < n; i++)
                        win->grid[i] = CELL_BLANK;
}

static void on_sigwinch(int sig) {
        (void)sig;
        resized = 1;
}

int watch_resize(void) {
        struct sigaction sa = { .sa_handler = on_sigwinch, .sa_flags = SA_RESTART };
        sigemptyset(&sa.sa_mask);

        return sigaction(SIGWINCH, &sa, NULL) != 0;
}

int resize_pending(void) {
        return resized;
}

void mark_resize(void) {
        resized = 1;
}

int resize_win(window *win) {
        /* cleared before the query so SIGWINCH racing the ioctl leaves the flag set and gets picked up next frame */
        resized = 0;

        int w, h;
        if (query_size(&w, &h))
                return 0;

        if (w == win->width && h == win->height)
                return 0;

        window next;
        if (alloc_win(&next, w, h))
                return 1;

        free(win->grid);
        free(win->out);
        *win = next;

        clear_win(win);
        return 0;
}
