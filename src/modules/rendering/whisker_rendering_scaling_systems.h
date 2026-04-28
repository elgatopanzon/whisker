/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_scaling
 * @created     : Monday Apr 06, 2026 15:22:24 CST
 */

#include "whisker_std.h"
#include "whisker_rendering.h"

// prepare the source and destination rendering rects
w_ecs_simple_system(whisker_rendering_scaling_rect_init, WM_RENDER_PHASE_PRE_SCALE, {
	struct w_rendering_render_config *render_config = w_rendering_get_render_config(world);
	struct w_rendering_render_state *render_state = w_rendering_get_render_state(world);

	w_rect source;
	source.x = 0;
	source.y = 0;
	source.width = render_config->render_resolution.x;
	source.height = render_config->render_resolution.y;

	w_rect dest;
	dest.x = 0;
	dest.y = 0;
	dest.width = source.width;
	dest.height = source.height;

	render_state->render_texture_source = source;
	render_state->render_texture_destination = dest;
});

// stretch scaling: destination matches window
w_ecs_simple_system(whisker_rendering_scaling_stretch, WM_RENDER_PHASE_ON_SCALE, {
	struct w_rendering_render_config *render_config = w_rendering_get_render_config(world);
	if (render_config->scaling_mode != W_RENDERING_SCALING_STRETCH) return;

	struct w_rendering_render_state *render_state = w_rendering_get_render_state(world);

	// set destination to window 1:1
	render_state->render_texture_destination.x = 0;
	render_state->render_texture_destination.y = 0;
	render_state->render_texture_destination.width = render_state->window_resolution.x;
	render_state->render_texture_destination.height = render_state->window_resolution.y;
});


// fit scaling: maintain aspect ratio with letterbox/pillarbox
w_ecs_simple_system(whisker_rendering_scaling_fit, WM_RENDER_PHASE_ON_SCALE, {
	struct w_rendering_render_config *render_config = w_rendering_get_render_config(world);
	if (render_config->scaling_mode != W_RENDERING_SCALING_FIT) return;

	struct w_rendering_render_state *render_state = w_rendering_get_render_state(world);
	w_rect source = render_state->render_texture_source;
	w_rect dest = render_state->render_texture_source;
	w_vec2 window = render_state->window_resolution;

	// calculate letterbox/pillarbox destination via scale
	float scale = fminf(window.x / source.width, window.y / source.height);
	render_state->render_texture_destination.width = (int)(source.width * scale);
	render_state->render_texture_destination.height = (int)(source.height * scale);
	render_state->render_texture_destination.x = (window.x - render_state->render_texture_destination.width) / 2;
	render_state->render_texture_destination.y = (window.y - render_state->render_texture_destination.height) / 2;
});

// integer scaling: scale up in multiples
w_ecs_simple_system(whisker_rendering_scaling_integer, WM_RENDER_PHASE_ON_SCALE, {
	struct w_rendering_render_config *render_config = w_rendering_get_render_config(world);
	if (render_config->scaling_mode != W_RENDERING_SCALING_INTEGER) return;

	struct w_rendering_render_state *render_state = w_rendering_get_render_state(world);
	w_rect source = render_state->render_texture_source;
	w_rect dest = render_state->render_texture_source;
	w_vec2 window = render_state->window_resolution;

	// clamp scaling factor to integer values
	int scale = (int)floorf(fminf(window.x / source.width, window.y / source.height));
	if (scale < 1) scale = 1;
	render_state->render_texture_destination.width = (int)(source.width * scale);
	render_state->render_texture_destination.height = (int)(source.height * scale);
	render_state->render_texture_destination.x = (window.x - render_state->render_texture_destination.width) / 2;
	render_state->render_texture_destination.y = (window.y - render_state->render_texture_destination.height) / 2;
});
