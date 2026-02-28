/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_hash_xxhash64
 * @created     : Saturday Feb 28, 2026 12:33:34 CST
 */

#include "whisker_hash_xxhash64.h"

/*****************************
*  internal hash functions  *
*****************************/

static inline uint64_t w_xxhash64_rotate_left_(uint64_t x, int r)
{
	return (x << r) | (x >> (64 - r));
}

static inline uint64_t w_xxhash64_round_(uint64_t acc, uint64_t input)
{
	acc += input * XXHASH64_PRIME_2;
	acc = w_xxhash64_rotate_left_(acc, 31);
	return acc * XXHASH64_PRIME_1;
}

static inline uint64_t w_xxhash64_merge_(uint64_t acc, uint64_t val)
{
	val = w_xxhash64_round_(0, val);
	acc ^= val;
	return acc * XXHASH64_PRIME_1 + XXHASH64_PRIME_4;
}

static inline uint64_t w_xxhash64_read64_(const uint8_t *p)
{
	uint64_t v;
	memcpy(&v, p, 8);
	return v;
}

static inline uint32_t w_xxhash64_read32_(const uint8_t *p)
{
	uint32_t v;
	memcpy(&v, p, 4);
	return v;
}


/***************************
*  public hash functions  *
***************************/

uint64_t w_xxhash64_hash(const void *data, size_t data_length, uint64_t seed)
{
    const uint8_t *data_chunks = (const uint8_t *)data;
    const uint8_t *end = data_chunks + data_length;
    uint64_t hash;
    
    // vectorisable for >= 32 length 8 bytes 4 vectors
    if (data_length >= 32) {
        // process 32-byte stripes with 4 accumulators
        uint64_t v1 = seed + XXHASH64_PRIME_1 + XXHASH64_PRIME_2;
        uint64_t v2 = seed + XXHASH64_PRIME_2;
        uint64_t v3 = seed;
        uint64_t v4 = seed - XXHASH64_PRIME_1;
        do {
            v1 = w_xxhash64_round_(v1, w_xxhash64_read64_(data_chunks));      data_chunks += 8;
            v2 = w_xxhash64_round_(v2, w_xxhash64_read64_(data_chunks));      data_chunks += 8;
            v3 = w_xxhash64_round_(v3, w_xxhash64_read64_(data_chunks));      data_chunks += 8;
            v4 = w_xxhash64_round_(v4, w_xxhash64_read64_(data_chunks));      data_chunks += 8;
        } while (data_chunks <= end - 32);
        // merge accumulators
        hash = w_xxhash64_rotate_left_(v1, 1) + w_xxhash64_rotate_left_(v2, 7) +
            w_xxhash64_rotate_left_(v3, 12) + w_xxhash64_rotate_left_(v4, 18);
        hash = w_xxhash64_merge_(hash, v1);
        hash = w_xxhash64_merge_(hash, v2);
        hash = w_xxhash64_merge_(hash, v3);
        hash = w_xxhash64_merge_(hash, v4);
    } else {
        hash = seed + XXHASH64_PRIME_5;
    }

    hash += (uint64_t)data_length;

    // process any remaining 8-byte chunks
    while (data_chunks + 8 <= end) {
        hash ^= w_xxhash64_round_(0, w_xxhash64_read64_(data_chunks));
        hash = w_xxhash64_rotate_left_(hash, 27) * XXHASH64_PRIME_1 + XXHASH64_PRIME_4;
        data_chunks += 8;
    }

    // process any remaining 4-byte chunk
    if (data_chunks + 4 <= end) {
        hash ^= (uint64_t)w_xxhash64_read32_(data_chunks) * XXHASH64_PRIME_1;
        hash = w_xxhash64_rotate_left_(hash, 23) * XXHASH64_PRIME_2 + XXHASH64_PRIME_3;
        data_chunks += 4;
    }

    // process any remaining bytes
    while (data_chunks < end) {
        hash ^= (*data_chunks++) * XXHASH64_PRIME_5;
        hash = w_xxhash64_rotate_left_(hash, 11) * XXHASH64_PRIME_1;
    }

    // final avalanche
    hash ^= hash >> 33;
    hash *= XXHASH64_PRIME_2;
    hash ^= hash >> 29;
    hash *= XXHASH64_PRIME_3;
    hash ^= hash >> 32;
    return hash;
}
