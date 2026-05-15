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
		w_query_o(outline_color), \
		w_query_r(color), \
		w_query_r(render_position_3d), \
		w_query_r(render_rotation_3d), \
		w_query_r(render_scale_3d), \
		w_query_r(render_origin_3d), \
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
		case W_RENDERING_SHAPE_TYPE_POINT:
			verts_shape = w_shape_point_cross_verts;		
			verts_shape_length = W_SHAPE_POINT_CROSS_VERTS_LEN;		
			verts_outline = NULL;		
			verts_outline_length = 0;		
			cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_LINES;
			break;
		case W_RENDERING_SHAPE_TYPE_LINE:
			verts_shape = w_shape_line_lines;		
			verts_shape_length = W_SHAPE_LINE_LINES_LEN;		
			verts_outline = NULL;		
			verts_outline_length = 0;		
			cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_LINES;
			break;
		case W_RENDERING_SHAPE_TYPE_TRIANGLE:
			verts_shape = w_shape_triangle_tris;		
			verts_shape_length = W_SHAPE_TRIANGLE_TRIS_LEN;		
			verts_outline = NULL;		
			verts_outline_length = 0;		
			cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_TRIANGLES;
			break;
		case W_RENDERING_SHAPE_TYPE_RECT:
			verts_shape = w_shape_rect_tris;		
			verts_shape_length = W_SHAPE_RECT_TRIS_LEN;		
			verts_outline = w_shape_rect_lines;		
			verts_outline_length = W_SHAPE_RECT_LINES_LEN;		
			cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_TRIANGLES;
			break;
		case W_RENDERING_SHAPE_TYPE_CUBE:
			verts_shape = w_shape_cube_tris;		
			verts_shape_length = W_SHAPE_CUBE_TRIS_LEN;		
			verts_outline = w_shape_cube_lines;		
			verts_outline_length = W_SHAPE_CUBE_LINES_LEN;		
			cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_TRIANGLES;
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


/****************************
*  shape dispatch systems  *
****************************/

w_ecs_system(
	w_rendering_shapes_dispatch_draw_points,
	WM_RENDER_PHASE_PRE_WORLD,
	w_query(
		w_rendering_shapes_base_query,
		w_query_h(point),
	),
{
	float point_s = *w_query_get(point);
	w_vec3 scale = w_vec3_mul(((w_vec3){point_s, point_s, point_s}), *w_query_get(render_scale_3d));
	scale = w_vec3_mul(scale, w_shape_vert_scale);
	w_rendering_shapes_dispatch_draw_static_shape(
		world,
		w_ecs_render_layer(*w_query_get(render_layer)),
		W_RENDERING_SHAPE_TYPE_POINT,
		w_query_get(color),
		w_query_get_opt(outline_color),
		w_query_get(render_position_3d),
		w_query_get(render_rotation_3d),
		&scale,
		w_query_get(render_origin_3d)
	);
});

w_ecs_system(
	w_rendering_shapes_dispatch_draw_lines,
	WM_RENDER_PHASE_PRE_WORLD,
	w_query(
		w_rendering_shapes_base_query,
		w_query_h(line),
	),
{
	w_vec3 scale = *w_query_get(render_scale_3d);
	scale = w_vec3_mul(scale, w_shape_vert_scale);
	w_rendering_shapes_dispatch_draw_static_shape(
		world,
		w_ecs_render_layer(*w_query_get(render_layer)),
		W_RENDERING_SHAPE_TYPE_LINE,
		w_query_get(color),
		w_query_get_opt(outline_color),
		w_query_get(render_position_3d),
		w_query_get(render_rotation_3d),
		&scale,
		w_query_get(render_origin_3d)
	);
});

w_ecs_system(
	w_rendering_shapes_dispatch_draw_triangles,
	WM_RENDER_PHASE_PRE_WORLD,
	w_query(
		w_rendering_shapes_base_query,
		w_query_h(triangle),
	),
{
	w_vec3 scale = *w_query_get(render_scale_3d);
	scale = w_vec3_mul(scale, w_shape_vert_scale);
	w_rendering_shapes_dispatch_draw_static_shape(
		world,
		w_ecs_render_layer(*w_query_get(render_layer)),
		W_RENDERING_SHAPE_TYPE_TRIANGLE,
		w_query_get(color),
		w_query_get_opt(outline_color),
		w_query_get(render_position_3d),
		w_query_get(render_rotation_3d),
		&scale,
		w_query_get(render_origin_3d)
	);
});

w_ecs_system(
	w_rendering_shapes_dispatch_draw_rects,
	WM_RENDER_PHASE_PRE_WORLD,
	w_query(
		w_rendering_shapes_base_query,
		w_query_r(rectangle),
	),
{
	w_vec2 rect_s = *w_query_get(rectangle);
	w_vec3 scale = w_vec3_mul(((w_vec3){rect_s.x, 1.0f, rect_s.y}), *w_query_get(render_scale_3d));
	scale = w_vec3_mul(scale, w_shape_vert_scale);
	w_rendering_shapes_dispatch_draw_static_shape(
		world,
		w_ecs_render_layer(*w_query_get(render_layer)),
		W_RENDERING_SHAPE_TYPE_RECT,
		w_query_get(color),
		w_query_get_opt(outline_color),
		w_query_get(render_position_3d),
		w_query_get(render_rotation_3d),
		&scale,
		w_query_get(render_origin_3d)
	);
});

w_ecs_system(
	w_rendering_shapes_dispatch_draw_cubes,
	WM_RENDER_PHASE_PRE_WORLD,
	w_query(
		w_rendering_shapes_base_query,
		w_query_r(cube),
	),
{
	w_vec3 cube_s = *w_query_get(cube);
	w_vec3 scale = w_vec3_mul(cube_s, *w_query_get(render_scale_3d));
	scale = w_vec3_mul(scale, w_shape_vert_scale);
	w_rendering_shapes_dispatch_draw_static_shape(
		world,
		w_ecs_render_layer(*w_query_get(render_layer)),
		W_RENDERING_SHAPE_TYPE_CUBE,
		w_query_get(color),
		w_query_get(outline_color),
		w_query_get(render_position_3d),
		w_query_get(render_rotation_3d),
		&scale,
		w_query_get(render_origin_3d)
	);
});

#endif /* WHISKER_RENDERING_SHAPES_SYSTEMS_H */

