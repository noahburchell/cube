#include <string.h>

#include "shapes.h"

static constexpr char RAMP[] = { '.', ':', '-', '+', '*', '#' };
// static constexpr char RAMP[] = { '.', '+', 'o', 'O', '#', '@' };
// static constexpr char RAMP[] = { '-', '=', '+', '*', 'o', '#' };

static constexpr float PHI  = 1.6180340f;
static constexpr float IPHI = 0.6180340f;

static constexpr float RADIUS_SQRT3 = 1.7320508f;
static constexpr float RADIUS_ICOSA = 1.9021131f;

static_assert(RADIUS_SQRT3 <= MESH_MAX_RADIUS);
static_assert(RADIUS_ICOSA <= MESH_MAX_RADIUS);

static const vec3 cube_verts[] = {
        { -1, -1, -1 }, {  1, -1, -1 }, {  1,  1, -1 }, { -1,  1, -1 },
        { -1, -1,  1 }, {  1, -1,  1 }, {  1,  1,  1 }, { -1,  1,  1 },
};

static const tri cube_tris[] = {
        { { 0, 3, 2 }, RAMP[0] }, { { 0, 2, 1 }, RAMP[0] },
        { { 1, 2, 6 }, RAMP[1] }, { { 1, 6, 5 }, RAMP[1] },
        { { 0, 1, 5 }, RAMP[2] }, { { 0, 5, 4 }, RAMP[2] },
        { { 0, 4, 7 }, RAMP[3] }, { { 0, 7, 3 }, RAMP[3] },
        { { 3, 7, 6 }, RAMP[4] }, { { 3, 6, 2 }, RAMP[4] },
        { { 4, 5, 6 }, RAMP[5] }, { { 4, 6, 7 }, RAMP[5] },
};

static const vec3 tetrahedron_verts[] = {
        {  1,  1,  1 }, {  1, -1, -1 }, { -1,  1, -1 }, { -1, -1,  1 },
};

static const tri tetrahedron_tris[] = {
        { { 0, 1, 2 }, RAMP[0] },
        { { 0, 3, 1 }, RAMP[2] },
        { { 0, 2, 3 }, RAMP[4] },
        { { 1, 3, 2 }, RAMP[5] },
};

static const vec3 octahedron_verts[] = {
        { 1, 0, 0 }, { -1, 0, 0 },
        { 0, 0, 1 }, { 0, 0, -1 },
        { 0, 1, 0 }, { 0, -1, 0 },
};

static const tri octahedron_tris[] = {
        { { 2, 0, 4 }, RAMP[0] },
        { { 5, 0, 2 }, RAMP[1] },
        { { 4, 0, 3 }, RAMP[2] },
        { { 3, 0, 5 }, RAMP[3] },
        { { 4, 1, 2 }, RAMP[4] },
        { { 2, 1, 5 }, RAMP[5] },
        { { 3, 1, 4 }, RAMP[0] },
        { { 5, 1, 3 }, RAMP[1] },
};

static const vec3 dodecahedron_verts[] = {
        {  1,  1,  1 }, {  1,  1, -1 },
        {  1, -1,  1 }, {  1, -1, -1 },
        { -1,  1,  1 }, { -1,  1, -1 },
        { -1, -1,  1 }, { -1, -1, -1 },
        {  0,  IPHI,  PHI }, {  0,  IPHI, -PHI },
        {  0, -IPHI,  PHI }, {  0, -IPHI, -PHI },
        {  IPHI,  PHI, 0 }, {  IPHI, -PHI, 0 },
        { -IPHI,  PHI, 0 }, { -IPHI, -PHI, 0 },
        {  PHI, 0,  IPHI }, {  PHI, 0, -IPHI },
        { -PHI, 0,  IPHI }, { -PHI, 0, -IPHI },
};

static const tri dodecahedron_tris[] = {
        { {  1, 12,  0 }, RAMP[0] }, { {  1,  0, 16 }, RAMP[0] }, { {  1, 16, 17 }, RAMP[0] },
        { {  2, 16,  0 }, RAMP[1] }, { {  2,  0,  8 }, RAMP[1] }, { {  2,  8, 10 }, RAMP[1] },
        { {  4,  8,  0 }, RAMP[2] }, { {  4,  0, 12 }, RAMP[2] }, { {  4, 12, 14 }, RAMP[2] },
        { { 11,  9,  1 }, RAMP[3] }, { { 11,  1, 17 }, RAMP[3] }, { { 11, 17,  3 }, RAMP[3] },
        { { 14, 12,  1 }, RAMP[4] }, { { 14,  1,  9 }, RAMP[4] }, { { 14,  9,  5 }, RAMP[4] },
        { { 17, 16,  2 }, RAMP[5] }, { { 17,  2, 13 }, RAMP[5] }, { { 17, 13,  3 }, RAMP[5] },
        { { 15, 13,  2 }, RAMP[0] }, { { 15,  2, 10 }, RAMP[0] }, { { 15, 10,  6 }, RAMP[0] },
        { {  7, 11,  3 }, RAMP[1] }, { {  7,  3, 13 }, RAMP[1] }, { {  7, 13, 15 }, RAMP[1] },
        { { 19, 18,  4 }, RAMP[3] }, { { 19,  4, 14 }, RAMP[3] }, { { 19, 14,  5 }, RAMP[3] },
        { { 10,  8,  4 }, RAMP[4] }, { { 10,  4, 18 }, RAMP[4] }, { { 10, 18,  6 }, RAMP[4] },
        { {  7, 19,  5 }, RAMP[2] }, { {  7,  5,  9 }, RAMP[2] }, { {  7,  9, 11 }, RAMP[2] },
        { {  7, 15,  6 }, RAMP[5] }, { {  7,  6, 18 }, RAMP[5] }, { {  7, 18, 19 }, RAMP[5] },
};

static const vec3 icosahedron_verts[] = {
        { 0,  1,  PHI }, { 0,  1, -PHI },
        { 0, -1,  PHI }, { 0, -1, -PHI },
        {  1,  PHI, 0 }, {  1, -PHI, 0 },
        { -1,  PHI, 0 }, { -1, -PHI, 0 },
        {  PHI, 0,  1 }, {  PHI, 0, -1 },
        { -PHI, 0,  1 }, { -PHI, 0, -1 },
};

static const tri icosahedron_tris[] = {
        { {  8,  0,  2 }, RAMP[0] },
        { {  2,  0, 10 }, RAMP[1] },
        { {  6,  0,  4 }, RAMP[2] },
        { {  4,  0,  8 }, RAMP[3] },
        { { 10,  0,  6 }, RAMP[4] },
        { {  3,  1,  9 }, RAMP[5] },
        { { 11,  1,  3 }, RAMP[0] },
        { {  4,  1,  6 }, RAMP[1] },
        { {  9,  1,  4 }, RAMP[2] },
        { {  6,  1, 11 }, RAMP[3] },
        { {  5,  2,  7 }, RAMP[4] },
        { {  8,  2,  5 }, RAMP[5] },
        { {  7,  2, 10 }, RAMP[0] },
        { {  7,  3,  5 }, RAMP[1] },
        { {  5,  3,  9 }, RAMP[2] },
        { { 11,  3,  7 }, RAMP[3] },
        { {  9,  4,  8 }, RAMP[4] },
        { {  8,  5,  9 }, RAMP[0] },
        { { 10,  6, 11 }, RAMP[5] },
        { { 11,  7, 10 }, RAMP[1] },
};

#define COUNT(a) (sizeof (a) / sizeof *(a))

#define SHAPE(id, o, r) \
        { #id, (o), id##_verts, id##_tris, COUNT(id##_verts), COUNT(id##_tris), (r) }

#define CHECK_SHAPE(id) \
        static_assert(COUNT(id##_verts) <= MESH_MAX_VERTS); \
        static_assert(COUNT(id##_tris)  <= UINT8_MAX)

CHECK_SHAPE(cube);
CHECK_SHAPE(tetrahedron);
CHECK_SHAPE(octahedron);
CHECK_SHAPE(dodecahedron);
CHECK_SHAPE(icosahedron);

const mesh shapes[] = {
        SHAPE(cube,         'c', RADIUS_SQRT3),
        SHAPE(tetrahedron,  't', RADIUS_SQRT3),
        SHAPE(octahedron,   'o', 1.0f),
        SHAPE(dodecahedron, 'd', RADIUS_SQRT3),
        SHAPE(icosahedron,  'i', RADIUS_ICOSA),
};

const size_t nshapes = COUNT(shapes);

const mesh *shape_find_name(const char *name) {
        for (size_t i = 0; i < nshapes; i++)
                if (strcmp(shapes[i].name, name) == 0)
                        return &shapes[i];

        return nullptr;
}

const mesh *shape_find_opt(char opt) {
        for (size_t i = 0; i < nshapes; i++)
                if (shapes[i].opt == opt)
                        return &shapes[i];

        return nullptr;
}
