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
    struct name { type member1; type member2; }

#define DEFINE_VEC3_STRUCT(name, type, member1, member2, member3) \
    struct name { type member1; type member2; type member3; }

#define DEFINE_VEC4_STRUCT(name, type, member1, member2, member3, member4) \
    struct name { type member1; type member2; type member3; type member4; }

/*************
*  vectors  *
*************/

DEFINE_VEC2_STRUCT(vec2, float, x, y);
DEFINE_VEC2_STRUCT(vec2i, int, x, y);
DEFINE_VEC2_STRUCT(vec2u, uint, x, y);

DEFINE_VEC3_STRUCT(vec3, float, x, y, z);
DEFINE_VEC3_STRUCT(vec3i, int, x, y, z);
DEFINE_VEC3_STRUCT(vec3u, uint, x, y, z);

DEFINE_VEC4_STRUCT(vec4, float, x, y, z, w);
DEFINE_VEC4_STRUCT(vec4i, int, x, y, z, w);
DEFINE_VEC4_STRUCT(vec4u, uint, x, y, z, w);

/**************
*  matrices  *
**************/

struct mat2 
{
    float m[4];
};

struct mat3 
{
    float m[9];
};

struct mat4 
{
    float m[16];
};


/***********
*  color  *
***********/

DEFINE_VEC4_STRUCT(color, float, r, g, b, a);
DEFINE_VEC4_STRUCT(color8, uint8_t, r, g, b, a);


/**************
*  geometry  *
**************/

DEFINE_VEC4_STRUCT(rect, float, x, y, width, height);
DEFINE_VEC4_STRUCT(recti, uint, x, y, width, height);
DEFINE_VEC2_STRUCT(aabb2, struct vec2, min, max);
DEFINE_VEC2_STRUCT(aabb3, struct vec3, min, max);
DEFINE_VEC2_STRUCT(ray2, struct vec2, origin, direction_norm);
DEFINE_VEC2_STRUCT(ray3, struct vec3, origin, direction_norm);


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

