/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_hash_xxhash64
 * @created     : Saturday Feb 28, 2026 12:28:14 CST
 * @description : hash functions for xxHash64 algo
 */

#include "whisker_std.h"

#ifndef WHISKER_HASH_XXHASH64_H
#define WHISKER_HASH_XXHASH64_H

// xxHash64 primes                                                           
#define XXHASH64_PRIME_1 0x9E3779B185EBCA87ULL
#define XXHASH64_PRIME_2 0xC2B2AE3D27D4EB4FULL
#define XXHASH64_PRIME_3 0x165667B19E3779F9ULL
#define XXHASH64_PRIME_4 0x85EBCA77C2B2AE63ULL
#define XXHASH64_PRIME_5 0x27D4EB2F165667C5ULL

// convenience macros
// string literal
#define w_xxhash64_str(s) w_xxhash64_hash((s), sizeof(s) - 1, 0)
// any value
#define w_xxhash64(val) w_xxhash64_hash(&(val), sizeof(val), 0)

// internal hashing functions
static inline uint64_t w_xxhash64_rotate_left_(uint64_t x, int r);
static inline uint64_t w_xxhash64_round_(uint64_t acc, uint64_t input);
static inline uint64_t w_xxhash64_merge_(uint64_t acc, uint64_t val);
static inline uint64_t w_xxhash64_read64_(const uint8_t *p);
static inline uint32_t w_xxhash64_read32_(const uint8_t *p);

// get a uint64_t 64-bit hash for provided data
uint64_t w_xxhash64_hash(const void *data, size_t length, uint64_t seed);

#endif /* WHISKER_HASH_XXHASH64_H */

