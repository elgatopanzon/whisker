/**
 * @author      : ElGatoPanzon
 * @file        : whisker_streams
 * @created     : Wednesday May 27, 2026 12:04:15 CST
 * @description : Module provide input/output byte stream management
 */

#include "whisker_streams.h"
#include "whisker_streams_systems.h"

void wm_streams_init(struct w_ecs_world *world)
{
	struct w_scheduler_phase phase_stream_prepare = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "STREAM_PREPARE"};
	struct w_scheduler_phase phase_stream_post = {.enabled = true, .time_step_id = WM_TIMESTEP_POST, .name = "STREAM_POST"};

	w_ecs_register_system_phase_at(world, &phase_stream_prepare, WM_STREAM_PHASE_PREPARE);
	w_ecs_register_system_phase_at(world, &phase_stream_post, WM_STREAM_PHASE_POST);

	// assign phases order
	w_ecs_set_phase_chain(world,
		WM_PHASE_PRE_LOAD,
		WM_STREAM_PHASE_PREPARE,
	);

	w_ecs_set_phase_chain(world,
		WM_PHASE_POST,
		WM_STREAM_PHASE_POST,
	);

	// register systems
	wm_streams_handle_request_hot_register(world);
	wm_streams_handle_request_cold_register(world);
	wm_streams_auto_compact_register(world);
}

void wm_streams_free(struct w_ecs_world *world)
{
	(void)world;
}
