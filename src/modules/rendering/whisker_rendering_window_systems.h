/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_window_systems
 * @created     : Friday May 01, 2026 14:13:06 CST
 * @description : window systems handle window lifecycle commands
 */

#include "whisker_rendering.h"
#include "whisker_rendering_commands.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_RENDERING_WINDOW_SYSTEMS_H
#define WHISKER_RENDERING_WINDOW_SYSTEMS_H

w_ecs_simple_system(
	w_rendering_window_dispatch_init_window,
	WM_PHASE_ON_STARTUP,
{
	struct w_rendering_display_config *display_config = w_rendering_get_display_config(world);

	char *window_title = w_string_table_lookup(world->string_table, display_config->window_title_id);

	if (!window_title)
	{
		window_title = "Whisker (no window title)";
	}

	w_rendering_dispatch_render_cmd_value(
		W_RENDERING_CMD_INIT_WINDOW,
		struct w_rendering_cmd_init_window,
			.window_resolution = display_config->window_resolution, 
			.window_title = window_title
	);
});

w_ecs_simple_system(
	w_rendering_window_dispatch_close_window, 
	WM_PHASE_ON_SHUTDOWN,
{
	// currently no payload, upstream just implements this
	w_rendering_dispatch_render_cmd_no_payload(
		W_RENDERING_CMD_CLOSE_WINDOW
	);
});

w_ecs_simple_system(
	w_rendering_window_dispatch_handle_window_close, 
	WM_RENDER_PHASE_POST_DRAW,
{
	struct w_rendering_display_config *display_config = w_rendering_get_display_config(world);
	if (!display_config->handle_window_close)
		return;

	// currently no payload, upstream just implements this
	w_rendering_dispatch_render_cmd_no_payload(
		W_RENDERING_CMD_HANDLE_WINDOW_CLOSE
	);
});

#endif /* WHISKER_RENDERING_WINDOW_SYSTEMS_H */

