/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_id_pool
 * @created     : Saturday May 16, 2026 15:32:09 CST
 * @description : reusable thread-safe ID pool with LIFO recycling
 */

#include "whisker_std.h"
#include "whisker_id_pool.h"

void w_id_pool_init(struct w_id_pool *pool, size_t realloc_block_size)
{
	pool->realloc_block_size = realloc_block_size > 0
		? realloc_block_size
		: W_ID_POOL_REALLOC_BLOCK_SIZE;

	w_array_init_t(pool->recycled, pool->realloc_block_size);
	atomic_store(&pool->recycled_length, 0);
	atomic_store(&pool->next_id, 0);
}

void w_id_pool_free(struct w_id_pool *pool)
{
	free_null(pool->recycled);
	atomic_store(&pool->next_id, 0);
	atomic_store(&pool->recycled_length, 0);
}

w_id w_id_pool_request(struct w_id_pool *pool)
{
	w_id id;

	// try to pop from recycled stack (thread-safe via CAS)
	while (true)
	{
		size_t current_len = atomic_load(&pool->recycled_length);
		if (current_len > 0)
		{
			// try CAS decrement to claim a slot
			if (atomic_compare_exchange_weak(&pool->recycled_length, &current_len, current_len - 1))
			{
				// success - we own slot at current_len - 1
				id = pool->recycled[current_len - 1];
				break;
			}
			// CAS failed, retry
		}
		else
		{
			// stack empty, allocate new ID atomically
			id = atomic_fetch_add(&pool->next_id, 1);
			break;
		}
	}

	return id;
}

void w_id_pool_return(struct w_id_pool *pool, w_id id)
{
	// push to recycled stack (thread-safe via CAS)
	while (true)
	{
		size_t current_len = atomic_load(&pool->recycled_length);
		// ensure capacity before CAS
		w_array_ensure_alloc_block_size(pool->recycled, current_len + 1, pool->realloc_block_size);
		// try CAS increment to claim a slot
		if (atomic_compare_exchange_weak(&pool->recycled_length, &current_len, current_len + 1))
		{
			// success - we own slot at current_len
			pool->recycled[current_len] = id;
			break;
		}
		// CAS failed, retry
	}
}
