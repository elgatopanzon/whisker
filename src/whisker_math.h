/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_math
 * @created     : Wednesday Apr 22, 2026 13:45:23 CST
 * @description : math macros for different types
 */

#include "whisker_std.h"
#include "whisker_types.h"

#ifndef WHISKER_MATH_H
#define WHISKER_MATH_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/************
*  w_vec3  *
************/

#define w_vec3_sub(a, b) ((w_vec3){ a.x - b.x, a.y - b.y, a.z - b.z })

#define w_vec3_cross(a, b) ((w_vec3){ \
	(a).y * (b).z - (a).z * (b).y, \
	(a).z * (b).x - (a).x * (b).z, \
	(a).x * (b).y - (a).y * (b).x })

#define w_vec3_normalise(v) ({ \
	float len = sqrtf((v).x * (v).x + (v).y * (v).y + (v).z * (v).z); \
	len > 0.0f ? (w_vec3){ (v).x / len, (v).y / len, (v).z / len } : (w_vec3){ 0.0f, 0.0f, 0.0f }; \
})


/************
*  w_quat  *
************/

#define w_quat_from_euler(pitch, yaw, roll) ((w_quat){ \
    .x = sinf(roll * 0.5f) * cosf(pitch * 0.5f) * cosf(yaw * 0.5f) - cosf(roll * 0.5f) * sinf(pitch * 0.5f) * sinf(yaw * 0.5f), \
    .y = cosf(roll * 0.5f) * sinf(pitch * 0.5f) * cosf(yaw * 0.5f) + sinf(roll * 0.5f) * cosf(pitch * 0.5f) * sinf(yaw * 0.5f), \
    .z = cosf(roll * 0.5f) * cosf(pitch * 0.5f) * sinf(yaw * 0.5f) - sinf(roll * 0.5f) * sinf(pitch * 0.5f) * cosf(yaw * 0.5f), \
    .w = cosf(roll * 0.5f) * cosf(pitch * 0.5f) * cosf(yaw * 0.5f) + sinf(roll * 0.5f) * sinf(pitch * 0.5f) * sinf(yaw * 0.5f) \
})

#define w_quat_normalise(q) ({ \
    float len = sqrtf((q).x * (q).x + (q).y * (q).y + (q).z * (q).z + (q).w * (q).w); \
    len > 0.0f ? \
        ((w_quat){ (q).x / len, (q).y / len, (q).z / len, (q).w / len }) : \
        ((w_quat){ 0.0f, 0.0f, 0.0f, 1.0f }); \
})


/************
*  w_mat4  *
************/

#define w_mat4_from_trs(pos, rot, scale) ({                       \
	w_mat4 _m;                                                    \
	float xx = (rot).x * (rot).x;                                 \
	float yy = (rot).y * (rot).y;                                 \
	float zz = (rot).z * (rot).z;                                 \
	float xy = (rot).x * (rot).y;                                 \
	float xz = (rot).x * (rot).z;                                 \
	float yz = (rot).y * (rot).z;                                 \
	float wx = (rot).w * (rot).x;                                 \
	float wy = (rot).w * (rot).y;                                 \
	float wz = (rot).w * (rot).z;                                 \
	                                                              \
	_m.m[0]  = (1.0f - 2.0f * (yy + zz)) * (scale).x;             \
	_m.m[1]  = (2.0f * (xy + wz)) * (scale).x;                    \
	_m.m[2]  = (2.0f * (xz - wy)) * (scale).x;                    \
	_m.m[3]  = 0.0f;                                              \
	                                                              \
	_m.m[4]  = (2.0f * (xy - wz)) * (scale).y;                    \
	_m.m[5]  = (1.0f - 2.0f * (xx + zz)) * (scale).y;             \
	_m.m[6]  = (2.0f * (yz + wx)) * (scale).y;                    \
	_m.m[7]  = 0.0f;                                              \
	                                                              \
	_m.m[8]  = (2.0f * (xz + wy)) * (scale).z;                    \
	_m.m[9]  = (2.0f * (yz - wx)) * (scale).z;                    \
	_m.m[10] = (1.0f - 2.0f * (xx + yy)) * (scale).z;             \
	_m.m[11] = 0.0f;                                              \
	                                                              \
	_m.m[12] = (pos).x;                                           \
	_m.m[13] = (pos).y;                                           \
	_m.m[14] = (pos).z;                                           \
	_m.m[15] = 1.0f;                                              \
	_m;                                                           \
})

#endif /* WHISKER_MATH_H */

