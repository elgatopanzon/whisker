/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_types
 * @created     : Thursday Mar 12, 2026 10:21:52 CST
 * @description : additional supported types
 */

#include "whisker_std.h"

#ifndef WHISKER_TYPES_H
#define WHISKER_TYPES_H

#define DEFINE_VEC2_STRUCT(name, type, member1, member2) \
    typedef struct { type member1; type member2; } name

#define DEFINE_VEC3_STRUCT(name, type, member1, member2, member3) \
    typedef struct { type member1; type member2; type member3; } name

#define DEFINE_VEC4_STRUCT(name, type, member1, member2, member3, member4) \
    typedef struct { type member1; type member2; type member3; type member4; } name

/*************
*  vectors  *
*************/

DEFINE_VEC2_STRUCT(w_vec2, float, x, y);
DEFINE_VEC2_STRUCT(w_vec2i, int, x, y);
DEFINE_VEC2_STRUCT(w_vec2u, uint, x, y);

DEFINE_VEC3_STRUCT(w_vec3, float, x, y, z);
DEFINE_VEC3_STRUCT(w_vec3i, int, x, y, z);
DEFINE_VEC3_STRUCT(w_vec3u, uint, x, y, z);

DEFINE_VEC4_STRUCT(w_vec4, float, x, y, z, w);
DEFINE_VEC4_STRUCT(w_vec4i, int, x, y, z, w);
DEFINE_VEC4_STRUCT(w_vec4u, uint, x, y, z, w);

/**************
*  matrices  *
**************/

typedef struct
{
    float m[4];
} w_mat2;

typedef struct
{
    float m[9];
} w_mat3;

typedef struct
{
    float m[16];
} w_mat4;


/***********
*  color  *
***********/

DEFINE_VEC4_STRUCT(w_color, float, r, g, b, a);
DEFINE_VEC4_STRUCT(w_color8, uint8_t, r, g, b, a);


/**************
*  geometry  *
**************/

DEFINE_VEC4_STRUCT(w_rect, float, x, y, width, height);
DEFINE_VEC4_STRUCT(w_recti, uint, x, y, width, height);
DEFINE_VEC2_STRUCT(w_aabb2, w_vec2, min, max);
DEFINE_VEC2_STRUCT(w_aabb3, w_vec3, min, max);
DEFINE_VEC2_STRUCT(w_ray2, w_vec2, origin, direction_norm);
DEFINE_VEC2_STRUCT(w_ray3, w_vec3, origin, direction_norm);


/****************
*  pack unions  *
****************/

// little-endian: right/low values come first in memory
typedef union { uint32_t packed; struct { uint16_t right; uint16_t left; }; } w_pack16x2;
typedef union { uint64_t packed; struct { uint16_t d; uint16_t c; uint16_t b; uint16_t a; }; } w_pack16x4;
typedef union { uint32_t packed; struct { uint8_t d; uint8_t c; uint8_t b; uint8_t a; }; } w_pack8x4;
typedef union { uint64_t packed; struct { uint8_t h; uint8_t g; uint8_t f; uint8_t e; uint8_t d; uint8_t c; uint8_t b; uint8_t a; }; } w_pack8x8;
typedef union { uint64_t packed; struct { uint32_t right; uint32_t left; }; } w_pack32x2;

#define to_w_pack(type, val) ((type){ .packed = (val) })
#define from_w_pack(pack_var) ((pack_var).packed)

#endif /* WHISKER_TYPES_H */

