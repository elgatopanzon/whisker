/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_arena
 * @created     : Friday Feb 27, 2026 19:30:20 CST
 */

#include "whisker_arena.h"
#include "whisker_memory.h"
#include "whisker_macros.h"

// allocate a block
static struct w_arena_block *w_arena_alloc_block_(size_t block_size)
{
	// allocate extra space for alignment padding
	size_t align_pad = alignof(max_align_t) - 1;
	struct w_arena_block *b = w_mem_xmalloc(sizeof(struct w_arena_block) + block_size + align_pad);
	b->next = NULL;
	// align ptr to max_align_t so first allocation is already aligned
	// end is ptr + block_size to preserve full usable capacity
	b->ptr = (unsigned char *)ALIGN_UP((uintptr_t)b->data, alignof(max_align_t));
	b->end = b->ptr + block_size;

	return b;
}


void w_arena_init(struct w_arena *a, size_t block_size)
{
	a->first = w_arena_alloc_block_(block_size);
	a->current = a->first;
	a->block_size = block_size;
}

// slow path: allocate new block when current is full
__attribute__((noinline))
void *w_arena_malloc_slow_(struct w_arena *a, size_t size)
{
	struct w_arena_block *b = w_arena_alloc_block_(MAX(size, a->block_size));

	// assign new block as current
	a->current->next = b;
	a->current = b;

	// allocate from fresh block (ptr already at data, properly aligned)
	void *p = b->ptr;
	b->ptr += size;

	return p;
}

void *w_arena_calloc(struct w_arena *a, size_t size)
{
	void *p = w_arena_malloc(a, size);

	memset(p, 0, size);

	return p;
}

void w_arena_clear(struct w_arena *a)
{
	struct w_arena_block *b = a->first->next;

	// recursive free all blocks, keep first
	while (b)
	{
		struct w_arena_block *n = b->next;
		free_null(b);
		b = n;
	}

	a->first->next = NULL;
	// reset ptr to aligned start (end doesn't change)
	a->first->ptr = (unsigned char *)ALIGN_UP((uintptr_t)a->first->data, alignof(max_align_t));
	a->current = a->first;
}

void w_arena_free(struct w_arena *a)
{
	// cleanup blocks + first
	w_arena_clear(a);
	free_null(a->first);
}
