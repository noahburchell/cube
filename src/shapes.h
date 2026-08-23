#ifndef SHAPES_H
#define SHAPES_H

#include <stddef.h>
#include <stdint.h>

typedef struct vec3 {
        float x, y, z;
} vec3;

typedef struct tri {
        uint8_t v[3];
        char c;
} tri;

typedef struct mesh {
        const char *name;
        const vec3 *verts;
        const tri *tris;
        uint8_t nverts, ntris;
        float radius;
} mesh;

constexpr size_t MESH_MAX_VERTS  = 20;
constexpr float  MESH_MAX_RADIUS = 1.9021131f;

extern const mesh shapes[];
extern const size_t nshapes;

const mesh *shape_find(const char *name);

#endif
