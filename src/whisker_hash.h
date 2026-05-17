/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_hash
 * @created     : Saturday May 16, 2026 22:35:38 CST
 * @description : 
 */

#include "whisker_std.h"

#ifndef WHISKER_HASH_H
#define WHISKER_HASH_H

#define w_hash_combine(h, v) \
    ((h) ^ ((size_t)(v) + 0x9e3779b9 + ((h) << 6) + ((h) >> 2)))
// Count variadic args (up to 8)
#define W_NARGS(...) W_NARGS_(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1)
#define W_NARGS_(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
// Dispatcher
#define w_hash(...) W_HASH_N(W_NARGS(__VA_ARGS__), __VA_ARGS__)
#define W_HASH_N(n, ...) W_HASH_N_(n, __VA_ARGS__)
#define W_HASH_N_(n, ...) W_HASH_##n(__VA_ARGS__)
// Individual implementations
#define W_HASH_1(a) ((size_t)(a))
#define W_HASH_2(a, b) w_hash_combine(W_HASH_1(a), (b))
#define W_HASH_3(a, b, c) w_hash_combine(W_HASH_2(a, b), (c))
#define W_HASH_4(a, b, c, d) w_hash_combine(W_HASH_3(a, b, c), (d))
#define W_HASH_5(a, b, c, d, e) w_hash_combine(W_HASH_4(a, b, c, d), (e))
#define W_HASH_6(a, b, c, d, e, f) w_hash_combine(W_HASH_5(a, b, c, d, e), (f))
#define W_HASH_7(a, b, c, d, e, f, g) w_hash_combine(W_HASH_6(a, b, c, d, e, f), (g))
#define W_HASH_8(a, b, c, d, e, f, g, h) w_hash_combine(W_HASH_7(a, b, c, d, e, f, g), (h))


#endif /* WHISKER_HASH_H */

