/**
 * @author      : ElGatoPanzon
 * @file        : benchmark_whisker_arena
 * @created     : 2026-02-27
 * @description : benchmarks for arena allocator vs system malloc
 */

#include "ubench.h"
#include "whisker_arena.h"

#include <stdlib.h>

#define ALLOC_COUNT 1000
#define ALLOC_SIZE_SMALL 64
#define ALLOC_SIZE_MEDIUM 256
#define BLOCK_SIZE_DEFAULT 4096
#define BLOCK_SIZE_SMALL 512


// throughput: many small allocations with arena
struct bench_arena_throughput { struct w_arena arena; };
UBENCH_F_SETUP(bench_arena_throughput) { w_arena_init(&ubench_fixture->arena, BLOCK_SIZE_DEFAULT); }
UBENCH_F_TEARDOWN(bench_arena_throughput) { w_arena_free(&ubench_fixture->arena); }

UBENCH_F(bench_arena_throughput, arena_1000_allocs_64b)
{
	for (int i = 0; i < ALLOC_COUNT; i++)
	{
		void *p = w_arena_malloc(&ubench_fixture->arena, ALLOC_SIZE_SMALL);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_arena_throughput, malloc_1000_allocs_64b)
{
	void *ptrs[ALLOC_COUNT];
	for (int i = 0; i < ALLOC_COUNT; i++)
	{
		ptrs[i] = malloc(ALLOC_SIZE_SMALL);
		UBENCH_DO_NOTHING(ptrs[i]);
	}
	for (int i = 0; i < ALLOC_COUNT; i++)
	{
		free(ptrs[i]);
	}
}


// arena vs malloc: single allocation comparison
struct bench_arena_single { struct w_arena arena; void *ptr; };
UBENCH_F_SETUP(bench_arena_single) { w_arena_init(&ubench_fixture->arena, BLOCK_SIZE_DEFAULT); ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_arena_single) { w_arena_free(&ubench_fixture->arena); free(ubench_fixture->ptr); }

UBENCH_F(bench_arena_single, arena_64b)
{
	void *p = w_arena_malloc(&ubench_fixture->arena, ALLOC_SIZE_SMALL);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_arena_single, malloc_64b)
{
	ubench_fixture->ptr = malloc(ALLOC_SIZE_SMALL);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_arena_single, arena_256b)
{
	void *p = w_arena_malloc(&ubench_fixture->arena, ALLOC_SIZE_MEDIUM);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_arena_single, malloc_256b)
{
	ubench_fixture->ptr = malloc(ALLOC_SIZE_MEDIUM);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// calloc comparison: arena calloc vs system calloc
struct bench_arena_calloc { struct w_arena arena; void *ptr; };
UBENCH_F_SETUP(bench_arena_calloc) { w_arena_init(&ubench_fixture->arena, BLOCK_SIZE_DEFAULT); ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_arena_calloc) { w_arena_free(&ubench_fixture->arena); free(ubench_fixture->ptr); }

UBENCH_F(bench_arena_calloc, arena_calloc_64b)
{
	void *p = w_arena_calloc(&ubench_fixture->arena, ALLOC_SIZE_SMALL);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_arena_calloc, calloc_64b)
{
	ubench_fixture->ptr = calloc(1, ALLOC_SIZE_SMALL);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_arena_calloc, arena_calloc_256b)
{
	void *p = w_arena_calloc(&ubench_fixture->arena, ALLOC_SIZE_MEDIUM);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_arena_calloc, calloc_256b)
{
	ubench_fixture->ptr = calloc(1, ALLOC_SIZE_MEDIUM);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// clear/reuse cycle performance
struct bench_arena_clear { struct w_arena arena; };
UBENCH_F_SETUP(bench_arena_clear) { w_arena_init(&ubench_fixture->arena, BLOCK_SIZE_DEFAULT); }
UBENCH_F_TEARDOWN(bench_arena_clear) { w_arena_free(&ubench_fixture->arena); }

UBENCH_F(bench_arena_clear, alloc_clear_cycle)
{
	// allocate enough to fill one block
	for (int i = 0; i < 64; i++)
	{
		void *p = w_arena_malloc(&ubench_fixture->arena, ALLOC_SIZE_SMALL);
		UBENCH_DO_NOTHING(p);
	}
	w_arena_clear(&ubench_fixture->arena);
}

UBENCH_F(bench_arena_clear, alloc_clear_cycle_multiblock)
{
	// allocate enough to span multiple blocks
	for (int i = 0; i < 256; i++)
	{
		void *p = w_arena_malloc(&ubench_fixture->arena, ALLOC_SIZE_SMALL);
		UBENCH_DO_NOTHING(p);
	}
	w_arena_clear(&ubench_fixture->arena);
}


// block expansion overhead: measure cost when arena needs new blocks
struct bench_arena_expansion { struct w_arena arena; };
UBENCH_F_SETUP(bench_arena_expansion) { w_arena_init(&ubench_fixture->arena, BLOCK_SIZE_SMALL); }
UBENCH_F_TEARDOWN(bench_arena_expansion) { w_arena_free(&ubench_fixture->arena); }

UBENCH_F(bench_arena_expansion, within_block)
{
	// allocate within single block (512 bytes / 64 = 8 allocs)
	for (int i = 0; i < 8; i++)
	{
		void *p = w_arena_malloc(&ubench_fixture->arena, ALLOC_SIZE_SMALL);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_arena_expansion, force_expansion)
{
	// allocate across block boundaries (512 bytes / 64 = 8, so 16 forces expansion)
	for (int i = 0; i < 16; i++)
	{
		void *p = w_arena_malloc(&ubench_fixture->arena, ALLOC_SIZE_SMALL);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_arena_expansion, force_multi_expansion)
{
	// allocate across multiple block boundaries
	for (int i = 0; i < 64; i++)
	{
		void *p = w_arena_malloc(&ubench_fixture->arena, ALLOC_SIZE_SMALL);
		UBENCH_DO_NOTHING(p);
	}
}


// sequential allocation pattern: simulates typical usage
struct bench_arena_sequential { struct w_arena arena; };
UBENCH_F_SETUP(bench_arena_sequential) { w_arena_init(&ubench_fixture->arena, BLOCK_SIZE_DEFAULT); }
UBENCH_F_TEARDOWN(bench_arena_sequential) { w_arena_free(&ubench_fixture->arena); }

UBENCH_F(bench_arena_sequential, mixed_sizes)
{
	// simulate typical mixed-size allocation pattern
	for (int i = 0; i < 100; i++)
	{
		void *p1 = w_arena_malloc(&ubench_fixture->arena, 16);
		void *p2 = w_arena_malloc(&ubench_fixture->arena, 64);
		void *p3 = w_arena_malloc(&ubench_fixture->arena, 128);
		UBENCH_DO_NOTHING(p1);
		UBENCH_DO_NOTHING(p2);
		UBENCH_DO_NOTHING(p3);
	}
}

UBENCH_F(bench_arena_sequential, malloc_mixed_sizes)
{
	void *ptrs[300];
	int idx = 0;
	for (int i = 0; i < 100; i++)
	{
		ptrs[idx++] = malloc(16);
		ptrs[idx++] = malloc(64);
		ptrs[idx++] = malloc(128);
	}
	for (int i = 0; i < 300; i++)
	{
		UBENCH_DO_NOTHING(ptrs[i]);
		free(ptrs[i]);
	}
}


// large block stress: arena with oversized allocations
struct bench_arena_large { struct w_arena arena; void *ptr; };
UBENCH_F_SETUP(bench_arena_large) { w_arena_init(&ubench_fixture->arena, BLOCK_SIZE_DEFAULT); ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_arena_large) { w_arena_free(&ubench_fixture->arena); free(ubench_fixture->ptr); }

UBENCH_F(bench_arena_large, arena_1k)
{
	void *p = w_arena_malloc(&ubench_fixture->arena, 1024);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_arena_large, malloc_1k)
{
	ubench_fixture->ptr = malloc(1024);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// bulk free comparison: arena clear vs individual frees
#define BULK_FREE_COUNT 10000
struct bench_bulk_free { struct w_arena arena; void *ptrs[BULK_FREE_COUNT]; };
UBENCH_F_SETUP(bench_bulk_free) { w_arena_init(&ubench_fixture->arena, BLOCK_SIZE_DEFAULT); }
UBENCH_F_TEARDOWN(bench_bulk_free) { w_arena_free(&ubench_fixture->arena); }

UBENCH_F(bench_bulk_free, arena_clear_10k)
{
	// allocate 10k small chunks
	for (int i = 0; i < BULK_FREE_COUNT; i++)
	{
		void *p = w_arena_malloc(&ubench_fixture->arena, ALLOC_SIZE_SMALL);
		UBENCH_DO_NOTHING(p);
	}
	// single clear operation
	w_arena_clear(&ubench_fixture->arena);
}

UBENCH_F(bench_bulk_free, malloc_free_10k)
{
	// allocate 10k small chunks
	for (int i = 0; i < BULK_FREE_COUNT; i++)
	{
		ubench_fixture->ptrs[i] = malloc(ALLOC_SIZE_SMALL);
		UBENCH_DO_NOTHING(ubench_fixture->ptrs[i]);
	}
	// must free each individually
	for (int i = 0; i < BULK_FREE_COUNT; i++)
	{
		free(ubench_fixture->ptrs[i]);
	}
}


// cache locality comparison: sequential access arena vs scattered malloc
#define CACHE_COUNT 4096
#define CACHE_CHUNK 64
struct bench_cache_locality { struct w_arena arena; void *arena_ptrs[CACHE_COUNT]; void *malloc_ptrs[CACHE_COUNT]; };
UBENCH_F_SETUP(bench_cache_locality)
{
	w_arena_init(&ubench_fixture->arena, BLOCK_SIZE_DEFAULT);
	// pre-allocate arena chunks (contiguous)
	for (int i = 0; i < CACHE_COUNT; i++)
	{
		ubench_fixture->arena_ptrs[i] = w_arena_malloc(&ubench_fixture->arena, CACHE_CHUNK);
	}
	// pre-allocate malloc chunks (scattered)
	for (int i = 0; i < CACHE_COUNT; i++)
	{
		ubench_fixture->malloc_ptrs[i] = malloc(CACHE_CHUNK);
	}
}
UBENCH_F_TEARDOWN(bench_cache_locality)
{
	w_arena_free(&ubench_fixture->arena);
	for (int i = 0; i < CACHE_COUNT; i++)
	{
		free(ubench_fixture->malloc_ptrs[i]);
	}
}

UBENCH_F(bench_cache_locality, arena_sequential_read)
{
	unsigned char sum = 0;
	// iterate through arena memory (contiguous, cache-friendly)
	for (int i = 0; i < CACHE_COUNT; i++)
	{
		unsigned char *p = ubench_fixture->arena_ptrs[i];
		sum += p[0] + p[CACHE_CHUNK - 1];
	}
	UBENCH_DO_NOTHING(&sum);
}

UBENCH_F(bench_cache_locality, malloc_sequential_read)
{
	unsigned char sum = 0;
	// iterate through malloc memory (scattered, cache-unfriendly)
	for (int i = 0; i < CACHE_COUNT; i++)
	{
		unsigned char *p = ubench_fixture->malloc_ptrs[i];
		sum += p[0] + p[CACHE_CHUNK - 1];
	}
	UBENCH_DO_NOTHING(&sum);
}


// fragmentation over time: repeated alloc/clear cycles
#define FRAG_ALLOCS 1000
#define FRAG_CYCLES 50
struct bench_fragmentation { struct w_arena arena; void *ptrs[FRAG_ALLOCS]; };
UBENCH_F_SETUP(bench_fragmentation) { w_arena_init(&ubench_fixture->arena, BLOCK_SIZE_DEFAULT); }
UBENCH_F_TEARDOWN(bench_fragmentation) { w_arena_free(&ubench_fixture->arena); }

UBENCH_F(bench_fragmentation, arena_50_cycles)
{
	// repeated alloc/clear cycles - arena maintains consistent performance
	for (int cycle = 0; cycle < FRAG_CYCLES; cycle++)
	{
		for (int i = 0; i < FRAG_ALLOCS; i++)
		{
			void *p = w_arena_malloc(&ubench_fixture->arena, ALLOC_SIZE_SMALL);
			UBENCH_DO_NOTHING(p);
		}
		w_arena_clear(&ubench_fixture->arena);
	}
}

UBENCH_F(bench_fragmentation, malloc_50_cycles)
{
	// repeated alloc/free cycles - malloc may fragment over time
	for (int cycle = 0; cycle < FRAG_CYCLES; cycle++)
	{
		for (int i = 0; i < FRAG_ALLOCS; i++)
		{
			ubench_fixture->ptrs[i] = malloc(ALLOC_SIZE_SMALL);
			UBENCH_DO_NOTHING(ubench_fixture->ptrs[i]);
		}
		for (int i = 0; i < FRAG_ALLOCS; i++)
		{
			free(ubench_fixture->ptrs[i]);
		}
	}
}


UBENCH_MAIN();
