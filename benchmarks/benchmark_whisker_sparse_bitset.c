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
#define BENCH_ARENA_SIZE       (2 * 1024 * 1024)
#define BENCH_ARENA_SIZE_LARGE (4 * 1024 * 1024)
#define BENCH_ARENA_SIZE_2M    (8 * 1024 * 1024)
#define BENCH_ARENA_SIZE_4M    (16 * 1024 * 1024)

// scaled op counts and bit ranges
#define BENCH_OP_COUNT_10K    10000
#define BENCH_OP_COUNT_100K   100000
#define BENCH_OP_COUNT_1M     1000000
#define BENCH_OP_COUNT_2M     2000000
#define BENCH_OP_COUNT_4M     4000000
#define BENCH_BIT_RANGE_100K  655360
#define BENCH_BIT_RANGE_1M    6553600
#define BENCH_BIT_RANGE_2M    13107200
#define BENCH_BIT_RANGE_4M    26214400

// wide-gap sparse pattern: 100000 bits at stride 100 across 0-10M
#define BENCH_WIDE_GAP_STRIDE 100
#define BENCH_WIDE_GAP_COUNT  100000
#define BENCH_WIDE_GAP_MAX    (BENCH_WIDE_GAP_COUNT * BENCH_WIDE_GAP_STRIDE)

// flat array word counts for each range
#define FLAT_WORDS_1K        (BENCH_BIT_RANGE / 64)
#define FLAT_WORDS_SPARSE    (BENCH_SPARSE_RANGE / 64 + 1)
#define FLAT_WORDS_DENSE     (BENCH_DENSE_RANGE / 64 + 1)
#define FLAT_WORDS_10K       (BENCH_BIT_RANGE / 64)
#define FLAT_WORDS_100K      (BENCH_BIT_RANGE_100K / 64)
#define FLAT_WORDS_1M        (BENCH_BIT_RANGE_1M / 64)
#define FLAT_WORDS_2M        (BENCH_BIT_RANGE_2M / 64)
#define FLAT_WORDS_4M        (BENCH_BIT_RANGE_4M / 64)
#define FLAT_WORDS_WIDE_GAP  (BENCH_WIDE_GAP_MAX / 64 + 1)

// flat array bit operations
#define FLAT_SET(arr, i)    ((arr)[(i) >> 6] |= (1ULL << ((i) & 63)))
#define FLAT_GET(arr, i)    (((arr)[(i) >> 6] >> ((i) & 63)) & 1)
#define FLAT_CLEAR(arr, i)  ((arr)[(i) >> 6] &= ~(1ULL << ((i) & 63)))

// skip table: 1 bit per 64 flat words; tracks which 64-word blocks are non-empty
#define SKIP_WORDS_1K    ((FLAT_WORDS_1K + 63) / 64)
#define SKIP_WORDS_10K   ((FLAT_WORDS_10K + 63) / 64)
#define SKIP_WORDS_100K  ((FLAT_WORDS_100K + 63) / 64)
#define SKIP_WORDS_1M    ((FLAT_WORDS_1M + 63) / 64)
#define SKIP_WORDS_2M    ((FLAT_WORDS_2M + 63) / 64)
#define SKIP_WORDS_4M    ((FLAT_WORDS_4M + 63) / 64)

// sparse iterate arena sizes (pages allocate page_size_ * 8 bytes each via arena)
#define BENCH_ARENA_SIZE_16M   (16 * 1024 * 1024)
#define BENCH_ARENA_SIZE_32M   (32 * 1024 * 1024)

// sparse iterate: bits at i*stride for i in [0,count), forcing large empty gaps
// 1k/10k/100k all span ~10M bits so flat arrays are comparable; 1m/2m span 100M/200M
#define BENCH_SPARSE_ITER_STRIDE_1K    10000
#define BENCH_SPARSE_ITER_STRIDE_10K   1000
#define BENCH_SPARSE_ITER_STRIDE_100K  100
#define BENCH_SPARSE_ITER_STRIDE_1M    100
#define BENCH_SPARSE_ITER_STRIDE_2M    100

#define BENCH_SPARSE_ITER_RANGE_1K    ((uint64_t)BENCH_OP_COUNT     * BENCH_SPARSE_ITER_STRIDE_1K)
#define BENCH_SPARSE_ITER_RANGE_10K   ((uint64_t)BENCH_OP_COUNT_10K * BENCH_SPARSE_ITER_STRIDE_10K)
#define BENCH_SPARSE_ITER_RANGE_100K  ((uint64_t)BENCH_OP_COUNT_100K * BENCH_SPARSE_ITER_STRIDE_100K)
#define BENCH_SPARSE_ITER_RANGE_1M    ((uint64_t)BENCH_OP_COUNT_1M   * BENCH_SPARSE_ITER_STRIDE_1M)
#define BENCH_SPARSE_ITER_RANGE_2M    ((uint64_t)BENCH_OP_COUNT_2M   * BENCH_SPARSE_ITER_STRIDE_2M)

#define FLAT_WORDS_SPARSE_ITER_1K    (BENCH_SPARSE_ITER_RANGE_1K   / 64 + 1)
#define FLAT_WORDS_SPARSE_ITER_10K   (BENCH_SPARSE_ITER_RANGE_10K  / 64 + 1)
#define FLAT_WORDS_SPARSE_ITER_100K  (BENCH_SPARSE_ITER_RANGE_100K / 64 + 1)
#define FLAT_WORDS_SPARSE_ITER_1M    (BENCH_SPARSE_ITER_RANGE_1M   / 64 + 1)
#define FLAT_WORDS_SPARSE_ITER_2M    (BENCH_SPARSE_ITER_RANGE_2M   / 64 + 1)

#define SKIP_WORDS_SPARSE_ITER_1K    ((FLAT_WORDS_SPARSE_ITER_1K   + 63) / 64)
#define SKIP_WORDS_SPARSE_ITER_10K   ((FLAT_WORDS_SPARSE_ITER_10K  + 63) / 64)
#define SKIP_WORDS_SPARSE_ITER_100K  ((FLAT_WORDS_SPARSE_ITER_100K + 63) / 64)
#define SKIP_WORDS_SPARSE_ITER_1M    ((FLAT_WORDS_SPARSE_ITER_1M   + 63) / 64)
#define SKIP_WORDS_SPARSE_ITER_2M    ((FLAT_WORDS_SPARSE_ITER_2M   + 63) / 64)

// ultra-sparse iterate: 0.01% density with stride ~10000
// ultrasparse_1k: 1000 bits across 10M range
// ultrasparse_10k: 10000 bits across 100M range
// ultrasparse_100k: 100000 bits across 1B range
#define BENCH_ULTRASPARSE_STRIDE        10000
#define BENCH_ULTRASPARSE_1K_COUNT      1000
#define BENCH_ULTRASPARSE_10K_COUNT     10000
#define BENCH_ULTRASPARSE_100K_COUNT    100000
#define BENCH_ULTRASPARSE_1K_RANGE      ((uint64_t)BENCH_ULTRASPARSE_1K_COUNT * BENCH_ULTRASPARSE_STRIDE)
#define BENCH_ULTRASPARSE_10K_RANGE     ((uint64_t)BENCH_ULTRASPARSE_10K_COUNT * BENCH_ULTRASPARSE_STRIDE)
#define BENCH_ULTRASPARSE_100K_RANGE    ((uint64_t)BENCH_ULTRASPARSE_100K_COUNT * BENCH_ULTRASPARSE_STRIDE)

#define FLAT_WORDS_ULTRASPARSE_1K       (BENCH_ULTRASPARSE_1K_RANGE / 64 + 1)
#define FLAT_WORDS_ULTRASPARSE_10K      (BENCH_ULTRASPARSE_10K_RANGE / 64 + 1)
#define FLAT_WORDS_ULTRASPARSE_100K     (BENCH_ULTRASPARSE_100K_RANGE / 64 + 1)

#define SKIP_WORDS_ULTRASPARSE_1K       ((FLAT_WORDS_ULTRASPARSE_1K + 63) / 64)
#define SKIP_WORDS_ULTRASPARSE_10K      ((FLAT_WORDS_ULTRASPARSE_10K + 63) / 64)
#define SKIP_WORDS_ULTRASPARSE_100K     ((FLAT_WORDS_ULTRASPARSE_100K + 63) / 64)

// arena sizes for ultra-sparse (100k needs large arena for 1B range)
#define BENCH_ARENA_SIZE_64M   (64 * 1024 * 1024)
#define BENCH_ARENA_SIZE_128M  (128 * 1024 * 1024)

// intersection benchmarks: AND of multiple bitsets at 30% and 70% density
#define BENCH_INTERSECT_RANGE       100000
#define BENCH_INTERSECT_30_COUNT    30000
#define BENCH_INTERSECT_70_COUNT    70000
#define FLAT_WORDS_INTERSECT        (BENCH_INTERSECT_RANGE / 64 + 1)
#define SKIP_WORDS_INTERSECT        ((FLAT_WORDS_INTERSECT + 63) / 64)

// 1M intersection benchmarks: realistic entity counts
#define BENCH_INTERSECT_1M_RANGE         1000000
#define FLAT_WORDS_INTERSECT_1M          (BENCH_INTERSECT_1M_RANGE / 64 + 1)
#define SKIP_WORDS_INTERSECT_1M          ((FLAT_WORDS_INTERSECT_1M + 63) / 64)

// ultra-sparse bit counts for 1M range
#define BENCH_INTERSECT_1M_ULTRASPARSE_1     1
#define BENCH_INTERSECT_1M_ULTRASPARSE_100   100
#define BENCH_INTERSECT_1M_ULTRASPARSE_1K    1000

// low density bit counts for 1M range
#define BENCH_INTERSECT_1M_LOW_10PCT    100000
#define BENCH_INTERSECT_1M_LOW_15PCT    150000

// high density bit counts for 1M range
#define BENCH_INTERSECT_1M_HIGH_70PCT   700000
#define BENCH_INTERSECT_1M_HIGH_90PCT   900000

#define FLAT_SKIP_SET(flat, skip, i) \
	do { \
		uint64_t _skip_w_ = (uint64_t)(i) >> 6; \
		(flat)[_skip_w_] |= (1ULL << ((uint64_t)(i) & 63)); \
		(skip)[_skip_w_ >> 6] |= (1ULL << (_skip_w_ & 63)); \
	} while(0)


// ============================================================================
// pre-generated random indices (deterministic seed)
// ============================================================================

static uint64_t g_rand_1k[BENCH_OP_COUNT];
static uint64_t g_rand_sparse[BENCH_OP_COUNT];
static uint64_t g_rand_dense[BENCH_OP_COUNT];
static uint64_t g_rand_10k[BENCH_OP_COUNT_10K];
static uint64_t g_rand_100k[BENCH_OP_COUNT_100K];
static uint64_t g_rand_1m[BENCH_OP_COUNT_1M];
static uint64_t g_rand_2m[BENCH_OP_COUNT_2M];
static uint64_t g_rand_4m[BENCH_OP_COUNT_4M];
static uint64_t g_wide_gap[BENCH_WIDE_GAP_COUNT];

static void init_rand_indices(void)
{
	srand(42);
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		g_rand_1k[i]     = (uint64_t)rand() % BENCH_BIT_RANGE;
		g_rand_sparse[i] = (uint64_t)rand() % BENCH_SPARSE_RANGE;
		g_rand_dense[i]  = (uint64_t)rand() % BENCH_DENSE_RANGE;
	}
	for (int i = 0; i < BENCH_OP_COUNT_10K; i++)
	{
		g_rand_10k[i] = (uint64_t)rand() % BENCH_BIT_RANGE;
	}
	for (int i = 0; i < BENCH_OP_COUNT_100K; i++)
	{
		g_rand_100k[i] = (uint64_t)rand() % BENCH_BIT_RANGE_100K;
	}
	for (int i = 0; i < BENCH_OP_COUNT_1M; i++)
	{
		g_rand_1m[i] = (uint64_t)rand() % BENCH_BIT_RANGE_1M;
	}
	for (int i = 0; i < BENCH_OP_COUNT_2M; i++)
	{
		g_rand_2m[i] = (uint64_t)rand() % BENCH_BIT_RANGE_2M;
	}
	for (int i = 0; i < BENCH_OP_COUNT_4M; i++)
	{
		g_rand_4m[i] = (uint64_t)rand() % BENCH_BIT_RANGE_4M;
	}
	for (int i = 0; i < BENCH_WIDE_GAP_COUNT; i++)
	{
		g_wide_gap[i] = (uint64_t)i * BENCH_WIDE_GAP_STRIDE;
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
	uint64_t flat_skip[FLAT_WORDS_1K];
	uint64_t skip_table[SKIP_WORDS_1K];
};

UBENCH_F_SETUP(bitset_set_1k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_16,   &ubench_fixture->arena, 16);
	w_sparse_bitset_init(&ubench_fixture->bs_64,   &ubench_fixture->arena, 64);
	w_sparse_bitset_init(&ubench_fixture->bs_256,  &ubench_fixture->arena, 256);
	w_sparse_bitset_init(&ubench_fixture->bs_1024, &ubench_fixture->arena, 1024);
	memset(ubench_fixture->flat,       0, sizeof(ubench_fixture->flat));
	memset(ubench_fixture->flat_skip,  0, sizeof(ubench_fixture->flat_skip));
	memset(ubench_fixture->skip_table, 0, sizeof(ubench_fixture->skip_table));
}

UBENCH_F_TEARDOWN(bitset_set_1k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_16);
	w_sparse_bitset_free(&ubench_fixture->bs_64);
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_sparse_bitset_free(&ubench_fixture->bs_1024);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bitset_set_1k, set_1k_16_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_16, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_16);
}

UBENCH_F(bitset_set_1k, set_1k_64_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_64, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_64);
}

UBENCH_F(bitset_set_1k, set_1k_256_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_256);
}

UBENCH_F(bitset_set_1k, set_1k_1024_page_size)
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

UBENCH_F(bitset_set_1k, set_1k_flat_skip)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->flat_skip);
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
	uint64_t flat_skip[FLAT_WORDS_1K];
	uint64_t skip_table[SKIP_WORDS_1K];
};

UBENCH_F_SETUP(bitset_get_1k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_16,   &ubench_fixture->arena, 16);
	w_sparse_bitset_init(&ubench_fixture->bs_64,   &ubench_fixture->arena, 64);
	w_sparse_bitset_init(&ubench_fixture->bs_256,  &ubench_fixture->arena, 256);
	w_sparse_bitset_init(&ubench_fixture->bs_1024, &ubench_fixture->arena, 1024);
	memset(ubench_fixture->flat,       0, sizeof(ubench_fixture->flat));
	memset(ubench_fixture->flat_skip,  0, sizeof(ubench_fixture->flat_skip));
	memset(ubench_fixture->skip_table, 0, sizeof(ubench_fixture->skip_table));

	// pre-populate all structures with the random bit indices
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_16,   g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_64,   g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_256,  g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_1024, g_rand_1k[i]);
		FLAT_SET(ubench_fixture->flat, g_rand_1k[i]);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, g_rand_1k[i]);
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

UBENCH_F(bitset_get_1k, get_1k_16_page_size)
{
	bool result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		result = w_sparse_bitset_get(&ubench_fixture->bs_16, g_rand_1k[i]);
		UBENCH_DO_NOTHING(&result);
	}
}

UBENCH_F(bitset_get_1k, get_1k_64_page_size)
{
	bool result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		result = w_sparse_bitset_get(&ubench_fixture->bs_64, g_rand_1k[i]);
		UBENCH_DO_NOTHING(&result);
	}
}

UBENCH_F(bitset_get_1k, get_1k_256_page_size)
{
	bool result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		result = w_sparse_bitset_get(&ubench_fixture->bs_256, g_rand_1k[i]);
		UBENCH_DO_NOTHING(&result);
	}
}

UBENCH_F(bitset_get_1k, get_1k_1024_page_size)
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

UBENCH_F(bitset_get_1k, get_1k_flat_skip)
{
	uint64_t result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		result = FLAT_GET(ubench_fixture->flat_skip, g_rand_1k[i]);
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
	uint64_t flat_skip[FLAT_WORDS_1K];
	uint64_t skip_table[SKIP_WORDS_1K];
};

UBENCH_F_SETUP(bitset_clear_1k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_16,   &ubench_fixture->arena, 16);
	w_sparse_bitset_init(&ubench_fixture->bs_64,   &ubench_fixture->arena, 64);
	w_sparse_bitset_init(&ubench_fixture->bs_256,  &ubench_fixture->arena, 256);
	w_sparse_bitset_init(&ubench_fixture->bs_1024, &ubench_fixture->arena, 1024);
	memset(ubench_fixture->flat,       0, sizeof(ubench_fixture->flat));
	memset(ubench_fixture->flat_skip,  0, sizeof(ubench_fixture->flat_skip));
	memset(ubench_fixture->skip_table, 0, sizeof(ubench_fixture->skip_table));

	// pre-populate so clear has bits to work with on first iteration
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_16,   g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_64,   g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_256,  g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_1024, g_rand_1k[i]);
		FLAT_SET(ubench_fixture->flat, g_rand_1k[i]);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, g_rand_1k[i]);
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

UBENCH_F(bitset_clear_1k, clear_1k_16_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_clear(&ubench_fixture->bs_16, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_16);
}

UBENCH_F(bitset_clear_1k, clear_1k_64_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_clear(&ubench_fixture->bs_64, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_64);
}

UBENCH_F(bitset_clear_1k, clear_1k_256_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_clear(&ubench_fixture->bs_256, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_256);
}

UBENCH_F(bitset_clear_1k, clear_1k_1024_page_size)
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

UBENCH_F(bitset_clear_1k, clear_1k_flat_skip)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		FLAT_CLEAR(ubench_fixture->flat_skip, g_rand_1k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->flat_skip);
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
	uint64_t flat_skip[FLAT_WORDS_1K];
	uint64_t skip_table[SKIP_WORDS_1K];
};

UBENCH_F_SETUP(bitset_mixed_1k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_16,   &ubench_fixture->arena, 16);
	w_sparse_bitset_init(&ubench_fixture->bs_64,   &ubench_fixture->arena, 64);
	w_sparse_bitset_init(&ubench_fixture->bs_256,  &ubench_fixture->arena, 256);
	w_sparse_bitset_init(&ubench_fixture->bs_1024, &ubench_fixture->arena, 1024);
	memset(ubench_fixture->flat,       0, sizeof(ubench_fixture->flat));
	memset(ubench_fixture->flat_skip,  0, sizeof(ubench_fixture->flat_skip));
	memset(ubench_fixture->skip_table, 0, sizeof(ubench_fixture->skip_table));

	// pre-populate half the indices so get/clear have data to work with
	for (int i = 0; i < BENCH_OP_COUNT / 2; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_16,   g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_64,   g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_256,  g_rand_1k[i]);
		w_sparse_bitset_set(&ubench_fixture->bs_1024, g_rand_1k[i]);
		FLAT_SET(ubench_fixture->flat, g_rand_1k[i]);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, g_rand_1k[i]);
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

UBENCH_F(bitset_mixed_1k, mixed_1k_16_page_size)
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

UBENCH_F(bitset_mixed_1k, mixed_1k_64_page_size)
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

UBENCH_F(bitset_mixed_1k, mixed_1k_256_page_size)
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

UBENCH_F(bitset_mixed_1k, mixed_1k_1024_page_size)
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

UBENCH_F(bitset_mixed_1k, mixed_1k_flat_skip)
{
	uint64_t result;
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		int op = i % 3;
		if (op == 0)
		{
			FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, g_rand_1k[i]);
		}
		else if (op == 1)
		{
			result = FLAT_GET(ubench_fixture->flat_skip, g_rand_1k[i]);
			UBENCH_DO_NOTHING(&result);
		}
		else
		{
			FLAT_CLEAR(ubench_fixture->flat_skip, g_rand_1k[i]);
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->flat_skip);
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

UBENCH_F(bitset_sparse, set_sparse_16_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_16, g_rand_sparse[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_16);
}

UBENCH_F(bitset_sparse, set_sparse_64_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_64, g_rand_sparse[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_64);
}

UBENCH_F(bitset_sparse, set_sparse_256_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_sparse[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_256);
}

UBENCH_F(bitset_sparse, set_sparse_1024_page_size)
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

UBENCH_F(bitset_dense, set_dense_16_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_16, g_rand_dense[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_16);
}

UBENCH_F(bitset_dense, set_dense_64_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_64, g_rand_dense[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_64);
}

UBENCH_F(bitset_dense, set_dense_256_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_dense[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_256);
}

UBENCH_F(bitset_dense, set_dense_1024_page_size)
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


// ============================================================================
// SET 10000 RANDOM BITS: 256-page vs flat (0-65535 range)
// ============================================================================

struct bitset_set_10k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t flat[FLAT_WORDS_10K];
};

UBENCH_F_SETUP(bitset_set_10k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	memset(ubench_fixture->flat, 0, sizeof(ubench_fixture->flat));
}

UBENCH_F_TEARDOWN(bitset_set_10k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bitset_set_10k, set_10k_256_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT_10K; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_10k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_256);
}

UBENCH_F(bitset_set_10k, set_10k_flat)
{
	for (int i = 0; i < BENCH_OP_COUNT_10K; i++)
	{
		FLAT_SET(ubench_fixture->flat, g_rand_10k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->flat);
}


// ============================================================================
// SET 100000 RANDOM BITS: 256-page vs flat (0-655359 range)
// ============================================================================

struct bitset_set_100k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t flat[FLAT_WORDS_100K];
};

UBENCH_F_SETUP(bitset_set_100k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	memset(ubench_fixture->flat, 0, sizeof(ubench_fixture->flat));
}

UBENCH_F_TEARDOWN(bitset_set_100k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bitset_set_100k, set_100k_256_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT_100K; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_100k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_256);
}

UBENCH_F(bitset_set_100k, set_100k_flat)
{
	for (int i = 0; i < BENCH_OP_COUNT_100K; i++)
	{
		FLAT_SET(ubench_fixture->flat, g_rand_100k[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->flat);
}


// ============================================================================
// SET 1000000 RANDOM BITS: 256-page vs flat (0-6553599 range)
// ============================================================================

struct bitset_set_1m
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t flat[FLAT_WORDS_1M];
};

UBENCH_F_SETUP(bitset_set_1m)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_LARGE);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	memset(ubench_fixture->flat, 0, sizeof(ubench_fixture->flat));
}

UBENCH_F_TEARDOWN(bitset_set_1m)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bitset_set_1m, set_1m_256_page_size)
{
	for (int i = 0; i < BENCH_OP_COUNT_1M; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_1m[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_256);
}

UBENCH_F(bitset_set_1m, set_1m_flat)
{
	for (int i = 0; i < BENCH_OP_COUNT_1M; i++)
	{
		FLAT_SET(ubench_fixture->flat, g_rand_1m[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->flat);
}


// ============================================================================
// WIDE-GAP SPARSE: 100000 bits at stride 100 across 0-10M range
// ============================================================================

struct bitset_sparse_100k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t flat[FLAT_WORDS_WIDE_GAP];
};

UBENCH_F_SETUP(bitset_sparse_100k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_LARGE);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	memset(ubench_fixture->flat, 0, sizeof(ubench_fixture->flat));
}

UBENCH_F_TEARDOWN(bitset_sparse_100k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bitset_sparse_100k, set_sparse_100k_256_page_size)
{
	for (int i = 0; i < BENCH_WIDE_GAP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_wide_gap[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->bs_256);
}

UBENCH_F(bitset_sparse_100k, set_sparse_100k_flat)
{
	for (int i = 0; i < BENCH_WIDE_GAP_COUNT; i++)
	{
		FLAT_SET(ubench_fixture->flat, g_wide_gap[i]);
	}
	UBENCH_DO_NOTHING(&ubench_fixture->flat);
}


// ============================================================================
// ITERATE 1000 SET BITS: sparse_256 vs flat vs flat_skip
// ============================================================================

struct bitset_iterate_1k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t flat[FLAT_WORDS_1K];
	uint64_t flat_skip[FLAT_WORDS_1K];
	uint64_t skip_table[SKIP_WORDS_1K];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_1k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	memset(ubench_fixture->flat,       0, sizeof(ubench_fixture->flat));
	memset(ubench_fixture->flat_skip,  0, sizeof(ubench_fixture->flat_skip));
	memset(ubench_fixture->skip_table, 0, sizeof(ubench_fixture->skip_table));
	ubench_fixture->result = malloc(BENCH_OP_COUNT * sizeof(uint64_t));

	for (int i = 0; i < BENCH_OP_COUNT; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_1k[i]);
		FLAT_SET(ubench_fixture->flat, g_rand_1k[i]);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, g_rand_1k[i]);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_1k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_1k, iterate_1k_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_1k, iterate_1k_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_1K; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_1k, iterate_1k_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_1K; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_1K ? base_w + 64 : FLAT_WORDS_1K;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// ITERATE 10000 SET BITS: sparse_256 vs flat vs flat_skip
// ============================================================================

struct bitset_iterate_10k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t flat[FLAT_WORDS_10K];
	uint64_t flat_skip[FLAT_WORDS_10K];
	uint64_t skip_table[SKIP_WORDS_10K];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_10k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	memset(ubench_fixture->flat,       0, sizeof(ubench_fixture->flat));
	memset(ubench_fixture->flat_skip,  0, sizeof(ubench_fixture->flat_skip));
	memset(ubench_fixture->skip_table, 0, sizeof(ubench_fixture->skip_table));
	ubench_fixture->result = malloc(BENCH_OP_COUNT_10K * sizeof(uint64_t));

	for (int i = 0; i < BENCH_OP_COUNT_10K; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_10k[i]);
		FLAT_SET(ubench_fixture->flat, g_rand_10k[i]);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, g_rand_10k[i]);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_10k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_10k, iterate_10k_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_10k, iterate_10k_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_10K; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_10k, iterate_10k_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_10K; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_10K ? base_w + 64 : FLAT_WORDS_10K;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// ITERATE 100000 SET BITS: sparse_256 vs flat vs flat_skip
// ============================================================================

struct bitset_iterate_100k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t flat[FLAT_WORDS_100K];
	uint64_t flat_skip[FLAT_WORDS_100K];
	uint64_t skip_table[SKIP_WORDS_100K];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_100k)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	memset(ubench_fixture->flat,       0, sizeof(ubench_fixture->flat));
	memset(ubench_fixture->flat_skip,  0, sizeof(ubench_fixture->flat_skip));
	memset(ubench_fixture->skip_table, 0, sizeof(ubench_fixture->skip_table));
	ubench_fixture->result = malloc(BENCH_OP_COUNT_100K * sizeof(uint64_t));

	for (int i = 0; i < BENCH_OP_COUNT_100K; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_100k[i]);
		FLAT_SET(ubench_fixture->flat, g_rand_100k[i]);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, g_rand_100k[i]);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_100k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_100k, iterate_100k_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_100k, iterate_100k_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_100K; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_100k, iterate_100k_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_100K; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_100K ? base_w + 64 : FLAT_WORDS_100K;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// ITERATE 1000000 SET BITS: sparse_256 vs flat vs flat_skip
// ============================================================================

struct bitset_iterate_1m
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t flat[FLAT_WORDS_1M];
	uint64_t flat_skip[FLAT_WORDS_1M];
	uint64_t skip_table[SKIP_WORDS_1M];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_1m)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_LARGE);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	memset(ubench_fixture->flat,       0, sizeof(ubench_fixture->flat));
	memset(ubench_fixture->flat_skip,  0, sizeof(ubench_fixture->flat_skip));
	memset(ubench_fixture->skip_table, 0, sizeof(ubench_fixture->skip_table));
	ubench_fixture->result = malloc(BENCH_OP_COUNT_1M * sizeof(uint64_t));

	for (int i = 0; i < BENCH_OP_COUNT_1M; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_1m[i]);
		FLAT_SET(ubench_fixture->flat, g_rand_1m[i]);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, g_rand_1m[i]);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_1m)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_1m, iterate_1m_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_1m, iterate_1m_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_1M; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_1m, iterate_1m_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_1M; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_1M ? base_w + 64 : FLAT_WORDS_1M;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// ITERATE 2000000 SET BITS: sparse_256 vs flat vs flat_skip
// ============================================================================

struct bitset_iterate_2m
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t flat[FLAT_WORDS_2M];
	uint64_t flat_skip[FLAT_WORDS_2M];
	uint64_t skip_table[SKIP_WORDS_2M];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_2m)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_2M);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	memset(ubench_fixture->flat,       0, sizeof(ubench_fixture->flat));
	memset(ubench_fixture->flat_skip,  0, sizeof(ubench_fixture->flat_skip));
	memset(ubench_fixture->skip_table, 0, sizeof(ubench_fixture->skip_table));
	ubench_fixture->result = malloc(BENCH_OP_COUNT_2M * sizeof(uint64_t));

	for (int i = 0; i < BENCH_OP_COUNT_2M; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_2m[i]);
		FLAT_SET(ubench_fixture->flat, g_rand_2m[i]);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, g_rand_2m[i]);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_2m)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_2m, iterate_2m_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_2m, iterate_2m_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_2M; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_2m, iterate_2m_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_2M; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_2M ? base_w + 64 : FLAT_WORDS_2M;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// ITERATE 4000000 SET BITS: sparse_256 vs flat vs flat_skip
// ============================================================================

struct bitset_iterate_4m
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t flat[FLAT_WORDS_4M];
	uint64_t flat_skip[FLAT_WORDS_4M];
	uint64_t skip_table[SKIP_WORDS_4M];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_4m)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_4M);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	memset(ubench_fixture->flat,       0, sizeof(ubench_fixture->flat));
	memset(ubench_fixture->flat_skip,  0, sizeof(ubench_fixture->flat_skip));
	memset(ubench_fixture->skip_table, 0, sizeof(ubench_fixture->skip_table));
	ubench_fixture->result = malloc(BENCH_OP_COUNT_4M * sizeof(uint64_t));

	for (int i = 0; i < BENCH_OP_COUNT_4M; i++)
	{
		w_sparse_bitset_set(&ubench_fixture->bs_256, g_rand_4m[i]);
		FLAT_SET(ubench_fixture->flat, g_rand_4m[i]);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, g_rand_4m[i]);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_4m)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_4m, iterate_4m_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_4m, iterate_4m_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_4M; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_4m, iterate_4m_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_4M; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_4M ? base_w + 64 : FLAT_WORDS_4M;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// ITERATE 1000 SPARSE BITS: sparse_256 vs flat vs flat_skip
// bits at i*10000, ~10M range, ~610 pages allocated (1 bit/page)
// ============================================================================

struct bitset_iterate_sparse_1k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t *flat;
	uint64_t *flat_skip;
	uint64_t *skip_table;
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_sparse_1k)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	ubench_fixture->flat       = calloc(FLAT_WORDS_SPARSE_ITER_1K, sizeof(uint64_t));
	ubench_fixture->flat_skip  = calloc(FLAT_WORDS_SPARSE_ITER_1K, sizeof(uint64_t));
	ubench_fixture->skip_table = calloc(SKIP_WORDS_SPARSE_ITER_1K, sizeof(uint64_t));
	ubench_fixture->result     = malloc(BENCH_OP_COUNT * sizeof(uint64_t));

	for (uint64_t i = 0; i < BENCH_OP_COUNT; i++)
	{
		uint64_t idx = i * BENCH_SPARSE_ITER_STRIDE_1K;
		w_sparse_bitset_set(&ubench_fixture->bs_256, idx);
		FLAT_SET(ubench_fixture->flat, idx);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, idx);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_sparse_1k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->flat);
	free(ubench_fixture->flat_skip);
	free(ubench_fixture->skip_table);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_1k, iterate_sparse_1k_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_1k, iterate_sparse_1k_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_SPARSE_ITER_1K; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_1k, iterate_sparse_1k_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_SPARSE_ITER_1K; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_SPARSE_ITER_1K ? base_w + 64 : FLAT_WORDS_SPARSE_ITER_1K;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// ITERATE 10000 SPARSE BITS: sparse_256 vs flat vs flat_skip
// bits at i*1000, ~10M range, ~625 pages allocated (~16 bits/page)
// ============================================================================

struct bitset_iterate_sparse_10k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t *flat;
	uint64_t *flat_skip;
	uint64_t *skip_table;
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_sparse_10k)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	ubench_fixture->flat       = calloc(FLAT_WORDS_SPARSE_ITER_10K, sizeof(uint64_t));
	ubench_fixture->flat_skip  = calloc(FLAT_WORDS_SPARSE_ITER_10K, sizeof(uint64_t));
	ubench_fixture->skip_table = calloc(SKIP_WORDS_SPARSE_ITER_10K, sizeof(uint64_t));
	ubench_fixture->result     = malloc(BENCH_OP_COUNT_10K * sizeof(uint64_t));

	for (uint64_t i = 0; i < BENCH_OP_COUNT_10K; i++)
	{
		uint64_t idx = i * BENCH_SPARSE_ITER_STRIDE_10K;
		w_sparse_bitset_set(&ubench_fixture->bs_256, idx);
		FLAT_SET(ubench_fixture->flat, idx);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, idx);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_sparse_10k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->flat);
	free(ubench_fixture->flat_skip);
	free(ubench_fixture->skip_table);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_10k, iterate_sparse_10k_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_10k, iterate_sparse_10k_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_SPARSE_ITER_10K; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_10k, iterate_sparse_10k_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_SPARSE_ITER_10K; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_SPARSE_ITER_10K ? base_w + 64 : FLAT_WORDS_SPARSE_ITER_10K;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// ITERATE 100000 SPARSE BITS: sparse_256 vs flat vs flat_skip
// bits at i*100, ~10M range, ~613 pages allocated (~163 bits/page)
// ============================================================================

struct bitset_iterate_sparse_100k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t *flat;
	uint64_t *flat_skip;
	uint64_t *skip_table;
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_sparse_100k)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_LARGE);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	ubench_fixture->flat       = calloc(FLAT_WORDS_SPARSE_ITER_100K, sizeof(uint64_t));
	ubench_fixture->flat_skip  = calloc(FLAT_WORDS_SPARSE_ITER_100K, sizeof(uint64_t));
	ubench_fixture->skip_table = calloc(SKIP_WORDS_SPARSE_ITER_100K, sizeof(uint64_t));
	ubench_fixture->result     = malloc(BENCH_OP_COUNT_100K * sizeof(uint64_t));

	for (uint64_t i = 0; i < BENCH_OP_COUNT_100K; i++)
	{
		uint64_t idx = i * BENCH_SPARSE_ITER_STRIDE_100K;
		w_sparse_bitset_set(&ubench_fixture->bs_256, idx);
		FLAT_SET(ubench_fixture->flat, idx);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, idx);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_sparse_100k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->flat);
	free(ubench_fixture->flat_skip);
	free(ubench_fixture->skip_table);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_100k, iterate_sparse_100k_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_100k, iterate_sparse_100k_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_SPARSE_ITER_100K; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_100k, iterate_sparse_100k_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_SPARSE_ITER_100K; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_SPARSE_ITER_100K ? base_w + 64 : FLAT_WORDS_SPARSE_ITER_100K;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// ITERATE 1000000 SPARSE BITS: sparse_256 vs flat vs flat_skip
// bits at i*100, ~100M range, ~6104 pages allocated (~163 bits/page)
// ============================================================================

struct bitset_iterate_sparse_1m
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t *flat;
	uint64_t *flat_skip;
	uint64_t *skip_table;
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_sparse_1m)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_16M);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	ubench_fixture->flat       = calloc(FLAT_WORDS_SPARSE_ITER_1M, sizeof(uint64_t));
	ubench_fixture->flat_skip  = calloc(FLAT_WORDS_SPARSE_ITER_1M, sizeof(uint64_t));
	ubench_fixture->skip_table = calloc(SKIP_WORDS_SPARSE_ITER_1M, sizeof(uint64_t));
	ubench_fixture->result     = malloc(BENCH_OP_COUNT_1M * sizeof(uint64_t));

	for (uint64_t i = 0; i < BENCH_OP_COUNT_1M; i++)
	{
		uint64_t idx = i * BENCH_SPARSE_ITER_STRIDE_1M;
		w_sparse_bitset_set(&ubench_fixture->bs_256, idx);
		FLAT_SET(ubench_fixture->flat, idx);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, idx);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_sparse_1m)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->flat);
	free(ubench_fixture->flat_skip);
	free(ubench_fixture->skip_table);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_1m, iterate_sparse_1m_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_1m, iterate_sparse_1m_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_SPARSE_ITER_1M; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_1m, iterate_sparse_1m_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_SPARSE_ITER_1M; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_SPARSE_ITER_1M ? base_w + 64 : FLAT_WORDS_SPARSE_ITER_1M;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// ITERATE 2000000 SPARSE BITS: sparse_256 vs flat vs flat_skip
// bits at i*100, ~200M range, ~12208 pages allocated (~163 bits/page)
// ============================================================================

struct bitset_iterate_sparse_2m
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t *flat;
	uint64_t *flat_skip;
	uint64_t *skip_table;
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_sparse_2m)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_32M);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	ubench_fixture->flat       = calloc(FLAT_WORDS_SPARSE_ITER_2M, sizeof(uint64_t));
	ubench_fixture->flat_skip  = calloc(FLAT_WORDS_SPARSE_ITER_2M, sizeof(uint64_t));
	ubench_fixture->skip_table = calloc(SKIP_WORDS_SPARSE_ITER_2M, sizeof(uint64_t));
	ubench_fixture->result     = malloc(BENCH_OP_COUNT_2M * sizeof(uint64_t));

	for (uint64_t i = 0; i < BENCH_OP_COUNT_2M; i++)
	{
		uint64_t idx = i * BENCH_SPARSE_ITER_STRIDE_2M;
		w_sparse_bitset_set(&ubench_fixture->bs_256, idx);
		FLAT_SET(ubench_fixture->flat, idx);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, idx);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_sparse_2m)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->flat);
	free(ubench_fixture->flat_skip);
	free(ubench_fixture->skip_table);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_2m, iterate_sparse_2m_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_2m, iterate_sparse_2m_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_SPARSE_ITER_2M; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_sparse_2m, iterate_sparse_2m_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_SPARSE_ITER_2M; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_SPARSE_ITER_2M ? base_w + 64 : FLAT_WORDS_SPARSE_ITER_2M;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// ITERATE ULTRA-SPARSE 1K BITS: 0.01% density, 1000 bits across 10M range
// ============================================================================

struct bitset_iterate_ultrasparse_1k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t *flat;
	uint64_t *flat_skip;
	uint64_t *skip_table;
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_ultrasparse_1k)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	ubench_fixture->flat       = calloc(FLAT_WORDS_ULTRASPARSE_1K, sizeof(uint64_t));
	ubench_fixture->flat_skip  = calloc(FLAT_WORDS_ULTRASPARSE_1K, sizeof(uint64_t));
	ubench_fixture->skip_table = calloc(SKIP_WORDS_ULTRASPARSE_1K, sizeof(uint64_t));
	ubench_fixture->result     = malloc(BENCH_ULTRASPARSE_1K_COUNT * sizeof(uint64_t));

	for (uint64_t i = 0; i < BENCH_ULTRASPARSE_1K_COUNT; i++)
	{
		uint64_t idx = i * BENCH_ULTRASPARSE_STRIDE;
		w_sparse_bitset_set(&ubench_fixture->bs_256, idx);
		FLAT_SET(ubench_fixture->flat, idx);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, idx);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_ultrasparse_1k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->flat);
	free(ubench_fixture->flat_skip);
	free(ubench_fixture->skip_table);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_ultrasparse_1k, iterate_ultrasparse_1k_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_ultrasparse_1k, iterate_ultrasparse_1k_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_ULTRASPARSE_1K; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_ultrasparse_1k, iterate_ultrasparse_1k_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_ULTRASPARSE_1K; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_ULTRASPARSE_1K ? base_w + 64 : FLAT_WORDS_ULTRASPARSE_1K;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// ITERATE ULTRA-SPARSE 10K BITS: 0.01% density, 10000 bits across 100M range
// ============================================================================

struct bitset_iterate_ultrasparse_10k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t *flat;
	uint64_t *flat_skip;
	uint64_t *skip_table;
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_ultrasparse_10k)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_16M);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	ubench_fixture->flat       = calloc(FLAT_WORDS_ULTRASPARSE_10K, sizeof(uint64_t));
	ubench_fixture->flat_skip  = calloc(FLAT_WORDS_ULTRASPARSE_10K, sizeof(uint64_t));
	ubench_fixture->skip_table = calloc(SKIP_WORDS_ULTRASPARSE_10K, sizeof(uint64_t));
	ubench_fixture->result     = malloc(BENCH_ULTRASPARSE_10K_COUNT * sizeof(uint64_t));

	for (uint64_t i = 0; i < BENCH_ULTRASPARSE_10K_COUNT; i++)
	{
		uint64_t idx = i * BENCH_ULTRASPARSE_STRIDE;
		w_sparse_bitset_set(&ubench_fixture->bs_256, idx);
		FLAT_SET(ubench_fixture->flat, idx);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, idx);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_ultrasparse_10k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->flat);
	free(ubench_fixture->flat_skip);
	free(ubench_fixture->skip_table);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_ultrasparse_10k, iterate_ultrasparse_10k_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_ultrasparse_10k, iterate_ultrasparse_10k_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_ULTRASPARSE_10K; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_ultrasparse_10k, iterate_ultrasparse_10k_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_ULTRASPARSE_10K; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_ULTRASPARSE_10K ? base_w + 64 : FLAT_WORDS_ULTRASPARSE_10K;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// ITERATE ULTRA-SPARSE 100K BITS: 0.01% density, 100000 bits across 1B range
// ============================================================================

struct bitset_iterate_ultrasparse_100k
{
	struct w_arena arena;
	struct w_sparse_bitset bs_256;
	uint64_t *flat;
	uint64_t *flat_skip;
	uint64_t *skip_table;
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_iterate_ultrasparse_100k)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_128M);
	w_sparse_bitset_init(&ubench_fixture->bs_256, &ubench_fixture->arena, 256);
	ubench_fixture->flat       = calloc(FLAT_WORDS_ULTRASPARSE_100K, sizeof(uint64_t));
	ubench_fixture->flat_skip  = calloc(FLAT_WORDS_ULTRASPARSE_100K, sizeof(uint64_t));
	ubench_fixture->skip_table = calloc(SKIP_WORDS_ULTRASPARSE_100K, sizeof(uint64_t));
	ubench_fixture->result     = malloc(BENCH_ULTRASPARSE_100K_COUNT * sizeof(uint64_t));

	for (uint64_t i = 0; i < BENCH_ULTRASPARSE_100K_COUNT; i++)
	{
		uint64_t idx = i * BENCH_ULTRASPARSE_STRIDE;
		w_sparse_bitset_set(&ubench_fixture->bs_256, idx);
		FLAT_SET(ubench_fixture->flat, idx);
		FLAT_SKIP_SET(ubench_fixture->flat_skip, ubench_fixture->skip_table, idx);
	}
}

UBENCH_F_TEARDOWN(bitset_iterate_ultrasparse_100k)
{
	w_sparse_bitset_free(&ubench_fixture->bs_256);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->flat);
	free(ubench_fixture->flat_skip);
	free(ubench_fixture->skip_table);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_iterate_ultrasparse_100k, iterate_ultrasparse_100k_sparse_256)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs = &ubench_fixture->bs_256;
	for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
	{
		uint64_t lword = bs->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= bs->pages_length) continue;
			struct w_sparse_bitset_page *page = &bs->pages[page_index];
			if (!page->bits || page->first_set == UINT32_MAX) continue;
			for (uint32_t w = page->first_set; w <= page->last_set; w++)
			{
				uint64_t word = page->bits[w];
				while (word)
				{
					int b = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)b;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_ultrasparse_100k, iterate_ultrasparse_100k_flat)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_ULTRASPARSE_100K; w++)
	{
		uint64_t word = ubench_fixture->flat[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_iterate_ultrasparse_100k, iterate_ultrasparse_100k_flat_skip)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_ULTRASPARSE_100K; sw++)
	{
		if (!ubench_fixture->skip_table[sw]) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_ULTRASPARSE_100K ? base_w + 64 : FLAT_WORDS_ULTRASPARSE_100K;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// INTERSECTION BENCHMARKS: AND of 2, 3, and 5 bitsets at 30%/70% density
// ============================================================================

struct bitset_intersect_2
{
	struct w_arena arena;
	struct w_sparse_bitset bs_30_a;
	struct w_sparse_bitset bs_30_b;
	struct w_sparse_bitset bs_70_a;
	struct w_sparse_bitset bs_70_b;
	uint64_t *flat_30_a;
	uint64_t *flat_30_b;
	uint64_t *flat_70_a;
	uint64_t *flat_70_b;
	uint64_t *flat_skip_30_a;
	uint64_t *flat_skip_30_b;
	uint64_t *flat_skip_70_a;
	uint64_t *flat_skip_70_b;
	uint64_t *skip_30_a;
	uint64_t *skip_30_b;
	uint64_t *skip_70_a;
	uint64_t *skip_70_b;
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_intersect_2)
{
	init_rand_indices();
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_sparse_bitset_init(&ubench_fixture->bs_30_a, &ubench_fixture->arena, 256);
	w_sparse_bitset_init(&ubench_fixture->bs_30_b, &ubench_fixture->arena, 256);
	w_sparse_bitset_init(&ubench_fixture->bs_70_a, &ubench_fixture->arena, 256);
	w_sparse_bitset_init(&ubench_fixture->bs_70_b, &ubench_fixture->arena, 256);

	ubench_fixture->flat_30_a = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
	ubench_fixture->flat_30_b = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
	ubench_fixture->flat_70_a = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
	ubench_fixture->flat_70_b = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
	ubench_fixture->flat_skip_30_a = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
	ubench_fixture->flat_skip_30_b = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
	ubench_fixture->flat_skip_70_a = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
	ubench_fixture->flat_skip_70_b = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
	ubench_fixture->skip_30_a = calloc(SKIP_WORDS_INTERSECT, sizeof(uint64_t));
	ubench_fixture->skip_30_b = calloc(SKIP_WORDS_INTERSECT, sizeof(uint64_t));
	ubench_fixture->skip_70_a = calloc(SKIP_WORDS_INTERSECT, sizeof(uint64_t));
	ubench_fixture->skip_70_b = calloc(SKIP_WORDS_INTERSECT, sizeof(uint64_t));
	ubench_fixture->result = malloc(BENCH_INTERSECT_RANGE * sizeof(uint64_t));

	srand(42);
	for (uint64_t i = 0; i < BENCH_INTERSECT_30_COUNT; i++)
	{
		uint64_t idx_a = (uint64_t)rand() % BENCH_INTERSECT_RANGE;
		uint64_t idx_b = (uint64_t)rand() % BENCH_INTERSECT_RANGE;
		w_sparse_bitset_set(&ubench_fixture->bs_30_a, idx_a);
		w_sparse_bitset_set(&ubench_fixture->bs_30_b, idx_b);
		FLAT_SET(ubench_fixture->flat_30_a, idx_a);
		FLAT_SET(ubench_fixture->flat_30_b, idx_b);
		FLAT_SKIP_SET(ubench_fixture->flat_skip_30_a, ubench_fixture->skip_30_a, idx_a);
		FLAT_SKIP_SET(ubench_fixture->flat_skip_30_b, ubench_fixture->skip_30_b, idx_b);
	}
	for (uint64_t i = 0; i < BENCH_INTERSECT_70_COUNT; i++)
	{
		uint64_t idx_a = (uint64_t)rand() % BENCH_INTERSECT_RANGE;
		uint64_t idx_b = (uint64_t)rand() % BENCH_INTERSECT_RANGE;
		w_sparse_bitset_set(&ubench_fixture->bs_70_a, idx_a);
		w_sparse_bitset_set(&ubench_fixture->bs_70_b, idx_b);
		FLAT_SET(ubench_fixture->flat_70_a, idx_a);
		FLAT_SET(ubench_fixture->flat_70_b, idx_b);
		FLAT_SKIP_SET(ubench_fixture->flat_skip_70_a, ubench_fixture->skip_70_a, idx_a);
		FLAT_SKIP_SET(ubench_fixture->flat_skip_70_b, ubench_fixture->skip_70_b, idx_b);
	}
}

UBENCH_F_TEARDOWN(bitset_intersect_2)
{
	w_sparse_bitset_free(&ubench_fixture->bs_30_a);
	w_sparse_bitset_free(&ubench_fixture->bs_30_b);
	w_sparse_bitset_free(&ubench_fixture->bs_70_a);
	w_sparse_bitset_free(&ubench_fixture->bs_70_b);
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->flat_30_a);
	free(ubench_fixture->flat_30_b);
	free(ubench_fixture->flat_70_a);
	free(ubench_fixture->flat_70_b);
	free(ubench_fixture->flat_skip_30_a);
	free(ubench_fixture->flat_skip_30_b);
	free(ubench_fixture->flat_skip_70_a);
	free(ubench_fixture->flat_skip_70_b);
	free(ubench_fixture->skip_30_a);
	free(ubench_fixture->skip_30_b);
	free(ubench_fixture->skip_70_a);
	free(ubench_fixture->skip_70_b);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_intersect_2, intersect_2_sparse_30)
{
	uint64_t count = 0;
	struct w_sparse_bitset *a = &ubench_fixture->bs_30_a;
	struct w_sparse_bitset *b = &ubench_fixture->bs_30_b;
	uint64_t max_li = a->lookup_pages_length < b->lookup_pages_length ? a->lookup_pages_length : b->lookup_pages_length;
	for (uint64_t li = 0; li < max_li; li++)
	{
		uint64_t lword = a->lookup_pages[li] & b->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= a->pages_length || page_index >= b->pages_length) continue;
			struct w_sparse_bitset_page *pa = &a->pages[page_index];
			struct w_sparse_bitset_page *pb = &b->pages[page_index];
			if (!pa->bits || !pb->bits) continue;
			uint32_t first = pa->first_set > pb->first_set ? pa->first_set : pb->first_set;
			uint32_t last = pa->last_set < pb->last_set ? pa->last_set : pb->last_set;
			if (first > last || first == UINT32_MAX) continue;
			for (uint32_t w = first; w <= last; w++)
			{
				uint64_t word = pa->bits[w] & pb->bits[w];
				while (word)
				{
					int bit = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * a->page_size_ + w) * 64 + (uint64_t)bit;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_2, intersect_2_sparse_70)
{
	uint64_t count = 0;
	struct w_sparse_bitset *a = &ubench_fixture->bs_70_a;
	struct w_sparse_bitset *b = &ubench_fixture->bs_70_b;
	uint64_t max_li = a->lookup_pages_length < b->lookup_pages_length ? a->lookup_pages_length : b->lookup_pages_length;
	for (uint64_t li = 0; li < max_li; li++)
	{
		uint64_t lword = a->lookup_pages[li] & b->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			if (page_index >= a->pages_length || page_index >= b->pages_length) continue;
			struct w_sparse_bitset_page *pa = &a->pages[page_index];
			struct w_sparse_bitset_page *pb = &b->pages[page_index];
			if (!pa->bits || !pb->bits) continue;
			uint32_t first = pa->first_set > pb->first_set ? pa->first_set : pb->first_set;
			uint32_t last = pa->last_set < pb->last_set ? pa->last_set : pb->last_set;
			if (first > last || first == UINT32_MAX) continue;
			for (uint32_t w = first; w <= last; w++)
			{
				uint64_t word = pa->bits[w] & pb->bits[w];
				while (word)
				{
					int bit = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * a->page_size_ + w) * 64 + (uint64_t)bit;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_2, intersect_2_flat_30)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT; w++)
	{
		uint64_t word = ubench_fixture->flat_30_a[w] & ubench_fixture->flat_30_b[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_2, intersect_2_flat_70)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT; w++)
	{
		uint64_t word = ubench_fixture->flat_70_a[w] & ubench_fixture->flat_70_b[w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_2, intersect_2_flat_skip_30)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT; sw++)
	{
		uint64_t skip_word = ubench_fixture->skip_30_a[sw] & ubench_fixture->skip_30_b[sw];
		if (!skip_word) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT ? base_w + 64 : FLAT_WORDS_INTERSECT;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip_30_a[w] & ubench_fixture->flat_skip_30_b[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_2, intersect_2_flat_skip_70)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT; sw++)
	{
		uint64_t skip_word = ubench_fixture->skip_70_a[sw] & ubench_fixture->skip_70_b[sw];
		if (!skip_word) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT ? base_w + 64 : FLAT_WORDS_INTERSECT;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip_70_a[w] & ubench_fixture->flat_skip_70_b[w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// INTERSECTION OF 3 BITSETS
// ============================================================================

struct bitset_intersect_3
{
	struct w_arena arena;
	struct w_sparse_bitset bs_30[3];
	struct w_sparse_bitset bs_70[3];
	uint64_t *flat_30[3];
	uint64_t *flat_70[3];
	uint64_t *flat_skip_30[3];
	uint64_t *flat_skip_70[3];
	uint64_t *skip_30[3];
	uint64_t *skip_70[3];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_intersect_3)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_LARGE);
	for (int j = 0; j < 3; j++)
	{
		w_sparse_bitset_init(&ubench_fixture->bs_30[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_70[j], &ubench_fixture->arena, 256);
		ubench_fixture->flat_30[j] = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
		ubench_fixture->flat_70[j] = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
		ubench_fixture->flat_skip_30[j] = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
		ubench_fixture->flat_skip_70[j] = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
		ubench_fixture->skip_30[j] = calloc(SKIP_WORDS_INTERSECT, sizeof(uint64_t));
		ubench_fixture->skip_70[j] = calloc(SKIP_WORDS_INTERSECT, sizeof(uint64_t));
	}
	ubench_fixture->result = malloc(BENCH_INTERSECT_RANGE * sizeof(uint64_t));

	srand(42);
	for (int j = 0; j < 3; j++)
	{
		for (uint64_t i = 0; i < BENCH_INTERSECT_30_COUNT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_30[j], idx);
			FLAT_SET(ubench_fixture->flat_30[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_30[j], ubench_fixture->skip_30[j], idx);
		}
		for (uint64_t i = 0; i < BENCH_INTERSECT_70_COUNT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_70[j], idx);
			FLAT_SET(ubench_fixture->flat_70[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_70[j], ubench_fixture->skip_70[j], idx);
		}
	}
}

UBENCH_F_TEARDOWN(bitset_intersect_3)
{
	for (int j = 0; j < 3; j++)
	{
		w_sparse_bitset_free(&ubench_fixture->bs_30[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_70[j]);
		free(ubench_fixture->flat_30[j]);
		free(ubench_fixture->flat_70[j]);
		free(ubench_fixture->flat_skip_30[j]);
		free(ubench_fixture->flat_skip_70[j]);
		free(ubench_fixture->skip_30[j]);
		free(ubench_fixture->skip_70[j]);
	}
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_intersect_3, intersect_3_sparse_30)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs[3] = {&ubench_fixture->bs_30[0], &ubench_fixture->bs_30[1], &ubench_fixture->bs_30[2]};
	uint64_t max_li = bs[0]->lookup_pages_length;
	for (int j = 1; j < 3; j++)
		if (bs[j]->lookup_pages_length < max_li) max_li = bs[j]->lookup_pages_length;
	for (uint64_t li = 0; li < max_li; li++)
	{
		uint64_t lword = bs[0]->lookup_pages[li] & bs[1]->lookup_pages[li] & bs[2]->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			bool valid = true;
			for (int j = 0; j < 3; j++)
				if (page_index >= bs[j]->pages_length || !bs[j]->pages[page_index].bits) valid = false;
			if (!valid) continue;
			uint32_t first = 0, last = UINT32_MAX;
			for (int j = 0; j < 3; j++)
			{
				if (bs[j]->pages[page_index].first_set > first) first = bs[j]->pages[page_index].first_set;
				if (bs[j]->pages[page_index].last_set < last) last = bs[j]->pages[page_index].last_set;
			}
			if (first > last || first == UINT32_MAX) continue;
			for (uint32_t w = first; w <= last; w++)
			{
				uint64_t word = bs[0]->pages[page_index].bits[w] & bs[1]->pages[page_index].bits[w] & bs[2]->pages[page_index].bits[w];
				while (word)
				{
					int bit = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs[0]->page_size_ + w) * 64 + (uint64_t)bit;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_3, intersect_3_sparse_70)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs[3] = {&ubench_fixture->bs_70[0], &ubench_fixture->bs_70[1], &ubench_fixture->bs_70[2]};
	uint64_t max_li = bs[0]->lookup_pages_length;
	for (int j = 1; j < 3; j++)
		if (bs[j]->lookup_pages_length < max_li) max_li = bs[j]->lookup_pages_length;
	for (uint64_t li = 0; li < max_li; li++)
	{
		uint64_t lword = bs[0]->lookup_pages[li] & bs[1]->lookup_pages[li] & bs[2]->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			bool valid = true;
			for (int j = 0; j < 3; j++)
				if (page_index >= bs[j]->pages_length || !bs[j]->pages[page_index].bits) valid = false;
			if (!valid) continue;
			uint32_t first = 0, last = UINT32_MAX;
			for (int j = 0; j < 3; j++)
			{
				if (bs[j]->pages[page_index].first_set > first) first = bs[j]->pages[page_index].first_set;
				if (bs[j]->pages[page_index].last_set < last) last = bs[j]->pages[page_index].last_set;
			}
			if (first > last || first == UINT32_MAX) continue;
			for (uint32_t w = first; w <= last; w++)
			{
				uint64_t word = bs[0]->pages[page_index].bits[w] & bs[1]->pages[page_index].bits[w] & bs[2]->pages[page_index].bits[w];
				while (word)
				{
					int bit = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs[0]->page_size_ + w) * 64 + (uint64_t)bit;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_3, intersect_3_flat_30)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT; w++)
	{
		uint64_t word = ubench_fixture->flat_30[0][w] & ubench_fixture->flat_30[1][w] & ubench_fixture->flat_30[2][w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_3, intersect_3_flat_70)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT; w++)
	{
		uint64_t word = ubench_fixture->flat_70[0][w] & ubench_fixture->flat_70[1][w] & ubench_fixture->flat_70[2][w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_3, intersect_3_flat_skip_30)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT; sw++)
	{
		uint64_t skip_word = ubench_fixture->skip_30[0][sw] & ubench_fixture->skip_30[1][sw] & ubench_fixture->skip_30[2][sw];
		if (!skip_word) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT ? base_w + 64 : FLAT_WORDS_INTERSECT;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip_30[0][w] & ubench_fixture->flat_skip_30[1][w] & ubench_fixture->flat_skip_30[2][w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_3, intersect_3_flat_skip_70)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT; sw++)
	{
		uint64_t skip_word = ubench_fixture->skip_70[0][sw] & ubench_fixture->skip_70[1][sw] & ubench_fixture->skip_70[2][sw];
		if (!skip_word) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT ? base_w + 64 : FLAT_WORDS_INTERSECT;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip_70[0][w] & ubench_fixture->flat_skip_70[1][w] & ubench_fixture->flat_skip_70[2][w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// INTERSECTION OF 5 BITSETS
// ============================================================================

struct bitset_intersect_5
{
	struct w_arena arena;
	struct w_sparse_bitset bs_30[5];
	struct w_sparse_bitset bs_70[5];
	uint64_t *flat_30[5];
	uint64_t *flat_70[5];
	uint64_t *flat_skip_30[5];
	uint64_t *flat_skip_70[5];
	uint64_t *skip_30[5];
	uint64_t *skip_70[5];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_intersect_5)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_16M);
	for (int j = 0; j < 5; j++)
	{
		w_sparse_bitset_init(&ubench_fixture->bs_30[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_70[j], &ubench_fixture->arena, 256);
		ubench_fixture->flat_30[j] = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
		ubench_fixture->flat_70[j] = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
		ubench_fixture->flat_skip_30[j] = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
		ubench_fixture->flat_skip_70[j] = calloc(FLAT_WORDS_INTERSECT, sizeof(uint64_t));
		ubench_fixture->skip_30[j] = calloc(SKIP_WORDS_INTERSECT, sizeof(uint64_t));
		ubench_fixture->skip_70[j] = calloc(SKIP_WORDS_INTERSECT, sizeof(uint64_t));
	}
	ubench_fixture->result = malloc(BENCH_INTERSECT_RANGE * sizeof(uint64_t));

	srand(42);
	for (int j = 0; j < 5; j++)
	{
		for (uint64_t i = 0; i < BENCH_INTERSECT_30_COUNT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_30[j], idx);
			FLAT_SET(ubench_fixture->flat_30[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_30[j], ubench_fixture->skip_30[j], idx);
		}
		for (uint64_t i = 0; i < BENCH_INTERSECT_70_COUNT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_70[j], idx);
			FLAT_SET(ubench_fixture->flat_70[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_70[j], ubench_fixture->skip_70[j], idx);
		}
	}
}

UBENCH_F_TEARDOWN(bitset_intersect_5)
{
	for (int j = 0; j < 5; j++)
	{
		w_sparse_bitset_free(&ubench_fixture->bs_30[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_70[j]);
		free(ubench_fixture->flat_30[j]);
		free(ubench_fixture->flat_70[j]);
		free(ubench_fixture->flat_skip_30[j]);
		free(ubench_fixture->flat_skip_70[j]);
		free(ubench_fixture->skip_30[j]);
		free(ubench_fixture->skip_70[j]);
	}
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

UBENCH_F(bitset_intersect_5, intersect_5_sparse_30)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs[5] = {&ubench_fixture->bs_30[0], &ubench_fixture->bs_30[1], &ubench_fixture->bs_30[2], &ubench_fixture->bs_30[3], &ubench_fixture->bs_30[4]};
	uint64_t max_li = bs[0]->lookup_pages_length;
	for (int j = 1; j < 5; j++)
		if (bs[j]->lookup_pages_length < max_li) max_li = bs[j]->lookup_pages_length;
	for (uint64_t li = 0; li < max_li; li++)
	{
		uint64_t lword = bs[0]->lookup_pages[li] & bs[1]->lookup_pages[li] & bs[2]->lookup_pages[li] & bs[3]->lookup_pages[li] & bs[4]->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			bool valid = true;
			for (int j = 0; j < 5; j++)
				if (page_index >= bs[j]->pages_length || !bs[j]->pages[page_index].bits) valid = false;
			if (!valid) continue;
			uint32_t first = 0, last = UINT32_MAX;
			for (int j = 0; j < 5; j++)
			{
				if (bs[j]->pages[page_index].first_set > first) first = bs[j]->pages[page_index].first_set;
				if (bs[j]->pages[page_index].last_set < last) last = bs[j]->pages[page_index].last_set;
			}
			if (first > last || first == UINT32_MAX) continue;
			for (uint32_t w = first; w <= last; w++)
			{
				uint64_t word = bs[0]->pages[page_index].bits[w] & bs[1]->pages[page_index].bits[w] & bs[2]->pages[page_index].bits[w] & bs[3]->pages[page_index].bits[w] & bs[4]->pages[page_index].bits[w];
				while (word)
				{
					int bit = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs[0]->page_size_ + w) * 64 + (uint64_t)bit;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_5, intersect_5_sparse_70)
{
	uint64_t count = 0;
	struct w_sparse_bitset *bs[5] = {&ubench_fixture->bs_70[0], &ubench_fixture->bs_70[1], &ubench_fixture->bs_70[2], &ubench_fixture->bs_70[3], &ubench_fixture->bs_70[4]};
	uint64_t max_li = bs[0]->lookup_pages_length;
	for (int j = 1; j < 5; j++)
		if (bs[j]->lookup_pages_length < max_li) max_li = bs[j]->lookup_pages_length;
	for (uint64_t li = 0; li < max_li; li++)
	{
		uint64_t lword = bs[0]->lookup_pages[li] & bs[1]->lookup_pages[li] & bs[2]->lookup_pages[li] & bs[3]->lookup_pages[li] & bs[4]->lookup_pages[li];
		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;
			bool valid = true;
			for (int j = 0; j < 5; j++)
				if (page_index >= bs[j]->pages_length || !bs[j]->pages[page_index].bits) valid = false;
			if (!valid) continue;
			uint32_t first = 0, last = UINT32_MAX;
			for (int j = 0; j < 5; j++)
			{
				if (bs[j]->pages[page_index].first_set > first) first = bs[j]->pages[page_index].first_set;
				if (bs[j]->pages[page_index].last_set < last) last = bs[j]->pages[page_index].last_set;
			}
			if (first > last || first == UINT32_MAX) continue;
			for (uint32_t w = first; w <= last; w++)
			{
				uint64_t word = bs[0]->pages[page_index].bits[w] & bs[1]->pages[page_index].bits[w] & bs[2]->pages[page_index].bits[w] & bs[3]->pages[page_index].bits[w] & bs[4]->pages[page_index].bits[w];
				while (word)
				{
					int bit = __builtin_ctzll(word);
					word &= word - 1;
					ubench_fixture->result[count++] = (page_index * bs[0]->page_size_ + w) * 64 + (uint64_t)bit;
				}
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_5, intersect_5_flat_30)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT; w++)
	{
		uint64_t word = ubench_fixture->flat_30[0][w] & ubench_fixture->flat_30[1][w] & ubench_fixture->flat_30[2][w] & ubench_fixture->flat_30[3][w] & ubench_fixture->flat_30[4][w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_5, intersect_5_flat_70)
{
	uint64_t count = 0;
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT; w++)
	{
		uint64_t word = ubench_fixture->flat_70[0][w] & ubench_fixture->flat_70[1][w] & ubench_fixture->flat_70[2][w] & ubench_fixture->flat_70[3][w] & ubench_fixture->flat_70[4][w];
		while (word)
		{
			int b = __builtin_ctzll(word);
			word &= word - 1;
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_5, intersect_5_flat_skip_30)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT; sw++)
	{
		uint64_t skip_word = ubench_fixture->skip_30[0][sw] & ubench_fixture->skip_30[1][sw] & ubench_fixture->skip_30[2][sw] & ubench_fixture->skip_30[3][sw] & ubench_fixture->skip_30[4][sw];
		if (!skip_word) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT ? base_w + 64 : FLAT_WORDS_INTERSECT;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip_30[0][w] & ubench_fixture->flat_skip_30[1][w] & ubench_fixture->flat_skip_30[2][w] & ubench_fixture->flat_skip_30[3][w] & ubench_fixture->flat_skip_30[4][w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}

UBENCH_F(bitset_intersect_5, intersect_5_flat_skip_70)
{
	uint64_t count = 0;
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT; sw++)
	{
		uint64_t skip_word = ubench_fixture->skip_70[0][sw] & ubench_fixture->skip_70[1][sw] & ubench_fixture->skip_70[2][sw] & ubench_fixture->skip_70[3][sw] & ubench_fixture->skip_70[4][sw];
		if (!skip_word) continue;
		uint64_t base_w = sw * 64;
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT ? base_w + 64 : FLAT_WORDS_INTERSECT;
		for (uint64_t w = base_w; w < end_w; w++)
		{
			uint64_t word = ubench_fixture->flat_skip_70[0][w] & ubench_fixture->flat_skip_70[1][w] & ubench_fixture->flat_skip_70[2][w] & ubench_fixture->flat_skip_70[3][w] & ubench_fixture->flat_skip_70[4][w];
			while (word)
			{
				int b = __builtin_ctzll(word);
				word &= word - 1;
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b;
			}
		}
	}
	UBENCH_DO_NOTHING(&ubench_fixture->result);
}


// ============================================================================
// 1M INTERSECTION: ULTRA-SPARSE (1 bit, 100 bits, 1k bits in 1M range)
// ============================================================================

// 2-way ultrasparse fixture
struct bitset_intersect_1m_ultrasparse_2
{
	struct w_arena arena;
	struct w_sparse_bitset bs_1[2];
	struct w_sparse_bitset bs_100[2];
	struct w_sparse_bitset bs_1k[2];
	uint64_t *flat_1[2];
	uint64_t *flat_100[2];
	uint64_t *flat_1k[2];
	uint64_t *flat_skip_1[2];
	uint64_t *flat_skip_100[2];
	uint64_t *flat_skip_1k[2];
	uint64_t *skip_1[2];
	uint64_t *skip_100[2];
	uint64_t *skip_1k[2];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_intersect_1m_ultrasparse_2)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_32M);
	for (int j = 0; j < 2; j++)
	{
		w_sparse_bitset_init(&ubench_fixture->bs_1[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_100[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_1k[j], &ubench_fixture->arena, 256);
		ubench_fixture->flat_1[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_100[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_1k[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_1[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_100[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_1k[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_1[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_100[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_1k[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
	}
	ubench_fixture->result = malloc(BENCH_INTERSECT_1M_RANGE * sizeof(uint64_t));

	for (int j = 0; j < 2; j++)
	{
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_ULTRASPARSE_1; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_1[j], idx);
			FLAT_SET(ubench_fixture->flat_1[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_1[j], ubench_fixture->skip_1[j], idx);
		}
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_ULTRASPARSE_100; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_100[j], idx);
			FLAT_SET(ubench_fixture->flat_100[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_100[j], ubench_fixture->skip_100[j], idx);
		}
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_ULTRASPARSE_1K; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_1k[j], idx);
			FLAT_SET(ubench_fixture->flat_1k[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_1k[j], ubench_fixture->skip_1k[j], idx);
		}
	}
}

UBENCH_F_TEARDOWN(bitset_intersect_1m_ultrasparse_2)
{
	for (int j = 0; j < 2; j++)
	{
		w_sparse_bitset_free(&ubench_fixture->bs_1[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_100[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_1k[j]);
		free(ubench_fixture->flat_1[j]);
		free(ubench_fixture->flat_100[j]);
		free(ubench_fixture->flat_1k[j]);
		free(ubench_fixture->flat_skip_1[j]);
		free(ubench_fixture->flat_skip_100[j]);
		free(ubench_fixture->flat_skip_1k[j]);
		free(ubench_fixture->skip_1[j]);
		free(ubench_fixture->skip_100[j]);
		free(ubench_fixture->skip_1k[j]);
	}
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

// 2-way intersection macros for 1M ultrasparse
#define INTERSECT_1M_SPARSE_2(name, bs_arr) \
UBENCH_F(bitset_intersect_1m_ultrasparse_2, name) \
{ \
	uint64_t count = 0; \
	struct w_sparse_bitset *a = &ubench_fixture->bs_arr[0]; \
	struct w_sparse_bitset *b = &ubench_fixture->bs_arr[1]; \
	uint64_t max_li = a->lookup_pages_length < b->lookup_pages_length ? a->lookup_pages_length : b->lookup_pages_length; \
	for (uint64_t li = 0; li < max_li; li++) \
	{ \
		uint64_t lword = a->lookup_pages[li] & b->lookup_pages[li]; \
		while (lword) \
		{ \
			int lbit = __builtin_ctzll(lword); \
			lword &= lword - 1; \
			uint64_t page_index = li * 64 + (uint64_t)lbit; \
			if (page_index >= a->pages_length || page_index >= b->pages_length) continue; \
			struct w_sparse_bitset_page *pa = &a->pages[page_index]; \
			struct w_sparse_bitset_page *pb = &b->pages[page_index]; \
			if (!pa->bits || !pb->bits) continue; \
			uint32_t first = pa->first_set > pb->first_set ? pa->first_set : pb->first_set; \
			uint32_t last = pa->last_set < pb->last_set ? pa->last_set : pb->last_set; \
			if (first > last || first == UINT32_MAX) continue; \
			for (uint32_t w = first; w <= last; w++) \
			{ \
				uint64_t word = pa->bits[w] & pb->bits[w]; \
				while (word) \
				{ \
					int bit = __builtin_ctzll(word); \
					word &= word - 1; \
					ubench_fixture->result[count++] = (page_index * a->page_size_ + w) * 64 + (uint64_t)bit; \
				} \
			} \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_FLAT_2(name, flat_arr) \
UBENCH_F(bitset_intersect_1m_ultrasparse_2, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT_1M; w++) \
	{ \
		uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w]; \
		while (word) \
		{ \
			int b = __builtin_ctzll(word); \
			word &= word - 1; \
			ubench_fixture->result[count++] = w * 64 + (uint64_t)b; \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_FLAT_SKIP_2(name, flat_arr, skip_arr) \
UBENCH_F(bitset_intersect_1m_ultrasparse_2, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT_1M; sw++) \
	{ \
		uint64_t skip_word = ubench_fixture->skip_arr[0][sw] & ubench_fixture->skip_arr[1][sw]; \
		if (!skip_word) continue; \
		uint64_t base_w = sw * 64; \
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT_1M ? base_w + 64 : FLAT_WORDS_INTERSECT_1M; \
		for (uint64_t w = base_w; w < end_w; w++) \
		{ \
			uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w]; \
			while (word) \
			{ \
				int b = __builtin_ctzll(word); \
				word &= word - 1; \
				ubench_fixture->result[count++] = w * 64 + (uint64_t)b; \
			} \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

// 2-way ultrasparse benchmarks
INTERSECT_1M_SPARSE_2(intersect_1m_ultrasparse_1bit_2_sparse, bs_1)
INTERSECT_1M_FLAT_2(intersect_1m_ultrasparse_1bit_2_flat, flat_1)
INTERSECT_1M_FLAT_SKIP_2(intersect_1m_ultrasparse_1bit_2_flat_skip, flat_skip_1, skip_1)

INTERSECT_1M_SPARSE_2(intersect_1m_ultrasparse_100_2_sparse, bs_100)
INTERSECT_1M_FLAT_2(intersect_1m_ultrasparse_100_2_flat, flat_100)
INTERSECT_1M_FLAT_SKIP_2(intersect_1m_ultrasparse_100_2_flat_skip, flat_skip_100, skip_100)

INTERSECT_1M_SPARSE_2(intersect_1m_ultrasparse_1k_2_sparse, bs_1k)
INTERSECT_1M_FLAT_2(intersect_1m_ultrasparse_1k_2_flat, flat_1k)
INTERSECT_1M_FLAT_SKIP_2(intersect_1m_ultrasparse_1k_2_flat_skip, flat_skip_1k, skip_1k)


// 3-way ultrasparse fixture
struct bitset_intersect_1m_ultrasparse_3
{
	struct w_arena arena;
	struct w_sparse_bitset bs_1[3];
	struct w_sparse_bitset bs_100[3];
	struct w_sparse_bitset bs_1k[3];
	uint64_t *flat_1[3];
	uint64_t *flat_100[3];
	uint64_t *flat_1k[3];
	uint64_t *flat_skip_1[3];
	uint64_t *flat_skip_100[3];
	uint64_t *flat_skip_1k[3];
	uint64_t *skip_1[3];
	uint64_t *skip_100[3];
	uint64_t *skip_1k[3];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_intersect_1m_ultrasparse_3)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_32M);
	for (int j = 0; j < 3; j++)
	{
		w_sparse_bitset_init(&ubench_fixture->bs_1[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_100[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_1k[j], &ubench_fixture->arena, 256);
		ubench_fixture->flat_1[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_100[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_1k[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_1[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_100[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_1k[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_1[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_100[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_1k[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
	}
	ubench_fixture->result = malloc(BENCH_INTERSECT_1M_RANGE * sizeof(uint64_t));

	for (int j = 0; j < 3; j++)
	{
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_ULTRASPARSE_1; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_1[j], idx);
			FLAT_SET(ubench_fixture->flat_1[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_1[j], ubench_fixture->skip_1[j], idx);
		}
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_ULTRASPARSE_100; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_100[j], idx);
			FLAT_SET(ubench_fixture->flat_100[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_100[j], ubench_fixture->skip_100[j], idx);
		}
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_ULTRASPARSE_1K; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_1k[j], idx);
			FLAT_SET(ubench_fixture->flat_1k[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_1k[j], ubench_fixture->skip_1k[j], idx);
		}
	}
}

UBENCH_F_TEARDOWN(bitset_intersect_1m_ultrasparse_3)
{
	for (int j = 0; j < 3; j++)
	{
		w_sparse_bitset_free(&ubench_fixture->bs_1[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_100[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_1k[j]);
		free(ubench_fixture->flat_1[j]);
		free(ubench_fixture->flat_100[j]);
		free(ubench_fixture->flat_1k[j]);
		free(ubench_fixture->flat_skip_1[j]);
		free(ubench_fixture->flat_skip_100[j]);
		free(ubench_fixture->flat_skip_1k[j]);
		free(ubench_fixture->skip_1[j]);
		free(ubench_fixture->skip_100[j]);
		free(ubench_fixture->skip_1k[j]);
	}
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

// 3-way intersection macros
#define INTERSECT_1M_SPARSE_3(name, bs_arr) \
UBENCH_F(bitset_intersect_1m_ultrasparse_3, name) \
{ \
	uint64_t count = 0; \
	struct w_sparse_bitset *bs[3] = {&ubench_fixture->bs_arr[0], &ubench_fixture->bs_arr[1], &ubench_fixture->bs_arr[2]}; \
	uint64_t max_li = bs[0]->lookup_pages_length; \
	for (int j = 1; j < 3; j++) if (bs[j]->lookup_pages_length < max_li) max_li = bs[j]->lookup_pages_length; \
	for (uint64_t li = 0; li < max_li; li++) \
	{ \
		uint64_t lword = bs[0]->lookup_pages[li] & bs[1]->lookup_pages[li] & bs[2]->lookup_pages[li]; \
		while (lword) \
		{ \
			int lbit = __builtin_ctzll(lword); \
			lword &= lword - 1; \
			uint64_t page_index = li * 64 + (uint64_t)lbit; \
			bool valid = true; \
			for (int j = 0; j < 3; j++) if (page_index >= bs[j]->pages_length || !bs[j]->pages[page_index].bits) valid = false; \
			if (!valid) continue; \
			uint32_t first = 0, last = UINT32_MAX; \
			for (int j = 0; j < 3; j++) { \
				if (bs[j]->pages[page_index].first_set > first) first = bs[j]->pages[page_index].first_set; \
				if (bs[j]->pages[page_index].last_set < last) last = bs[j]->pages[page_index].last_set; \
			} \
			if (first > last || first == UINT32_MAX) continue; \
			for (uint32_t w = first; w <= last; w++) \
			{ \
				uint64_t word = bs[0]->pages[page_index].bits[w] & bs[1]->pages[page_index].bits[w] & bs[2]->pages[page_index].bits[w]; \
				while (word) { int bit = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = (page_index * bs[0]->page_size_ + w) * 64 + (uint64_t)bit; } \
			} \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_FLAT_3(name, flat_arr) \
UBENCH_F(bitset_intersect_1m_ultrasparse_3, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT_1M; w++) \
	{ \
		uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w] & ubench_fixture->flat_arr[2][w]; \
		while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_FLAT_SKIP_3(name, flat_arr, skip_arr) \
UBENCH_F(bitset_intersect_1m_ultrasparse_3, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT_1M; sw++) \
	{ \
		uint64_t skip_word = ubench_fixture->skip_arr[0][sw] & ubench_fixture->skip_arr[1][sw] & ubench_fixture->skip_arr[2][sw]; \
		if (!skip_word) continue; \
		uint64_t base_w = sw * 64; \
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT_1M ? base_w + 64 : FLAT_WORDS_INTERSECT_1M; \
		for (uint64_t w = base_w; w < end_w; w++) \
		{ \
			uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w] & ubench_fixture->flat_arr[2][w]; \
			while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

// 3-way ultrasparse benchmarks
INTERSECT_1M_SPARSE_3(intersect_1m_ultrasparse_1bit_3_sparse, bs_1)
INTERSECT_1M_FLAT_3(intersect_1m_ultrasparse_1bit_3_flat, flat_1)
INTERSECT_1M_FLAT_SKIP_3(intersect_1m_ultrasparse_1bit_3_flat_skip, flat_skip_1, skip_1)

INTERSECT_1M_SPARSE_3(intersect_1m_ultrasparse_100_3_sparse, bs_100)
INTERSECT_1M_FLAT_3(intersect_1m_ultrasparse_100_3_flat, flat_100)
INTERSECT_1M_FLAT_SKIP_3(intersect_1m_ultrasparse_100_3_flat_skip, flat_skip_100, skip_100)

INTERSECT_1M_SPARSE_3(intersect_1m_ultrasparse_1k_3_sparse, bs_1k)
INTERSECT_1M_FLAT_3(intersect_1m_ultrasparse_1k_3_flat, flat_1k)
INTERSECT_1M_FLAT_SKIP_3(intersect_1m_ultrasparse_1k_3_flat_skip, flat_skip_1k, skip_1k)


// 5-way ultrasparse fixture
struct bitset_intersect_1m_ultrasparse_5
{
	struct w_arena arena;
	struct w_sparse_bitset bs_1[5];
	struct w_sparse_bitset bs_100[5];
	struct w_sparse_bitset bs_1k[5];
	uint64_t *flat_1[5];
	uint64_t *flat_100[5];
	uint64_t *flat_1k[5];
	uint64_t *flat_skip_1[5];
	uint64_t *flat_skip_100[5];
	uint64_t *flat_skip_1k[5];
	uint64_t *skip_1[5];
	uint64_t *skip_100[5];
	uint64_t *skip_1k[5];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_intersect_1m_ultrasparse_5)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_32M);
	for (int j = 0; j < 5; j++)
	{
		w_sparse_bitset_init(&ubench_fixture->bs_1[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_100[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_1k[j], &ubench_fixture->arena, 256);
		ubench_fixture->flat_1[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_100[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_1k[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_1[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_100[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_1k[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_1[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_100[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_1k[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
	}
	ubench_fixture->result = malloc(BENCH_INTERSECT_1M_RANGE * sizeof(uint64_t));

	for (int j = 0; j < 5; j++)
	{
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_ULTRASPARSE_1; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_1[j], idx);
			FLAT_SET(ubench_fixture->flat_1[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_1[j], ubench_fixture->skip_1[j], idx);
		}
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_ULTRASPARSE_100; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_100[j], idx);
			FLAT_SET(ubench_fixture->flat_100[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_100[j], ubench_fixture->skip_100[j], idx);
		}
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_ULTRASPARSE_1K; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_1k[j], idx);
			FLAT_SET(ubench_fixture->flat_1k[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_1k[j], ubench_fixture->skip_1k[j], idx);
		}
	}
}

UBENCH_F_TEARDOWN(bitset_intersect_1m_ultrasparse_5)
{
	for (int j = 0; j < 5; j++)
	{
		w_sparse_bitset_free(&ubench_fixture->bs_1[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_100[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_1k[j]);
		free(ubench_fixture->flat_1[j]);
		free(ubench_fixture->flat_100[j]);
		free(ubench_fixture->flat_1k[j]);
		free(ubench_fixture->flat_skip_1[j]);
		free(ubench_fixture->flat_skip_100[j]);
		free(ubench_fixture->flat_skip_1k[j]);
		free(ubench_fixture->skip_1[j]);
		free(ubench_fixture->skip_100[j]);
		free(ubench_fixture->skip_1k[j]);
	}
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

// 5-way intersection macros
#define INTERSECT_1M_SPARSE_5(name, bs_arr) \
UBENCH_F(bitset_intersect_1m_ultrasparse_5, name) \
{ \
	uint64_t count = 0; \
	struct w_sparse_bitset *bs[5] = {&ubench_fixture->bs_arr[0], &ubench_fixture->bs_arr[1], &ubench_fixture->bs_arr[2], &ubench_fixture->bs_arr[3], &ubench_fixture->bs_arr[4]}; \
	uint64_t max_li = bs[0]->lookup_pages_length; \
	for (int j = 1; j < 5; j++) if (bs[j]->lookup_pages_length < max_li) max_li = bs[j]->lookup_pages_length; \
	for (uint64_t li = 0; li < max_li; li++) \
	{ \
		uint64_t lword = bs[0]->lookup_pages[li] & bs[1]->lookup_pages[li] & bs[2]->lookup_pages[li] & bs[3]->lookup_pages[li] & bs[4]->lookup_pages[li]; \
		while (lword) \
		{ \
			int lbit = __builtin_ctzll(lword); \
			lword &= lword - 1; \
			uint64_t page_index = li * 64 + (uint64_t)lbit; \
			bool valid = true; \
			for (int j = 0; j < 5; j++) if (page_index >= bs[j]->pages_length || !bs[j]->pages[page_index].bits) valid = false; \
			if (!valid) continue; \
			uint32_t first = 0, last = UINT32_MAX; \
			for (int j = 0; j < 5; j++) { \
				if (bs[j]->pages[page_index].first_set > first) first = bs[j]->pages[page_index].first_set; \
				if (bs[j]->pages[page_index].last_set < last) last = bs[j]->pages[page_index].last_set; \
			} \
			if (first > last || first == UINT32_MAX) continue; \
			for (uint32_t w = first; w <= last; w++) \
			{ \
				uint64_t word = bs[0]->pages[page_index].bits[w] & bs[1]->pages[page_index].bits[w] & bs[2]->pages[page_index].bits[w] & bs[3]->pages[page_index].bits[w] & bs[4]->pages[page_index].bits[w]; \
				while (word) { int bit = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = (page_index * bs[0]->page_size_ + w) * 64 + (uint64_t)bit; } \
			} \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_FLAT_5(name, flat_arr) \
UBENCH_F(bitset_intersect_1m_ultrasparse_5, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT_1M; w++) \
	{ \
		uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w] & ubench_fixture->flat_arr[2][w] & ubench_fixture->flat_arr[3][w] & ubench_fixture->flat_arr[4][w]; \
		while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_FLAT_SKIP_5(name, flat_arr, skip_arr) \
UBENCH_F(bitset_intersect_1m_ultrasparse_5, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT_1M; sw++) \
	{ \
		uint64_t skip_word = ubench_fixture->skip_arr[0][sw] & ubench_fixture->skip_arr[1][sw] & ubench_fixture->skip_arr[2][sw] & ubench_fixture->skip_arr[3][sw] & ubench_fixture->skip_arr[4][sw]; \
		if (!skip_word) continue; \
		uint64_t base_w = sw * 64; \
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT_1M ? base_w + 64 : FLAT_WORDS_INTERSECT_1M; \
		for (uint64_t w = base_w; w < end_w; w++) \
		{ \
			uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w] & ubench_fixture->flat_arr[2][w] & ubench_fixture->flat_arr[3][w] & ubench_fixture->flat_arr[4][w]; \
			while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

// 5-way ultrasparse benchmarks
INTERSECT_1M_SPARSE_5(intersect_1m_ultrasparse_1bit_5_sparse, bs_1)
INTERSECT_1M_FLAT_5(intersect_1m_ultrasparse_1bit_5_flat, flat_1)
INTERSECT_1M_FLAT_SKIP_5(intersect_1m_ultrasparse_1bit_5_flat_skip, flat_skip_1, skip_1)

INTERSECT_1M_SPARSE_5(intersect_1m_ultrasparse_100_5_sparse, bs_100)
INTERSECT_1M_FLAT_5(intersect_1m_ultrasparse_100_5_flat, flat_100)
INTERSECT_1M_FLAT_SKIP_5(intersect_1m_ultrasparse_100_5_flat_skip, flat_skip_100, skip_100)

INTERSECT_1M_SPARSE_5(intersect_1m_ultrasparse_1k_5_sparse, bs_1k)
INTERSECT_1M_FLAT_5(intersect_1m_ultrasparse_1k_5_flat, flat_1k)
INTERSECT_1M_FLAT_SKIP_5(intersect_1m_ultrasparse_1k_5_flat_skip, flat_skip_1k, skip_1k)


// ============================================================================
// 1M INTERSECTION: LOW DENSITY (10%, 15%)
// ============================================================================

// 2-way low density fixture
struct bitset_intersect_1m_low_2
{
	struct w_arena arena;
	struct w_sparse_bitset bs_10[2];
	struct w_sparse_bitset bs_15[2];
	uint64_t *flat_10[2];
	uint64_t *flat_15[2];
	uint64_t *flat_skip_10[2];
	uint64_t *flat_skip_15[2];
	uint64_t *skip_10[2];
	uint64_t *skip_15[2];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_intersect_1m_low_2)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_64M);
	for (int j = 0; j < 2; j++)
	{
		w_sparse_bitset_init(&ubench_fixture->bs_10[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_15[j], &ubench_fixture->arena, 256);
		ubench_fixture->flat_10[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_15[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_10[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_15[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_10[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_15[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
	}
	ubench_fixture->result = malloc(BENCH_INTERSECT_1M_RANGE * sizeof(uint64_t));

	for (int j = 0; j < 2; j++)
	{
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_LOW_10PCT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_10[j], idx);
			FLAT_SET(ubench_fixture->flat_10[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_10[j], ubench_fixture->skip_10[j], idx);
		}
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_LOW_15PCT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_15[j], idx);
			FLAT_SET(ubench_fixture->flat_15[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_15[j], ubench_fixture->skip_15[j], idx);
		}
	}
}

UBENCH_F_TEARDOWN(bitset_intersect_1m_low_2)
{
	for (int j = 0; j < 2; j++)
	{
		w_sparse_bitset_free(&ubench_fixture->bs_10[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_15[j]);
		free(ubench_fixture->flat_10[j]);
		free(ubench_fixture->flat_15[j]);
		free(ubench_fixture->flat_skip_10[j]);
		free(ubench_fixture->flat_skip_15[j]);
		free(ubench_fixture->skip_10[j]);
		free(ubench_fixture->skip_15[j]);
	}
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

// Low density 2-way
#define INTERSECT_1M_LOW_SPARSE_2(name, bs_arr) \
UBENCH_F(bitset_intersect_1m_low_2, name) \
{ \
	uint64_t count = 0; \
	struct w_sparse_bitset *a = &ubench_fixture->bs_arr[0]; \
	struct w_sparse_bitset *b = &ubench_fixture->bs_arr[1]; \
	uint64_t max_li = a->lookup_pages_length < b->lookup_pages_length ? a->lookup_pages_length : b->lookup_pages_length; \
	for (uint64_t li = 0; li < max_li; li++) { \
		uint64_t lword = a->lookup_pages[li] & b->lookup_pages[li]; \
		while (lword) { \
			int lbit = __builtin_ctzll(lword); lword &= lword - 1; \
			uint64_t page_index = li * 64 + (uint64_t)lbit; \
			if (page_index >= a->pages_length || page_index >= b->pages_length) continue; \
			struct w_sparse_bitset_page *pa = &a->pages[page_index]; \
			struct w_sparse_bitset_page *pb = &b->pages[page_index]; \
			if (!pa->bits || !pb->bits) continue; \
			uint32_t first = pa->first_set > pb->first_set ? pa->first_set : pb->first_set; \
			uint32_t last = pa->last_set < pb->last_set ? pa->last_set : pb->last_set; \
			if (first > last || first == UINT32_MAX) continue; \
			for (uint32_t w = first; w <= last; w++) { \
				uint64_t word = pa->bits[w] & pb->bits[w]; \
				while (word) { int bit = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = (page_index * a->page_size_ + w) * 64 + (uint64_t)bit; } \
			} \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_LOW_FLAT_2(name, flat_arr) \
UBENCH_F(bitset_intersect_1m_low_2, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT_1M; w++) { \
		uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w]; \
		while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_LOW_FLAT_SKIP_2(name, flat_arr, skip_arr) \
UBENCH_F(bitset_intersect_1m_low_2, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT_1M; sw++) { \
		uint64_t skip_word = ubench_fixture->skip_arr[0][sw] & ubench_fixture->skip_arr[1][sw]; \
		if (!skip_word) continue; \
		uint64_t base_w = sw * 64; \
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT_1M ? base_w + 64 : FLAT_WORDS_INTERSECT_1M; \
		for (uint64_t w = base_w; w < end_w; w++) { \
			uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w]; \
			while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

INTERSECT_1M_LOW_SPARSE_2(intersect_1m_low_10pct_2_sparse, bs_10)
INTERSECT_1M_LOW_FLAT_2(intersect_1m_low_10pct_2_flat, flat_10)
INTERSECT_1M_LOW_FLAT_SKIP_2(intersect_1m_low_10pct_2_flat_skip, flat_skip_10, skip_10)

INTERSECT_1M_LOW_SPARSE_2(intersect_1m_low_15pct_2_sparse, bs_15)
INTERSECT_1M_LOW_FLAT_2(intersect_1m_low_15pct_2_flat, flat_15)
INTERSECT_1M_LOW_FLAT_SKIP_2(intersect_1m_low_15pct_2_flat_skip, flat_skip_15, skip_15)


// 3-way low density fixture
struct bitset_intersect_1m_low_3
{
	struct w_arena arena;
	struct w_sparse_bitset bs_10[3];
	struct w_sparse_bitset bs_15[3];
	uint64_t *flat_10[3];
	uint64_t *flat_15[3];
	uint64_t *flat_skip_10[3];
	uint64_t *flat_skip_15[3];
	uint64_t *skip_10[3];
	uint64_t *skip_15[3];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_intersect_1m_low_3)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_64M);
	for (int j = 0; j < 3; j++)
	{
		w_sparse_bitset_init(&ubench_fixture->bs_10[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_15[j], &ubench_fixture->arena, 256);
		ubench_fixture->flat_10[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_15[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_10[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_15[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_10[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_15[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
	}
	ubench_fixture->result = malloc(BENCH_INTERSECT_1M_RANGE * sizeof(uint64_t));

	for (int j = 0; j < 3; j++)
	{
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_LOW_10PCT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_10[j], idx);
			FLAT_SET(ubench_fixture->flat_10[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_10[j], ubench_fixture->skip_10[j], idx);
		}
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_LOW_15PCT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_15[j], idx);
			FLAT_SET(ubench_fixture->flat_15[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_15[j], ubench_fixture->skip_15[j], idx);
		}
	}
}

UBENCH_F_TEARDOWN(bitset_intersect_1m_low_3)
{
	for (int j = 0; j < 3; j++)
	{
		w_sparse_bitset_free(&ubench_fixture->bs_10[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_15[j]);
		free(ubench_fixture->flat_10[j]);
		free(ubench_fixture->flat_15[j]);
		free(ubench_fixture->flat_skip_10[j]);
		free(ubench_fixture->flat_skip_15[j]);
		free(ubench_fixture->skip_10[j]);
		free(ubench_fixture->skip_15[j]);
	}
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

// Low density 3-way
#define INTERSECT_1M_LOW_SPARSE_3(name, bs_arr) \
UBENCH_F(bitset_intersect_1m_low_3, name) \
{ \
	uint64_t count = 0; \
	struct w_sparse_bitset *bs[3] = {&ubench_fixture->bs_arr[0], &ubench_fixture->bs_arr[1], &ubench_fixture->bs_arr[2]}; \
	uint64_t max_li = bs[0]->lookup_pages_length; \
	for (int j = 1; j < 3; j++) if (bs[j]->lookup_pages_length < max_li) max_li = bs[j]->lookup_pages_length; \
	for (uint64_t li = 0; li < max_li; li++) { \
		uint64_t lword = bs[0]->lookup_pages[li] & bs[1]->lookup_pages[li] & bs[2]->lookup_pages[li]; \
		while (lword) { \
			int lbit = __builtin_ctzll(lword); lword &= lword - 1; \
			uint64_t page_index = li * 64 + (uint64_t)lbit; \
			bool valid = true; \
			for (int j = 0; j < 3; j++) if (page_index >= bs[j]->pages_length || !bs[j]->pages[page_index].bits) valid = false; \
			if (!valid) continue; \
			uint32_t first = 0, last = UINT32_MAX; \
			for (int j = 0; j < 3; j++) { if (bs[j]->pages[page_index].first_set > first) first = bs[j]->pages[page_index].first_set; if (bs[j]->pages[page_index].last_set < last) last = bs[j]->pages[page_index].last_set; } \
			if (first > last || first == UINT32_MAX) continue; \
			for (uint32_t w = first; w <= last; w++) { \
				uint64_t word = bs[0]->pages[page_index].bits[w] & bs[1]->pages[page_index].bits[w] & bs[2]->pages[page_index].bits[w]; \
				while (word) { int bit = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = (page_index * bs[0]->page_size_ + w) * 64 + (uint64_t)bit; } \
			} \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_LOW_FLAT_3(name, flat_arr) \
UBENCH_F(bitset_intersect_1m_low_3, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT_1M; w++) { \
		uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w] & ubench_fixture->flat_arr[2][w]; \
		while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_LOW_FLAT_SKIP_3(name, flat_arr, skip_arr) \
UBENCH_F(bitset_intersect_1m_low_3, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT_1M; sw++) { \
		uint64_t skip_word = ubench_fixture->skip_arr[0][sw] & ubench_fixture->skip_arr[1][sw] & ubench_fixture->skip_arr[2][sw]; \
		if (!skip_word) continue; \
		uint64_t base_w = sw * 64; \
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT_1M ? base_w + 64 : FLAT_WORDS_INTERSECT_1M; \
		for (uint64_t w = base_w; w < end_w; w++) { \
			uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w] & ubench_fixture->flat_arr[2][w]; \
			while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

INTERSECT_1M_LOW_SPARSE_3(intersect_1m_low_10pct_3_sparse, bs_10)
INTERSECT_1M_LOW_FLAT_3(intersect_1m_low_10pct_3_flat, flat_10)
INTERSECT_1M_LOW_FLAT_SKIP_3(intersect_1m_low_10pct_3_flat_skip, flat_skip_10, skip_10)

INTERSECT_1M_LOW_SPARSE_3(intersect_1m_low_15pct_3_sparse, bs_15)
INTERSECT_1M_LOW_FLAT_3(intersect_1m_low_15pct_3_flat, flat_15)
INTERSECT_1M_LOW_FLAT_SKIP_3(intersect_1m_low_15pct_3_flat_skip, flat_skip_15, skip_15)


// 5-way low density fixture
struct bitset_intersect_1m_low_5
{
	struct w_arena arena;
	struct w_sparse_bitset bs_10[5];
	struct w_sparse_bitset bs_15[5];
	uint64_t *flat_10[5];
	uint64_t *flat_15[5];
	uint64_t *flat_skip_10[5];
	uint64_t *flat_skip_15[5];
	uint64_t *skip_10[5];
	uint64_t *skip_15[5];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_intersect_1m_low_5)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_64M);
	for (int j = 0; j < 5; j++)
	{
		w_sparse_bitset_init(&ubench_fixture->bs_10[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_15[j], &ubench_fixture->arena, 256);
		ubench_fixture->flat_10[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_15[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_10[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_15[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_10[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_15[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
	}
	ubench_fixture->result = malloc(BENCH_INTERSECT_1M_RANGE * sizeof(uint64_t));

	for (int j = 0; j < 5; j++)
	{
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_LOW_10PCT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_10[j], idx);
			FLAT_SET(ubench_fixture->flat_10[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_10[j], ubench_fixture->skip_10[j], idx);
		}
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_LOW_15PCT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_15[j], idx);
			FLAT_SET(ubench_fixture->flat_15[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_15[j], ubench_fixture->skip_15[j], idx);
		}
	}
}

UBENCH_F_TEARDOWN(bitset_intersect_1m_low_5)
{
	for (int j = 0; j < 5; j++)
	{
		w_sparse_bitset_free(&ubench_fixture->bs_10[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_15[j]);
		free(ubench_fixture->flat_10[j]);
		free(ubench_fixture->flat_15[j]);
		free(ubench_fixture->flat_skip_10[j]);
		free(ubench_fixture->flat_skip_15[j]);
		free(ubench_fixture->skip_10[j]);
		free(ubench_fixture->skip_15[j]);
	}
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

// Low density 5-way
#define INTERSECT_1M_LOW_SPARSE_5(name, bs_arr) \
UBENCH_F(bitset_intersect_1m_low_5, name) \
{ \
	uint64_t count = 0; \
	struct w_sparse_bitset *bs[5] = {&ubench_fixture->bs_arr[0], &ubench_fixture->bs_arr[1], &ubench_fixture->bs_arr[2], &ubench_fixture->bs_arr[3], &ubench_fixture->bs_arr[4]}; \
	uint64_t max_li = bs[0]->lookup_pages_length; \
	for (int j = 1; j < 5; j++) if (bs[j]->lookup_pages_length < max_li) max_li = bs[j]->lookup_pages_length; \
	for (uint64_t li = 0; li < max_li; li++) { \
		uint64_t lword = bs[0]->lookup_pages[li] & bs[1]->lookup_pages[li] & bs[2]->lookup_pages[li] & bs[3]->lookup_pages[li] & bs[4]->lookup_pages[li]; \
		while (lword) { \
			int lbit = __builtin_ctzll(lword); lword &= lword - 1; \
			uint64_t page_index = li * 64 + (uint64_t)lbit; \
			bool valid = true; \
			for (int j = 0; j < 5; j++) if (page_index >= bs[j]->pages_length || !bs[j]->pages[page_index].bits) valid = false; \
			if (!valid) continue; \
			uint32_t first = 0, last = UINT32_MAX; \
			for (int j = 0; j < 5; j++) { if (bs[j]->pages[page_index].first_set > first) first = bs[j]->pages[page_index].first_set; if (bs[j]->pages[page_index].last_set < last) last = bs[j]->pages[page_index].last_set; } \
			if (first > last || first == UINT32_MAX) continue; \
			for (uint32_t w = first; w <= last; w++) { \
				uint64_t word = bs[0]->pages[page_index].bits[w] & bs[1]->pages[page_index].bits[w] & bs[2]->pages[page_index].bits[w] & bs[3]->pages[page_index].bits[w] & bs[4]->pages[page_index].bits[w]; \
				while (word) { int bit = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = (page_index * bs[0]->page_size_ + w) * 64 + (uint64_t)bit; } \
			} \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_LOW_FLAT_5(name, flat_arr) \
UBENCH_F(bitset_intersect_1m_low_5, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT_1M; w++) { \
		uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w] & ubench_fixture->flat_arr[2][w] & ubench_fixture->flat_arr[3][w] & ubench_fixture->flat_arr[4][w]; \
		while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_LOW_FLAT_SKIP_5(name, flat_arr, skip_arr) \
UBENCH_F(bitset_intersect_1m_low_5, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT_1M; sw++) { \
		uint64_t skip_word = ubench_fixture->skip_arr[0][sw] & ubench_fixture->skip_arr[1][sw] & ubench_fixture->skip_arr[2][sw] & ubench_fixture->skip_arr[3][sw] & ubench_fixture->skip_arr[4][sw]; \
		if (!skip_word) continue; \
		uint64_t base_w = sw * 64; \
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT_1M ? base_w + 64 : FLAT_WORDS_INTERSECT_1M; \
		for (uint64_t w = base_w; w < end_w; w++) { \
			uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w] & ubench_fixture->flat_arr[2][w] & ubench_fixture->flat_arr[3][w] & ubench_fixture->flat_arr[4][w]; \
			while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

INTERSECT_1M_LOW_SPARSE_5(intersect_1m_low_10pct_5_sparse, bs_10)
INTERSECT_1M_LOW_FLAT_5(intersect_1m_low_10pct_5_flat, flat_10)
INTERSECT_1M_LOW_FLAT_SKIP_5(intersect_1m_low_10pct_5_flat_skip, flat_skip_10, skip_10)

INTERSECT_1M_LOW_SPARSE_5(intersect_1m_low_15pct_5_sparse, bs_15)
INTERSECT_1M_LOW_FLAT_5(intersect_1m_low_15pct_5_flat, flat_15)
INTERSECT_1M_LOW_FLAT_SKIP_5(intersect_1m_low_15pct_5_flat_skip, flat_skip_15, skip_15)


// ============================================================================
// 1M INTERSECTION: HIGH DENSITY (70%, 90%)
// ============================================================================

// 2-way high density fixture
struct bitset_intersect_1m_high_2
{
	struct w_arena arena;
	struct w_sparse_bitset bs_70[2];
	struct w_sparse_bitset bs_90[2];
	uint64_t *flat_70[2];
	uint64_t *flat_90[2];
	uint64_t *flat_skip_70[2];
	uint64_t *flat_skip_90[2];
	uint64_t *skip_70[2];
	uint64_t *skip_90[2];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_intersect_1m_high_2)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_128M);
	for (int j = 0; j < 2; j++)
	{
		w_sparse_bitset_init(&ubench_fixture->bs_70[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_90[j], &ubench_fixture->arena, 256);
		ubench_fixture->flat_70[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_90[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_70[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_90[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_70[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_90[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
	}
	ubench_fixture->result = malloc(BENCH_INTERSECT_1M_RANGE * sizeof(uint64_t));

	for (int j = 0; j < 2; j++)
	{
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_HIGH_70PCT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_70[j], idx);
			FLAT_SET(ubench_fixture->flat_70[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_70[j], ubench_fixture->skip_70[j], idx);
		}
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_HIGH_90PCT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_90[j], idx);
			FLAT_SET(ubench_fixture->flat_90[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_90[j], ubench_fixture->skip_90[j], idx);
		}
	}
}

UBENCH_F_TEARDOWN(bitset_intersect_1m_high_2)
{
	for (int j = 0; j < 2; j++)
	{
		w_sparse_bitset_free(&ubench_fixture->bs_70[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_90[j]);
		free(ubench_fixture->flat_70[j]);
		free(ubench_fixture->flat_90[j]);
		free(ubench_fixture->flat_skip_70[j]);
		free(ubench_fixture->flat_skip_90[j]);
		free(ubench_fixture->skip_70[j]);
		free(ubench_fixture->skip_90[j]);
	}
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

// High density 2-way
#define INTERSECT_1M_HIGH_SPARSE_2(name, bs_arr) \
UBENCH_F(bitset_intersect_1m_high_2, name) \
{ \
	uint64_t count = 0; \
	struct w_sparse_bitset *a = &ubench_fixture->bs_arr[0]; \
	struct w_sparse_bitset *b = &ubench_fixture->bs_arr[1]; \
	uint64_t max_li = a->lookup_pages_length < b->lookup_pages_length ? a->lookup_pages_length : b->lookup_pages_length; \
	for (uint64_t li = 0; li < max_li; li++) { \
		uint64_t lword = a->lookup_pages[li] & b->lookup_pages[li]; \
		while (lword) { \
			int lbit = __builtin_ctzll(lword); lword &= lword - 1; \
			uint64_t page_index = li * 64 + (uint64_t)lbit; \
			if (page_index >= a->pages_length || page_index >= b->pages_length) continue; \
			struct w_sparse_bitset_page *pa = &a->pages[page_index]; \
			struct w_sparse_bitset_page *pb = &b->pages[page_index]; \
			if (!pa->bits || !pb->bits) continue; \
			uint32_t first = pa->first_set > pb->first_set ? pa->first_set : pb->first_set; \
			uint32_t last = pa->last_set < pb->last_set ? pa->last_set : pb->last_set; \
			if (first > last || first == UINT32_MAX) continue; \
			for (uint32_t w = first; w <= last; w++) { \
				uint64_t word = pa->bits[w] & pb->bits[w]; \
				while (word) { int bit = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = (page_index * a->page_size_ + w) * 64 + (uint64_t)bit; } \
			} \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_HIGH_FLAT_2(name, flat_arr) \
UBENCH_F(bitset_intersect_1m_high_2, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT_1M; w++) { \
		uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w]; \
		while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_HIGH_FLAT_SKIP_2(name, flat_arr, skip_arr) \
UBENCH_F(bitset_intersect_1m_high_2, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT_1M; sw++) { \
		uint64_t skip_word = ubench_fixture->skip_arr[0][sw] & ubench_fixture->skip_arr[1][sw]; \
		if (!skip_word) continue; \
		uint64_t base_w = sw * 64; \
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT_1M ? base_w + 64 : FLAT_WORDS_INTERSECT_1M; \
		for (uint64_t w = base_w; w < end_w; w++) { \
			uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w]; \
			while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

INTERSECT_1M_HIGH_SPARSE_2(intersect_1m_high_70pct_2_sparse, bs_70)
INTERSECT_1M_HIGH_FLAT_2(intersect_1m_high_70pct_2_flat, flat_70)
INTERSECT_1M_HIGH_FLAT_SKIP_2(intersect_1m_high_70pct_2_flat_skip, flat_skip_70, skip_70)

INTERSECT_1M_HIGH_SPARSE_2(intersect_1m_high_90pct_2_sparse, bs_90)
INTERSECT_1M_HIGH_FLAT_2(intersect_1m_high_90pct_2_flat, flat_90)
INTERSECT_1M_HIGH_FLAT_SKIP_2(intersect_1m_high_90pct_2_flat_skip, flat_skip_90, skip_90)


// 3-way high density fixture
struct bitset_intersect_1m_high_3
{
	struct w_arena arena;
	struct w_sparse_bitset bs_70[3];
	struct w_sparse_bitset bs_90[3];
	uint64_t *flat_70[3];
	uint64_t *flat_90[3];
	uint64_t *flat_skip_70[3];
	uint64_t *flat_skip_90[3];
	uint64_t *skip_70[3];
	uint64_t *skip_90[3];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_intersect_1m_high_3)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_128M);
	for (int j = 0; j < 3; j++)
	{
		w_sparse_bitset_init(&ubench_fixture->bs_70[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_90[j], &ubench_fixture->arena, 256);
		ubench_fixture->flat_70[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_90[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_70[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_90[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_70[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_90[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
	}
	ubench_fixture->result = malloc(BENCH_INTERSECT_1M_RANGE * sizeof(uint64_t));

	for (int j = 0; j < 3; j++)
	{
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_HIGH_70PCT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_70[j], idx);
			FLAT_SET(ubench_fixture->flat_70[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_70[j], ubench_fixture->skip_70[j], idx);
		}
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_HIGH_90PCT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_90[j], idx);
			FLAT_SET(ubench_fixture->flat_90[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_90[j], ubench_fixture->skip_90[j], idx);
		}
	}
}

UBENCH_F_TEARDOWN(bitset_intersect_1m_high_3)
{
	for (int j = 0; j < 3; j++)
	{
		w_sparse_bitset_free(&ubench_fixture->bs_70[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_90[j]);
		free(ubench_fixture->flat_70[j]);
		free(ubench_fixture->flat_90[j]);
		free(ubench_fixture->flat_skip_70[j]);
		free(ubench_fixture->flat_skip_90[j]);
		free(ubench_fixture->skip_70[j]);
		free(ubench_fixture->skip_90[j]);
	}
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

// High density 3-way
#define INTERSECT_1M_HIGH_SPARSE_3(name, bs_arr) \
UBENCH_F(bitset_intersect_1m_high_3, name) \
{ \
	uint64_t count = 0; \
	struct w_sparse_bitset *bs[3] = {&ubench_fixture->bs_arr[0], &ubench_fixture->bs_arr[1], &ubench_fixture->bs_arr[2]}; \
	uint64_t max_li = bs[0]->lookup_pages_length; \
	for (int j = 1; j < 3; j++) if (bs[j]->lookup_pages_length < max_li) max_li = bs[j]->lookup_pages_length; \
	for (uint64_t li = 0; li < max_li; li++) { \
		uint64_t lword = bs[0]->lookup_pages[li] & bs[1]->lookup_pages[li] & bs[2]->lookup_pages[li]; \
		while (lword) { \
			int lbit = __builtin_ctzll(lword); lword &= lword - 1; \
			uint64_t page_index = li * 64 + (uint64_t)lbit; \
			bool valid = true; \
			for (int j = 0; j < 3; j++) if (page_index >= bs[j]->pages_length || !bs[j]->pages[page_index].bits) valid = false; \
			if (!valid) continue; \
			uint32_t first = 0, last = UINT32_MAX; \
			for (int j = 0; j < 3; j++) { if (bs[j]->pages[page_index].first_set > first) first = bs[j]->pages[page_index].first_set; if (bs[j]->pages[page_index].last_set < last) last = bs[j]->pages[page_index].last_set; } \
			if (first > last || first == UINT32_MAX) continue; \
			for (uint32_t w = first; w <= last; w++) { \
				uint64_t word = bs[0]->pages[page_index].bits[w] & bs[1]->pages[page_index].bits[w] & bs[2]->pages[page_index].bits[w]; \
				while (word) { int bit = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = (page_index * bs[0]->page_size_ + w) * 64 + (uint64_t)bit; } \
			} \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_HIGH_FLAT_3(name, flat_arr) \
UBENCH_F(bitset_intersect_1m_high_3, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT_1M; w++) { \
		uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w] & ubench_fixture->flat_arr[2][w]; \
		while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_HIGH_FLAT_SKIP_3(name, flat_arr, skip_arr) \
UBENCH_F(bitset_intersect_1m_high_3, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT_1M; sw++) { \
		uint64_t skip_word = ubench_fixture->skip_arr[0][sw] & ubench_fixture->skip_arr[1][sw] & ubench_fixture->skip_arr[2][sw]; \
		if (!skip_word) continue; \
		uint64_t base_w = sw * 64; \
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT_1M ? base_w + 64 : FLAT_WORDS_INTERSECT_1M; \
		for (uint64_t w = base_w; w < end_w; w++) { \
			uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w] & ubench_fixture->flat_arr[2][w]; \
			while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

INTERSECT_1M_HIGH_SPARSE_3(intersect_1m_high_70pct_3_sparse, bs_70)
INTERSECT_1M_HIGH_FLAT_3(intersect_1m_high_70pct_3_flat, flat_70)
INTERSECT_1M_HIGH_FLAT_SKIP_3(intersect_1m_high_70pct_3_flat_skip, flat_skip_70, skip_70)

INTERSECT_1M_HIGH_SPARSE_3(intersect_1m_high_90pct_3_sparse, bs_90)
INTERSECT_1M_HIGH_FLAT_3(intersect_1m_high_90pct_3_flat, flat_90)
INTERSECT_1M_HIGH_FLAT_SKIP_3(intersect_1m_high_90pct_3_flat_skip, flat_skip_90, skip_90)


// 5-way high density fixture
struct bitset_intersect_1m_high_5
{
	struct w_arena arena;
	struct w_sparse_bitset bs_70[5];
	struct w_sparse_bitset bs_90[5];
	uint64_t *flat_70[5];
	uint64_t *flat_90[5];
	uint64_t *flat_skip_70[5];
	uint64_t *flat_skip_90[5];
	uint64_t *skip_70[5];
	uint64_t *skip_90[5];
	uint64_t *result;
};

UBENCH_F_SETUP(bitset_intersect_1m_high_5)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_128M);
	for (int j = 0; j < 5; j++)
	{
		w_sparse_bitset_init(&ubench_fixture->bs_70[j], &ubench_fixture->arena, 256);
		w_sparse_bitset_init(&ubench_fixture->bs_90[j], &ubench_fixture->arena, 256);
		ubench_fixture->flat_70[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_90[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_70[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->flat_skip_90[j] = calloc(FLAT_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_70[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
		ubench_fixture->skip_90[j] = calloc(SKIP_WORDS_INTERSECT_1M, sizeof(uint64_t));
	}
	ubench_fixture->result = malloc(BENCH_INTERSECT_1M_RANGE * sizeof(uint64_t));

	for (int j = 0; j < 5; j++)
	{
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_HIGH_70PCT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_70[j], idx);
			FLAT_SET(ubench_fixture->flat_70[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_70[j], ubench_fixture->skip_70[j], idx);
		}
		srand(42);
		for (uint64_t i = 0; i < BENCH_INTERSECT_1M_HIGH_90PCT; i++)
		{
			uint64_t idx = (uint64_t)rand() % BENCH_INTERSECT_1M_RANGE;
			w_sparse_bitset_set(&ubench_fixture->bs_90[j], idx);
			FLAT_SET(ubench_fixture->flat_90[j], idx);
			FLAT_SKIP_SET(ubench_fixture->flat_skip_90[j], ubench_fixture->skip_90[j], idx);
		}
	}
}

UBENCH_F_TEARDOWN(bitset_intersect_1m_high_5)
{
	for (int j = 0; j < 5; j++)
	{
		w_sparse_bitset_free(&ubench_fixture->bs_70[j]);
		w_sparse_bitset_free(&ubench_fixture->bs_90[j]);
		free(ubench_fixture->flat_70[j]);
		free(ubench_fixture->flat_90[j]);
		free(ubench_fixture->flat_skip_70[j]);
		free(ubench_fixture->flat_skip_90[j]);
		free(ubench_fixture->skip_70[j]);
		free(ubench_fixture->skip_90[j]);
	}
	w_arena_free(&ubench_fixture->arena);
	free(ubench_fixture->result);
}

// High density 5-way
#define INTERSECT_1M_HIGH_SPARSE_5(name, bs_arr) \
UBENCH_F(bitset_intersect_1m_high_5, name) \
{ \
	uint64_t count = 0; \
	struct w_sparse_bitset *bs[5] = {&ubench_fixture->bs_arr[0], &ubench_fixture->bs_arr[1], &ubench_fixture->bs_arr[2], &ubench_fixture->bs_arr[3], &ubench_fixture->bs_arr[4]}; \
	uint64_t max_li = bs[0]->lookup_pages_length; \
	for (int j = 1; j < 5; j++) if (bs[j]->lookup_pages_length < max_li) max_li = bs[j]->lookup_pages_length; \
	for (uint64_t li = 0; li < max_li; li++) { \
		uint64_t lword = bs[0]->lookup_pages[li] & bs[1]->lookup_pages[li] & bs[2]->lookup_pages[li] & bs[3]->lookup_pages[li] & bs[4]->lookup_pages[li]; \
		while (lword) { \
			int lbit = __builtin_ctzll(lword); lword &= lword - 1; \
			uint64_t page_index = li * 64 + (uint64_t)lbit; \
			bool valid = true; \
			for (int j = 0; j < 5; j++) if (page_index >= bs[j]->pages_length || !bs[j]->pages[page_index].bits) valid = false; \
			if (!valid) continue; \
			uint32_t first = 0, last = UINT32_MAX; \
			for (int j = 0; j < 5; j++) { if (bs[j]->pages[page_index].first_set > first) first = bs[j]->pages[page_index].first_set; if (bs[j]->pages[page_index].last_set < last) last = bs[j]->pages[page_index].last_set; } \
			if (first > last || first == UINT32_MAX) continue; \
			for (uint32_t w = first; w <= last; w++) { \
				uint64_t word = bs[0]->pages[page_index].bits[w] & bs[1]->pages[page_index].bits[w] & bs[2]->pages[page_index].bits[w] & bs[3]->pages[page_index].bits[w] & bs[4]->pages[page_index].bits[w]; \
				while (word) { int bit = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = (page_index * bs[0]->page_size_ + w) * 64 + (uint64_t)bit; } \
			} \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_HIGH_FLAT_5(name, flat_arr) \
UBENCH_F(bitset_intersect_1m_high_5, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t w = 0; w < FLAT_WORDS_INTERSECT_1M; w++) { \
		uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w] & ubench_fixture->flat_arr[2][w] & ubench_fixture->flat_arr[3][w] & ubench_fixture->flat_arr[4][w]; \
		while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

#define INTERSECT_1M_HIGH_FLAT_SKIP_5(name, flat_arr, skip_arr) \
UBENCH_F(bitset_intersect_1m_high_5, name) \
{ \
	uint64_t count = 0; \
	for (uint64_t sw = 0; sw < SKIP_WORDS_INTERSECT_1M; sw++) { \
		uint64_t skip_word = ubench_fixture->skip_arr[0][sw] & ubench_fixture->skip_arr[1][sw] & ubench_fixture->skip_arr[2][sw] & ubench_fixture->skip_arr[3][sw] & ubench_fixture->skip_arr[4][sw]; \
		if (!skip_word) continue; \
		uint64_t base_w = sw * 64; \
		uint64_t end_w = base_w + 64 < FLAT_WORDS_INTERSECT_1M ? base_w + 64 : FLAT_WORDS_INTERSECT_1M; \
		for (uint64_t w = base_w; w < end_w; w++) { \
			uint64_t word = ubench_fixture->flat_arr[0][w] & ubench_fixture->flat_arr[1][w] & ubench_fixture->flat_arr[2][w] & ubench_fixture->flat_arr[3][w] & ubench_fixture->flat_arr[4][w]; \
			while (word) { int b = __builtin_ctzll(word); word &= word - 1; ubench_fixture->result[count++] = w * 64 + (uint64_t)b; } \
		} \
	} \
	UBENCH_DO_NOTHING(&ubench_fixture->result); \
}

INTERSECT_1M_HIGH_SPARSE_5(intersect_1m_high_70pct_5_sparse, bs_70)
INTERSECT_1M_HIGH_FLAT_5(intersect_1m_high_70pct_5_flat, flat_70)
INTERSECT_1M_HIGH_FLAT_SKIP_5(intersect_1m_high_70pct_5_flat_skip, flat_skip_70, skip_70)

INTERSECT_1M_HIGH_SPARSE_5(intersect_1m_high_90pct_5_sparse, bs_90)
INTERSECT_1M_HIGH_FLAT_5(intersect_1m_high_90pct_5_flat, flat_90)
INTERSECT_1M_HIGH_FLAT_SKIP_5(intersect_1m_high_90pct_5_flat_skip, flat_skip_90, skip_90)


UBENCH_MAIN();
