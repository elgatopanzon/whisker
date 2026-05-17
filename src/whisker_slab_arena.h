/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_slab_arena
 * @created     : Saturday May 16, 2026 15:04:33 CST
 * @description : arena-backed slab allocator
 */

#include "whisker.h"

#ifndef WHISKER_SLAB_ARENA_H
#define WHISKER_SLAB_ARENA_H

// hybrid slab arena block size
#define W_SLAB_ARENA_MIN_BLOCK_SIZE 4096
#define W_SLAB_ARENA_MIN_SLOTS_PER_BLOCK 16
#define W_SLAB_POOL_THRESHOLD (256 * 1024)  // 256KB max slab size until 1 block

// 1KB   > 16 slots x 1KB = 16KB block
// 256KB > 16 slots × 256KB = 4MB block
// 1MB   > 1 slot × 1MB = 1MB block
// 100MB > 1 slot × 100MB = 100MB block
#define W_SLAB_ARENA_SLOTS_PER_BLOCK(slab_size) \
    ((slab_size) > W_SLAB_POOL_THRESHOLD \
        ? 1 \
        : w_maxf(W_SLAB_ARENA_MIN_SLOTS_PER_BLOCK, W_SLAB_ARENA_MIN_BLOCK_SIZE / (slab_size)))
#define W_SLAB_ARENA_BLOCK_SIZE(slab_size) \
    ((slab_size) * W_SLAB_ARENA_SLOTS_PER_BLOCK(slab_size))

#define W_SLAB_POOL_HANDLE_ARRAY_REALLOC_BLOCK_SIZE 256

#define w_slab_arena_round_size(size) ({ \
    size_t _s = (size); \
    _s <= 8 ? 8 : \
    _s <= 128 ? ((_s + 7) & ~(size_t)7) : \
    ({ \
        size_t _step = w_next_pow2(_s) >> 4; \
        _step = _step < 16 ? 16 : _step; \
        ((_s + _step - 1) / _step) * _step; \
    }); \
})

#define w_slab_arena_size_to_class(size) ({ \
    size_t _s = (size); \
    size_t _class; \
    if (_s <= 128) { \
        /* Tier 0: 8-byte steps, classes 0-15 */ \
        _class = (_s >> 3) - 1; \
    } else { \
        /* Tier 1+: 8 classes per tier */ \
        /* Find tier: log2(size) - 7 */ \
        size_t _log2 = 64 - __builtin_clzll(_s - 1); \
        size_t _tier = _log2 - 7; \
        size_t _tier_base = (size_t)1 << _log2; \
        size_t _prev_base = _tier_base >> 1; \
        size_t _step = _tier_base >> 4; \
        /* 16 classes in tier 0, then 8 per tier after */ \
        size_t _tier_start_class = 16 + (_tier - 1) * 8; \
        size_t _offset = (_s - _prev_base) / _step - 1; \
        _class = _tier_start_class + _offset; \
    } \
    _class; \
})

#define w_slab_arena_class_to_size(cls) ({ \
    size_t _result; \
    if ((cls) < 16) { \
        _result = ((cls) + 1) * 8; \
    } else { \
        size_t _tier = ((cls) - 16) / 8 + 1; \
        size_t _offset = ((cls) - 16) % 8; \
        size_t _tier_base = 128 << _tier; \
        size_t _step = _tier_base >> 4; \
        _result = (_tier_base >> 1) + (_offset + 1) * _step; \
    } \
    _result; \
})

struct w_slab_arena_entry 
{
	// storage class this entry points to
	// class = size rounded to power of 2
	size_t slab_size;

	// index into the slab where the data is
	// note: not pointer, its not stable
	size_t slab_index;

	// the actual original allocation size before rounded
	// used to compute wastage/optimise with custom classes
	size_t actual_size;
};

struct w_slab_arena_slab 
{
	// storage class size of this slabs slots
	size_t slab_size;

	// arena where this slab stores its data
	struct w_arena arena;

	// id pool to manage slabs slots
	struct w_id_pool slots;

	// pointers per-slot to allocated arena data
	w_array_declare(void *, slot_pointers);
};

struct w_slab_arena 
{
	// every size has the chance of getting its own arena
	// size = index -> arena of all those allocations
	w_array_declare(struct w_slab_arena_slab *, slabs);

	// holds void* pointers to the actual data in the slot
	// this allows them to move when accessed just by the slot
	w_array_declare(void *, handle_lookup);

	// each slots metadata, since slot # doesn't mean anything alone
	w_array_declare(struct w_slab_arena_entry, handle_entries);

	// id pool to manage global handle IDs
	struct w_id_pool handle_pool;
};

#endif /* WHISKER_SLAB_ARENA_H */

// init a slab arena's slabs, handle lookup and handle pool
void w_slab_arena_init(struct w_slab_arena *slab_arena);

// free a slab arena's slabs, handle lookup and handle pool
void w_slab_arena_free(struct w_slab_arena *slab_arena);

// allocate something in the slab arena and get back the handle
size_t w_slab_arena_malloc(struct w_slab_arena *slab_arena, size_t size);

// realloc a handle with a new size (allows growth and shrink)
size_t w_slab_arena_realloc(struct w_slab_arena *slab_arena, size_t handle, size_t new_size);

// return the handle and internally free the given slot in the slab arena
void w_slab_arena_free_handle(struct w_slab_arena *slab_arena, size_t handle);

#define w_slab_arena_resolve_handle(slab_arena, handle) \
    ((slab_arena)->handle_lookup[handle])

#define w_slab_arena_resolve_safe_handle(slab_arena, handle) \
    ({ \
        void *ptr = NULL; \
        if ((handle) < (slab_arena)->handle_lookup_length) { \
            ptr = w_slab_arena_resolve_handle(slab_arena, handle); \
        } \
        ptr; \
    })

#define w_slab_arena_get_handle_entry(slab_arena, handle) \
	&slab_arena->handle_entries[handle]
