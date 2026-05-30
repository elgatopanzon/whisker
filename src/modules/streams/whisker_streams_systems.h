/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_streams_systems
 * @created     : Wednesday May 27, 2026 18:05:39 CST
 * @description : systems for streams module
 */

#include "whisker_streams.h"

#ifndef WHISKER_STREAMS_SYSTEMS_H
#define WHISKER_STREAMS_SYSTEMS_H

// handle stream hot requests
w_ecs_system(
	wm_streams_handle_request_hot,
	WM_STREAM_PHASE_PREPARE,
	w_query(
		w_query_h(req_stream_buffer_hot),
		w_query_r(stream_buffer_size),
		// skip if already has handle
		// note: dont support INVALID handles
		w_query_n(stream_buffer_handle),
	),
{
	wm_managed_alloc_malloc(world, stream_buffer_handle, entity, *w_query_get(stream_buffer_size));

	// set/reset offset and length
	w_set_default(entity, stream_buffer_offset);
	w_set_default(entity, stream_buffer_length);
	w_set_tag(entity, stream_available, false);

	// clear request tag
	w_set_tag(entity, req_stream_buffer_hot, false);
});

// handle stream cold requests
w_ecs_system(
	wm_streams_handle_request_cold,
	WM_STREAM_PHASE_POST,
	w_query(
		w_query_h(req_stream_buffer_cold),
		// it must have it, because this system removes it
		w_query_h(stream_buffer_handle),
	),
{
	// free and remove the buffer handle
	wm_managed_alloc_free_handle(world, stream_buffer_handle, entity);
	w_remove(entity, stream_buffer_handle);

	// clear available tag
	// note: offset and length can stay as-is until hot
	w_set_tag(entity, stream_available, false);

	// clear request tag
	w_set_tag(entity, req_stream_buffer_cold, false);
});

// compact stream buffers opted into automatic end-of-frame compaction
w_ecs_system(
	wm_streams_auto_compact,
	WM_STREAM_PHASE_POST,
	w_query(
		w_query_h(stream_auto_compact),
		w_query_r(stream_buffer_handle),
		w_query_r(stream_buffer_length),
		w_query_r(stream_buffer_offset),
	),
{
	uint64_t *length = w_query_get(stream_buffer_length);
	uint64_t *offset = w_query_get(stream_buffer_offset);

	// dont allow offset to exceed valid length
	if (*offset > *length)
	{
		*offset = *length;
	}

	// when offset and length match all consumers caught up
	if (*offset == *length)
	{
		*offset = 0;
		*length = 0;

		// clear available
		w_set_tag(entity, stream_available, false);
	}
	// if offset if valid and offset is < length then shift
	else if (*offset > 0 && *offset < *length)
	{
		// memmove to start of buffer
		size_t remaining = (*length - *offset);
		uint8_t *buffer = wm_streams_get_buffer(world, entity);
		memmove(buffer, buffer + *offset, remaining);
		*length = remaining;
		*offset = 0;
	}
});

#endif /* WHISKER_STREAMS_SYSTEMS_H */
