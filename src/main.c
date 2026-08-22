#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include "shapes.h"
#include "window.h"

#define FPS         60
#define FRAME_NS    (1000000000L / FPS)

#define CAM_DIST    4.0f
#define CELL_ASPECT 2.0f  
#define FILL        0.85f 

#define ALT_ON    "\033[?1049h"
#define ALT_OFF   "\033[?1049l"
#define CURS_OFF  "\033[?25l"
#define CURS_ON   "\033[?25h"

typedef struct mat3 {
        vec3 cx, cy, cz;
} mat3;

// whole mesh is a single allocation instead of parallel arrays
typedef struct pvert {
        float x, y;
} pvert;

static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t caught;

static void emit(const char *s) {
        size_t n = strlen(s);
        while (n) {
                ssize_t k = write(STDOUT_FILENO, s, n);
                if (k <= 0) {
                        if (k < 0 && errno == EINTR)
                                continue;
                        return;
                }
                s += k;
                n -= (size_t)k;
        }
}

static struct termios saved_term;
static int term_saved;

static void term_setup(void) {
        if (tcgetattr(STDIN_FILENO, &saved_term) == 0) {
                struct termios raw = saved_term;
                raw.c_lflag &= ~(unsigned)(ICANON | ECHO);
                raw.c_cc[VMIN]  = 0;   /* reads never block so keys can be polled */
                raw.c_cc[VTIME] = 0;
                term_saved = tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
        }
        emit(ALT_ON CURS_OFF);
}

static void term_restore(void) {
        emit(CURS_ON ALT_OFF);
        if (term_saved)
                tcsetattr(STDIN_FILENO, TCSANOW, &saved_term);
}

static void install(int sig, void (*handler)(int), int flags) {
        struct sigaction sa = { .sa_handler = handler, .sa_flags = flags };
        sigfillset(&sa.sa_mask);
        sigaction(sig, &sa, NULL);
}

static void on_quit(int sig) {
        caught = sig;
        running = 0;
}

static void on_stop(int sig) {
        term_restore();
        install(sig, SIG_DFL, 0);
        raise(sig);
}

static void on_cont(int sig) {
        (void)sig;
        term_setup();
        mark_resize();
        install(SIGTSTP, on_stop, 0);
}

static mat3 rotation(float ax, float ay, float az) {
        const float sa = sinf(ax), ca = cosf(ax);
        const float sb = sinf(ay), cb = cosf(ay);
        const float sc = sinf(az), cc = cosf(az);

        return (mat3){
                .cx = { cb * cc,                cb * sc,                -sb     },
                .cy = { sa * sb * cc - ca * sc, sa * sb * sc + ca * cc, sa * cb },
                .cz = { ca * sb * cc + sa * sc, ca * sb * sc - sa * cc, ca * cb },
        };
}

static float fit_scale(const window *win, float r) {
        const float m = sqrtf(CAM_DIST * CAM_DIST - r * r) / r;

        float by_w = FILL * (float)win->width  * 0.5f * m / CELL_ASPECT;
        float by_h = FILL * (float)win->height * 0.5f * m;

        return by_w < by_h ? by_w : by_h;
}

static inline void fill_span(cell_t *restrict row, float xl, float xr, float wmax, char c) {
        if (xl > xr) {
                const float s = xl;
                xl = xr;
                xr = s;
        }

        if (xl < 0.0f) xl = 0.0f;
        if (xr > wmax) xr = wmax;
        if (!(xl <= xr))
                return;

        int a = (int)xl;
        a += ((float)a < xl);

        const int b = (int)xr;
        if (a > b)
                return;

        if (sizeof(cell_t) == 1)
                memset(row + a, (unsigned char)c, (size_t)(b - a + 1));
        else
                for (int x = a; x <= b; x++)
                        row[x].c = c;
}

static void fill_tri(const window *win, const float fx[static 3], const float fy[static 3], char c) {
        float x0 = fx[0], y0 = fy[0];
        float x1 = fx[1], y1 = fy[1];
        float x2 = fx[2], y2 = fy[2];
        float s;

        if (y0 > y1) { s = x0; x0 = x1; x1 = s; s = y0; y0 = y1; y1 = s; }
        if (y1 > y2) { s = x1; x1 = x2; x2 = s; s = y1; y1 = y2; y2 = s; }
        if (y0 > y1) { s = x0; x0 = x1; x1 = s; s = y0; y0 = y1; y1 = s; }

        const float dfull = y2 - y0;
        if (!(dfull > 0.0f))
                return;

        const int w = win->width;
        const float wmax = (float)(w - 1);

        float ytf = ceilf(y0);
        float ybf = floorf(y2);
        if (ytf < 0.0f) ytf = 0.0f;
        if (ybf > (float)(win->height - 1)) ybf = (float)(win->height - 1);
        if (!(ytf <= ybf))
                return;

        const int ytop = (int)ytf;
        const int ybot = (int)ybf;

        const float mfull = (x2 - x0) / dfull;
        const float dup = y1 - y0;
        const float dlo = y2 - y1;

        int ymid;
        if (!(dup > 0.0f))
                ymid = ytop - 1;
        else if (!(dlo > 0.0f))
                ymid = ybot;
        else {
                float ymf = floorf(y1);
                if (ymf < (float)(ytop - 1)) ymf = (float)(ytop - 1);
                if (ymf > (float)ybot)       ymf = (float)ybot;
                ymid = (int)ymf;
        }

        cell_t *row = win->grid + (size_t)ytop * (size_t)w;
        int y = ytop;

        if (ymid >= ytop) {
                const float mup = (x1 - x0) / dup;

                for (; y <= ymid; y++, row += w) {
                        const float d = (float)y - y0;
                        fill_span(row, x0 + d * mfull, x0 + d * mup, wmax, c);
                }
        }

        if (y <= ybot) {
                const float mlo = (x2 - x1) / dlo;

                for (; y <= ybot; y++, row += w) {
                        const float yf = (float)y;
                        fill_span(row, x0 + (yf - y0) * mfull, x1 + (yf - y1) * mlo, wmax, c);
                }
        }
}

static void draw_mesh(const window *win, const mesh *m, float radius, pvert *restrict pv, float t) {
        const float scale = fit_scale(win, radius);

        const float cx = (float)(win->width  - 1) * 0.5f;
        const float cy = (float)(win->height - 1) * 0.5f;

        const mat3 rot = rotation(t * 0.70f, t * 1.10f, t * 0.35f);

        const float xx = rot.cx.x, xy = rot.cx.y, xz = rot.cx.z;
        const float yx = rot.cy.x, yy = rot.cy.y, yz = rot.cy.z;
        const float zx = rot.cz.x, zy = rot.cz.y, zz = rot.cz.z;

        const vec3 *restrict verts = m->verts;

        for (size_t i = 0; i < m->nverts; i++) {
                const float vx = verts[i].x, vy = verts[i].y, vz = verts[i].z;
                const float k = scale / (xz * vx + yz * vy + zz * vz + CAM_DIST);

                pv[i].x = cx + (xx * vx + yx * vy + zx * vz) * k * CELL_ASPECT;
                pv[i].y = cy + (xy * vx + yy * vy + zy * vz) * k;
        }

        const tri *restrict tris = m->tris;

        for (size_t f = 0; f < m->ntris; f++) {
                const uint32_t *v = tris[f].v;

                const float ax = pv[v[0]].x, ay = pv[v[0]].y;
                const float bx = pv[v[1]].x, by = pv[v[1]].y;
                const float dx = pv[v[2]].x, dy = pv[v[2]].y;

                if ((bx - ax) * (dy - ay) - (by - ay) * (dx - ax) >= 0.0f)
                        continue;

                const float qx[3] = { ax, bx, dx };
                const float qy[3] = { ay, by, dy };

                fill_tri(win, qx, qy, tris[f].c);
        }
}

static int quit_requested(void) {
        if (!term_saved)
                return 0;
        char buf[16];
        ssize_t n = read(STDIN_FILENO, buf, sizeof buf);

        if (n == 1 && buf[0] == 27)
                return 1;

        for (ssize_t i = 0; i < n; i++)
                if (buf[i] == 'q' || buf[i] == 'Q')
                        return 1;

        return 0;
}

static int parse_dim(const char *s, int *out) {
        char *end;

        errno = 0;
        long v = strtol(s, &end, 10);

        if (end == s || *end != '\0' || errno == ERANGE || v <= 0 || v > INT_MAX)
                return 1;

        *out = (int)v;
        return 0;
}

static void usage(FILE *out, const char *prog) {
        fprintf(out, "usage: %s [--shape]\n\nshapes:\n", prog);

        for (size_t i = 0; i < nshapes; i++)
                fprintf(out, "  --%s%s\n", shapes[i].name, i == 0 ? " (default)" : "");
}

int main(int argc, char **argv) {
        window win;

        const mesh *shape = &shapes[0];
        int dim[2], ndim = 0;

        for (int i = 1; i < argc; i++) {
                if (strncmp(argv[i], "--", 2) == 0) {
                        if (strcmp(argv[i], "--help") == 0) {
                                usage(stdout, argv[0]);
                                return 0;
                        }

                        const mesh *m = shape_find(argv[i] + 2);
                        if (!m) {
                                fprintf(stderr, "cube: unknown option '%s'\n\n", argv[i]);
                                usage(stderr, argv[0]);
                                return 1;
                        }
                        shape = m;
                } else if (ndim < 2) {
                        if (parse_dim(argv[i], &dim[ndim])) {
                                fprintf(stderr, "cube: invalid dimension '%s'\n\n", argv[i]);
                                usage(stderr, argv[0]);
                                return 1;
                        }
                        ndim++;
                } else {
                        fprintf(stderr, "cube: unexpected argument '%s'\n\n", argv[i]);
                        usage(stderr, argv[0]);
                        return 1;
                }
        }

        if (ndim == 1) {
                fprintf(stderr, "cube: both a width and a height are required\n\n");
                usage(stderr, argv[0]);
                return 1;
        }

        if (!isatty(STDOUT_FILENO)) {
                fprintf(stderr, "cube: stdout is not a terminal\n");
                return 1;
        }

        const float radius = mesh_radius(shape);

        if (!(radius > 0.0f)) {
                fprintf(stderr, "cube: shape has no usable geometry\n");
                return 1;
        }

        if (radius >= CAM_DIST) {
                fprintf(stderr, "cube: shape radius %g exceeds camera distance %g\n",
                        (double)radius, (double)CAM_DIST);
                return 1;
        }

        // a vla here would put an objs vertex count on the stack
        pvert *pv = malloc(shape->nverts * sizeof *pv);
        if (!pv) {
                fprintf(stderr, "cube: out of memory\n");
                return 1;
        }

        if (ndim == 2 ? init_win(&win, dim[0], dim[1]) : init_win_auto(&win)) {
                fprintf(stderr, "cube: %s\n", win_error());
                free(pv);
                return 1;
        }

        if (watch_resize())
                fprintf(stderr, "cube: failed to install SIGWINCH handler, resizing will not track\n");

        install(SIGINT,  on_quit, 0);
        install(SIGTERM, on_quit, 0);
        install(SIGHUP,  on_quit, 0);
        install(SIGQUIT, on_quit, 0);
        install(SIGTSTP, on_stop, 0);
        install(SIGCONT, on_cont, SA_RESTART);
        install(SIGPIPE, SIG_IGN, 0);
        install(SIGTTOU, SIG_IGN, 0);
        install(SIGTTIN, SIG_IGN, 0);

        term_setup();

        struct timespec start, next;
        clock_gettime(CLOCK_MONOTONIC, &start);
        next = start;

        int status = 0;
        const char *err = NULL;

        while (running) {
                if (resize_pending() && resize_win(&win)) {
                        err = win_error();
                        status = 1;
                        break;
                }

                struct timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);

                float t = (float)(now.tv_sec - start.tv_sec)
                        + (float)(now.tv_nsec - start.tv_nsec) / 1e9f;

                long behind = (long)(now.tv_sec - next.tv_sec) * 1000000000L
                            + (now.tv_nsec - next.tv_nsec);
                if (behind > FRAME_NS)
                        next = now;

                clear_win(&win);
                draw_mesh(&win, shape, radius, pv, t);

                if (draw_win(&win)) {
                        err = "failed to write to terminal";
                        status = 1;
                        break;
                }
                if (quit_requested())
                        break;

                next.tv_nsec += FRAME_NS;
                if (next.tv_nsec >= 1000000000L) {
                        next.tv_nsec -= 1000000000L;
                        next.tv_sec++;
                }
                clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        }

        term_restore();
        destroy_win(&win);
        free(pv);

        if (err)
                fprintf(stderr, "cube: %s\n", err);

        if (caught) {
                install((int)caught, SIG_DFL, 0);
                raise((int)caught);
        }

        return status;
}
