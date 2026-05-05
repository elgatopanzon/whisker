/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_shapes_systems
 * @created     : Thursday Apr 30, 2026 19:17:19 CST
 * @description : static primitive shape systems
 */

#include "whisker_components.h"
#include "whisker_rendering.h"
#include "whisker_rendering_commands.h"
#include "whisker_rendering_shapes.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_RENDERING_SHAPES_SYSTEMS_H
#define WHISKER_RENDERING_SHAPES_SYSTEMS_H

#define w_rendering_shapes_base_query \
		w_query_o(shape_outline_color), \
		w_query_r(color), \
		w_query_r(position_3d), \
		w_query_r(rotation_3d), \
		w_query_r(scale_3d), \
		w_query_r(origin_3d), \
		w_query_r(render_layer), \
		w_query_n(hidden) \

static inline void w_rendering_shapes_dispatch_draw_static_shape(
	struct w_ecs_world *world,
	int phase_priority,
	enum W_RENDERING_SHAPE_TYPE type,
	const w_color8 *color,
	const w_color8 *outline_color,
	const w_vec3 *position,
	const w_quat *rotation,
	const w_vec3 *scale,
	const w_vec3 *origin
) {
	struct w_rendering_cmd_draw_verts cmd = {0};

	cmd.position = *position;
	cmd.rotation = *rotation;
	cmd.scale = *scale;
	cmd.origin = *origin;

	const w_vec3 *verts_shape;
	const w_vec3 *verts_outline;
	size_t verts_shape_length;
	size_t verts_outline_length;

	switch (type) {
		case W_RENDERING_SHAPE_TYPE_RECT:
			verts_shape = w_shape_rect_tris;		
			verts_shape_length = W_SHAPE_RECT_TRIS_LEN;		
			verts_outline = w_shape_rect_lines;		
			verts_outline_length = W_SHAPE_RECT_LINES_LEN;		
			break;
		case W_RENDERING_SHAPE_TYPE_CUBE:
			verts_shape = w_shape_cube_tris;		
			verts_shape_length = W_SHAPE_CUBE_TRIS_LEN;		
			verts_outline = w_shape_cube_lines;		
			verts_outline_length = W_SHAPE_CUBE_LINES_LEN;		
			break;
		default:
			return;
			break;
	}

	// draw shape verts
	if (color && color->a > 0)
	{
		cmd.color = *color;
		cmd.verts = verts_shape;
		cmd.verts_length = verts_shape_length;
		cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_TRIANGLES;

		w_rendering_dispatch_render_cmd(
			W_RENDERING_CMD_DRAW_VERTS, 
			phase_priority,
			&cmd
		);
	}
	// draw shape outline verts
	if (outline_color && outline_color->a > 0)
	{
		cmd.color = *outline_color;
		cmd.verts = verts_outline;
		cmd.verts_length = verts_outline_length;
		cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_LINES;

		w_rendering_dispatch_render_cmd(
			W_RENDERING_CMD_DRAW_VERTS, 
			phase_priority,
			&cmd
		);
	}
}


/*************************
*  WORLD phase systems  *
*************************/

w_ecs_system(
	w_rendering_shapes_dispatch_draw_rects,
	WM_PHASE_PRE_RENDER,
	w_query(
		w_rendering_shapes_base_query,
		w_query_h(shape_type_rect),
	),
{
	w_rendering_shapes_dispatch_draw_static_shape(
		world,
		w_ecs_render_layer(*w_query_get(render_layer)),
		W_RENDERING_SHAPE_TYPE_RECT,
		w_query_get(color),
		w_query_get_opt(shape_outline_color),
		w_query_get(position_3d),
		w_query_get(rotation_3d),
		w_query_get(scale_3d),
		w_query_get(origin_3d)
	);
});

w_ecs_system(
	w_rendering_shapes_dispatch_draw_cubes,
	WM_PHASE_PRE_RENDER,
	w_query(
		w_rendering_shapes_base_query,
		w_query_r(shape_type_cube),
	),
{
	w_rendering_shapes_dispatch_draw_static_shape(
		world,
		w_ecs_render_layer(*w_query_get(render_layer)),
		W_RENDERING_SHAPE_TYPE_CUBE,
		w_query_get(color),
		w_query_get(shape_outline_color),
		w_query_get(position_3d),
		w_query_get(rotation_3d),
		w_query_get(scale_3d),
		w_query_get(origin_3d)
	);
});

#endif /* WHISKER_RENDERING_SHAPES_SYSTEMS_H */

