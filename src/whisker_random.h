/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_random
 * @created     : Sunday Mar 01, 2026 17:30:46 CST
 * @description : random generation utils
 */

#include "whisker_std.h"

#ifndef WHISKER_RANDOM_H
#define WHISKER_RANDOM_H

#include <stdint.h>
#include <stddef.h>

// get random bytes functions
#if defined(_WIN32)
	#include <windows.h>
	#include <bcrypt.h>
	#define w_rand_bytes(buf, len) \
		BCryptGenRandom(NULL, (buf), (ULONG)(len), BCRYPT_USE_SYSTEM_PREFERRED_RNG)
#elif defined(__APPLE__) || defined(__ANDROID__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    #include <stdlib.h>
    #define w_rand_bytes(buf, len) arc4random_buf((buf), (len))
#elif defined(__linux__)
	#include <sys/random.h>
	#define w_rand_bytes(buf, len) getrandom((buf), (len), 0)
#else
	#include <stdio.h>
	#define w_rand_bytes(buf, len) do { \
		FILE* _f = fopen("/dev/urandom", "rb"); \
		fread((buf), 1, (len), _f); \
		fclose(_f); \
	} while(0)
#endif

// generate random hex string of exactly hex_count hex chars + null
// buf must be at least (hex_count + 1) chars
#define w_rand_chars(buf, hex_count) do { \
	size_t _byte_count = ((hex_count) + 1) / 2; \
	uint8_t _w_rand_tmp[_byte_count]; \
	w_rand_bytes(_w_rand_tmp, _byte_count); \
	for (size_t _w_rand_i = 0; _w_rand_i < (hex_count); _w_rand_i++) { \
		uint8_t _b = _w_rand_tmp[_w_rand_i / 2]; \
		(buf)[_w_rand_i] = "0123456789abcdef"[(_b >> (4 - 4 * (_w_rand_i & 1))) & 0x0F]; \
	} \
	(buf)[hex_count] = '\0'; \
} while(0)

// generate random float in [0.0, 1.0)
static inline float w_rand_float(void) {
	uint32_t r;
	w_rand_bytes((uint8_t *)&r, sizeof(r));
	return (float)(r >> 8) / 16777216.0f;
}

// PCG (Permuted Congruential Generator) functions
// standalone stateless functions - caller manages state and increment

// PCG multiplier constant
#define W_PCG_MULTIPLIER 6364136223846793005ULL

// initialize PCG state and increment from seed and stream
// increment is always odd: (stream << 1) | 1
static inline void w_pcg_init(uint64_t *state, uint64_t *inc, uint64_t seed, uint64_t stream) {
	*inc = (stream << 1) | 1;
	*state = 0;
	*state = *state * W_PCG_MULTIPLIER + *inc;
	*state += seed;
	*state = *state * W_PCG_MULTIPLIER + *inc;
}

// advance PCG state and return 32-bit output (PCG-XSH-RR variant)
static inline uint32_t w_pcg_step(uint64_t *state, uint64_t inc) {
	uint64_t oldstate = *state;
	*state = oldstate * W_PCG_MULTIPLIER + inc;
	uint32_t xorshifted = (uint32_t)(((oldstate >> 18) ^ oldstate) >> 27);
	uint32_t rot = (uint32_t)(oldstate >> 59);
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

// generate double in [0.0, 1.0)
static inline double w_pcg_next_double(uint64_t *state, uint64_t inc) {
	uint32_t r = w_pcg_step(state, inc);
	return (double)r / 4294967296.0;
}

// generate int64 in [min, max] inclusive
static inline int64_t w_pcg_next_int64(uint64_t *state, uint64_t inc, int64_t min, int64_t max) {
	if (min > max) {
		int64_t tmp = min;
		min = max;
		max = tmp;
	}
	uint64_t range = (uint64_t)(max - min) + 1;
	if (range == 0) {
		// full 64-bit range requested
		uint32_t lo = w_pcg_step(state, inc);
		uint32_t hi = w_pcg_step(state, inc);
		return (int64_t)(((uint64_t)hi << 32) | lo);
	}
	// rejection sampling for unbiased result
	uint64_t threshold = (-range) % range;
	uint64_t r;
	do {
		uint32_t lo = w_pcg_step(state, inc);
		uint32_t hi = w_pcg_step(state, inc);
		r = ((uint64_t)hi << 32) | lo;
	} while (r < threshold);
	return min + (int64_t)(r % range);
}

#endif /* WHISKER_RANDOM_H */

