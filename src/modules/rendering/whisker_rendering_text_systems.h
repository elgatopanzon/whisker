/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_text_systems
 * @created     : Wednesday May 13, 2026 16:45:15 CST
 * @description : systems to dispatch text draw commands
 */

#include "whisker.h"
#include "whisker_components.h"
#include "whisker_rendering.h"
#include "whisker_rendering_components.h"
#include "whisker_rendering_commands.h"

#ifndef WHISKER_RENDERING_TEXT_SYSTEMS_H
#define WHISKER_RENDERING_TEXT_SYSTEMS_H

w_ecs_system(
	w_rendering_text_dispatch_draw_text,
	WM_RENDER_PHASE_PRE_WORLD,
	w_query(
		// transform
		w_query_r(render_position_3d),
		w_query_r(render_rotation_3d),
		w_query_r(render_scale_3d),
		w_query_r(render_origin_3d),

		// required layer
		w_query_r(render_layer),
		w_query_n(hidden),

		// required text components
		w_query_r(string),
		w_query_r(font_size),

		// optional text components
		w_query_o(font_spacing),
		w_query_o(font_line_height),
		w_query_o(font_color),
		w_query_o(font_shadow),
		w_query_o(font_shadow_color),
		w_query_o(font_alignment),
		w_query_o(font_max_width),
	),
{
	w_rendering_dispatch_render_cmd_value(W_RENDERING_CMD_DRAW_TEXT, w_ecs_render_layer(*w_query_get(render_layer)), struct w_rendering_cmd_draw_text,
		.text = *w_query_get(string),
		.position = *w_query_get(render_position_3d),
		.scale = *w_query_get(render_scale_3d),
		.origin = *w_query_get(render_origin_3d),
		.rotation = *w_query_get(render_rotation_3d),
		.font_size = *w_query_get(font_size),
		.font_line_height = *w_query_get_opt_or_default(font_line_height),
		.font_spacing = *w_query_get_opt_or_default(font_spacing),
		.font_color = *w_query_get_opt_or_default(font_color),
		.font_shadow = *w_query_get_opt_or_default(font_shadow),
		.font_shadow_color = *w_query_get_opt_or_default(font_shadow_color),
		.font_alignment = *w_query_get_opt_or_default(font_alignment),
		.max_width = *w_query_get_opt_or_default(font_max_width),
    );
});

#endif /* WHISKER_RENDERING_TEXT_SYSTEMS_H */

