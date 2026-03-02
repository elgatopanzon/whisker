/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : benchmark_whisker_sparse_bitset
 * @created     : Monday Mar 02, 2026 16:56:18 CST
 * @description : benchmarks comparing sparse bitset page sizes vs flat uint64_t array
 */

#include "whisker_std.h"
#include "whisker_sparse_bitset.h"
#include "whisker_arena.h"

#include "ubench.h"

#include <stdlib.h>
#include <string.h>


// ============================================================================
// constants
// ============================================================================

#define BENCH_OP_COUNT      1000
#define BENCH_BIT_RANGE     65536
#define BENCH_SPARSE_RANGE  1000000
#define BENCH_DENSE_RANGE   1024
#define BENCH_ARENA_SIZE    (2 * 1024 * 1024)

// flat array word counts for each range
#define FLAT_WORDS_1K      (BENCH_BIT_RANGE / 64)
#define FLAT_WORDS_SPARSE  (BENCH_SPARSE_RANGE / 64 + 1)
#define FLAT_WORDS_DENSE   (BENCH_DENSE_RANGE / 64 + 1)

// flat array bit operations
#define FLAT_SET(arr, i)    ((arr)[(i) >> 6] |= (1ULL << ((i) & 63)))
#define FLAT_GET(arr, i)    (((arr)[(i) >> 6] >> ((i) & 63)) & 1)
#define FLAT_CLEAR(arr, i)  ((arr)[(i) >> 6] &= ~(1ULL << ((i) & 63)))


// ============================================================================
// pre-generated random indices (deterministic seed)
// ============================================================================

static uint64_t g_rand_1k[BENCH_OP_COUNT];
static uint64_t g_rand_sparse[BENCH_OP_COUNT];
static uint64_t g_rand_dense[BENCH_OP_COUNT];

static void init_rand_indices(void)
{
	srand(42);
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		g_rand_1k[i]     = (uint64_t)rand() % BENCH_BIT_RANGE;
		g_rand_sparse[i] = (uint64_t)rand() % BENCH_SPARSE_RANGE;
		g_rand_dense[i]  = (uint64_t)rand() % BENCH_DENSE_RANGE;
	}
}


// ============================================================================
// SET 1000 RANDOM BITS: page sizes vs flat
// ============================================================================

struct bitset_set_1k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_16;
	struct w_sparse_bitset bs_64;
	struct w_sparse_bitset bs_256;
	struct w_sparse_bitset bs_1024;
	uint64_t flat[FLAT_WORDS_1K];
};

UBENCH_F_SETUP(bitset_set_1k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_16,   &ubench_fixture->arena, 16);
	w_sparse_bitset_init(&ubench_fixture->bs_64,   &ubench_fixture->arena, 64);
	w_sparse_bitset_init(&ubench_fixture->bs_256,  &ubench_fixture->arena, 256);
	w_sparse_bitset_init(&ubench_fixture->bs_1024, &ubench_fixture->arena, 1024);
	memset(ubench_fixture->flat, 0, sizeof(ubench_fixture->flat));
}

UBENCH_F_TEARDOWN(bitset_set_1k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_16);
	w_sparse_bitset_free(&ubench_fixture->bs_64);
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_sparse_bitset_free(&ubench_fixture->bs_1024);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bitset_set_1k, set_1k_16_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_16, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_16);
}

UBENCH_F(bitset_set_1k, set_1k_64_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_64, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_64);
}

UBENCH_F(bitset_set_1k, set_1k_256_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_256);
}

UBENCH_F(bitset_set_1k, set_1k_1024_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_1024, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_1024);
}

UBENCH_F(bitset_set_1k, set_1k_flat)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		FLAT_SET(ubench_fixture->flat, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->flat);
}


// ============================================================================
// GET 1000 RANDOM BITS: page sizes vs flat
// ============================================================================

struct bitset_get_1k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_16;
	struct w_sparse_bitset bs_64;
	struct w_sparse_bitset bs_256;
	struct w_sparse_bitset bs_1024;
	uint64_t flat[FLAT_WORDS_1K];
};

UBENCH_F_SETUP(bitset_get_1k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_16,   &ubench_fixture->arena, 16);
	w_sparse_bitset_init(&ubench_fixture->bs_64,   &ubench_fixture->arena, 64);
	w_sparse_bitset_init(&ubench_fixture->bs_256,  &ubench_fixture->arena, 256);
	w_sparse_bitset_init(&ubench_fixture->bs_1024, &ubench_fixture->arena, 1024);
	memset(ubench_fixture->flat, 0, sizeof(ubench_fixture->flat));

	// pre-populate all structures with the random bit indices
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_16,   g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_64,   g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_256,  g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_1024, g_rand_1k[i]);
		FLAT_SET(ubench_fixture->flat, g_rand_1k[i]);
	}
}

UBENCH_F_TEARDOWN(bitset_get_1k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_16);
	w_sparse_bitset_free(&ubench_fixture->bs_64);
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_sparse_bitset_free(&ubench_fixture->bs_1024);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bitset_get_1k, get_1k_16_page)
{
	bool result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		result = w_sparse_bitset_get(&ubench_fixture->bs_16, g_rand_1k[i]);
		UBENCH_DO_NOTHING(&result);
	}
}

UBENCH_F(bitset_get_1k, get_1k_64_page)
{
	bool result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		result = w_sparse_bitset_get(&ubench_fixture->bs_64, g_rand_1k[i]);
		UBENCH_DO_NOTHING(&result);
	}
}

UBENCH_F(bitset_get_1k, get_1k_256_page)
{
	bool result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		result = w_sparse_bitset_get(&ubench_fixture->bs_256, g_rand_1k[i]);
		UBENCH_DO_NOTHING(&result);
	}
}

UBENCH_F(bitset_get_1k, get_1k_1024_page)
{
	bool result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		result = w_sparse_bitset_get(&ubench_fixture->bs_1024, g_rand_1k[i]);
		UBENCH_DO_NOTHING(&result);
	}
}

UBENCH_F(bitset_get_1k, get_1k_flat)
{
	uint64_t result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		result = FLAT_GET(ubench_fixture->flat, g_rand_1k[i]);
		UBENCH_DO_NOTHING(&result);
	}
}


// ============================================================================
// CLEAR 1000 RANDOM BITS: page sizes vs flat
// ============================================================================

struct bitset_clear_1k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_16;
	struct w_sparse_bitset bs_64;
	struct w_sparse_bitset bs_256;
	struct w_sparse_bitset bs_1024;
	uint64_t flat[FLAT_WORDS_1K];
};

UBENCH_F_SETUP(bitset_clear_1k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_16,   &ubench_fixture->arena, 16);
	w_sparse_bitset_init(&ubench_fixture->bs_64,   &ubench_fixture->arena, 64);
	w_sparse_bitset_init(&ubench_fixture->bs_256,  &ubench_fixture->arena, 256);
	w_sparse_bitset_init(&ubench_fixture->bs_1024, &ubench_fixture->arena, 1024);
	memset(ubench_fixture->flat, 0, sizeof(ubench_fixture->flat));

	// pre-populate so clear has bits to work with on first iteration
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_16,   g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_64,   g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_256,  g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_1024, g_rand_1k[i]);
		FLAT_SET(ubench_fixture->flat, g_rand_1k[i]);
	}
}

UBENCH_F_TEARDOWN(bitset_clear_1k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_16);
	w_sparse_bitset_free(&ubench_fixture->bs_64);
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_sparse_bitset_free(&ubench_fixture->bs_1024);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bitset_clear_1k, clear_1k_16_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_clear(&ubench_fixture->bs_16, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_16);
}

UBENCH_F(bitset_clear_1k, clear_1k_64_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_clear(&ubench_fixture->bs_64, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_64);
}

UBENCH_F(bitset_clear_1k, clear_1k_256_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_clear(&ubench_fixture->bs_256, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_256);
}

UBENCH_F(bitset_clear_1k, clear_1k_1024_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_clear(&ubench_fixture->bs_1024, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_1024);
}

UBENCH_F(bitset_clear_1k, clear_1k_flat)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		FLAT_CLEAR(ubench_fixture->flat, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->flat);
}


// ============================================================================
// MIXED SET/GET/CLEAR 1000 OPS: page sizes vs flat
// ============================================================================

struct bitset_mixed_1k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_16;
	struct w_sparse_bitset bs_64;
	struct w_sparse_bitset bs_256;
	struct w_sparse_bitset bs_1024;
	uint64_t flat[FLAT_WORDS_1K];
};

UBENCH_F_SETUP(bitset_mixed_1k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_16,   &ubench_fixture->arena, 16);
	w_sparse_bitset_init(&ubench_fixture->bs_64,   &ubench_fixture->arena, 64);
	w_sparse_bitset_init(&ubench_fixture->bs_256,  &ubench_fixture->arena, 256);
	w_sparse_bitset_init(&ubench_fixture->bs_1024, &ubench_fixture->arena, 1024);
	memset(ubench_fixture->flat, 0, sizeof(ubench_fixture->flat));

	// pre-populate half the indices so get/clear have data to work with
	for (int i = 0; i < BENCH_OP_COUNT / 2; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_16,   g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_64,   g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_256,  g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_1024, g_rand_1k[i]);
		FLAT_SET(ubench_fixture->flat, g_rand_1k[i]);
	}
}

UBENCH_F_TEARDOWN(bitset_mixed_1k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_16);
	w_sparse_bitset_free(&ubench_fixture->bs_64);
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_sparse_bitset_free(&ubench_fixture->bs_1024);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bitset_mixed_1k, mixed_1k_16_page)
{
	bool result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		int op = i % 3;
		if (op == 0)
		{
			w_sparse_bitset_set(&ubench_fixture->bs_16, g_rand_1k[i]);
		}
		else if (op == 1)
		{
			result = w_sparse_bitset_get(&ubench_fixture->bs_16, g_rand_1k[i]);
			UBENCH_DO_NOTHING(&result);
		}
		else
		{
			w_sparse_bitset_clear(&ubench_fixture->bs_16, g_rand_1k[i]);
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_16);
}

UBENCH_F(bitset_mixed_1k, mixed_1k_64_page)
{
	bool result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		int op = i % 3;
		if (op == 0)
		{
			w_sparse_bitset_set(&ubench_fixture->bs_64, g_rand_1k[i]);
		}
		else if (op == 1)
		{
			result = w_sparse_bitset_get(&ubench_fixture->bs_64, g_rand_1k[i]);
			UBENCH_DO_NOTHING(&result);
		}
		else
		{
			w_sparse_bitset_clear(&ubench_fixture->bs_64, g_rand_1k[i]);
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_64);
}

UBENCH_F(bitset_mixed_1k, mixed_1k_256_page)
{
	bool result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		int op = i % 3;
		if (op == 0)
		{
			w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_1k[i]);
		}
		else if (op == 1)
		{
			result = w_sparse_bitset_get(&ubench_fixture->bs_256, g_rand_1k[i]);
			UBENCH_DO_NOTHING(&result);
		}
		else
		{
			w_sparse_bitset_clear(&ubench_fixture->bs_256, g_rand_1k[i]);
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_256);
}

UBENCH_F(bitset_mixed_1k, mixed_1k_1024_page)
{
	bool result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		int op = i % 3;
		if (op == 0)
		{
			w_sparse_bitset_set(&ubench_fixture->bs_1024, g_rand_1k[i]);
		}
		else if (op == 1)
		{
			result = w_sparse_bitset_get(&ubench_fixture->bs_1024, g_rand_1k[i]);
			UBENCH_DO_NOTHING(&result);
		}
		else
		{
			w_sparse_bitset_clear(&ubench_fixture->bs_1024, g_rand_1k[i]);
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_1024);
}

UBENCH_F(bitset_mixed_1k, mixed_1k_flat)
{
	uint64_t result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		int op = i % 3;
		if (op == 0)
		{
			FLAT_SET(ubench_fixture->flat, g_rand_1k[i]);
		}
		else if (op == 1)
		{
			result = FLAT_GET(ubench_fixture->flat, g_rand_1k[i]);
			UBENCH_DO_NOTHING(&result);
		}
		else
		{
			FLAT_CLEAR(ubench_fixture->flat, g_rand_1k[i]);
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->flat);
}


// ============================================================================
// SPARSE PATTERN: few bits set across 0-1000000 range
// ============================================================================

struct bitset_sparse
{
	struct w_arena arena;
	struct w_sparse_bitset bs_16;
	struct w_sparse_bitset bs_64;
	struct w_sparse_bitset bs_256;
	struct w_sparse_bitset bs_1024;
	uint64_t flat[FLAT_WORDS_SPARSE];
};

UBENCH_F_SETUP(bitset_sparse)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_16,   &ubench_fixture->arena, 16);
	w_sparse_bitset_init(&ubench_fixture->bs_64,   &ubench_fixture->arena, 64);
	w_sparse_bitset_init(&ubench_fixture->bs_256,  &ubench_fixture->arena, 256);
	w_sparse_bitset_init(&ubench_fixture->bs_1024, &ubench_fixture->arena, 1024);
	memset(ubench_fixture->flat, 0, sizeof(ubench_fixture->flat));
}

UBENCH_F_TEARDOWN(bitset_sparse)
{
	w_sparse_bitset_free(&ubench_fixture->bs_16);
	w_sparse_bitset_free(&ubench_fixture->bs_64);
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_sparse_bitset_free(&ubench_fixture->bs_1024);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bitset_sparse, set_sparse_16_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_16, g_rand_sparse[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_16);
}

UBENCH_F(bitset_sparse, set_sparse_64_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_64, g_rand_sparse[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_64);
}

UBENCH_F(bitset_sparse, set_sparse_256_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_sparse[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_256);
}

UBENCH_F(bitset_sparse, set_sparse_1024_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_1024, g_rand_sparse[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_1024);
}

UBENCH_F(bitset_sparse, set_sparse_flat)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		FLAT_SET(ubench_fixture->flat, g_rand_sparse[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->flat);
}


// ============================================================================
// DENSE PATTERN: many bits set in 0-1023 range
// ============================================================================

struct bitset_dense
{
	struct w_arena arena;
	struct w_sparse_bitset bs_16;
	struct w_sparse_bitset bs_64;
	struct w_sparse_bitset bs_256;
	struct w_sparse_bitset bs_1024;
	uint64_t flat[FLAT_WORDS_DENSE];
};

UBENCH_F_SETUP(bitset_dense)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_16,   &ubench_fixture->arena, 16);
	w_sparse_bitset_init(&ubench_fixture->bs_64,   &ubench_fixture->arena, 64);
	w_sparse_bitset_init(&ubench_fixture->bs_256,  &ubench_fixture->arena, 256);
	w_sparse_bitset_init(&ubench_fixture->bs_1024, &ubench_fixture->arena, 1024);
	memset(ubench_fixture->flat, 0, sizeof(ubench_fixture->flat));
}

UBENCH_F_TEARDOWN(bitset_dense)
{
	w_sparse_bitset_free(&ubench_fixture->bs_16);
	w_sparse_bitset_free(&ubench_fixture->bs_64);
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_sparse_bitset_free(&ubench_fixture->bs_1024);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bitset_dense, set_dense_16_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_16, g_rand_dense[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_16);
}

UBENCH_F(bitset_dense, set_dense_64_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_64, g_rand_dense[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_64);
}

UBENCH_F(bitset_dense, set_dense_256_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_dense[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_256);
}

UBENCH_F(bitset_dense, set_dense_1024_page)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_1024, g_rand_dense[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_1024);
}

UBENCH_F(bitset_dense, set_dense_flat)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		FLAT_SET(ubench_fixture->flat, g_rand_dense[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->flat);
}


UBENCH_MAIN();
