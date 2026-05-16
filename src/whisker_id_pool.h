/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_id_pool
 * @created     : Saturday May 16, 2026 15:32:09 CST
 * @description : reusable thread-safe ID pool with LIFO recycling
 */

#include "whisker_std.h"
#include "whisker_array.h"

#include <stdatomic.h>

#ifndef WHISKER_ID_POOL_H
#define WHISKER_ID_POOL_H

// invalid ID sentinel
#define W_ID_POOL_INVALID UINT32_MAX

// default block size for recycled stack reallocation
#ifndef W_ID_POOL_REALLOC_BLOCK_SIZE
#define W_ID_POOL_REALLOC_BLOCK_SIZE 4096
#endif

// ID type
typedef uint32_t w_id;

struct w_id_pool
{
	// next fresh ID (thread-safe via atomic)
	_Atomic w_id next_id;

	// recycled IDs stack (thread-safe via CAS on length)
	w_array_declare(w_id, recycled);

	// realloc block size for recycled stack
	size_t realloc_block_size;
};

// init ID pool with optional custom block size (0 = default)
void w_id_pool_init(struct w_id_pool *pool, size_t realloc_block_size);

// free ID pool resources
void w_id_pool_free(struct w_id_pool *pool);

// request an ID (thread-safe)
w_id w_id_pool_request(struct w_id_pool *pool);

// return an ID to the pool (thread-safe)
void w_id_pool_return(struct w_id_pool *pool, w_id id);

// get count of IDs currently in use
#define w_id_pool_alive_count(p) ((p)->next_id - (p)->recycled_length)

// get count of recycled IDs available
#define w_id_pool_recycled_count(p) ((p)->recycled_length)

// get total IDs ever allocated
#define w_id_pool_total_count(p) ((p)->next_id)

#endif /* WHISKER_ID_POOL_H */
