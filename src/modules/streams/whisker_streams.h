/**
 * @author      : ElGatoPanzon
 * @file        : whisker_streams
 * @created     : Wednesday May 27, 2026 12:04:15 CST
 * @description : Module provide input/output byte stream management
 */

#ifndef WHISKER_STREAMS_H
#define WHISKER_STREAMS_H

#include "whisker.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/managed_alloc/whisker_managed_alloc.h"

/*******************
*  custom phases  *
*******************/
// the stream phases position input operations in the LOAD phases, and output
// operations in the POST phases

// PRE_CONSUME: producers append bytes to the stream before readers/drainers
// ON_CONSUME: consumers read/drain bytes and advance the stream offset
// POST_CONSUME: cleanup after consumers advance the stream offset
enum WM_STREAM_PHASE 
{ 
	// runs after pre_load
	WM_STREAM_PHASE_INPUT_PRE_CONSUME = 200,
	// runs after post
	WM_STREAM_PHASE_OUTPUT_PRE_CONSUME = 201,

	// runs after on_load
	WM_STREAM_PHASE_INPUT_ON_CONSUME = 202,
	// runs after post
	WM_STREAM_PHASE_OUTPUT_ON_CONSUME = 203,

	// runs after post_load
	WM_STREAM_PHASE_INPUT_POST_CONSUME = 204,
	// runs after post
	WM_STREAM_PHASE_OUTPUT_POST_CONSUME = 205,
};


/****************
*  components  *
****************/

#ifndef WM_STREAMS_INPUT_BUFFER_SIZE
#define WM_STREAMS_INPUT_BUFFER_SIZE 65534
#endif
#ifndef WM_STREAMS_OUTPUT_BUFFER_SIZE
#define WM_STREAMS_OUTPUT_BUFFER_SIZE 65534
#endif

// streams offers input and output buffers managed by the module
w_ecs_define_component(uint64_t, stream_input_buffer_size, WM_STREAMS_INPUT_BUFFER_SIZE);
w_ecs_define_component(uint64_t, stream_output_buffer_size, WM_STREAMS_OUTPUT_BUFFER_SIZE);

w_ecs_define_component(uint64_t, stream_input_buffer_handle, WM_MANAGED_ALLOC_INVALID_HANDLE);
w_ecs_define_component(uint64_t, stream_output_buffer_handle, WM_MANAGED_ALLOC_INVALID_HANDLE);

// offset defines the current position in the input/output buffer to
// consume/drain from
w_ecs_define_component(uint64_t, stream_input_buffer_offset, 0);
w_ecs_define_component(uint64_t, stream_output_buffer_offset, 0);

// length defines how much of the bytes are valid for consume/drain
w_ecs_define_component(uint64_t, stream_input_buffer_length, 0);
w_ecs_define_component(uint64_t, stream_output_buffer_length, 0);

// these tags are set when input/output bytes are available
w_ecs_define_tag(stream_input_available);
w_ecs_define_tag(stream_output_available);

// lifecycle tags allow allocating/freeing the input/output buffers
w_ecs_define_tag(req_stream_input_buffer_hot);
w_ecs_define_tag(req_stream_input_buffer_cold);
w_ecs_define_tag(req_stream_output_buffer_hot);
w_ecs_define_tag(req_stream_output_buffer_cold);


/************
*  macros  *
************/
#define wm_streams_get_input_buffer(world, entity) \
        wm_managed_alloc_resolve_handle((world),stream_input_buffer_handle, (entity))

#define wm_streams_get_output_buffer(world, entity) \
        wm_managed_alloc_resolve_handle((world),stream_output_buffer_handle, (entity))


// initialize the streams module
void wm_streams_init(struct w_ecs_world *world);

// cleanup the streams module
void wm_streams_free(struct w_ecs_world *world);

#endif /* WHISKER_STREAMS_H */
