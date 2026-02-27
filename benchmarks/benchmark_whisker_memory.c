/**
 * @author      : ElGatoPanzon
 * @file        : benchmark_whisker_memory
 * @created     : 2026-02-26
 * @description : benchmarks comparing malloc vs w_mem_xmalloc_ overhead
 */

#include "ubench.h"
#include "whisker_memory.h"

#include <stdlib.h>


// small allocation: 64 bytes
struct bench_alloc_64 { void *ptr; };
UBENCH_F_SETUP(bench_alloc_64) { ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_alloc_64) { free(ubench_fixture->ptr); }

UBENCH_F(bench_alloc_64, malloc)
{
	ubench_fixture->ptr = malloc(64);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_alloc_64, xmalloc)
{
	ubench_fixture->ptr = w_mem_xmalloc(64);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// medium allocation: 1KB
struct bench_alloc_1k { void *ptr; };
UBENCH_F_SETUP(bench_alloc_1k) { ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_alloc_1k) { free(ubench_fixture->ptr); }

UBENCH_F(bench_alloc_1k, malloc)
{
	ubench_fixture->ptr = malloc(1024);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_alloc_1k, xmalloc)
{
	ubench_fixture->ptr = w_mem_xmalloc(1024);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// large allocation: 64KB
struct bench_alloc_64k { void *ptr; };
UBENCH_F_SETUP(bench_alloc_64k) { ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_alloc_64k) { free(ubench_fixture->ptr); }

UBENCH_F(bench_alloc_64k, malloc)
{
	ubench_fixture->ptr = malloc(65536);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_alloc_64k, xmalloc)
{
	ubench_fixture->ptr = w_mem_xmalloc(65536);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// xcalloc vs calloc: 64 bytes
struct bench_calloc_64 { void *ptr; };
UBENCH_F_SETUP(bench_calloc_64) { ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_calloc_64) { free(ubench_fixture->ptr); }

UBENCH_F(bench_calloc_64, calloc)
{
	ubench_fixture->ptr = calloc(1, 64);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_calloc_64, xcalloc)
{
	ubench_fixture->ptr = w_mem_xcalloc(1, 64);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// xcalloc vs calloc: 1KB
struct bench_calloc_1k { void *ptr; };
UBENCH_F_SETUP(bench_calloc_1k) { ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_calloc_1k) { free(ubench_fixture->ptr); }

UBENCH_F(bench_calloc_1k, calloc)
{
	ubench_fixture->ptr = calloc(1, 1024);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_calloc_1k, xcalloc)
{
	ubench_fixture->ptr = w_mem_xcalloc(1, 1024);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// xcalloc vs calloc: 64KB
struct bench_calloc_64k { void *ptr; };
UBENCH_F_SETUP(bench_calloc_64k) { ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_calloc_64k) { free(ubench_fixture->ptr); }

UBENCH_F(bench_calloc_64k, calloc)
{
	ubench_fixture->ptr = calloc(1, 65536);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_calloc_64k, xcalloc)
{
	ubench_fixture->ptr = w_mem_xcalloc(1, 65536);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// xrealloc vs realloc: 64 bytes -> 128 bytes
struct bench_realloc_64 { void *ptr; };
UBENCH_F_SETUP(bench_realloc_64) { ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_realloc_64) { free(ubench_fixture->ptr); }

UBENCH_F(bench_realloc_64, realloc)
{
	ubench_fixture->ptr = malloc(64);
	ubench_fixture->ptr = realloc(ubench_fixture->ptr, 128);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_realloc_64, xrealloc)
{
	ubench_fixture->ptr = w_mem_xmalloc(64);
	ubench_fixture->ptr = w_mem_xrealloc(ubench_fixture->ptr, 128);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// xrealloc vs realloc: 1KB -> 2KB
struct bench_realloc_1k { void *ptr; };
UBENCH_F_SETUP(bench_realloc_1k) { ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_realloc_1k) { free(ubench_fixture->ptr); }

UBENCH_F(bench_realloc_1k, realloc)
{
	ubench_fixture->ptr = malloc(1024);
	ubench_fixture->ptr = realloc(ubench_fixture->ptr, 2048);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_realloc_1k, xrealloc)
{
	ubench_fixture->ptr = w_mem_xmalloc(1024);
	ubench_fixture->ptr = w_mem_xrealloc(ubench_fixture->ptr, 2048);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// xrealloc vs realloc: 64KB -> 128KB
struct bench_realloc_64k { void *ptr; };
UBENCH_F_SETUP(bench_realloc_64k) { ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_realloc_64k) { free(ubench_fixture->ptr); }

UBENCH_F(bench_realloc_64k, realloc)
{
	ubench_fixture->ptr = malloc(65536);
	ubench_fixture->ptr = realloc(ubench_fixture->ptr, 131072);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_realloc_64k, xrealloc)
{
	ubench_fixture->ptr = w_mem_xmalloc(65536);
	ubench_fixture->ptr = w_mem_xrealloc(ubench_fixture->ptr, 131072);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// thin wrappers: 64 bytes
struct bench_thin_64 { void *ptr; };
UBENCH_F_SETUP(bench_thin_64) { ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_thin_64) { free(ubench_fixture->ptr); }

UBENCH_F(bench_thin_64, malloc)
{
	ubench_fixture->ptr = malloc(64);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_64, w_mem_malloc)
{
	ubench_fixture->ptr = w_mem_malloc(64);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_64, calloc)
{
	ubench_fixture->ptr = calloc(1, 64);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_64, w_mem_calloc)
{
	ubench_fixture->ptr = w_mem_calloc(1, 64);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_64, realloc)
{
	ubench_fixture->ptr = malloc(64);
	ubench_fixture->ptr = realloc(ubench_fixture->ptr, 128);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_64, w_mem_realloc)
{
	ubench_fixture->ptr = malloc(64);
	ubench_fixture->ptr = w_mem_realloc(ubench_fixture->ptr, 128);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// thin wrappers: 1KB
struct bench_thin_1k { void *ptr; };
UBENCH_F_SETUP(bench_thin_1k) { ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_thin_1k) { free(ubench_fixture->ptr); }

UBENCH_F(bench_thin_1k, malloc)
{
	ubench_fixture->ptr = malloc(1024);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_1k, w_mem_malloc)
{
	ubench_fixture->ptr = w_mem_malloc(1024);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_1k, calloc)
{
	ubench_fixture->ptr = calloc(1, 1024);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_1k, w_mem_calloc)
{
	ubench_fixture->ptr = w_mem_calloc(1, 1024);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_1k, realloc)
{
	ubench_fixture->ptr = malloc(1024);
	ubench_fixture->ptr = realloc(ubench_fixture->ptr, 2048);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_1k, w_mem_realloc)
{
	ubench_fixture->ptr = malloc(1024);
	ubench_fixture->ptr = w_mem_realloc(ubench_fixture->ptr, 2048);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


// thin wrappers: 64KB
struct bench_thin_64k { void *ptr; };
UBENCH_F_SETUP(bench_thin_64k) { ubench_fixture->ptr = NULL; }
UBENCH_F_TEARDOWN(bench_thin_64k) { free(ubench_fixture->ptr); }

UBENCH_F(bench_thin_64k, malloc)
{
	ubench_fixture->ptr = malloc(65536);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_64k, w_mem_malloc)
{
	ubench_fixture->ptr = w_mem_malloc(65536);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_64k, calloc)
{
	ubench_fixture->ptr = calloc(1, 65536);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_64k, w_mem_calloc)
{
	ubench_fixture->ptr = w_mem_calloc(1, 65536);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_64k, realloc)
{
	ubench_fixture->ptr = malloc(65536);
	ubench_fixture->ptr = realloc(ubench_fixture->ptr, 131072);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}

UBENCH_F(bench_thin_64k, w_mem_realloc)
{
	ubench_fixture->ptr = malloc(65536);
	ubench_fixture->ptr = w_mem_realloc(ubench_fixture->ptr, 131072);
	UBENCH_DO_NOTHING(ubench_fixture->ptr);
	free(ubench_fixture->ptr);
	ubench_fixture->ptr = NULL;
}


UBENCH_MAIN();
