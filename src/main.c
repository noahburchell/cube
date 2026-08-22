#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
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
        vec3 r;
        float x, y;
} pvert;

static volatile sig_atomic_t running = 1;

static void on_quit(int sig) {
        (void)sig;
        running = 0;
}

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

static vec3 rotate(mat3 m, vec3 v) {
        return (vec3){
                m.cx.x * v.x + m.cy.x * v.y + m.cz.x * v.z,
                m.cx.y * v.x + m.cy.y * v.y + m.cz.y * v.z,
                m.cx.z * v.x + m.cy.z * v.y + m.cz.z * v.z,
        };
}

static float fit_scale(const window *win, float r) {
        const float m = sqrtf(CAM_DIST * CAM_DIST - r * r) / r;

        float by_w = FILL * (float)win->width  * 0.5f * m / CELL_ASPECT;
        float by_h = FILL * (float)win->height * 0.5f * m;

        return by_w < by_h ? by_w : by_h;
}

static void fill_tri(window *win, const float fx[3], const float fy[3], char c) {
        float ymin = fy[0], ymax = fy[0];
        for (int i = 1; i < 3; i++) {
                if (fy[i] < ymin) ymin = fy[i];
                if (fy[i] > ymax) ymax = fy[i];
        }

        int top = (int)ceilf(ymin);
        int bot = (int)floorf(ymax);
        if (top < 0) top = 0;
        if (bot > win->height - 1) bot = win->height - 1;

        for (int y = top; y <= bot; y++) {
                float xl = HUGE_VALF, xr = -HUGE_VALF;

                for (int i = 0; i < 3; i++) {
                        int j = (i + 1) % 3;
                        float ya = fy[i], yb = fy[j];

                        if (ya == yb)
                                continue;

                        float lo = ya < yb ? ya : yb;
                        float hi = ya < yb ? yb : ya;
                        if ((float)y < lo || (float)y > hi)
                                continue;

                        float x = fx[i] + ((float)y - ya) * (fx[j] - fx[i]) / (yb - ya);
                        if (x < xl) xl = x;
                        if (x > xr) xr = x;
                }
                if (xl > xr)
                        continue;

                int a = (int)ceilf(xl);
                int b = (int)floorf(xr);
                if (a < 0) a = 0;
                if (b > win->width - 1) b = win->width - 1;

                cell_t *row = win->grid + (size_t)y * win->width;
                for (int x = a; x <= b; x++)
                        row[x].c = c;
        }
}

static void draw_mesh(window *win, const mesh *m, float radius, pvert *pv, float t) {
        const float scale = fit_scale(win, radius);

        const float cx = (float)(win->width  - 1) * 0.5f;
        const float cy = (float)(win->height - 1) * 0.5f;

        const mat3 rot = rotation(t * 0.70f, t * 1.10f, t * 0.35f);

        for (size_t i = 0; i < m->nverts; i++) {
                pv[i].r = rotate(rot, m->verts[i]);

                float k = scale / (pv[i].r.z + CAM_DIST);
                pv[i].x = cx + pv[i].r.x * k * CELL_ASPECT;
                pv[i].y = cy + pv[i].r.y * k;
        }

        for (size_t f = 0; f < m->ntris; f++) {
                const uint32_t *v = m->tris[f].v;

                vec3 a = pv[v[0]].r, b = pv[v[1]].r, d = pv[v[2]].r;
                vec3 e1 = { b.x - a.x, b.y - a.y, b.z - a.z };
                vec3 e2 = { d.x - a.x, d.y - a.y, d.z - a.z };

                vec3 n = { e1.y * e2.z - e1.z * e2.y,
                           e1.z * e2.x - e1.x * e2.z,
                           e1.x * e2.y - e1.y * e2.x };

                vec3 mid = { (a.x + b.x + d.x) / 3.0f,
                             (a.y + b.y + d.y) / 3.0f,
                             (a.z + b.z + d.z) / 3.0f };

                if (n.x * mid.x + n.y * mid.y + n.z * (mid.z + CAM_DIST) >= 0.0f)
                        continue;

                float qx[3], qy[3];
                for (int i = 0; i < 3; i++) {
                        qx[i] = pv[v[i]].x;
                        qy[i] = pv[v[i]].y;
                }
                fill_tri(win, qx, qy, m->tris[f].c);
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

static void usage(const char *prog) {
        fprintf(stderr, "usage: %s [--shape]\n\nshapes:\n", prog);

        for (size_t i = 0; i < nshapes; i++)
                fprintf(stderr, "  --%s%s\n", shapes[i].name, i == 0 ? " (default)" : "");
}

int main(int argc, char **argv) {
        window win;

        const mesh *shape = &shapes[0];
        int dim[2], ndim = 0;

        for (int i = 1; i < argc; i++) {
                if (strncmp(argv[i], "--", 2) == 0) {
                        if (strcmp(argv[i], "--help") == 0) {
                                usage(argv[0]);
                                return 0;
                        }

                        const mesh *m = shape_find(argv[i] + 2);
                        if (!m) {
                                fprintf(stderr, "Unknown option '%s'\n\n", argv[i]);
                                usage(argv[0]);
                                return 1;
                        }
                        shape = m;
                } else if (ndim < 2) {
                        dim[ndim++] = atoi(argv[i]);
                } else {
                        fprintf(stderr, "Unexpected argument '%s'\n\n", argv[i]);
                        usage(argv[0]);
                        return 1;
                }
        }

        if (ndim == 1) {
                fprintf(stderr, "Both a width and a height are required\n\n");
                usage(argv[0]);
                return 1;
        }

        const float radius = mesh_radius(shape);

        // a vla here would put an objs vertex count on the stack
        pvert *pv = malloc(shape->nverts * sizeof *pv);
        if (!pv) {
                fprintf(stderr, "PANIC - Failed to allocate vertex scratch! No fallback path!\n");
                return 1;
        }

        if (ndim == 2) {
                if (init_win(&win, dim[0], dim[1])) {
                        free(pv);
                        return 1;
                }
        } else {
                if (init_win_auto(&win)) {
                        free(pv);
                        return 1;
                }
        }

        if (watch_resize())
                fprintf(stderr, "Failed to install SIGWINCH handler, resizing will not track\n");

        struct sigaction sa = { .sa_handler = on_quit };
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);

        term_setup();

        struct timespec start, next;
        clock_gettime(CLOCK_MONOTONIC, &start);
        next = start;

        while (running) {
                if (resize_pending())
                        resize_win(&win);

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

                if (draw_win(&win))
                        break;
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

        return 0;
}
