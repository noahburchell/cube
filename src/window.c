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

#define MOVE_MAX 24

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

        size_t obytes;
        if (ckd_add(&obytes, (size_t)w, MOVE_MAX) || ckd_mul(&obytes, obytes, (size_t)h)) {
                win_err = "window dimensions too large";
                return 1;
        }

        win->grid = malloc(n);
        win->prev = malloc(n);
        win->out  = malloc(obytes);

        if (!win->grid || !win->prev || !win->out) {
                free(win->grid);
                free(win->prev);
                free(win->out);
                win->grid = NULL;
                win->prev = NULL;
                win->out  = NULL;
                win_err = "out of memory";
                return 1;
        }

        win->width   = w;
        win->height  = h;
        win->out_cap = obytes;
        win->dirty   = 1;

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
        free(win->prev);
        free(win->out);
        win->grid = NULL;
        win->prev = NULL;
        win->out = NULL;
        win->width = 0;
        win->height = 0;
        win->out_cap = 0;
        win->dirty = 1;
}

static char *put_uint(char *p, unsigned v) {
        char d[10];
        int n = 0;

        do {
                d[n++] = (char)('0' + v % 10u);
                v /= 10u;
        } while (v);

        while (n)
                *p++ = d[--n];

        return p;
}

static char *put_move(char *p, int y, int x) {
        *p++ = '\033';
        *p++ = '[';
        p = put_uint(p, (unsigned)y + 1u);
        *p++ = ';';
        p = put_uint(p, (unsigned)x + 1u);
        *p++ = 'H';

        return p;
}

int draw_win(window *win) {
        const int w = win->width;
        const int h = win->height;
        const int all = win->dirty;

        const cell_t *restrict grid = win->grid;
        cell_t *restrict prev = win->prev;
        char *o = win->out;

        for (int y = 0; y < h; y++) {
                const size_t off = (size_t)y * (size_t)w;
                const cell_t *restrict cur = grid + off;
                cell_t *restrict old = prev + off;

                int a = 0, b = w - 1;

                if (!all) {
                        if (memcmp(cur, old, (size_t)w * sizeof *cur) == 0)
                                continue;

                        while (a < w && cur[a].c == old[a].c)
                                a++;
                        if (a == w)
                                continue;

                        while (cur[b].c == old[b].c)
                                b--;
                }

                const size_t n = (size_t)(b - a + 1);

                o = put_move(o, y, a);

                if (sizeof(cell_t) == 1)
                        memcpy(o, cur + a, n);
                else
                        for (size_t i = 0; i < n; i++)
                                o[i] = cur[(size_t)a + i].c;
                o += n;

                memcpy(old + a, cur + a, n * sizeof *cur);
        }

        win->dirty = 0;

        const char *p = win->out;
        size_t left = (size_t)(o - win->out);

        while (left) {
                ssize_t k = write(STDOUT_FILENO, p, left);
                if (k == (ssize_t)left)
                        return 0;

                if (k > 0) {
                        p += k;
                        left -= (size_t)k;
                        continue;
                }
                if (k == 0)
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

        return 0;
}

void clear_win(const window *win) {
        cell_t *restrict grid = win->grid;
        const size_t n = (size_t)win->width * (size_t)win->height;

        if (sizeof(cell_t) == 1)
                memset(grid, CELL_BLANK.c, n);
        else
                for (size_t i = 0; i < n; i++)
                        grid[i] = CELL_BLANK;
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

        win->dirty = 1;

        int w, h;
        if (query_size(&w, &h))
                return 0;

        if (w == win->width && h == win->height)
                return 0;

        window next;
        if (alloc_win(&next, w, h))
                return 1;

        free(win->grid);
        free(win->prev);
        free(win->out);
        *win = next;

        clear_win(win);
        return 0;
}
