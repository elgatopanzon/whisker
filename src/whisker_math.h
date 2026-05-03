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

#ifndef W_PI
#define W_PI 3.14159265358979323846
#endif
#ifndef W_DEG2RAD
#define W_DEG2RAD (W_PI / 180.0f)
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

// for screen-space/UI, rotate -90° around X
#define W_QUAT_SCREEN_FACING ((w_quat){-0.7071f, 0, 0, 0.7071f})

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

/* identity quaternion (no rotation) */
#define w_quat_identity() ((w_quat){ 0.0f, 0.0f, 0.0f, 1.0f })

/* quaternion from axis + angle (radians); axis need not be normalised */
#define w_quat_from_axis_angle(axis, angle) ({ \
    float _hs = sinf((angle) * 0.5f); \
    float _ax = (axis).x, _ay = (axis).y, _az = (axis).z; \
    float _len = sqrtf(_ax*_ax + _ay*_ay + _az*_az); \
    if (_len > 0.0f) { _ax /= _len; _ay /= _len; _az /= _len; } \
    (w_quat){ _ax * _hs, _ay * _hs, _az * _hs, cosf((angle) * 0.5f) }; \
})

/* rotation quaternions around a single axis (angle in radians) */
#define w_quat_rotation_x(angle) \
    ((w_quat){ sinf((angle) * 0.5f), 0.0f, 0.0f, cosf((angle) * 0.5f) })
#define w_quat_rotation_y(angle) \
    ((w_quat){ 0.0f, sinf((angle) * 0.5f), 0.0f, cosf((angle) * 0.5f) })
#define w_quat_rotation_z(angle) \
    ((w_quat){ 0.0f, 0.0f, sinf((angle) * 0.5f), cosf((angle) * 0.5f) })

/* Hamilton product: compose rotation a then b */
#define w_quat_mul(a, b) ((w_quat){ \
    (a).w*(b).x + (a).x*(b).w + (a).y*(b).z - (a).z*(b).y, \
    (a).w*(b).y - (a).x*(b).z + (a).y*(b).w + (a).z*(b).x, \
    (a).w*(b).z + (a).x*(b).y - (a).y*(b).x + (a).z*(b).w, \
    (a).w*(b).w - (a).x*(b).x - (a).y*(b).y - (a).z*(b).z  \
})

/* negate xyz component -- same rotation direction reversed */
#define w_quat_conjugate(q) \
    ((w_quat){ -(q).x, -(q).y, -(q).z, (q).w })

/* multiplicative inverse; for unit quaternions this equals conjugate */
#define w_quat_inverse(q) ({ \
    float _d = (q).x*(q).x + (q).y*(q).y + (q).z*(q).z + (q).w*(q).w; \
    _d > 0.0f ? \
        ((w_quat){ -(q).x/_d, -(q).y/_d, -(q).z/_d, (q).w/_d }) : \
        w_quat_identity(); \
})

/* American-spelling alias */
#define w_quat_normalize(q) w_quat_normalise(q)

/* scalar dot product of two quaternions */
#define w_quat_dot(a, b) \
    ((a).x*(b).x + (a).y*(b).y + (a).z*(b).z + (a).w*(b).w)

/* spherical linear interpolation; t in [0,1] */
#define w_quat_slerp(a, b, t) ({ \
    float _dot = (a).x*(b).x + (a).y*(b).y + (a).z*(b).z + (a).w*(b).w; \
    w_quat _qb = (b); \
    if (_dot < 0.0f) { \
        _qb.x = -(b).x; _qb.y = -(b).y; _qb.z = -(b).z; _qb.w = -(b).w; \
        _dot = -_dot; \
    } \
    w_quat _r; \
    if (_dot > 0.9995f) { \
        /* quaternions nearly parallel: lerp + normalise */ \
        _r = (w_quat){ \
            (a).x + (t) * (_qb.x - (a).x), \
            (a).y + (t) * (_qb.y - (a).y), \
            (a).z + (t) * (_qb.z - (a).z), \
            (a).w + (t) * (_qb.w - (a).w) \
        }; \
        _r = w_quat_normalise(_r); \
    } else { \
        float _theta_0 = acosf(_dot); \
        float _theta   = _theta_0 * (t); \
        float _st0     = sinf(_theta_0); \
        float _s0      = cosf(_theta) - _dot * sinf(_theta) / _st0; \
        float _s1      = sinf(_theta) / _st0; \
        _r = (w_quat){ \
            _s0*(a).x + _s1*_qb.x, \
            _s0*(a).y + _s1*_qb.y, \
            _s0*(a).z + _s1*_qb.z, \
            _s0*(a).w + _s1*_qb.w \
        }; \
    } \
    _r; \
})

/* decompose unit quaternion to axis (w_vec3 *) and angle (float *, radians) */
#define w_quat_to_axis_angle(q, axis_ptr, angle_ptr) do { \
    float _cw = (q).w; \
    if (_cw >  1.0f) _cw =  1.0f; \
    if (_cw < -1.0f) _cw = -1.0f; \
    *(angle_ptr) = 2.0f * acosf(_cw); \
    float _s = sqrtf(1.0f - _cw * _cw); \
    if (_s < 0.0001f) { \
        (axis_ptr)->x = 1.0f; (axis_ptr)->y = 0.0f; (axis_ptr)->z = 0.0f; \
    } else { \
        (axis_ptr)->x = (q).x / _s; \
        (axis_ptr)->y = (q).y / _s; \
        (axis_ptr)->z = (q).z / _s; \
    } \
} while (0)

/* convert to Euler angles (radians) returned as w_vec3{pitch, yaw, roll} */
#define w_quat_to_euler(q) ({ \
    float _sr = 2.0f * ((q).w*(q).x + (q).y*(q).z); \
    float _cr = 1.0f - 2.0f * ((q).x*(q).x + (q).y*(q).y); \
    float _pitch = atan2f(_sr, _cr); \
    float _sp = 2.0f * ((q).w*(q).y - (q).z*(q).x); \
    float _yaw; \
    if      (_sp >=  1.0f) _yaw =  (float)W_PI * 0.5f; \
    else if (_sp <= -1.0f) _yaw = -(float)W_PI * 0.5f; \
    else                   _yaw =  asinf(_sp); \
    float _sy = 2.0f * ((q).w*(q).z + (q).x*(q).y); \
    float _cy = 1.0f - 2.0f * ((q).y*(q).y + (q).z*(q).z); \
    float _roll = atan2f(_sy, _cy); \
    (w_vec3){ _pitch, _yaw, _roll }; \
})

/* rotate a w_vec3 by a unit quaternion */
#define w_quat_rotate_vec3(q, v) ({ \
    float _ux = (q).x, _uy = (q).y, _uz = (q).z; \
    float _vx = (v).x, _vy = (v).y, _vz = (v).z; \
    float _s  = (q).w; \
    float _udotv = _ux*_vx + _uy*_vy + _uz*_vz; \
    float _udotu = _ux*_ux + _uy*_uy + _uz*_uz; \
    float _cx = _uy*_vz - _uz*_vy; \
    float _cy = _uz*_vx - _ux*_vz; \
    float _cz = _ux*_vy - _uy*_vx; \
    (w_vec3){ \
        2.0f*_udotv*_ux + (_s*_s - _udotu)*_vx + 2.0f*_s*_cx, \
        2.0f*_udotv*_uy + (_s*_s - _udotu)*_vy + 2.0f*_s*_cy, \
        2.0f*_udotv*_uz + (_s*_s - _udotu)*_vz + 2.0f*_s*_cz  \
    }; \
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

