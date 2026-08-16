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

typedef struct vec3 {
        float x, y, z;
} vec3;

static const vec3 verts[8] = {
        { -1, -1, -1 }, {  1, -1, -1 }, {  1,  1, -1 }, { -1,  1, -1 },
        { -1, -1,  1 }, {  1, -1,  1 }, {  1,  1,  1 }, { -1,  1,  1 },
};

static const struct face {
        unsigned char v[4];
        char c;
} faces[6] = {
        { { 0, 1, 2, 3 }, '.' },   // back  
        { { 1, 5, 6, 2 }, ':' },   // right 
        { { 0, 1, 5, 4 }, '-' },   // top   
        { { 0, 4, 7, 3 }, '+' },   // left  
        { { 3, 2, 6, 7 }, '#' },   // bottom
        { { 4, 5, 6, 7 }, '@' },   // front 
};

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
                raw.c_cc[VMIN]  = 0;   /* reads never block, so keys can be polled */
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

static vec3 rotate(vec3 v, float ax, float ay, float az) {
        float s = sinf(ax), c = cosf(ax);
        float y = v.y * c - v.z * s;
        float z = v.y * s + v.z * c;

        s = sinf(ay), c = cosf(ay);
        float x = v.x * c + z * s;
        z = -v.x * s + z * c;

        s = sinf(az), c = cosf(az);
        return (vec3){ x * c - y * s, x * s + y * c, z };
}

static float fit_scale(const window *win) {
        const float r = 1.7320508f;
        const float m = sqrtf(CAM_DIST * CAM_DIST - r * r) / r;

        float by_w = FILL * (float)win->width  * 0.5f * m / CELL_ASPECT;
        float by_h = FILL * (float)win->height * 0.5f * m;

        return by_w < by_h ? by_w : by_h;
}

static void fill_quad(window *win, const float fx[4], const float fy[4], char c) {
        float ymin = fy[0], ymax = fy[0];
        for (int i = 1; i < 4; i++) {
                if (fy[i] < ymin) ymin = fy[i];
                if (fy[i] > ymax) ymax = fy[i];
        }

        int top = (int)ceilf(ymin);
        int bot = (int)floorf(ymax);
        if (top < 0) top = 0;
        if (bot > win->height - 1) bot = win->height - 1;

        for (int y = top; y <= bot; y++) {
                float xl = HUGE_VALF, xr = -HUGE_VALF;

                for (int i = 0; i < 4; i++) {
                        int j = (i + 1) & 3;
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

static void draw_cube(window *win, float t) {
        const float scale = fit_scale(win);

        const float cx = (float)(win->width  - 1) * 0.5f;
        const float cy = (float)(win->height - 1) * 0.5f;

        vec3 r[8];
        float px[8], py[8];

        for (int i = 0; i < 8; i++) {
                r[i] = rotate(verts[i], t * 0.70f, t * 1.10f, t * 0.35f);

                float k = scale / (r[i].z + CAM_DIST);
                px[i] = cx + r[i].x * k * CELL_ASPECT;
                py[i] = cy + r[i].y * k;
        }

        for (int f = 0; f < 6; f++) {
                const unsigned char *v = faces[f].v;

                vec3 a = r[v[0]], b = r[v[1]], d = r[v[3]];
                vec3 e1 = { b.x - a.x, b.y - a.y, b.z - a.z };
                vec3 e2 = { d.x - a.x, d.y - a.y, d.z - a.z };

                vec3 n = { e1.y * e2.z - e1.z * e2.y,
                           e1.z * e2.x - e1.x * e2.z,
                           e1.x * e2.y - e1.y * e2.x };

                vec3 mid = { 0, 0, 0 };
                for (int i = 0; i < 4; i++) {
                        mid.x += r[v[i]].x * 0.25f;
                        mid.y += r[v[i]].y * 0.25f;
                        mid.z += r[v[i]].z * 0.25f;
                }
                if (n.x * mid.x + n.y * mid.y + n.z * mid.z < 0.0f) {
                        n.x = -n.x; n.y = -n.y; n.z = -n.z;
                }

                if (n.x * mid.x + n.y * mid.y + n.z * (mid.z + CAM_DIST) >= 0.0f)
                        continue;

                float qx[4], qy[4];
                for (int i = 0; i < 4; i++) {
                        qx[i] = px[v[i]];
                        qy[i] = py[v[i]];
                }
                fill_quad(win, qx, qy, faces[f].c);
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

int main(int argc, char **argv) {
        window win;

        if (argc >= 3) {
                if (init_win(&win, atoi(argv[1]), atoi(argv[2])))
                        return 1;
        } else {
                if (init_win_auto(&win))
                        return 1;
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
                draw_cube(&win, t);

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

        return 0;
}
