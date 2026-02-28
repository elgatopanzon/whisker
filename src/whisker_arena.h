/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_arena
 * @created     : Friday Feb 27, 2026 19:28:14 CST
 * @description : block-based arena allocator with free list
 */

#include "whisker_std.h"
#include "whisker_memory.h"
#include "whisker_macros.h"

#ifndef WHISKER_ARENA_H
#define WHISKER_ARENA_H

// the arena is made up of w_arena_block structs
// these structs are for the typical sized arena blocks
struct w_arena_block
{
	struct w_arena_block *next;
	unsigned char *ptr;  // current allocation position
	unsigned char *end;  // end of block data region
	unsigned char data[];
};

// main w_arena struct
// holds pointer to the first block ever created and current
struct w_arena 
{
	struct w_arena_block *first;
	struct w_arena_block *current;
	size_t block_size;
};

// initialise a w_arena struct with its first block
void w_arena_init(struct w_arena *a, size_t block_size);

// slow path for w_arena_malloc macro: allocates a new block and bumps from it
void *w_arena_malloc_slow_(struct w_arena *a, size_t size);

// allocate size bytes with given arena (fast path inlined, slow path outlined)
#define w_arena_malloc(a, size) ({ \
    struct w_arena *_a = (a); \
    size_t _size = (size); \
    unsigned char *_p = (unsigned char *)ALIGN_UP((uintptr_t)_a->current->ptr, alignof(max_align_t)); \
    if (likely(_p + _size <= _a->current->end)) { \
        _a->current->ptr = _p + _size; \
    } else { \
        _p = w_arena_malloc_slow_(_a, _size); \
    } \
    (void *)_p; \
})

// allocate size bytes with given arena, calloc
void *w_arena_calloc(struct w_arena *a, size_t size);

// clear and free all arena's blocks except first block
void w_arena_clear(struct w_arena *a);

// clear and free all blocks including first
void w_arena_free(struct w_arena *a);

#endif /* WHISKER_ARENA_H */
