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
enum WM_STREAM_PHASE 
{ 
	// runs after PRE_LOAD so streams created by load/accept systems can be
	// allocated before the rest of the frame uses them.
	WM_STREAM_PHASE_PREPARE = 200,

	// runs at the end of the frame, after users have advanced offsets.
	WM_STREAM_PHASE_POST = 201,
};


/****************
*  components  *
****************/

#ifndef WM_STREAMS_BUFFER_SIZE
#define WM_STREAMS_BUFFER_SIZE 65534
#endif

w_ecs_define_component(uint64_t, stream_buffer_size, WM_STREAMS_BUFFER_SIZE);
w_ecs_define_component(uint64_t, stream_buffer_handle, WM_MANAGED_ALLOC_INVALID_HANDLE);

// offset defines the current position in the stream buffer to consume/drain
// from.
w_ecs_define_component(uint64_t, stream_buffer_offset, 0);

// length defines how much of the bytes are valid for consume/drain.
w_ecs_define_component(uint64_t, stream_buffer_length, 0);

// this tag is set when bytes are available.
w_ecs_define_tag(stream_available);

// opt into automatic end-of-frame buffer compaction.
w_ecs_define_tag(stream_auto_compact);

// lifecycle tags allow allocating/freeing the stream buffer.
w_ecs_define_tag(req_stream_buffer_hot);
w_ecs_define_tag(req_stream_buffer_cold);


/************
*  macros  *
************/
#define wm_streams_get_buffer(world, stream_entity) \
        wm_managed_alloc_resolve_handle((world), stream_buffer_handle, (stream_entity))


// initialize the streams module
void wm_streams_init(struct w_ecs_world *world);

// cleanup the streams module
void wm_streams_free(struct w_ecs_world *world);

#endif /* WHISKER_STREAMS_H */
