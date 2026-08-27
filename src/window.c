#define _DEFAULT_SOURCE

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdckdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "window.h"

constexpr int FALLBACK_COLS = 100;
constexpr int FALLBACK_ROWS = 50;

constexpr int MAX_COLS = 4096;
constexpr int MAX_ROWS = 4096;

constexpr size_t MOVE_MAX = 24;

static volatile sig_atomic_t resized;

static const char *win_err;
static bool guessed;

const char *win_error(void) {
        return win_err ? win_err : "unknown window error";
}

static int alloc_win(window *win, int w, int h) {
        if (w <= 0 || h <= 0) {
                win_err = "invalid window dimensions";
                return 1;
        }

        size_t cells, outbytes, total;

        if (ckd_mul(&cells, (size_t)w, (size_t)h)
         || ckd_add(&outbytes, (size_t)w, MOVE_MAX)
         || ckd_mul(&outbytes, outbytes, (size_t)h)
         || ckd_mul(&total, cells, (size_t)2)
         || ckd_add(&total, total, outbytes)) {
                win_err = "window dimensions too large";
                return 1;
        }

        char *mem = malloc(total);
        if (!mem) {
                win_err = "out of memory";
                return 1;
        }

        win->width  = w;
        win->height = h;
        win->out    = mem;
        win->grid   = mem + outbytes;
        win->prev   = win->grid + cells;
        win->dirty  = true;

        return 0;
}

static int query_size(int *w, int *h) {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0 || ws.ws_row == 0)
                return 1;

        *w = ws.ws_col > MAX_COLS ? MAX_COLS : ws.ws_col;
        *h = ws.ws_row > MAX_ROWS ? MAX_ROWS : ws.ws_row;
        return 0;
}

bool win_guessed(void) {
        return guessed;
}

int init_win(window *win) {
        int w, h;
        if (query_size(&w, &h)) {
                guessed = true;
                w = FALLBACK_COLS;
                h = FALLBACK_ROWS;
        }

        if (alloc_win(win, w, h))
                return 1;

        clear_win(win);
        return 0;
}

void destroy_win(window *win) {
        free(win->out);
        *win = (window){};
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
        const bool all = win->dirty;

        const char *restrict grid = win->grid;
        const char *restrict prev = win->prev;
        char *o = win->out;

        for (int y = 0; y < h; y++) {
                const size_t off = (size_t)y * (size_t)w;
                const char *restrict cur = grid + off;
                const char *restrict old = prev + off;

                int a = 0, b = w - 1;

                if (!all) {
                        if (memcmp(cur, old, (size_t)w) == 0)
                                continue;

                        while (cur[a] == old[a])
                                a++;
                        while (cur[b] == old[b])
                                b--;
                }

                const size_t n = (size_t)(b - a + 1);

                o = put_move(o, y, a);
                memcpy(o, cur + a, n);
                o += n;
        }

        win->dirty = false;

        char *drawn = win->grid;
        win->grid = win->prev;
        win->prev = drawn;

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
        memset(win->grid, ' ', (size_t)win->width * (size_t)win->height);
}

static void on_sigwinch([[maybe_unused]] int sig) {
        resized = 1;
}

int watch_resize(void) {
        struct sigaction sa = { .sa_handler = on_sigwinch, .sa_flags = SA_RESTART };
        sigemptyset(&sa.sa_mask);

        return sigaction(SIGWINCH, &sa, nullptr) != 0;
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

        win->dirty = true;

        int w, h;
        if (query_size(&w, &h))
                return 0;

        if (w == win->width && h == win->height)
                return 0;

        window next;
        if (alloc_win(&next, w, h))
                return 1;

        free(win->out);
        *win = next;

        clear_win(win);
        return 0;
}
