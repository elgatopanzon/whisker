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
#elif defined(__linux__)
	#include <sys/random.h>
	#define w_rand_bytes(buf, len) getrandom((buf), (len), 0)
#elif defined(__APPLE__) || defined(__FreeBSD__)
	#include <stdlib.h>
	#define w_rand_bytes(buf, len) arc4random_buf((buf), (len))
#else
	#include <stdio.h>
	#define w_rand_bytes(buf, len) do { \
		FILE* _f = fopen("/dev/urandom", "rb"); \
		fread((buf), 1, (len), _f); \
		fclose(_f); \
	} while(0)
#endif

#endif /* WHISKER_RANDOM_H */

