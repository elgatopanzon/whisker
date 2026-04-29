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

#define w_mat4_look_at(eye, target, up) ({                            \
    w_mat4 _m;                                                        \
    /* forward = normalize(eye - target) */                           \
    float _fx = (eye).x - (target).x;                                 \
    float _fy = (eye).y - (target).y;                                 \
    float _fz = (eye).z - (target).z;                                 \
    float _flen = sqrtf(_fx*_fx + _fy*_fy + _fz*_fz);                 \
    if (_flen > 0.0f) {                                               \
        _fx /= _flen; _fy /= _flen; _fz /= _flen;                     \
    }                                                                 \
    /* right = normalize(up × forward) */                             \
    float _rx = (up).y * _fz - (up).z * _fy;                          \
    float _ry = (up).z * _fx - (up).x * _fz;                          \
    float _rz = (up).x * _fy - (up).y * _fx;                          \
    float _rlen = sqrtf(_rx*_rx + _ry*_ry + _rz*_rz);                 \
    if (_rlen > 0.0f) {                                               \
        _rx /= _rlen; _ry /= _rlen; _rz /= _rlen;                     \
    }                                                                 \
    /* up = forward × right (orthonormalize) */                       \
    float _ux = _fy * _rz - _fz * _ry;                                \
    float _uy = _fz * _rx - _fx * _rz;                                \
    float _uz = _fx * _ry - _fy * _rx;                                \
    /* column-major view matrix */                                    \
    _m.m[0]  = _rx;                                                   \
    _m.m[1]  = _ux;                                                   \
    _m.m[2]  = _fx;                                                   \
    _m.m[3]  = 0.0f;                                                  \
    _m.m[4]  = _ry;                                                   \
    _m.m[5]  = _uy;                                                   \
    _m.m[6]  = _fy;                                                   \
    _m.m[7]  = 0.0f;                                                  \
    _m.m[8]  = _rz;                                                   \
    _m.m[9]  = _uz;                                                   \
    _m.m[10] = _fz;                                                   \
    _m.m[11] = 0.0f;                                                  \
    _m.m[12] = -(_rx*(eye).x + _ry*(eye).y + _rz*(eye).z);            \
    _m.m[13] = -(_ux*(eye).x + _uy*(eye).y + _uz*(eye).z);            \
    _m.m[14] = -(_fx*(eye).x + _fy*(eye).y + _fz*(eye).z);            \
    _m.m[15] = 1.0f;                                                  \
    _m;                                                               \
})

#endif /* WHISKER_MATH_H */

