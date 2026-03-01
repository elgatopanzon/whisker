/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : benchmark_whisker_hash_algos
 * @created     : Saturday Feb 28, 2026 13:52:14 CST
 * @description : benchmarks comparing xxhash64 vs fnv1a hash algorithms
 */

#include "whisker_std.h"
#include "whisker_hash_xxhash64.h"
#include "whisker_hash_fnv1a.h"
#include "ubench.h"

#include <string.h>


// test data for various sizes
static uint8_t test_data_1b[1];
static uint8_t test_data_4b[4];
static uint8_t test_data_8b[8];
static uint8_t test_data_16b[16];
static uint8_t test_data_32b[32];
static uint8_t test_data_64b[64];
static uint8_t test_data_128b[128];
static uint8_t test_data_256b[256];
static uint8_t test_data_512b[512];
static uint8_t test_data_1024b[1024];

// real-world pattern test data
static const char *short_id_8 = "user1234";
static const char *short_id_16 = "session_abc12345";
static const char *file_path_64 = "/home/user/projects/whisker/src/whisker_hash_xxhash64.c";
static const char *uuid_36 = "550e8400-e29b-41d4-a716-446655440000";

// throughput test constants
#define THROUGHPUT_ITERATIONS 1000


// ============================================================================
// SIZE BENCHMARKS: 1 byte
// ============================================================================
struct bench_hash_1b { uint64_t result; };
UBENCH_F_SETUP(bench_hash_1b) { memset(test_data_1b, 0xAB, 1); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_1b) { (void)ubench_fixture; }

UBENCH_F(bench_hash_1b, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(test_data_1b, 1, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_1b, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(test_data_1b, 1, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// SIZE BENCHMARKS: 4 bytes
// ============================================================================
struct bench_hash_4b { uint64_t result; };
UBENCH_F_SETUP(bench_hash_4b) { memset(test_data_4b, 0xAB, 4); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_4b) { (void)ubench_fixture; }

UBENCH_F(bench_hash_4b, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(test_data_4b, 4, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_4b, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(test_data_4b, 4, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// SIZE BENCHMARKS: 8 bytes
// ============================================================================
struct bench_hash_8b { uint64_t result; };
UBENCH_F_SETUP(bench_hash_8b) { memset(test_data_8b, 0xAB, 8); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_8b) { (void)ubench_fixture; }

UBENCH_F(bench_hash_8b, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(test_data_8b, 8, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_8b, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(test_data_8b, 8, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// SIZE BENCHMARKS: 16 bytes
// ============================================================================
struct bench_hash_16b { uint64_t result; };
UBENCH_F_SETUP(bench_hash_16b) { memset(test_data_16b, 0xAB, 16); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_16b) { (void)ubench_fixture; }

UBENCH_F(bench_hash_16b, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(test_data_16b, 16, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_16b, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(test_data_16b, 16, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// SIZE BENCHMARKS: 32 bytes
// ============================================================================
struct bench_hash_32b { uint64_t result; };
UBENCH_F_SETUP(bench_hash_32b) { memset(test_data_32b, 0xAB, 32); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_32b) { (void)ubench_fixture; }

UBENCH_F(bench_hash_32b, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(test_data_32b, 32, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_32b, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(test_data_32b, 32, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// SIZE BENCHMARKS: 64 bytes
// ============================================================================
struct bench_hash_64b { uint64_t result; };
UBENCH_F_SETUP(bench_hash_64b) { memset(test_data_64b, 0xAB, 64); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_64b) { (void)ubench_fixture; }

UBENCH_F(bench_hash_64b, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(test_data_64b, 64, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_64b, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(test_data_64b, 64, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// SIZE BENCHMARKS: 128 bytes
// ============================================================================
struct bench_hash_128b { uint64_t result; };
UBENCH_F_SETUP(bench_hash_128b) { memset(test_data_128b, 0xAB, 128); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_128b) { (void)ubench_fixture; }

UBENCH_F(bench_hash_128b, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(test_data_128b, 128, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_128b, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(test_data_128b, 128, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// SIZE BENCHMARKS: 256 bytes
// ============================================================================
struct bench_hash_256b { uint64_t result; };
UBENCH_F_SETUP(bench_hash_256b) { memset(test_data_256b, 0xAB, 256); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_256b) { (void)ubench_fixture; }

UBENCH_F(bench_hash_256b, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(test_data_256b, 256, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_256b, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(test_data_256b, 256, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// SIZE BENCHMARKS: 512 bytes
// ============================================================================
struct bench_hash_512b { uint64_t result; };
UBENCH_F_SETUP(bench_hash_512b) { memset(test_data_512b, 0xAB, 512); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_512b) { (void)ubench_fixture; }

UBENCH_F(bench_hash_512b, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(test_data_512b, 512, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_512b, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(test_data_512b, 512, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// SIZE BENCHMARKS: 1024 bytes
// ============================================================================
struct bench_hash_1024b { uint64_t result; };
UBENCH_F_SETUP(bench_hash_1024b) { memset(test_data_1024b, 0xAB, 1024); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_1024b) { (void)ubench_fixture; }

UBENCH_F(bench_hash_1024b, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(test_data_1024b, 1024, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_1024b, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(test_data_1024b, 1024, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// REAL-WORLD: short identifier (8 chars)
// ============================================================================
struct bench_hash_short_id_8 { uint64_t result; };
UBENCH_F_SETUP(bench_hash_short_id_8) { ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_short_id_8) { (void)ubench_fixture; }

UBENCH_F(bench_hash_short_id_8, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(short_id_8, 8, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_short_id_8, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(short_id_8, 8, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// REAL-WORLD: short identifier (16 chars)
// ============================================================================
struct bench_hash_short_id_16 { uint64_t result; };
UBENCH_F_SETUP(bench_hash_short_id_16) { ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_short_id_16) { (void)ubench_fixture; }

UBENCH_F(bench_hash_short_id_16, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(short_id_16, 16, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_short_id_16, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(short_id_16, 16, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// REAL-WORLD: UUID (36 chars)
// ============================================================================
struct bench_hash_uuid { uint64_t result; };
UBENCH_F_SETUP(bench_hash_uuid) { ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_uuid) { (void)ubench_fixture; }

UBENCH_F(bench_hash_uuid, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(uuid_36, 36, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_uuid, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(uuid_36, 36, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// REAL-WORLD: file path (~56 chars)
// ============================================================================
struct bench_hash_filepath { uint64_t result; size_t len; };
UBENCH_F_SETUP(bench_hash_filepath) { ubench_fixture->result = 0; ubench_fixture->len = strlen(file_path_64); }
UBENCH_F_TEARDOWN(bench_hash_filepath) { (void)ubench_fixture; }

UBENCH_F(bench_hash_filepath, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash(file_path_64, ubench_fixture->len, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_filepath, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash(file_path_64, ubench_fixture->len, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// THROUGHPUT: 1000 hashes of 16 bytes
// ============================================================================
struct bench_throughput_16b { uint64_t result; };
UBENCH_F_SETUP(bench_throughput_16b) { memset(test_data_16b, 0xCD, 16); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_throughput_16b) { (void)ubench_fixture; }

UBENCH_F(bench_throughput_16b, xxhash64_1000x)
{
	uint64_t acc = 0;
	for (int i = 0; i < THROUGHPUT_ITERATIONS; i++)
	{
		acc ^= w_xxhash64_hash(test_data_16b, 16, 0);
	}
	ubench_fixture->result = acc;
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_throughput_16b, fnv1a_1000x)
{
	uint64_t acc = 0;
	for (int i = 0; i < THROUGHPUT_ITERATIONS; i++)
	{
		acc ^= w_fnv1a_hash(test_data_16b, 16, 0);
	}
	ubench_fixture->result = acc;
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// THROUGHPUT: 1000 hashes of 64 bytes
// ============================================================================
struct bench_throughput_64b { uint64_t result; };
UBENCH_F_SETUP(bench_throughput_64b) { memset(test_data_64b, 0xCD, 64); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_throughput_64b) { (void)ubench_fixture; }

UBENCH_F(bench_throughput_64b, xxhash64_1000x)
{
	uint64_t acc = 0;
	for (int i = 0; i < THROUGHPUT_ITERATIONS; i++)
	{
		acc ^= w_xxhash64_hash(test_data_64b, 64, 0);
	}
	ubench_fixture->result = acc;
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_throughput_64b, fnv1a_1000x)
{
	uint64_t acc = 0;
	for (int i = 0; i < THROUGHPUT_ITERATIONS; i++)
	{
		acc ^= w_fnv1a_hash(test_data_64b, 64, 0);
	}
	ubench_fixture->result = acc;
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// THROUGHPUT: 1000 hashes of 256 bytes
// ============================================================================
struct bench_throughput_256b { uint64_t result; };
UBENCH_F_SETUP(bench_throughput_256b) { memset(test_data_256b, 0xCD, 256); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_throughput_256b) { (void)ubench_fixture; }

UBENCH_F(bench_throughput_256b, xxhash64_1000x)
{
	uint64_t acc = 0;
	for (int i = 0; i < THROUGHPUT_ITERATIONS; i++)
	{
		acc ^= w_xxhash64_hash(test_data_256b, 256, 0);
	}
	ubench_fixture->result = acc;
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_throughput_256b, fnv1a_1000x)
{
	uint64_t acc = 0;
	for (int i = 0; i < THROUGHPUT_ITERATIONS; i++)
	{
		acc ^= w_fnv1a_hash(test_data_256b, 256, 0);
	}
	ubench_fixture->result = acc;
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// THROUGHPUT: 1000 hashes of 1024 bytes
// ============================================================================
struct bench_throughput_1024b { uint64_t result; };
UBENCH_F_SETUP(bench_throughput_1024b) { memset(test_data_1024b, 0xCD, 1024); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_throughput_1024b) { (void)ubench_fixture; }

UBENCH_F(bench_throughput_1024b, xxhash64_1000x)
{
	uint64_t acc = 0;
	for (int i = 0; i < THROUGHPUT_ITERATIONS; i++)
	{
		acc ^= w_xxhash64_hash(test_data_1024b, 1024, 0);
	}
	ubench_fixture->result = acc;
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_throughput_1024b, fnv1a_1000x)
{
	uint64_t acc = 0;
	for (int i = 0; i < THROUGHPUT_ITERATIONS; i++)
	{
		acc ^= w_fnv1a_hash(test_data_1024b, 1024, 0);
	}
	ubench_fixture->result = acc;
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// THROUGHPUT: 1000 short string hashes (simulating hash table keys)
// ============================================================================
struct bench_throughput_keys { uint64_t result; };
UBENCH_F_SETUP(bench_throughput_keys) { ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_throughput_keys) { (void)ubench_fixture; }

UBENCH_F(bench_throughput_keys, xxhash64_1000x)
{
	uint64_t acc = 0;
	for (int i = 0; i < THROUGHPUT_ITERATIONS; i++)
	{
		acc ^= w_xxhash64_hash(short_id_8, 8, 0);
	}
	ubench_fixture->result = acc;
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_throughput_keys, fnv1a_1000x)
{
	uint64_t acc = 0;
	for (int i = 0; i < THROUGHPUT_ITERATIONS; i++)
	{
		acc ^= w_fnv1a_hash(short_id_8, 8, 0);
	}
	ubench_fixture->result = acc;
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// SEEDED: xxhash64 with seed vs without (fnv1a has no seed)
// ============================================================================
struct bench_hash_seeded { uint64_t result; };
UBENCH_F_SETUP(bench_hash_seeded) { memset(test_data_64b, 0xEF, 64); ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_seeded) { (void)ubench_fixture; }

UBENCH_F(bench_hash_seeded, xxhash64_no_seed)
{
	ubench_fixture->result = w_xxhash64_hash(test_data_64b, 64, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_seeded, xxhash64_with_seed)
{
	ubench_fixture->result = w_xxhash64_hash(test_data_64b, 64, 0xDEADBEEFCAFEBABEULL);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// CONVENIENCE MACROS: w_xxhash64_str vs w_fnv1a_str
// ============================================================================
struct bench_hash_macro { uint64_t result; };
UBENCH_F_SETUP(bench_hash_macro) { ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_macro) { (void)ubench_fixture; }

UBENCH_F(bench_hash_macro, xxhash64_str_macro)
{
	ubench_fixture->result = w_xxhash64_str("test_string_key", 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_macro, fnv1a_str_macro)
{
	ubench_fixture->result = w_fnv1a_str("test_string_key", 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// MIXED WORKLOAD: alternating sizes (simulates realistic usage)
// ============================================================================
struct bench_mixed_workload { uint64_t result; };
UBENCH_F_SETUP(bench_mixed_workload)
{
	memset(test_data_8b, 0x11, 8);
	memset(test_data_32b, 0x22, 32);
	memset(test_data_128b, 0x33, 128);
	memset(test_data_512b, 0x44, 512);
	ubench_fixture->result = 0;
}
UBENCH_F_TEARDOWN(bench_mixed_workload) { (void)ubench_fixture; }

UBENCH_F(bench_mixed_workload, xxhash64_mixed_100x)
{
	uint64_t acc = 0;
	for (int i = 0; i < 100; i++)
	{
		acc ^= w_xxhash64_hash(test_data_8b, 8, 0);
		acc ^= w_xxhash64_hash(test_data_32b, 32, 0);
		acc ^= w_xxhash64_hash(test_data_128b, 128, 0);
		acc ^= w_xxhash64_hash(test_data_512b, 512, 0);
	}
	ubench_fixture->result = acc;
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_mixed_workload, fnv1a_mixed_100x)
{
	uint64_t acc = 0;
	for (int i = 0; i < 100; i++)
	{
		acc ^= w_fnv1a_hash(test_data_8b, 8, 0);
		acc ^= w_fnv1a_hash(test_data_32b, 32, 0);
		acc ^= w_fnv1a_hash(test_data_128b, 128, 0);
		acc ^= w_fnv1a_hash(test_data_512b, 512, 0);
	}
	ubench_fixture->result = acc;
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// EMPTY INPUT: edge case with zero-length data
// ============================================================================
struct bench_hash_empty { uint64_t result; };
UBENCH_F_SETUP(bench_hash_empty) { ubench_fixture->result = 0; }
UBENCH_F_TEARDOWN(bench_hash_empty) { (void)ubench_fixture; }

UBENCH_F(bench_hash_empty, xxhash64)
{
	ubench_fixture->result = w_xxhash64_hash("", 0, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bench_hash_empty, fnv1a)
{
	ubench_fixture->result = w_fnv1a_hash("", 0, 0);
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


UBENCH_MAIN();
