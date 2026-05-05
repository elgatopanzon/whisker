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
#ifndef W_RAD2DEG
#define W_RAD2DEG (180.0f / W_PI)
#endif
#ifndef W_EPSILON
#define W_EPSILON 1e-6f
#endif


/**************
*  scalars   *
**************/

#define w_minf(a, b) ((a) < (b) ? (a) : (b))
#define w_maxf(a, b) ((a) > (b) ? (a) : (b))
#define w_absf(a) ((a) < 0.0f ? -(a) : (a))
#define w_signf(a) ((a) > 0.0f ? 1.0f : ((a) < 0.0f ? -1.0f : 0.0f))
#define w_clampf(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))
#define w_lerpf(a, b, t) ((a) + ((b) - (a)) * (t))
#define w_remapf(v, in_lo, in_hi, out_lo, out_hi) \
	((out_lo) + ((v) - (in_lo)) / ((in_hi) - (in_lo)) * ((out_hi) - (out_lo)))
#define w_smoothstepf(edge0, edge1, x) ({ \
	float _t = w_clampf(((x) - (edge0)) / ((edge1) - (edge0)), 0.0f, 1.0f); \
	_t * _t * (3.0f - 2.0f * _t); \
})
#define w_smootherstepf(edge0, edge1, x) ({ \
	float _t = w_clampf(((x) - (edge0)) / ((edge1) - (edge0)), 0.0f, 1.0f); \
	_t * _t * _t * (_t * (_t * 6.0f - 15.0f) + 10.0f); \
})
#define w_deg2rad(deg) ((deg) * (float)W_DEG2RAD)
#define w_rad2deg(rad) ((rad) * (float)W_RAD2DEG)


/************
*  w_vec2  *
************/

#define w_vec2_add(a, b) ((w_vec2){ (a).x + (b).x, (a).y + (b).y })
#define w_vec2_sub(a, b) ((w_vec2){ (a).x - (b).x, (a).y - (b).y })
#define w_vec2_mul(a, b) ((w_vec2){ (a).x * (b).x, (a).y * (b).y })
#define w_vec2_div(a, b) ((w_vec2){ (a).x / (b).x, (a).y / (b).y })
#define w_vec2_scale(v, s) ((w_vec2){ (v).x * (s), (v).y * (s) })
#define w_vec2_negate(v) ((w_vec2){ -(v).x, -(v).y })

#define w_vec2_dot(a, b) ((a).x * (b).x + (a).y * (b).y)
#define w_vec2_length_squared(v) ((v).x * (v).x + (v).y * (v).y)
#define w_vec2_length(v) sqrtf(w_vec2_length_squared(v))
#define w_vec2_distance(a, b) ({ \
	float _dx = (b).x - (a).x; \
	float _dy = (b).y - (a).y; \
	sqrtf(_dx * _dx + _dy * _dy); \
})

#define w_vec2_normalise(v) ({ \
	float _len = w_vec2_length(v); \
	_len > 0.0f ? (w_vec2){ (v).x / _len, (v).y / _len } : (w_vec2){ 0.0f, 0.0f }; \
})
#define w_vec2_normalize(v) w_vec2_normalise(v)

#define w_vec2_lerp(a, b, t) ((w_vec2){ \
	(a).x + ((b).x - (a).x) * (t), \
	(a).y + ((b).y - (a).y) * (t) \
})
#define w_vec2_clamp(v, lo, hi) ((w_vec2){ \
	w_clampf((v).x, (lo).x, (hi).x), \
	w_clampf((v).y, (lo).y, (hi).y) \
})
#define w_vec2_min(a, b) ((w_vec2){ w_minf((a).x, (b).x), w_minf((a).y, (b).y) })
#define w_vec2_max(a, b) ((w_vec2){ w_maxf((a).x, (b).x), w_maxf((a).y, (b).y) })

#define w_vec2_reflect(v, n) ({ \
	float _d = 2.0f * w_vec2_dot(v, n); \
	(w_vec2){ (v).x - _d * (n).x, (v).y - _d * (n).y }; \
})
#define w_vec2_perpendicular(v) ((w_vec2){ -(v).y, (v).x })
#define w_vec2_rotate(v, angle) ({ \
	float _c = cosf(angle); \
	float _s = sinf(angle); \
	(w_vec2){ (v).x * _c - (v).y * _s, (v).x * _s + (v).y * _c }; \
})


/************
*  w_vec3  *
************/

#define w_vec3_add(a, b) ((w_vec3){ (a).x + (b).x, (a).y + (b).y, (a).z + (b).z })
#define w_vec3_sub(a, b) ((w_vec3){ (a).x - (b).x, (a).y - (b).y, (a).z - (b).z })
#define w_vec3_mul(a, b) ((w_vec3){ (a).x * (b).x, (a).y * (b).y, (a).z * (b).z })
#define w_vec3_div(a, b) ((w_vec3){ (a).x / (b).x, (a).y / (b).y, (a).z / (b).z })
#define w_vec3_scale(v, s) ((w_vec3){ (v).x * (s), (v).y * (s), (v).z * (s) })
#define w_vec3_negate(v) ((w_vec3){ -(v).x, -(v).y, -(v).z })

#define w_vec3_dot(a, b) ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z)
#define w_vec3_cross(a, b) ((w_vec3){ \
	(a).y * (b).z - (a).z * (b).y, \
	(a).z * (b).x - (a).x * (b).z, \
	(a).x * (b).y - (a).y * (b).x })
#define w_vec3_length_squared(v) ((v).x * (v).x + (v).y * (v).y + (v).z * (v).z)
#define w_vec3_length(v) sqrtf(w_vec3_length_squared(v))
#define w_vec3_distance(a, b) ({ \
	float _dx = (b).x - (a).x; \
	float _dy = (b).y - (a).y; \
	float _dz = (b).z - (a).z; \
	sqrtf(_dx * _dx + _dy * _dy + _dz * _dz); \
})

#define w_vec3_normalise(v) ({ \
	float _len = sqrtf((v).x * (v).x + (v).y * (v).y + (v).z * (v).z); \
	_len > 0.0f ? (w_vec3){ (v).x / _len, (v).y / _len, (v).z / _len } : (w_vec3){ 0.0f, 0.0f, 0.0f }; \
})
#define w_vec3_normalize(v) w_vec3_normalise(v)

#define w_vec3_lerp(a, b, t) ((w_vec3){ \
	(a).x + ((b).x - (a).x) * (t), \
	(a).y + ((b).y - (a).y) * (t), \
	(a).z + ((b).z - (a).z) * (t) \
})
#define w_vec3_clamp(v, lo, hi) ((w_vec3){ \
	w_clampf((v).x, (lo).x, (hi).x), \
	w_clampf((v).y, (lo).y, (hi).y), \
	w_clampf((v).z, (lo).z, (hi).z) \
})
#define w_vec3_min(a, b) ((w_vec3){ \
	w_minf((a).x, (b).x), w_minf((a).y, (b).y), w_minf((a).z, (b).z) })
#define w_vec3_max(a, b) ((w_vec3){ \
	w_maxf((a).x, (b).x), w_maxf((a).y, (b).y), w_maxf((a).z, (b).z) })

#define w_vec3_reflect(v, n) ({ \
	float _d = 2.0f * w_vec3_dot(v, n); \
	(w_vec3){ (v).x - _d * (n).x, (v).y - _d * (n).y, (v).z - _d * (n).z }; \
})
#define w_vec3_project(v, onto) ({ \
	float _d = w_vec3_dot(onto, onto); \
	float _s = _d > 0.0f ? w_vec3_dot(v, onto) / _d : 0.0f; \
	(w_vec3){ (onto).x * _s, (onto).y * _s, (onto).z * _s }; \
})
#define w_vec3_reject(v, onto) ({ \
	w_vec3 _proj = w_vec3_project(v, onto); \
	(w_vec3){ (v).x - _proj.x, (v).y - _proj.y, (v).z - _proj.z }; \
})


/************
*  w_vec4  *
************/

#define w_vec4_add(a, b) ((w_vec4){ (a).x + (b).x, (a).y + (b).y, (a).z + (b).z, (a).w + (b).w })
#define w_vec4_sub(a, b) ((w_vec4){ (a).x - (b).x, (a).y - (b).y, (a).z - (b).z, (a).w - (b).w })
#define w_vec4_mul(a, b) ((w_vec4){ (a).x * (b).x, (a).y * (b).y, (a).z * (b).z, (a).w * (b).w })
#define w_vec4_div(a, b) ((w_vec4){ (a).x / (b).x, (a).y / (b).y, (a).z / (b).z, (a).w / (b).w })
#define w_vec4_scale(v, s) ((w_vec4){ (v).x * (s), (v).y * (s), (v).z * (s), (v).w * (s) })
#define w_vec4_negate(v) ((w_vec4){ -(v).x, -(v).y, -(v).z, -(v).w })

#define w_vec4_dot(a, b) ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z + (a).w * (b).w)
#define w_vec4_length_squared(v) ((v).x * (v).x + (v).y * (v).y + (v).z * (v).z + (v).w * (v).w)
#define w_vec4_length(v) sqrtf(w_vec4_length_squared(v))

#define w_vec4_normalise(v) ({ \
	float _len = w_vec4_length(v); \
	_len > 0.0f \
		? (w_vec4){ (v).x / _len, (v).y / _len, (v).z / _len, (v).w / _len } \
		: (w_vec4){ 0.0f, 0.0f, 0.0f, 0.0f }; \
})
#define w_vec4_normalize(v) w_vec4_normalise(v)

#define w_vec4_lerp(a, b, t) ((w_vec4){ \
	(a).x + ((b).x - (a).x) * (t), \
	(a).y + ((b).y - (a).y) * (t), \
	(a).z + ((b).z - (a).z) * (t), \
	(a).w + ((b).w - (a).w) * (t) \
})
#define w_vec4_clamp(v, lo, hi) ((w_vec4){ \
	w_clampf((v).x, (lo).x, (hi).x), \
	w_clampf((v).y, (lo).y, (hi).y), \
	w_clampf((v).z, (lo).z, (hi).z), \
	w_clampf((v).w, (lo).w, (hi).w) \
})
#define w_vec4_min(a, b) ((w_vec4){ \
	w_minf((a).x, (b).x), w_minf((a).y, (b).y), \
	w_minf((a).z, (b).z), w_minf((a).w, (b).w) })
#define w_vec4_max(a, b) ((w_vec4){ \
	w_maxf((a).x, (b).x), w_maxf((a).y, (b).y), \
	w_maxf((a).z, (b).z), w_maxf((a).w, (b).w) })


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
    float _len = sqrtf((q).x * (q).x + (q).y * (q).y + (q).z * (q).z + (q).w * (q).w); \
    _len > 0.0f ? \
        ((w_quat){ (q).x / _len, (q).y / _len, (q).z / _len, (q).w / _len }) : \
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

/* rotation quaternions around a single axis (angle in degrees) */
#define w_quat_rotation_x_deg(deg) w_quat_rotation_x((deg) * W_DEG2RAD)
#define w_quat_rotation_y_deg(deg) w_quat_rotation_y((deg) * W_DEG2RAD)
#define w_quat_rotation_z_deg(deg) w_quat_rotation_z((deg) * W_DEG2RAD)

/* Hamilton product: compose rotation a then b */
#define w_quat_mul(a, b) ((w_quat){ \
    (b).w*(a).x + (b).x*(a).w + (b).y*(a).z - (b).z*(a).y, \
    (b).w*(a).y - (b).x*(a).z + (b).y*(a).w + (b).z*(a).x, \
    (b).w*(a).z + (b).x*(a).y - (b).y*(a).x + (b).z*(a).w, \
    (b).w*(a).w - (b).x*(a).x - (b).y*(a).y - (b).z*(a).z  \
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

/* linear interpolation (not normalized -- useful as building block) */
#define w_quat_lerp(a, b, t) ((w_quat){ \
    (a).x + ((b).x - (a).x) * (t), \
    (a).y + ((b).y - (a).y) * (t), \
    (a).z + ((b).z - (a).z) * (t), \
    (a).w + ((b).w - (a).w) * (t) \
})

/* normalized linear interpolation (fast alternative to slerp) */
#define w_quat_nlerp(a, b, t) ({ \
    float _dot = w_quat_dot(a, b); \
    w_quat _tb = (b); \
    if (_dot < 0.0f) { \
        _tb.x = -(b).x; _tb.y = -(b).y; \
        _tb.z = -(b).z; _tb.w = -(b).w; \
    } \
    w_quat _r = (w_quat){ \
        (a).x + (t) * (_tb.x - (a).x), \
        (a).y + (t) * (_tb.y - (a).y), \
        (a).z + (t) * (_tb.z - (a).z), \
        (a).w + (t) * (_tb.w - (a).w) \
    }; \
    w_quat_normalise(_r); \
})

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

/* look_rotation: direction vector -> quaternion (up defaults to Y-up) */
#define w_quat_look_rotation(forward, up) ({ \
    w_vec3 _f = w_vec3_normalise(forward); \
    w_vec3 _r = w_vec3_normalise(w_vec3_cross(up, _f)); \
    w_vec3 _u = w_vec3_cross(_f, _r); \
    /* build rotation matrix -> quaternion */ \
    float _m00 = _r.x, _m01 = _u.x, _m02 = _f.x; \
    float _m10 = _r.y, _m11 = _u.y, _m12 = _f.y; \
    float _m20 = _r.z, _m21 = _u.z, _m22 = _f.z; \
    float _trace = _m00 + _m11 + _m22; \
    w_quat _q; \
    if (_trace > 0.0f) { \
        float _s = 0.5f / sqrtf(_trace + 1.0f); \
        _q.w = 0.25f / _s; \
        _q.x = (_m21 - _m12) * _s; \
        _q.y = (_m02 - _m20) * _s; \
        _q.z = (_m10 - _m01) * _s; \
    } else if (_m00 > _m11 && _m00 > _m22) { \
        float _s = 2.0f * sqrtf(1.0f + _m00 - _m11 - _m22); \
        _q.w = (_m21 - _m12) / _s; \
        _q.x = 0.25f * _s; \
        _q.y = (_m01 + _m10) / _s; \
        _q.z = (_m02 + _m20) / _s; \
    } else if (_m11 > _m22) { \
        float _s = 2.0f * sqrtf(1.0f + _m11 - _m00 - _m22); \
        _q.w = (_m02 - _m20) / _s; \
        _q.x = (_m01 + _m10) / _s; \
        _q.y = 0.25f * _s; \
        _q.z = (_m12 + _m21) / _s; \
    } else { \
        float _s = 2.0f * sqrtf(1.0f + _m22 - _m00 - _m11); \
        _q.w = (_m10 - _m01) / _s; \
        _q.x = (_m02 + _m20) / _s; \
        _q.y = (_m12 + _m21) / _s; \
        _q.z = 0.25f * _s; \
    } \
    w_quat_normalise(_q); \
})


/************
*  w_mat4  *
************/

/* column-major 4x4 matrix layout:
 * m[0]  m[4]  m[8]   m[12]
 * m[1]  m[5]  m[9]   m[13]
 * m[2]  m[6]  m[10]  m[14]
 * m[3]  m[7]  m[11]  m[15]
 */

#define w_mat4_identity() ((w_mat4){ .m = { \
    1, 0, 0, 0, \
    0, 1, 0, 0, \
    0, 0, 1, 0, \
    0, 0, 0, 1 }})

#define w_mat4_mul(a, b) ({ \
    w_mat4 _r; \
    for (int _c = 0; _c < 4; _c++) { \
        for (int _row = 0; _row < 4; _row++) { \
            _r.m[_c*4 + _row] = \
                (a).m[0*4 + _row] * (b).m[_c*4 + 0] + \
                (a).m[1*4 + _row] * (b).m[_c*4 + 1] + \
                (a).m[2*4 + _row] * (b).m[_c*4 + 2] + \
                (a).m[3*4 + _row] * (b).m[_c*4 + 3]; \
        } \
    } \
    _r; \
})

#define w_mat4_mul_vec4(m, v) ((w_vec4){ \
    (m).m[0]*(v).x + (m).m[4]*(v).y + (m).m[8]*(v).z  + (m).m[12]*(v).w, \
    (m).m[1]*(v).x + (m).m[5]*(v).y + (m).m[9]*(v).z  + (m).m[13]*(v).w, \
    (m).m[2]*(v).x + (m).m[6]*(v).y + (m).m[10]*(v).z + (m).m[14]*(v).w, \
    (m).m[3]*(v).x + (m).m[7]*(v).y + (m).m[11]*(v).z + (m).m[15]*(v).w  \
})

/* transform point (applies translation) */
#define w_mat4_transform_point(m, p) ((w_vec3){ \
    (m).m[0]*(p).x + (m).m[4]*(p).y + (m).m[8]*(p).z  + (m).m[12], \
    (m).m[1]*(p).x + (m).m[5]*(p).y + (m).m[9]*(p).z  + (m).m[13], \
    (m).m[2]*(p).x + (m).m[6]*(p).y + (m).m[10]*(p).z + (m).m[14]  \
})

/* transform direction (ignores translation) */
#define w_mat4_transform_direction(m, d) ((w_vec3){ \
    (m).m[0]*(d).x + (m).m[4]*(d).y + (m).m[8]*(d).z, \
    (m).m[1]*(d).x + (m).m[5]*(d).y + (m).m[9]*(d).z, \
    (m).m[2]*(d).x + (m).m[6]*(d).y + (m).m[10]*(d).z \
})

#define w_mat4_translate(tx, ty, tz) ((w_mat4){ .m = { \
    1, 0, 0, 0, \
    0, 1, 0, 0, \
    0, 0, 1, 0, \
    (tx), (ty), (tz), 1 }})

#define w_mat4_scale(sx, sy, sz) ((w_mat4){ .m = { \
    (sx), 0, 0, 0, \
    0, (sy), 0, 0, \
    0, 0, (sz), 0, \
    0, 0, 0, 1 }})

/* rotation matrix from quaternion */
#define w_mat4_rotate(q) ({ \
    float _xx = (q).x*(q).x, _yy = (q).y*(q).y, _zz = (q).z*(q).z; \
    float _xy = (q).x*(q).y, _xz = (q).x*(q).z, _yz = (q).y*(q).z; \
    float _wx = (q).w*(q).x, _wy = (q).w*(q).y, _wz = (q).w*(q).z; \
    (w_mat4){ .m = { \
        1-2*(_yy+_zz), 2*(_xy+_wz), 2*(_xz-_wy), 0, \
        2*(_xy-_wz), 1-2*(_xx+_zz), 2*(_yz+_wx), 0, \
        2*(_xz+_wy), 2*(_yz-_wx), 1-2*(_xx+_yy), 0, \
        0, 0, 0, 1 }}; \
})

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

/* decompose a TRS matrix into position, rotation (quat), scale */
#define w_mat4_decompose(mat, pos_ptr, rot_ptr, scale_ptr) do { \
    /* extract translation */ \
    (pos_ptr)->x = (mat).m[12]; \
    (pos_ptr)->y = (mat).m[13]; \
    (pos_ptr)->z = (mat).m[14]; \
    /* extract scale from column lengths */ \
    float _sx = sqrtf((mat).m[0]*(mat).m[0] + (mat).m[1]*(mat).m[1] + (mat).m[2]*(mat).m[2]); \
    float _sy = sqrtf((mat).m[4]*(mat).m[4] + (mat).m[5]*(mat).m[5] + (mat).m[6]*(mat).m[6]); \
    float _sz = sqrtf((mat).m[8]*(mat).m[8] + (mat).m[9]*(mat).m[9] + (mat).m[10]*(mat).m[10]); \
    (scale_ptr)->x = _sx; \
    (scale_ptr)->y = _sy; \
    (scale_ptr)->z = _sz; \
    /* build normalized rotation matrix */ \
    float _r00 = (mat).m[0]/_sx, _r01 = (mat).m[4]/_sy, _r02 = (mat).m[8]/_sz; \
    float _r10 = (mat).m[1]/_sx, _r11 = (mat).m[5]/_sy, _r12 = (mat).m[9]/_sz; \
    float _r20 = (mat).m[2]/_sx, _r21 = (mat).m[6]/_sy, _r22 = (mat).m[10]/_sz; \
    /* rotation matrix to quaternion */ \
    float _trace = _r00 + _r11 + _r22; \
    if (_trace > 0.0f) { \
        float _s = 0.5f / sqrtf(_trace + 1.0f); \
        (rot_ptr)->w = 0.25f / _s; \
        (rot_ptr)->x = (_r21 - _r12) * _s; \
        (rot_ptr)->y = (_r02 - _r20) * _s; \
        (rot_ptr)->z = (_r10 - _r01) * _s; \
    } else if (_r00 > _r11 && _r00 > _r22) { \
        float _s = 2.0f * sqrtf(1.0f + _r00 - _r11 - _r22); \
        (rot_ptr)->w = (_r21 - _r12) / _s; \
        (rot_ptr)->x = 0.25f * _s; \
        (rot_ptr)->y = (_r01 + _r10) / _s; \
        (rot_ptr)->z = (_r02 + _r20) / _s; \
    } else if (_r11 > _r22) { \
        float _s = 2.0f * sqrtf(1.0f + _r11 - _r00 - _r22); \
        (rot_ptr)->w = (_r02 - _r20) / _s; \
        (rot_ptr)->x = (_r01 + _r10) / _s; \
        (rot_ptr)->y = 0.25f * _s; \
        (rot_ptr)->z = (_r12 + _r21) / _s; \
    } else { \
        float _s = 2.0f * sqrtf(1.0f + _r22 - _r00 - _r11); \
        (rot_ptr)->w = (_r10 - _r01) / _s; \
        (rot_ptr)->x = (_r02 + _r20) / _s; \
        (rot_ptr)->y = (_r12 + _r21) / _s; \
        (rot_ptr)->z = 0.25f * _s; \
    } \
} while (0)

#define w_mat4_transpose(mat) ((w_mat4){ .m = { \
    (mat).m[0], (mat).m[4], (mat).m[8],  (mat).m[12], \
    (mat).m[1], (mat).m[5], (mat).m[9],  (mat).m[13], \
    (mat).m[2], (mat).m[6], (mat).m[10], (mat).m[14], \
    (mat).m[3], (mat).m[7], (mat).m[11], (mat).m[15] }})

/* 4x4 matrix inverse via cofactor expansion */
#define w_mat4_inverse(mat) ({ \
    const float *_s = (mat).m; \
    float _c00 = _s[5]*(_s[10]*_s[15]-_s[11]*_s[14]) - _s[9]*(_s[6]*_s[15]-_s[7]*_s[14]) + _s[13]*(_s[6]*_s[11]-_s[7]*_s[10]); \
    float _c01 = _s[4]*(_s[10]*_s[15]-_s[11]*_s[14]) - _s[8]*(_s[6]*_s[15]-_s[7]*_s[14]) + _s[12]*(_s[6]*_s[11]-_s[7]*_s[10]); \
    float _c02 = _s[4]*(_s[9]*_s[15]-_s[11]*_s[13]) - _s[8]*(_s[5]*_s[15]-_s[7]*_s[13]) + _s[12]*(_s[5]*_s[11]-_s[7]*_s[9]); \
    float _c03 = _s[4]*(_s[9]*_s[14]-_s[10]*_s[13]) - _s[8]*(_s[5]*_s[14]-_s[6]*_s[13]) + _s[12]*(_s[5]*_s[10]-_s[6]*_s[9]); \
    float _det = _s[0]*_c00 - _s[1]*_c01 + _s[2]*_c02 - _s[3]*_c03; \
    w_mat4 _inv; \
    if (w_absf(_det) < W_EPSILON) { \
        _inv = w_mat4_identity(); \
    } else { \
        float _id = 1.0f / _det; \
        _inv.m[0]  =  _c00 * _id; \
        _inv.m[1]  = -(_s[1]*(_s[10]*_s[15]-_s[11]*_s[14]) - _s[9]*(_s[2]*_s[15]-_s[3]*_s[14]) + _s[13]*(_s[2]*_s[11]-_s[3]*_s[10])) * _id; \
        _inv.m[2]  =  (_s[1]*(_s[6]*_s[15]-_s[7]*_s[14]) - _s[5]*(_s[2]*_s[15]-_s[3]*_s[14]) + _s[13]*(_s[2]*_s[7]-_s[3]*_s[6])) * _id; \
        _inv.m[3]  = -(_s[1]*(_s[6]*_s[11]-_s[7]*_s[10]) - _s[5]*(_s[2]*_s[11]-_s[3]*_s[10]) + _s[9]*(_s[2]*_s[7]-_s[3]*_s[6])) * _id; \
        _inv.m[4]  = -_c01 * _id; \
        _inv.m[5]  =  (_s[0]*(_s[10]*_s[15]-_s[11]*_s[14]) - _s[8]*(_s[2]*_s[15]-_s[3]*_s[14]) + _s[12]*(_s[2]*_s[11]-_s[3]*_s[10])) * _id; \
        _inv.m[6]  = -(_s[0]*(_s[6]*_s[15]-_s[7]*_s[14]) - _s[4]*(_s[2]*_s[15]-_s[3]*_s[14]) + _s[12]*(_s[2]*_s[7]-_s[3]*_s[6])) * _id; \
        _inv.m[7]  =  (_s[0]*(_s[6]*_s[11]-_s[7]*_s[10]) - _s[4]*(_s[2]*_s[11]-_s[3]*_s[10]) + _s[8]*(_s[2]*_s[7]-_s[3]*_s[6])) * _id; \
        _inv.m[8]  =  _c02 * _id; \
        _inv.m[9]  = -(_s[0]*(_s[9]*_s[15]-_s[11]*_s[13]) - _s[8]*(_s[1]*_s[15]-_s[3]*_s[13]) + _s[12]*(_s[1]*_s[11]-_s[3]*_s[9])) * _id; \
        _inv.m[10] =  (_s[0]*(_s[5]*_s[15]-_s[7]*_s[13]) - _s[4]*(_s[1]*_s[15]-_s[3]*_s[13]) + _s[12]*(_s[1]*_s[7]-_s[3]*_s[5])) * _id; \
        _inv.m[11] = -(_s[0]*(_s[5]*_s[11]-_s[7]*_s[9]) - _s[4]*(_s[1]*_s[11]-_s[3]*_s[9]) + _s[8]*(_s[1]*_s[7]-_s[3]*_s[5])) * _id; \
        _inv.m[12] = -_c03 * _id; \
        _inv.m[13] =  (_s[0]*(_s[9]*_s[14]-_s[10]*_s[13]) - _s[8]*(_s[1]*_s[14]-_s[2]*_s[13]) + _s[12]*(_s[1]*_s[10]-_s[2]*_s[9])) * _id; \
        _inv.m[14] = -(_s[0]*(_s[5]*_s[14]-_s[6]*_s[13]) - _s[4]*(_s[1]*_s[14]-_s[2]*_s[13]) + _s[12]*(_s[1]*_s[6]-_s[2]*_s[5])) * _id; \
        _inv.m[15] =  (_s[0]*(_s[5]*_s[10]-_s[6]*_s[9]) - _s[4]*(_s[1]*_s[10]-_s[2]*_s[9]) + _s[8]*(_s[1]*_s[6]-_s[2]*_s[5])) * _id; \
    } \
    _inv; \
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

/* perspective projection (fov_y in radians, aspect = width/height) */
#define w_mat4_perspective(fov_y, aspect, near, far) ({ \
    float _t = tanf((fov_y) * 0.5f); \
    float _range = (near) - (far); \
    (w_mat4){ .m = { \
        1.0f / ((aspect) * _t), 0, 0, 0, \
        0, 1.0f / _t, 0, 0, \
        0, 0, ((far) + (near)) / _range, -1, \
        0, 0, 2.0f * (far) * (near) / _range, 0 }}; \
})

/* orthographic projection */
#define w_mat4_orthographic(left, right, bottom, top, near, far) ((w_mat4){ .m = { \
    2.0f / ((right)-(left)), 0, 0, 0, \
    0, 2.0f / ((top)-(bottom)), 0, 0, \
    0, 0, -2.0f / ((far)-(near)), 0, \
    -((right)+(left))/((right)-(left)), -((top)+(bottom))/((top)-(bottom)), -((far)+(near))/((far)-(near)), 1 }})


/*************
*  w_color  *
*************/

#define w_color_lerp(a, b, t) ((w_color){ \
    (a).r + ((b).r - (a).r) * (t), \
    (a).g + ((b).g - (a).g) * (t), \
    (a).b + ((b).b - (a).b) * (t), \
    (a).a + ((b).a - (a).a) * (t) \
})

#define w_color_mul(a, b) ((w_color){ \
    (a).r * (b).r, (a).g * (b).g, (a).b * (b).b, (a).a * (b).a })

#define w_color_add(a, b) ((w_color){ \
    w_minf((a).r + (b).r, 1.0f), \
    w_minf((a).g + (b).g, 1.0f), \
    w_minf((a).b + (b).b, 1.0f), \
    w_minf((a).a + (b).a, 1.0f) })

#define w_color_scale(c, s) ((w_color){ \
    w_clampf((c).r * (s), 0.0f, 1.0f), \
    w_clampf((c).g * (s), 0.0f, 1.0f), \
    w_clampf((c).b * (s), 0.0f, 1.0f), \
    (c).a })

/* RGBA -> HSVA conversion (all floats 0-1, hue 0-1 representing 0-360) */
#define w_color_to_hsva(c) ({ \
    float _r = (c).r, _g = (c).g, _b = (c).b; \
    float _max = w_maxf(_r, w_maxf(_g, _b)); \
    float _min = w_minf(_r, w_minf(_g, _b)); \
    float _d = _max - _min; \
    float _h = 0.0f, _s = 0.0f, _v = _max; \
    if (_d > W_EPSILON) { \
        _s = _d / _max; \
        if (_r >= _max)      _h = (_g - _b) / _d; \
        else if (_g >= _max) _h = 2.0f + (_b - _r) / _d; \
        else                 _h = 4.0f + (_r - _g) / _d; \
        _h /= 6.0f; \
        if (_h < 0.0f) _h += 1.0f; \
    } \
    (w_color){ _h, _s, _v, (c).a }; \
})

/* HSVA -> RGBA conversion (h 0-1, s 0-1, v 0-1) */
#define w_color_from_hsva(c) ({ \
    float _h = (c).r * 6.0f, _s = (c).g, _v = (c).b; \
    int _i = (int)_h; \
    float _f = _h - _i; \
    float _p = _v * (1.0f - _s); \
    float _q = _v * (1.0f - _s * _f); \
    float _t = _v * (1.0f - _s * (1.0f - _f)); \
    w_color _out; \
    _out.a = (c).a; \
    switch (_i % 6) { \
        case 0: _out.r = _v; _out.g = _t; _out.b = _p; break; \
        case 1: _out.r = _q; _out.g = _v; _out.b = _p; break; \
        case 2: _out.r = _p; _out.g = _v; _out.b = _t; break; \
        case 3: _out.r = _p; _out.g = _q; _out.b = _v; break; \
        case 4: _out.r = _t; _out.g = _p; _out.b = _v; break; \
        default: _out.r = _v; _out.g = _p; _out.b = _q; break; \
    } \
    _out; \
})

/* convert between w_color (float) and w_color8 (uint8) */
#define w_color_to_color8(c) ((w_color8){ \
    (uint8_t)((c).r * 255.0f + 0.5f), \
    (uint8_t)((c).g * 255.0f + 0.5f), \
    (uint8_t)((c).b * 255.0f + 0.5f), \
    (uint8_t)((c).a * 255.0f + 0.5f) })

#define w_color8_to_color(c) ((w_color){ \
    (c).r / 255.0f, (c).g / 255.0f, (c).b / 255.0f, (c).a / 255.0f })

#endif /* WHISKER_MATH_H */
