/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_slab_arena
 * @created     : Saturday May 16, 2026 15:55:00 CST
 */

#include "whisker_std.h"

#include "whisker_slab_arena.h"

void w_slab_arena_init(struct w_slab_arena *slab_arena)
{
	// start with valid array with 0 slabs
	w_array_init_t(slab_arena->slabs, 0);

	w_array_init_t(slab_arena->handle_lookup, W_SLAB_POOL_HANDLE_ARRAY_REALLOC_BLOCK_SIZE);
	w_array_init_t(slab_arena->handle_entries, W_SLAB_POOL_HANDLE_ARRAY_REALLOC_BLOCK_SIZE);

	w_id_pool_init(&slab_arena->handle_pool, W_SLAB_POOL_HANDLE_ARRAY_REALLOC_BLOCK_SIZE);
}

void w_slab_arena_free(struct w_slab_arena *slab_arena)
{
	// free any remaining huge allocations (not managed by slabs)
	size_t max_handle = atomic_load(&slab_arena->handle_pool.next_id);
	for (size_t i = 0; i < max_handle; ++i)
	{
		if (slab_arena->handle_lookup[i] != NULL &&
			slab_arena->handle_entries[i].slab_index == UINT64_MAX)
		{
			free(slab_arena->handle_lookup[i]);
			slab_arena->handle_lookup[i] = NULL;
		}
	}

	// loop slabs, free valid arenas and id pools
	for (size_t i = 0; i < slab_arena->slabs_length; ++i)
	{
		struct w_slab_arena_slab *slab = slab_arena->slabs[i];

		// if it has a slab pointer its used
		if (slab)
		{
			w_arena_free(&slab->arena);
			w_id_pool_free(&slab->slots);
			free_null(slab->slot_pointers);
			free_null(slab);
		}
	}

	free_null(slab_arena->slabs);
	free_null(slab_arena->handle_lookup);
	free_null(slab_arena->handle_entries);
	w_id_pool_free(&slab_arena->handle_pool);
}


static void w_slab_arena_ensure_slab_exists(struct w_slab_arena *slab_arena, size_t slab_index, size_t slab_size)
{
	// ensure slabs array is large enough
	w_array_ensure_alloc_block_size(
		slab_arena->slabs,
		slab_index + 1,
		W_SLAB_POOL_HANDLE_ARRAY_REALLOC_BLOCK_SIZE
	);

	// init slab if required
	if (slab_arena->slabs[slab_index] == NULL)
	{
		struct w_slab_arena_slab *slab = w_mem_xcalloc_t(1, *slab);
		w_arena_init(&slab->arena, W_SLAB_ARENA_BLOCK_SIZE(slab_size));
		w_id_pool_init(&slab->slots, W_SLAB_POOL_HANDLE_ARRAY_REALLOC_BLOCK_SIZE);
		w_array_init_t(slab->slot_pointers, 0);
		slab_arena->slabs[slab_index] = slab;
		if (slab_index + 1 > slab_arena->slabs_length) {
			slab_arena->slabs_length = slab_index + 1;
		}
	}
}

static void w_slab_arena_ensure_slab_handle_valid(struct w_slab_arena *slab_arena, size_t slab_handle)
{
	w_array_ensure_alloc_block_size(
		slab_arena->handle_lookup,
		slab_handle + 1,
		W_SLAB_POOL_HANDLE_ARRAY_REALLOC_BLOCK_SIZE
	);
	w_array_ensure_alloc_block_size(
		slab_arena->handle_entries,
		slab_handle + 1,
		W_SLAB_POOL_HANDLE_ARRAY_REALLOC_BLOCK_SIZE
	);
}

size_t w_slab_arena_malloc(struct w_slab_arena *slab_arena, size_t size)
{
	// huge allocation: bypass slab system entirely
	if (size >= W_SLAB_ARENA_HUGE_THRESHOLD)
	{
		void *ptr = w_mem_xmalloc(size);

		size_t handle = w_id_pool_request(&slab_arena->handle_pool);
		w_slab_arena_ensure_slab_handle_valid(slab_arena, handle);

		slab_arena->handle_lookup[handle] = ptr;
		slab_arena->handle_entries[handle].slab_size = size;
		slab_arena->handle_entries[handle].slab_index = UINT64_MAX;
		slab_arena->handle_entries[handle].actual_size = size;

		return handle;
	}

	size_t slab_index = w_slab_arena_size_to_class(size);
	size_t slab_size = w_slab_arena_class_to_size(slab_index);

	// ensure the slab exists for this size class
	w_slab_arena_ensure_slab_exists(slab_arena, slab_index, slab_size);

	struct w_slab_arena_slab *slab = slab_arena->slabs[slab_index];

	// get a slab slot ID
	size_t slab_slot = w_id_pool_request(&slab->slots);

	// get slot pointer or alloc new one
	void *ptr;
    if (slab_slot >= slab->slot_pointers_length)
    {
        // new slot - allocate from arena
        ptr = w_arena_malloc(&slab->arena, slab_size);

		w_array_ensure_alloc_block_size(
			slab->slot_pointers,
			slab_slot + 1,
			W_SLAB_POOL_HANDLE_ARRAY_REALLOC_BLOCK_SIZE
		);

		// assign pointer
		slab->slot_pointers[slab->slot_pointers_length++] = ptr;
    }
    else
    {
        // recycled slot - pointer already stored
        ptr = slab->slot_pointers[slab_slot];
    }

    // get global handle + store
    size_t handle = w_id_pool_request(&slab_arena->handle_pool);

    // ensure handle arrays are sized for new handle
	w_slab_arena_ensure_slab_handle_valid(slab_arena, handle);

	// set global handle entry info
	slab_arena->handle_lookup[handle] = ptr;
	slab_arena->handle_entries[handle].slab_size = slab_size;
	slab_arena->handle_entries[handle].slab_index = slab_slot;
	slab_arena->handle_entries[handle].actual_size = size;

    return handle;
}

size_t w_slab_arena_realloc(struct w_slab_arena *slab_arena, size_t handle, size_t new_size)
{
	w_slab_arena_ensure_slab_handle_valid(slab_arena, handle);

	struct w_slab_arena_entry *entry = w_slab_arena_get_handle_entry(slab_arena, handle);

	// huge->huge realloc: use realloc directly
	if (entry->slab_index == UINT64_MAX && new_size >= W_SLAB_ARENA_HUGE_THRESHOLD)
	{
		if (new_size <= entry->slab_size)
		{
			entry->actual_size = new_size;
			return handle;
		}

		void *ptr = w_mem_xrealloc(slab_arena->handle_lookup[handle], new_size);
		slab_arena->handle_lookup[handle] = ptr;
		entry->slab_size = new_size;
		entry->actual_size = new_size;
		return handle;
	}

	// nothing to do if current slab fits
	if (entry->slab_size >= new_size && !(entry->slab_index == UINT64_MAX && new_size < W_SLAB_ARENA_HUGE_THRESHOLD))
	{
		entry->actual_size = new_size;
		return handle;
	}

	// crossing threshold or growing: alloc new, copy, free old
	void *old_ptr = w_slab_arena_resolve_handle(slab_arena, handle);
	size_t copy_size = entry->actual_size < new_size ? entry->actual_size : new_size;
	bool old_is_huge = (entry->slab_index == UINT64_MAX);

	if (old_is_huge)
	{
		// huge->slab: must alloc first, copy, then free (free releases the pointer)
		size_t new_handle = w_slab_arena_malloc(slab_arena, new_size);
		void *new_ptr = w_slab_arena_resolve_handle(slab_arena, new_handle);
		memcpy(new_ptr, old_ptr, copy_size);
		w_slab_arena_free_handle(slab_arena, handle);
		return new_handle;
	}

	// slab->slab or slab->huge: old pointer stays valid after free (arena-backed)
	w_slab_arena_free_handle(slab_arena, handle);
	handle = w_slab_arena_malloc(slab_arena, new_size);
	void *new_ptr = w_slab_arena_resolve_handle(slab_arena, handle);
	memcpy(new_ptr, old_ptr, copy_size);

	return handle;
}

void w_slab_arena_free_handle(struct w_slab_arena *slab_arena, size_t handle)
{
	w_slab_arena_ensure_slab_handle_valid(slab_arena, handle);

	struct w_slab_arena_entry *entry = w_slab_arena_get_handle_entry(slab_arena, handle);

	// huge allocation: free directly
	if (entry->slab_index == UINT64_MAX)
	{
		free(slab_arena->handle_lookup[handle]);
		w_id_pool_return(&slab_arena->handle_pool, handle);
		slab_arena->handle_lookup[handle] = NULL;
		return;
	}

	size_t class_index = w_slab_arena_size_to_class(entry->slab_size);
	struct w_slab_arena_slab *slab = slab_arena->slabs[class_index];

	// return slab slot ID
	w_id_pool_return(&slab->slots, entry->slab_index);

	// return global handle
	w_id_pool_return(&slab_arena->handle_pool, handle);

	// clear handle record
	slab_arena->handle_lookup[handle] = NULL;
}
