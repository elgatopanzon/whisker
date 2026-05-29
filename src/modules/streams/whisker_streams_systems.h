/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_streams_systems
 * @created     : Wednesday May 27, 2026 18:05:39 CST
 * @description : systems for streams module
 */

#include "whisker_streams.h"

#ifndef WHISKER_STREAMS_SYSTEMS_H
#define WHISKER_STREAMS_SYSTEMS_H

// TODO: evaluate in future if this should be converted to a stream entity implementation to remove the rigid input/output and duplicate system logic for both cases

// handle req_stream_input_buffer_hot requests
w_ecs_system(
	wm_streams_handle_request_input_buffer_hot,
	WM_STREAM_PHASE_INPUT_PRE_CONSUME,
	w_query(
		w_query_h(req_stream_input_buffer_hot),
		// pick up input buffer size
		w_query_r(stream_input_buffer_size),
		// skip if already has handle
		// note: dont support INVALID handles
		w_query_n(stream_input_buffer_handle),
	),
{
	// alloc input buffer and set handle
	wm_managed_alloc_malloc(world, stream_input_buffer_handle, entity, *w_query_get(stream_input_buffer_size));

	// set/reset offset and length
	w_set_default(entity, stream_input_buffer_offset);
	w_set_default(entity, stream_input_buffer_length);
	w_set_tag(entity, stream_input_available, false);

	// clear request tag
	w_set_tag(entity, req_stream_input_buffer_hot, false);
});

// handle req_stream_output_buffer_hot requests
w_ecs_system(
	wm_streams_handle_request_output_buffer_hot,
	WM_STREAM_PHASE_OUTPUT_PRE_CONSUME,
	w_query(
		w_query_h(req_stream_output_buffer_hot),
		// pick up output buffer size
		w_query_r(stream_output_buffer_size),
		// skip if already has handle
		// note: dont support INVALID handles
		w_query_n(stream_output_buffer_handle),
	),
{
	// alloc output buffer and set handle
	wm_managed_alloc_malloc(world, stream_output_buffer_handle, entity, *w_query_get(stream_output_buffer_size));

	// set/reset offset and length
	w_set_default(entity, stream_output_buffer_offset);
	w_set_default(entity, stream_output_buffer_length);
	w_set_tag(entity, stream_output_available, false);

	// clear request tag
	w_set_tag(entity, req_stream_output_buffer_hot, false);
});

// handle req_stream_input_buffer_cold requests
w_ecs_system(
	wm_streams_handle_request_input_buffer_cold,
	WM_STREAM_PHASE_INPUT_POST_CONSUME,
	w_query(
		w_query_h(req_stream_input_buffer_cold),
		// it must have it, because this system removes it
		w_query_h(stream_input_buffer_handle),
	),
{
	// free and remove the buffer handle
	wm_managed_alloc_free_handle(world, stream_input_buffer_handle, entity);
	w_remove(entity, stream_input_buffer_handle);

	// clear input available tag
	// note: offset and length can stay as-is until hot
	w_set_tag(entity, stream_input_available, false);

	// clear request tag
	w_set_tag(entity, req_stream_input_buffer_cold, false);
});

// handle req_stream_output_buffer_cold requests
w_ecs_system(
	wm_streams_handle_request_output_buffer_cold,
	WM_STREAM_PHASE_OUTPUT_POST_CONSUME,
	w_query(
		w_query_h(req_stream_output_buffer_cold),
		// it must have it, because this system removes it
		w_query_h(stream_output_buffer_handle),
	),
{
	// free and remove the buffer handle
	wm_managed_alloc_free_handle(world, stream_output_buffer_handle, entity);
	w_remove(entity, stream_output_buffer_handle);

	// clear output available tag
	// note: offset and length can stay as-is until hot
	w_set_tag(entity, stream_output_available, false);

	// clear request tag
	w_set_tag(entity, req_stream_output_buffer_cold, false);
});

// compact stream input buffer
w_ecs_system(
	wm_streams_compact_input_buffer,
	WM_STREAM_PHASE_INPUT_POST_CONSUME,
	w_query(
		w_query_r(stream_input_buffer_handle),
		w_query_r(stream_input_buffer_length),
		w_query_r(stream_input_buffer_offset),
	),
{
	uint64_t *length = w_query_get(stream_input_buffer_length);
	uint64_t *offset = w_query_get(stream_input_buffer_offset);

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

		// clear available input
		w_set_tag(entity, stream_input_available, false);
	}
	// if offset if valid and offset is < length then shift
	else if (*offset > 0 && *offset < *length)
	{
		// memmove to start of buffer
		size_t remaining = (*length - *offset);
		uint8_t *buffer = wm_managed_alloc_resolve_handle(world, stream_input_buffer_handle, entity);
		memmove(buffer, buffer + *offset, remaining);
		*length = remaining;
		*offset = 0;
	}
});

// compact stream output buffer
w_ecs_system(
	wm_streams_compact_output_buffer,
	WM_STREAM_PHASE_OUTPUT_POST_CONSUME,
	w_query(
		w_query_r(stream_output_buffer_handle),
		w_query_r(stream_output_buffer_length),
		w_query_r(stream_output_buffer_offset),
	),
{
	uint64_t *length = w_query_get(stream_output_buffer_length);
	uint64_t *offset = w_query_get(stream_output_buffer_offset);

	// dont allow offset to exceed valid length
	*offset = w_minf(*offset, *length);

	// when offset and length match all consumers caught up
	if (*offset == *length)
	{
		*offset = 0;
		*length = 0;

		// clear available output
		w_set_tag(entity, stream_output_available, false);
	}
	// if offset if valid and offset is < length then shift
	else if (*offset > 0 && *offset < *length)
	{
		// memmove to start of buffer
		size_t remaining = (*length - *offset);
		uint8_t *buffer = wm_managed_alloc_resolve_handle(world, stream_output_buffer_handle, entity);
		memmove(buffer, buffer + *offset, remaining);
		*length = remaining;
		*offset = 0;
	}
});

#endif /* WHISKER_STREAMS_SYSTEMS_H */

