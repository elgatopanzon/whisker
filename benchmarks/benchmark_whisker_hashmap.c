/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : benchmark_whisker_hashmap
 * @created     : Friday Feb 28, 2026 20:40:00 CST
 * @description : benchmarks comparing void* hashmap vs macro hashmap performance
 */

#include "whisker_std.h"
#include "whisker_hashmap.h"
#include "whisker_arena.h"
#include "whisker_hash_xxhash64.h"

#include "ubench.h"

#include <string.h>
#include <stdlib.h>


// ============================================================================
// declare typed hashmaps for benchmarks
// ============================================================================

w_hashmap_t_declare(int, int, int_int_hashmap);
w_hashmap_t_declare(uint64_t, uint64_t, u64_u64_hashmap);
w_hashmap_t_declare(const char *, int, str_int_hashmap);


// ============================================================================
// capacity test sizes
// ============================================================================

#define CAP_1K     1000
#define CAP_10K    10000
#define CAP_100K   100000

#define BUCKET_COUNT_SMALL   64
#define BUCKET_COUNT_MEDIUM  4096
#define BUCKET_COUNT_LARGE   65536


// ============================================================================
// SET THROUGHPUT: void* vs macro - integer keys
// ============================================================================

// set: 1000 int keys, 4096 buckets
struct bench_set_1k_4k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_set_1k_4k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, w_xxhash64_hash, NULL);
}
UBENCH_F_TEARDOWN(bench_set_1k_4k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_set_1k_4k_buckets, void_set_1k_int)
{
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->void_map);
}

UBENCH_F(bench_set_1k_4k_buckets, macro_set_1k_int)
{
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->macro_map);
}


// set: 10000 int keys, 4096 buckets
struct bench_set_10k_4k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_set_10k_4k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 4 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, w_xxhash64_hash, NULL);
}
UBENCH_F_TEARDOWN(bench_set_10k_4k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_set_10k_4k_buckets, void_set_10k_int)
{
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->void_map);
}

UBENCH_F(bench_set_10k_4k_buckets, macro_set_10k_int)
{
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->macro_map);
}


// set: 100000 int keys, 65536 buckets
struct bench_set_100k_64k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_set_100k_64k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 32 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_LARGE, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_LARGE, w_xxhash64_hash, NULL);
}
UBENCH_F_TEARDOWN(bench_set_100k_64k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_set_100k_64k_buckets, void_set_100k_int)
{
	for (int i = 0; i < CAP_100K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->void_map);
}

UBENCH_F(bench_set_100k_64k_buckets, macro_set_100k_int)
{
	for (int i = 0; i < CAP_100K; i++)
	{
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->macro_map);
}


// ============================================================================
// GET THROUGHPUT: void* vs macro - hit rate (all keys exist)
// ============================================================================

// get: 1000 hits, 4096 buckets
struct bench_get_hit_1k_4k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_get_hit_1k_4k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, w_xxhash64_hash, NULL);
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
}
UBENCH_F_TEARDOWN(bench_get_hit_1k_4k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_get_hit_1k_4k_buckets, void_get_1k_hit)
{
	int *result;
	for (int i = 0; i < CAP_1K; i++)
	{
		result = w_hashmap_get(&ubench_fixture->void_map, &i, sizeof(i));
		UBENCH_DO_NOTHING(result);
	}
}

UBENCH_F(bench_get_hit_1k_4k_buckets, macro_get_1k_hit)
{
	int *result;
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_t_get(&ubench_fixture->macro_map, i, result);
		UBENCH_DO_NOTHING(result);
	}
}


// get: 10000 hits, 4096 buckets
struct bench_get_hit_10k_4k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_get_hit_10k_4k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 4 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, w_xxhash64_hash, NULL);
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
}
UBENCH_F_TEARDOWN(bench_get_hit_10k_4k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_get_hit_10k_4k_buckets, void_get_10k_hit)
{
	int *result;
	for (int i = 0; i < CAP_10K; i++)
	{
		result = w_hashmap_get(&ubench_fixture->void_map, &i, sizeof(i));
		UBENCH_DO_NOTHING(result);
	}
}

UBENCH_F(bench_get_hit_10k_4k_buckets, macro_get_10k_hit)
{
	int *result;
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_t_get(&ubench_fixture->macro_map, i, result);
		UBENCH_DO_NOTHING(result);
	}
}


// ============================================================================
// GET THROUGHPUT: miss rate (keys don't exist)
// ============================================================================

// get: 1000 misses, 4096 buckets
struct bench_get_miss_1k_4k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_get_miss_1k_4k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, w_xxhash64_hash, NULL);
	// populate with keys 0-999
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
}
UBENCH_F_TEARDOWN(bench_get_miss_1k_4k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_get_miss_1k_4k_buckets, void_get_1k_miss)
{
	int *result;
	// lookup keys 1000-1999 (none exist)
	for (int i = CAP_1K; i < CAP_1K * 2; i++)
	{
		result = w_hashmap_get(&ubench_fixture->void_map, &i, sizeof(i));
		UBENCH_DO_NOTHING(result);
	}
}

UBENCH_F(bench_get_miss_1k_4k_buckets, macro_get_1k_miss)
{
	int *result;
	// lookup keys 1000-1999 (none exist)
	for (int i = CAP_1K; i < CAP_1K * 2; i++)
	{
		w_hashmap_t_get(&ubench_fixture->macro_map, i, result);
		UBENCH_DO_NOTHING(result);
	}
}


// ============================================================================
// BUCKET SIZING: small buckets (high collision)
// ============================================================================

// high collision set: 64 buckets, 10000 entries
struct bench_set_10k_64_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_set_10k_64_buckets)
{
	w_arena_init(&ubench_fixture->arena, 8 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_SMALL, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_SMALL, w_xxhash64_hash, NULL);
}
UBENCH_F_TEARDOWN(bench_set_10k_64_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_set_10k_64_buckets, void_set_10k)
{
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->void_map);
}

UBENCH_F(bench_set_10k_64_buckets, macro_set_10k)
{
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->macro_map);
}


// high collision get: 64 buckets, 10000 entries
struct bench_get_10k_64_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_get_10k_64_buckets)
{
	w_arena_init(&ubench_fixture->arena, 8 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_SMALL, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_SMALL, w_xxhash64_hash, NULL);
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
}
UBENCH_F_TEARDOWN(bench_get_10k_64_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_get_10k_64_buckets, void_get_10k)
{
	int *result;
	for (int i = 0; i < CAP_10K; i++)
	{
		result = w_hashmap_get(&ubench_fixture->void_map, &i, sizeof(i));
		UBENCH_DO_NOTHING(result);
	}
}

UBENCH_F(bench_get_10k_64_buckets, macro_get_10k)
{
	int *result;
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_t_get(&ubench_fixture->macro_map, i, result);
		UBENCH_DO_NOTHING(result);
	}
}


// ============================================================================
// BUCKET SIZING: large buckets (low collision)
// ============================================================================

// low collision set: 65536 buckets, 10000 entries
struct bench_set_10k_64k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_set_10k_64k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 8 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_LARGE, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_LARGE, w_xxhash64_hash, NULL);
}
UBENCH_F_TEARDOWN(bench_set_10k_64k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_set_10k_64k_buckets, void_set_10k)
{
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->void_map);
}

UBENCH_F(bench_set_10k_64k_buckets, macro_set_10k)
{
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->macro_map);
}


// low collision get: 65536 buckets, 10000 entries
struct bench_get_10k_64k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_get_10k_64k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 8 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_LARGE, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_LARGE, w_xxhash64_hash, NULL);
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
}
UBENCH_F_TEARDOWN(bench_get_10k_64k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_get_10k_64k_buckets, void_get_10k)
{
	int *result;
	for (int i = 0; i < CAP_10K; i++)
	{
		result = w_hashmap_get(&ubench_fixture->void_map, &i, sizeof(i));
		UBENCH_DO_NOTHING(result);
	}
}

UBENCH_F(bench_get_10k_64k_buckets, macro_get_10k)
{
	int *result;
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_t_get(&ubench_fixture->macro_map, i, result);
		UBENCH_DO_NOTHING(result);
	}
}


// ============================================================================
// REMOVE OPERATIONS
// ============================================================================

// remove: 1000 entries, 4096 buckets
struct bench_remove_1k_4k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_remove_1k_4k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, w_xxhash64_hash, NULL);
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
}
UBENCH_F_TEARDOWN(bench_remove_1k_4k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_remove_1k_4k_buckets, void_remove_1k)
{
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_remove(&ubench_fixture->void_map, &i, sizeof(i));
	}
	UBENCH_DO_NOTHING(&ubench_fixture->void_map);
}

UBENCH_F(bench_remove_1k_4k_buckets, macro_remove_1k)
{
	bool removed;
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_t_remove(&ubench_fixture->macro_map, i, removed);
		UBENCH_DO_NOTHING(&removed);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->macro_map);
}


// remove: 10000 entries, 4096 buckets
struct bench_remove_10k_4k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_remove_10k_4k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 4 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, w_xxhash64_hash, NULL);
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
}
UBENCH_F_TEARDOWN(bench_remove_10k_4k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_remove_10k_4k_buckets, void_remove_10k)
{
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_remove(&ubench_fixture->void_map, &i, sizeof(i));
	}
	UBENCH_DO_NOTHING(&ubench_fixture->void_map);
}

UBENCH_F(bench_remove_10k_4k_buckets, macro_remove_10k)
{
	bool removed;
	for (int i = 0; i < CAP_10K; i++)
	{
		w_hashmap_t_remove(&ubench_fixture->macro_map, i, removed);
		UBENCH_DO_NOTHING(&removed);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->macro_map);
}


// ============================================================================
// MIXED WORKLOADS: set/get/remove combined
// ============================================================================

// mixed: 80% get, 15% set, 5% remove, 4096 buckets
struct bench_mixed_10k_4k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_mixed_10k_4k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 4 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, w_xxhash64_hash, NULL);
	// pre-populate with 5000 entries
	for (int i = 0; i < 5000; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
}
UBENCH_F_TEARDOWN(bench_mixed_10k_4k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_mixed_10k_4k_buckets, void_mixed_10k_ops)
{
	int *result;
	int next_insert = 5000;
	int remove_idx = 0;
	for (int i = 0; i < CAP_10K; i++)
	{
		int op = i % 20;
		if (op < 16)
		{
			// 80% get
			int key = i % 5000;
			result = w_hashmap_get(&ubench_fixture->void_map, &key, sizeof(key));
			UBENCH_DO_NOTHING(result);
		}
		else if (op < 19)
		{
			// 15% set
			w_hashmap_set(&ubench_fixture->void_map, &next_insert, sizeof(next_insert), &next_insert);
			next_insert++;
		}
		else
		{
			// 5% remove
			w_hashmap_remove(&ubench_fixture->void_map, &remove_idx, sizeof(remove_idx));
			remove_idx++;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->void_map);
}

UBENCH_F(bench_mixed_10k_4k_buckets, macro_mixed_10k_ops)
{
	int *result;
	int next_insert = 5000;
	int remove_idx = 0;
	bool removed;
	for (int i = 0; i < CAP_10K; i++)
	{
		int op = i % 20;
		if (op < 16)
		{
			int key = i % 5000;
			w_hashmap_t_get(&ubench_fixture->macro_map, key, result);
			UBENCH_DO_NOTHING(result);
		}
		else if (op < 19)
		{
			w_hashmap_t_set(&ubench_fixture->macro_map, next_insert, next_insert);
			next_insert++;
		}
		else
		{
			w_hashmap_t_remove(&ubench_fixture->macro_map, remove_idx, removed);
			remove_idx++;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->macro_map);
}


// ============================================================================
// STRING KEY PERFORMANCE
// ============================================================================

// pre-generate string keys
static char g_string_keys[CAP_1K][32];

static void init_string_keys(void)
{
	for (int i = 0; i < CAP_1K; i++)
	{
		snprintf(g_string_keys[i], 32, "key_%08d", i);
	}
}


// string keys set: 1000, 4096 buckets
struct bench_str_set_1k_4k_buckets { struct w_arena arena; struct w_hashmap void_map; struct str_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_str_set_1k_4k_buckets)
{
	init_string_keys();
	w_arena_init(&ubench_fixture->arena, 2 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, w_hashmap_hash_str, w_hashmap_eq_str);
}
UBENCH_F_TEARDOWN(bench_str_set_1k_4k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_str_set_1k_4k_buckets, void_str_set_1k)
{
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, g_string_keys[i], strlen(g_string_keys[i]), &i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->void_map);
}

UBENCH_F(bench_str_set_1k_4k_buckets, macro_str_set_1k)
{
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_t_set(&ubench_fixture->macro_map, (const char *)g_string_keys[i], i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->macro_map);
}


// string keys get: 1000, 4096 buckets
struct bench_str_get_1k_4k_buckets { struct w_arena arena; struct w_hashmap void_map; struct str_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_str_get_1k_4k_buckets)
{
	init_string_keys();
	w_arena_init(&ubench_fixture->arena, 2 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, w_hashmap_hash_str, w_hashmap_eq_str);
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, g_string_keys[i], strlen(g_string_keys[i]), &i);
		w_hashmap_t_set(&ubench_fixture->macro_map, (const char *)g_string_keys[i], i);
	}
}
UBENCH_F_TEARDOWN(bench_str_get_1k_4k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_str_get_1k_4k_buckets, void_str_get_1k)
{
	int *result;
	for (int i = 0; i < CAP_1K; i++)
	{
		result = w_hashmap_get(&ubench_fixture->void_map, g_string_keys[i], strlen(g_string_keys[i]));
		UBENCH_DO_NOTHING(result);
	}
}

UBENCH_F(bench_str_get_1k_4k_buckets, macro_str_get_1k)
{
	int *result;
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_t_get(&ubench_fixture->macro_map, (const char *)g_string_keys[i], result);
		UBENCH_DO_NOTHING(result);
	}
}


// ============================================================================
// INTEGER vs STRING: comparison within same API
// ============================================================================

// int vs string set comparison (void* API), 4096 buckets
struct bench_int_vs_str_1k_4k_buckets { struct w_arena arena; struct w_hashmap int_map; struct w_hashmap str_map; };
UBENCH_F_SETUP(bench_int_vs_str_1k_4k_buckets)
{
	init_string_keys();
	w_arena_init(&ubench_fixture->arena, 4 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->int_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_init(&ubench_fixture->str_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
}
UBENCH_F_TEARDOWN(bench_int_vs_str_1k_4k_buckets)
{
	w_hashmap_free(&ubench_fixture->int_map);
	w_hashmap_free(&ubench_fixture->str_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_int_vs_str_1k_4k_buckets, void_int_keys_1k)
{
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_set(&ubench_fixture->int_map, &i, sizeof(i), &i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->int_map);
}

UBENCH_F(bench_int_vs_str_1k_4k_buckets, void_str_keys_1k)
{
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_set(&ubench_fixture->str_map, g_string_keys[i], strlen(g_string_keys[i]), &i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->str_map);
}


// ============================================================================
// STRESS TEST: large scale operations
// ============================================================================

// stress set: 100k, 65536 buckets
struct bench_stress_set_100k_64k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_stress_set_100k_64k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 64 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_LARGE, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_LARGE, w_xxhash64_hash, NULL);
}
UBENCH_F_TEARDOWN(bench_stress_set_100k_64k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_stress_set_100k_64k_buckets, void_stress_set_100k)
{
	for (int i = 0; i < CAP_100K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->void_map);
}

UBENCH_F(bench_stress_set_100k_64k_buckets, macro_stress_set_100k)
{
	for (int i = 0; i < CAP_100K; i++)
	{
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->macro_map);
}


// stress get: 100k populated, 65536 buckets
struct bench_stress_get_100k_64k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_stress_get_100k_64k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 64 * 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_LARGE, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_LARGE, w_xxhash64_hash, NULL);
	for (int i = 0; i < CAP_100K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
}
UBENCH_F_TEARDOWN(bench_stress_get_100k_64k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_stress_get_100k_64k_buckets, void_stress_get_100k)
{
	int *result;
	for (int i = 0; i < CAP_100K; i++)
	{
		result = w_hashmap_get(&ubench_fixture->void_map, &i, sizeof(i));
		UBENCH_DO_NOTHING(result);
	}
}

UBENCH_F(bench_stress_get_100k_64k_buckets, macro_stress_get_100k)
{
	int *result;
	for (int i = 0; i < CAP_100K; i++)
	{
		w_hashmap_t_get(&ubench_fixture->macro_map, i, result);
		UBENCH_DO_NOTHING(result);
	}
}


// ============================================================================
// OVERWRITE PERFORMANCE: update existing keys
// ============================================================================

// overwrite: update all 1000 keys, 4096 buckets
struct bench_overwrite_1k_4k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; };
UBENCH_F_SETUP(bench_overwrite_1k_4k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, w_xxhash64_hash, NULL);
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &i);
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i);
	}
}
UBENCH_F_TEARDOWN(bench_overwrite_1k_4k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_overwrite_1k_4k_buckets, void_overwrite_1k)
{
	for (int i = 0; i < CAP_1K; i++)
	{
		int new_val = i * 2;
		w_hashmap_set(&ubench_fixture->void_map, &i, sizeof(i), &new_val);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->void_map);
}

UBENCH_F(bench_overwrite_1k_4k_buckets, macro_overwrite_1k)
{
	for (int i = 0; i < CAP_1K; i++)
	{
		w_hashmap_t_set(&ubench_fixture->macro_map, i, i * 2);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->macro_map);
}


// ============================================================================
// SINGLE OPERATION LATENCY
// ============================================================================

// single set, 4096 buckets
struct bench_single_op_4k_buckets { struct w_arena arena; struct w_hashmap void_map; struct int_int_hashmap macro_map; int void_counter; int macro_counter; };
UBENCH_F_SETUP(bench_single_op_4k_buckets)
{
	w_arena_init(&ubench_fixture->arena, 1024 * 1024);
	w_hashmap_init(&ubench_fixture->void_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, sizeof(int), w_xxhash64_hash, NULL);
	w_hashmap_t_init(&ubench_fixture->macro_map, &ubench_fixture->arena, BUCKET_COUNT_MEDIUM, w_xxhash64_hash, NULL);
	ubench_fixture->void_counter = 0;
	ubench_fixture->macro_counter = 0;
}
UBENCH_F_TEARDOWN(bench_single_op_4k_buckets)
{
	w_hashmap_free(&ubench_fixture->void_map);
	w_hashmap_t_free(&ubench_fixture->macro_map);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_single_op_4k_buckets, void_single_set)
{
	int key = ubench_fixture->void_counter++;
	w_hashmap_set(&ubench_fixture->void_map, &key, sizeof(key), &key);
	UBENCH_DO_NOTHING(&ubench_fixture->void_map);
}

UBENCH_F(bench_single_op_4k_buckets, macro_single_set)
{
	int key = ubench_fixture->macro_counter++;
	w_hashmap_t_set(&ubench_fixture->macro_map, key, key);
	UBENCH_DO_NOTHING(&ubench_fixture->macro_map);
}


UBENCH_MAIN();
