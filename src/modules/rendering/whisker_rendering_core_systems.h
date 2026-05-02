/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_core_systems
 * @created     : Friday May 01, 2026 13:29:31 CST
 * @description : core rendering systems
 */

#include "whisker_rendering.h"
#include "whisker_rendering_commands.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_RENDERING_CORE_SYSTEMS_H
#define WHISKER_RENDERING_CORE_SYSTEMS_H

w_ecs_simple_system(
	w_rendering_core_flush_render_dispatch_buffer_dummy,
	WM_PHASE_POST,
{
	struct w_dispatch_buffer *render_buffer = w_rendering_get_render_dispatch_buffer(world);

	// this dummy system simply fakes consuming the render buffer in the case
	// that there is no render module consuming it.
	// when there's a real render module, its already consumed by this point
	w_dispatch_buffer_clear(render_buffer);
});


w_ecs_simple_system(
	w_rendering_core_dispatch_render_state_sync, 
	WM_RENDER_PHASE_ON_SYNC,
{
	w_rendering_dispatch_render_cmd_no_payload(
		W_RENDERING_CMD_RENDER_STATE_SYNC
	);
});

#endif /* WHISKER_RENDERING_CORE_SYSTEMS_H */

