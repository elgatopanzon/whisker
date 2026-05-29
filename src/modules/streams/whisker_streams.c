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
	// register custom phases
	struct w_scheduler_phase phase_stream_input_prepare = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "STREAM_INPUT_PRE_CONSUME"};
	struct w_scheduler_phase phase_stream_input_consume = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "STREAM_INPUT_ON_CONSUME"};
	struct w_scheduler_phase phase_stream_input_compact = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "STREAM_INPUT_POST_CONSUME"};

	struct w_scheduler_phase phase_stream_output_prepare = {.enabled = true, .time_step_id = WM_TIMESTEP_POST, .name = "STREAM_OUTPUT_PRE_CONSUME"};
	struct w_scheduler_phase phase_stream_output_drain = {.enabled = true, .time_step_id = WM_TIMESTEP_POST, .name = "STREAM_OUTPUT_ON_CONSUME"};
	struct w_scheduler_phase phase_stream_output_compact = {.enabled = true, .time_step_id = WM_TIMESTEP_POST, .name = "STREAM_OUTPUT_POST_CONSUME"};

	w_ecs_register_system_phase_at(world, &phase_stream_input_prepare, WM_STREAM_PHASE_INPUT_PRE_CONSUME);
	w_ecs_register_system_phase_at(world, &phase_stream_input_consume, WM_STREAM_PHASE_INPUT_ON_CONSUME);
	w_ecs_register_system_phase_at(world, &phase_stream_input_compact, WM_STREAM_PHASE_INPUT_POST_CONSUME);

	w_ecs_register_system_phase_at(world, &phase_stream_output_prepare, WM_STREAM_PHASE_OUTPUT_PRE_CONSUME);
	w_ecs_register_system_phase_at(world, &phase_stream_output_drain, WM_STREAM_PHASE_OUTPUT_ON_CONSUME);
	w_ecs_register_system_phase_at(world, &phase_stream_output_compact, WM_STREAM_PHASE_OUTPUT_POST_CONSUME);

	// assign phases order
	w_ecs_set_phase_chain(world,
		WM_PHASE_PRE_LOAD,
		WM_STREAM_PHASE_INPUT_PRE_CONSUME,
	);

	w_ecs_set_phase_chain(world,
		WM_PHASE_ON_LOAD,
		WM_STREAM_PHASE_INPUT_ON_CONSUME,
	);

	w_ecs_set_phase_chain(world,
		WM_PHASE_POST_LOAD,
		WM_STREAM_PHASE_INPUT_POST_CONSUME,
	);

	w_ecs_set_phase_chain(world,
		WM_PHASE_POST,
		WM_STREAM_PHASE_OUTPUT_PRE_CONSUME,
		WM_STREAM_PHASE_OUTPUT_ON_CONSUME,
		WM_STREAM_PHASE_OUTPUT_POST_CONSUME,
	);

	// register systems
	wm_streams_handle_request_input_buffer_hot_register(world);
	wm_streams_handle_request_output_buffer_hot_register(world);
	wm_streams_handle_request_input_buffer_cold_register(world);
	wm_streams_handle_request_output_buffer_cold_register(world);
	wm_streams_compact_input_buffer_register(world);
	wm_streams_compact_output_buffer_register(world);
}

void wm_streams_free(struct w_ecs_world *world)
{
	(void)world;
}
