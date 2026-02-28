/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_hash_fnv1a
 * @created     : Saturday Feb 28, 2026 13:32:58 CST
 * @description : hash functions for FNV-1a algo
 */

#include "whisker_std.h"

#ifndef WHISKER_HASH_FNV1A_H
#define WHISKER_HASH_FNV1A_H

#define FNV_1A_64_OFFSET_BASIS 0xcbf29ce484222325ULL
#define FNV_1A_64_PRIME        0x100000001b3ULL

// convenience macros
// string literal
#define w_fnv1a_str(s) w_fnv1a_hash((s), sizeof(s) - 1, 0)
// any value
#define w_fnv1a(val) w_fnv1a_hash(&(val), sizeof(val), 0)

// get a uint64_t 64-bit hash for provided data
uint64_t w_fnv1a_hash(const void *data, size_t len, uint64_t seed);

#endif /* WHISKER_HASH_FNV1A_H */

