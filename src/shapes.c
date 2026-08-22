#include <math.h>
#include <string.h>

#include "shapes.h"

static const vec3 cube_verts[8] = {
        { -1, -1, -1 }, {  1, -1, -1 }, {  1,  1, -1 }, { -1,  1, -1 },
        { -1, -1,  1 }, {  1, -1,  1 }, {  1,  1,  1 }, { -1,  1,  1 },
};

static const tri cube_tris[12] = {
        { { 0, 3, 2 }, '.' }, { { 0, 2, 1 }, '.' },   // -z
        { { 1, 2, 6 }, ':' }, { { 1, 6, 5 }, ':' },   // +x
        { { 0, 1, 5 }, '-' }, { { 0, 5, 4 }, '-' },   // -y
        { { 0, 4, 7 }, '+' }, { { 0, 7, 3 }, '+' },   // -x
        { { 3, 7, 6 }, '#' }, { { 3, 6, 2 }, '#' },   // +y
        { { 4, 5, 6 }, '@' }, { { 4, 6, 7 }, '@' },   // +z
};

static const vec3 tetra_verts[4] = {
        {  1,  1,  1 }, {  1, -1, -1 }, { -1,  1, -1 }, { -1, -1,  1 },
};

static const tri tetra_tris[4] = {
        { { 0, 1, 2 }, '.' },
        { { 0, 3, 1 }, '-' },
        { { 0, 2, 3 }, '#' },
        { { 1, 3, 2 }, '@' },
};

static const vec3 pyra_verts[5] = {
        { -1,  1, -1 }, {  1,  1, -1 }, {  1,  1,  1 }, { -1,  1,  1 },
        {  0, -1,  0 },
};

static const tri pyra_tris[6] = {
        { { 0, 3, 2 }, '#' }, { { 0, 2, 1 }, '#' },   // base
        { { 0, 1, 4 }, '.' },
        { { 1, 2, 4 }, ':' },
        { { 2, 3, 4 }, '+' },
        { { 3, 0, 4 }, '@' },
};

const mesh shapes[] = {
        { "cube",        cube_verts,  8, cube_tris,  12 },
        { "tetrahedron", tetra_verts, 4, tetra_tris,  4 },
        { "pyramid",     pyra_verts,  5, pyra_tris,   6 },
};

const size_t nshapes = sizeof shapes / sizeof shapes[0];

const mesh *shape_find(const char *name) {
        for (size_t i = 0; i < nshapes; i++)
                if (strcmp(shapes[i].name, name) == 0)
                        return &shapes[i];

        return NULL;
}

float mesh_radius(const mesh *m) {
        float r2 = 0.0f;

        for (size_t i = 0; i < m->nverts; i++) {
                const vec3 v = m->verts[i];
                const float d2 = v.x * v.x + v.y * v.y + v.z * v.z;

                if (d2 > r2)
                        r2 = d2;
        }

        return sqrtf(r2);
}
