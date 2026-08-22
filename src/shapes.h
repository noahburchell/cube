#ifndef SHAPES_H
#define SHAPES_H

#include <stddef.h>
#include <stdint.h>

typedef struct vec3 {
        float x, y, z;
} vec3;

// int rather than char for obj when i can be fucked adding that
typedef struct tri {
        uint32_t v[3];
        char c;
} tri;

typedef struct mesh {
        const char *name;
        const vec3 *verts;
        size_t nverts;
        const tri *tris;
        size_t ntris;
} mesh;

extern const mesh shapes[];
extern const size_t nshapes;

const mesh *shape_find(const char *name);

float mesh_radius(const mesh *m);

#endif
