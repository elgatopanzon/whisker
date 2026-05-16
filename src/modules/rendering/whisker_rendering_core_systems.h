/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_core_systems
 * @created     : Friday May 01, 2026 13:29:31 CST
 * @description : core rendering systems
 */

#include "whisker_components.h"
#include "whisker_rendering.h"
#include "whisker_rendering_commands.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_RENDERING_CORE_SYSTEMS_H
#define WHISKER_RENDERING_CORE_SYSTEMS_H

w_ecs_simple_system(
	w_rendering_core_flush_render_buffer_commands,
	WM_PHASE_FINAL_RENDER,
{
	// render dispatch buffer
	struct w_dispatch_buffer *render_buffer = w_rendering_get_render_dispatch_buffer(world);
	w_dispatch_buffer_sort(render_buffer, w_dispatch_buffer_compare_by_priority);

	void *render_cmd_payload_ptr;
	struct w_dispatch_entry *dispatch_entry;
	while ((dispatch_entry = w_dispatch_buffer_pop(render_buffer, &render_cmd_payload_ptr)))
	{
		enum W_RENDERING_CMD render_cmd = dispatch_entry->type_id;

		// trigger handler for each buffered command
		w_dispatch_buffer_run_handler(render_buffer, render_cmd, world, render_cmd_payload_ptr);
	}

	w_dispatch_buffer_clear(render_buffer);
});


w_ecs_simple_system(
	w_rendering_core_dispatch_render_state_sync, 
	WM_RENDER_PHASE_ON_SYNC,
{
	w_rendering_dispatch_render_cmd_no_payload(
		W_RENDERING_CMD_RENDER_STATE_SYNC,
		phase_priority
	);
});


w_ecs_simple_system(
	w_rendering_core_sync_transform_components,
	WM_RENDER_PHASE_PRE_SYNC,
{
	w_sync(position_3d, render_position_3d);
	w_sync(rotation_3d, render_rotation_3d);
	w_sync(scale_3d, render_scale_3d);
	w_sync(origin_3d, render_origin_3d);

	w_sync(position_2d, render_position_2d);
	w_sync(rotation_2d, render_rotation_2d);
	w_sync(scale_2d, render_scale_2d);
	w_sync(origin_2d, render_origin_2d);
});

#endif /* WHISKER_RENDERING_CORE_SYSTEMS_H */

