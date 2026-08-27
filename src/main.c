#define _GNU_SOURCE

#include <errno.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "shapes.h"
#include "window.h"

constexpr int  FPS      = 60;
constexpr long FRAME_NS = 1'000'000'000L / FPS;

constexpr float CAM_DIST    = 4.0f;
constexpr float CELL_ASPECT = 2.0f;
constexpr float FILL        = 0.85f;

static_assert(MESH_MAX_RADIUS < CAM_DIST);

constexpr float  SPIN_X = 0.70f;
constexpr float  SPIN_Y = 1.10f;
constexpr float  SPIN_Z = 0.35f;
constexpr double SPIN_PERIOD = 40.0 * 3.14159265358979324;

#define ALT_ON    "\033[?1049h"
#define ALT_OFF   "\033[?1049l"
#define CURS_OFF  "\033[?25l"
#define CURS_ON   "\033[?25h"

#define EMIT(s) emit(s, sizeof (s) - 1)

typedef struct mat3 {
        vec3 cx, cy, cz;
} mat3;

typedef struct pvert {
        float x, y;
} pvert;

static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t caught;

static void emit(const char *s, size_t n) {
        constexpr int STALLS_MAX = 64;

        int stalls = 0;

        while (n) {
                ssize_t k = write(STDOUT_FILENO, s, n);

                if (k > 0) {
                        s += (size_t)k;
                        n -= (size_t)k;
                        continue;
                }
                if (k == 0)
                        return;
                if (errno == EINTR)
                        continue;
                if (errno != EAGAIN)
                        return;

                // wait briefly for room then give up
                if (++stalls > STALLS_MAX)
                        return;

                struct pollfd pfd = { .fd = STDOUT_FILENO, .events = POLLOUT };
                if (poll(&pfd, 1, 20) < 0 && errno != EINTR)
                        return;
        }
}

static struct termios saved_term;
static volatile sig_atomic_t term_saved;

static void term_setup(void) {
        if (tcgetattr(STDIN_FILENO, &saved_term) == 0) {
                struct termios raw = saved_term;
                raw.c_lflag &= ~(unsigned)(ICANON | ECHO);
                raw.c_cc[VMIN]  = 0;   /* reads never block so keys can be polled */
                raw.c_cc[VTIME] = 0;
                term_saved = tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
        }
        EMIT(ALT_ON CURS_OFF);
}

static void term_restore(void) {
        EMIT(CURS_ON ALT_OFF);
        if (term_saved)
                tcsetattr(STDIN_FILENO, TCSANOW, &saved_term);
}

static void install(int sig, void (*handler)(int), int flags) {
        struct sigaction sa = { .sa_handler = handler, .sa_flags = flags };
        sigfillset(&sa.sa_mask);
        sigaction(sig, &sa, nullptr);
}

static void on_quit(int sig) {
        caught = sig;
        running = 0;
}


static void on_stop(int sig) {
        const int saved_errno = errno; // "errno" sounds kinda cute

        term_restore();
        install(sig, SIG_DFL, 0);
        raise(sig);

        errno = saved_errno;
}

static void on_cont([[maybe_unused]] int sig) {
        const int saved_errno = errno;

        term_setup();
        mark_resize();
        install(SIGTSTP, on_stop, 0);

        errno = saved_errno;
}

static mat3 rotation(float ax, float ay, float az) {
        float sa, ca, sb, cb, sc, cc;

        sincosf(ax, &sa, &ca);
        sincosf(ay, &sb, &cb);
        sincosf(az, &sc, &cc);

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

static inline void fill_span(char *restrict row, float xl, float xr, float wmax, char c) {
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
        if (a <= b)
                memset(row + a, (unsigned char)c, (size_t)(b - a + 1));
}

static void fill_tri(const window *win, float x0, float y0, float x1, float y1,
                     float x2, float y2, char c) {
        float s;

        if (y0 > y1) { s = x0; x0 = x1; x1 = s; s = y0; y0 = y1; y1 = s; }
        if (y1 > y2) { s = x1; x1 = x2; x2 = s; s = y1; y1 = y2; y2 = s; }
        if (y0 > y1) { s = x0; x0 = x1; x1 = s; s = y0; y0 = y1; y1 = s; }

        const float dfull = y2 - y0;
        if (!(dfull > 0.0f))
                return;

        float ytf = ceilf(y0);
        float ybf = floorf(y2);
        if (ytf < 0.0f) ytf = 0.0f;
        if (ybf > (float)(win->height - 1)) ybf = (float)(win->height - 1);
        if (!(ytf <= ybf))
                return;

        const int w = win->width;
        const float wmax = (float)(w - 1);

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

        char *row = win->grid + (size_t)ytop * (size_t)w;
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

static void draw_mesh(const window *win, const mesh *m, pvert *restrict pv, float scale, float t) {
        const float cx = (float)(win->width  - 1) * 0.5f;
        const float cy = (float)(win->height - 1) * 0.5f;

        const mat3 rot = rotation(t * SPIN_X, t * SPIN_Y, t * SPIN_Z);

        const float xx = rot.cx.x, xy = rot.cx.y, xz = rot.cx.z;
        const float yx = rot.cy.x, yy = rot.cy.y, yz = rot.cy.z;
        const float zx = rot.cz.x, zy = rot.cz.y, zz = rot.cz.z;

        const vec3 *restrict verts = m->verts;

        for (unsigned i = 0; i < m->nverts; i++) {
                const float vx = verts[i].x, vy = verts[i].y, vz = verts[i].z;
                const float k = scale / (xz * vx + yz * vy + zz * vz + CAM_DIST);

                pv[i].x = cx + (xx * vx + yx * vy + zx * vz) * k * CELL_ASPECT;
                pv[i].y = cy + (xy * vx + yy * vy + zy * vz) * k;
        }

        const tri *restrict tris = m->tris;

        for (unsigned f = 0; f < m->ntris; f++) {
                const uint8_t *v = tris[f].v;

                const float ax = pv[v[0]].x, ay = pv[v[0]].y;
                const float bx = pv[v[1]].x, by = pv[v[1]].y;
                const float dx = pv[v[2]].x, dy = pv[v[2]].y;

                if ((bx - ax) * (dy - ay) - (by - ay) * (dx - ax) >= 0.0f)
                        continue;

                fill_tri(win, ax, ay, bx, by, dx, dy, tris[f].c);
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

static void usage(FILE *out, const char *prog) {
        int width = (int)sizeof "help" - 1;

        for (size_t i = 0; i < nshapes; i++) {
                const int n = (int)strlen(shapes[i].name);
                if (n > width)
                        width = n;
        }
        width += 2;

        fprintf(out, "usage: %s [option]\n\noptions:\n  -h, --%-*sshow this help\n\nshapes:\n",
                prog, width, "help");

        for (size_t i = 0; i < nshapes; i++)
                if (i == 0)
                        fprintf(out, "  -%c, --%-*s(default)\n",
                                shapes[i].opt, width, shapes[i].name);
                else
                        fprintf(out, "  -%c, --%s\n", shapes[i].opt, shapes[i].name);

        fputs("\nq or esc quits\n", out);
}

static int bad_arg(const char *prog, const char *kind, const char *arg) {
        fprintf(stderr, "%s: %s '%s'\n\n", prog, kind, arg);
        usage(stderr, prog);

        return 1;
}

static int parse_args(int argc, char **argv, const char *prog, const mesh **shape) {
        bool opts = true;

        for (int i = 1; i < argc; i++) {
                const char *arg = argv[i];

                if (!arg)
                        break;

                if (!opts || arg[0] != '-')
                        return bad_arg(prog, "unexpected argument", arg);

                if (arg[1] == '\0')
                        return bad_arg(prog, "unknown option", arg);

                if (arg[1] == '-') {
                        if (arg[2] == '\0') {
                                opts = false;
                                continue;
                        }

                        if (strcmp(arg + 2, "help") == 0) {
                                usage(stdout, prog);
                                return -1;
                        }

                        const mesh *m = shape_find_name(arg + 2);
                        if (!m)
                                return bad_arg(prog, "unknown option", arg);

                        *shape = m;
                        continue;
                }

                if (arg[2] != '\0')
                        return bad_arg(prog, "unknown option", arg);

                if (arg[1] == 'h') {
                        usage(stdout, prog);
                        return -1;
                }

                const mesh *m = shape_find_opt(arg[1]);
                if (!m)
                        return bad_arg(prog, "unknown option", arg);

                *shape = m;
        }

        return 0;
}

int main(int argc, char **argv) {
        window win;

        const char *prog = argc > 0 && argv && argv[0] && argv[0][0] ? argv[0] : "cube";
        const mesh *shape = &shapes[0];

        switch (parse_args(argc, argv, prog, &shape)) {
        case 0:  break;
        case -1: return fflush(stdout) != 0; // --help but the write couldve failed
        default: return 1;
        }

        if (!isatty(STDOUT_FILENO)) {
                fprintf(stderr, "%s: stdout is not a terminal\n", prog);
                return 1;
        }

        if (init_win(&win)) {
                fprintf(stderr, "%s: %s\n", prog, win_error());
                return 1;
        }

        if (win_guessed())
                fprintf(stderr, "%s: terminal would not report its size, falling back to %dx%d\n",
                        prog, win.width, win.height);

        if (watch_resize())
                fprintf(stderr, "%s: failed to install SIGWINCH handler, resizing will not track\n",
                        prog);

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

        pvert pv[MESH_MAX_VERTS];
        float scale = fit_scale(&win, shape->radius);

        struct timespec start = {}, next;
        clock_gettime(CLOCK_MONOTONIC, &start);
        next = start;

        int status = 0;
        const char *err = nullptr;

        while (running) {
                if (resize_pending()) {
                        if (resize_win(&win)) {
                                err = win_error();
                                status = 1;
                                break;
                        }
                        scale = fit_scale(&win, shape->radius);
                }

                struct timespec now;
                if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
                        now = next; // cant fail here, but never read it unset

                const double elapsed = (double)(now.tv_sec - start.tv_sec)
                                     + (double)(now.tv_nsec - start.tv_nsec) * 1e-9;

                const time_t bsec  = now.tv_sec - next.tv_sec;
                const long   bnsec = now.tv_nsec - next.tv_nsec;

                if (bsec > 1 || (bsec >= 0 && (long)bsec * 1'000'000'000L + bnsec > FRAME_NS))
                        next = now;

                clear_win(&win);
                draw_mesh(&win, shape, pv, scale, (float)fmod(elapsed, SPIN_PERIOD));

                if (draw_win(&win)) {
                        err = "failed to write to terminal";
                        status = 1;
                        break;
                }
                if (quit_requested())
                        break;

                next.tv_nsec += FRAME_NS;
                if (next.tv_nsec >= 1'000'000'000L) {
                        next.tv_nsec -= 1'000'000'000L;
                        next.tv_sec++;
                }
                clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);
        }

        term_restore();
        destroy_win(&win);

        if (err)
                fprintf(stderr, "%s: %s\n", prog, err);

        if (caught) {
                install((int)caught, SIG_DFL, 0);
                raise((int)caught);
        }

        return status;
}
