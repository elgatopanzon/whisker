/**
 * @author      : ElGatoPanzon
 * @file        : whisker_buffers
 * @created     : Thursday Mar 26, 2026 20:32:23 CST
 * @description : ECS buffer module -- rental-based contiguous buffer allocation
 */

#ifndef WHISKER_BUFFERS_H
#define WHISKER_BUFFERS_H

#include "whisker.h"

/****************
*  components  *
****************/

// component name for buffer metadata stored on the buffer entity
/* buffer metadata: packed as w_pack32x2 for serialization support
 * left  = type_id (W_COMPONENT_TYPE enum)
 * right = type_size (element size in bytes, capped to 32 bits) */
w_ecs_define_component(w_pack32x2, w_buffer_meta, 0);

// pack/unpack macros for buffer metadata
#define W_BUFFER_META_PACK(type_id, type_size) \
	((w_buffer_meta){ .left = (type_id), .right = (uint32_t)(type_size) })
#define W_BUFFER_META_TYPE_ID(meta)   ((meta).left)
#define W_BUFFER_META_TYPE_SIZE(meta) ((meta).right)

// init the buffers module (registers meta component type)
void w_buffers_init(struct w_ecs_world *world);

// free the buffers module
void w_buffers_free(struct w_ecs_world *world);

/*****************************
*  handle layout             *
*****************************/

// handle packs offset (left) and length (right)
#define W_BUFFER_HANDLE_OFFSET(h) ((h).left)
#define W_BUFFER_HANDLE_LENGTH(h) ((h).right)
#define W_BUFFER_HANDLE_INVALID (w_pack32x2){ .left = UINT32_MAX, .right = 0 }

/*****************************
*  rental API                *
*****************************/

/* create/rent a contiguous range in a named buffer
 * first call with a given name creates the buffer component and stores metadata
 * subsequent calls rent additional ranges within the same buffer
 * returns handle with (offset, length), or INVALID on failure */
w_pack32x2 w_buffer_create(struct w_ecs_world *world, char *name,
	uint type_id, size_t type_size, uint32_t count);

// typed create: derives type_id and type_size from the C type name
#define w_buffer_create_typed(world, name, type, count) \
	w_buffer_create(world, name, W_COMPONENT_TYPE_##type, sizeof(type), count)

// return a rented range, clearing its bits for reuse
void w_buffer_return(struct w_ecs_world *world, char *name, w_pack32x2 handle);

// destroy an entire named buffer: frees all data and clears the component entry
void w_buffer_destroy(struct w_ecs_world *world, char *name);

/*****************************
*  data access               *
*****************************/

/* get pointer to start of handle's data range within the named buffer
 * user indexes from 0 relative to this pointer: ((T*)ptr)[i] */
void *w_buffer_get_ptr(struct w_ecs_world *world, char *name, w_pack32x2 handle);

// get length from handle (count of elements in the rented range)
uint32_t w_buffer_get_length(w_pack32x2 handle);

// get the underlying component entry for direct/fast access (NULL if buffer not created)
struct w_component_entry *w_buffer_get_entry(struct w_ecs_world *world, char *name);

// get buffer metadata (NULL if not a buffer)
w_buffer_meta *w_buffer_get_meta(struct w_ecs_world *world, char *name);

/*****************************
*  iteration                 *
*****************************/

/* iterate over all indices in a handle's contiguous range
 * provides uint32_t buf_index as the absolute buffer index */
#define w_buffer_for_each(handle, block) do { \
	uint32_t _bf_end = W_BUFFER_HANDLE_OFFSET(handle) + W_BUFFER_HANDLE_LENGTH(handle); \
	for (uint32_t buf_index = W_BUFFER_HANDLE_OFFSET(handle); buf_index < _bf_end; buf_index++) { \
		block \
	} \
} while(0)

#endif // WHISKER_BUFFERS_H
