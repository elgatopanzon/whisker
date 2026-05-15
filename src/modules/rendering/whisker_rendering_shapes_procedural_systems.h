/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_shapes_procedural_systems
 * @created     : Tuesday May 05, 2026 15:47:19 CST
 * @description : procedurally generated shape systems
 */

#include "whisker_components.h"
#include "whisker_rendering.h"
#include "whisker_rendering_commands.h"
#include "whisker_rendering_shapes.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_RENDERING_SHAPES_PROCEDURAL_SYSTEMS_H
#define WHISKER_RENDERING_SHAPES_PROCEDURAL_SYSTEMS_H

w_ecs_system(
	w_rendering_shapes_procedural_dispatch_draw_grid,
	WM_RENDER_PHASE_PRE_WORLD,
	w_query(
		w_query_r(grid),
		w_query_r(thickness),
		w_query_r(color),
		w_query_r(render_position_3d),
		w_query_r(render_rotation_3d),
		w_query_r(render_scale_3d),
		w_query_r(render_origin_3d),
		w_query_r(render_layer),
		w_query_n(hidden),
	),
{
	struct w_rendering_cmd_draw_verts cmd = {0};

	cmd.position = *w_query_get(render_position_3d);
	cmd.rotation = *w_query_get(render_rotation_3d);
	cmd.scale = *w_query_get(render_scale_3d);
	cmd.origin = *w_query_get(render_origin_3d);
	cmd.color = *w_query_get(color);
	cmd.scale = w_vec3_mul(cmd.scale, w_shape_vert_scale);

	cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_LINES;

	w_vec2 grid_size = *w_query_get(grid);

	int cells_x = grid_size.x;
	int cells_z = grid_size.y;

	float thickness_ = *w_query_get(thickness);

	// decide number of lines from thickness
	int thickness_lines = (thickness_) * 3;
	thickness_ /= 1000; // convert to smaller width (1.0 = 0.001)

	float line_offset = thickness_ / (thickness_lines - 1);
	float half_thickness = thickness_ / 2.0f;

	// calculate total verts: 2 per line * thickness_lines sub-lines per grid line
	int total_grid_lines = (cells_x + 1) + (cells_z + 1);
	int total_verts = total_grid_lines * thickness_lines * 2;

	// allocate from frame arena
	w_vec3 *grid_verts = w_ecs_frame_malloc(world, total_verts * sizeof(w_vec3));
	int v = 0;

	// use w_shape_vert_range for normalization bounds
	float vert_lo = (float)w_shape_vert_range[0];
	float vert_hi = (float)w_shape_vert_range[1];
	float vert_span = vert_hi - vert_lo;

	// vertical lines (span Z) - normalized to vert range
	for (int i = 0; i <= cells_x; i++) {
    	float x_base = (float)i / cells_x * vert_span + vert_lo;
    	for (int t = 0; t < thickness_lines; t++) {
        	float x = x_base - half_thickness + t * line_offset;
        	grid_verts[v++] = ((w_vec3){x, 0, vert_lo});
        	grid_verts[v++] = ((w_vec3){x, 0, vert_hi});
    	}
	}

	// horizontal lines (span X) - normalized to vert range
	for (int i = 0; i <= cells_z; i++) {
    	float z_base = (float)i / cells_z * vert_span + vert_lo;
    	for (int t = 0; t < thickness_lines; t++) {
        	float z = z_base - half_thickness + t * line_offset;
        	grid_verts[v++] = ((w_vec3){vert_lo, 0, z});
        	grid_verts[v++] = ((w_vec3){vert_hi, 0, z});
    	}
	}

	cmd.verts = grid_verts;
	cmd.verts_length = total_verts;

	// single dispatch for entire grid
	w_rendering_dispatch_render_cmd(
			W_RENDERING_CMD_DRAW_VERTS, 
			w_ecs_render_layer(*w_query_get(render_layer)), 
			&cmd
			);
	});


w_ecs_system(
	w_rendering_shapes_procedural_dispatch_draw_circle,
	WM_RENDER_PHASE_PRE_WORLD,
	w_query(
		// transform
		w_query_r(render_position_3d),
		w_query_r(render_rotation_3d),
		w_query_r(render_scale_3d),
		w_query_r(render_origin_3d),

		// required render components
		w_query_r(render_layer),
		w_query_n(hidden),

		// required shape components
		w_query_r(circle),
		w_query_r(color),

		// optional shape components
		w_query_o(shape_angle_start),
		w_query_o(shape_angle_end),
		w_query_o(shape_segments),
		w_query_o(shape_outline_color),
	),
{
	float diameter = *w_query_get(circle);
	float start_rad = *w_query_get_opt_or_default(shape_angle_start) * W_DEG2RAD;
	float end_rad = *w_query_get_opt_or_default(shape_angle_end) * W_DEG2RAD;
	int segments = *w_query_get_opt_or_default(shape_segments);

	w_color8 shape_color = *w_query_get(color);
	w_color8 outline_color = *w_query_get_opt_or_default(shape_outline_color);

	// first ensure the params are valid for generation
	if (!w_rendering_shape_sanitize_circle_params(&diameter, &start_rad, &end_rad, &segments))
    	continue;  // degenerate, skip

	// set default verts arrays and lengths
	w_vec3 *circle_verts = NULL;
	int circle_verts_count = 0;

	w_vec3 *circle_outline_verts = NULL;
	int circle_outline_verts_count = 0;

	// generate verts array for the shape
	if (shape_color.a > 0)
	{
		circle_verts_count = W_CIRCLE_VERTS_COUNT(segments);
		circle_verts = w_ecs_frame_malloc(world, circle_verts_count * sizeof(w_vec3));
		circle_verts_count = w_rendering_shape_generate_circle_verts(circle_verts, 0, segments, start_rad, end_rad, ((w_vec3){0,0,0}), true);
	}

	// generate verts array for just the outline as vec3 lines  
	if (outline_color.a > 0)
	{
    	// arc segments + 2 radial lines for partial circles
    	circle_outline_verts_count = W_CIRCLE_OUTLINE_VERTS_COUNT(segments, false);
    	circle_outline_verts = w_ecs_frame_malloc(world, circle_outline_verts_count * sizeof(w_vec3));
    	
    	// count is set and respects full/not full circle so extra memory doesn't matter
		circle_outline_verts_count = w_rendering_shape_generate_circle_outline_verts(circle_outline_verts, 0, segments, start_rad, end_rad, ((w_vec3){0,0,0}));
    }

	// prepare draw command
	struct w_rendering_cmd_draw_verts cmd = {0};

	cmd.position = *w_query_get(render_position_3d);
	cmd.rotation = *w_query_get(render_rotation_3d);
	cmd.scale = *w_query_get(render_scale_3d);
	cmd.origin = *w_query_get(render_origin_3d);
	cmd.scale = w_vec3_mul(cmd.scale, ((w_vec3){ diameter * w_shape_vert_scale.x, diameter * w_shape_vert_scale.y, diameter * w_shape_vert_scale.z}));

	if (shape_color.a > 0)
	{
		cmd.color = shape_color;
		cmd.verts = circle_verts;
		cmd.verts_length = circle_verts_count;
		cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_TRIANGLES;

		w_rendering_dispatch_render_cmd(
			W_RENDERING_CMD_DRAW_VERTS, 
			w_ecs_render_layer(*w_query_get(render_layer)), 
			&cmd
		);
	}

	if (outline_color.a > 0)
	{
		cmd.color = outline_color;
		cmd.verts = circle_outline_verts;
		cmd.verts_length = circle_outline_verts_count;
		cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_LINES;

		w_rendering_dispatch_render_cmd(
			W_RENDERING_CMD_DRAW_VERTS, 
			w_ecs_render_layer(*w_query_get(render_layer)), 
			&cmd
		);
	}
});

w_ecs_system(
	w_rendering_shapes_procedural_dispatch_draw_cylinder,
	WM_RENDER_PHASE_PRE_WORLD,
	w_query(
		// transform
		w_query_r(render_position_3d),
		w_query_r(render_rotation_3d),
		w_query_r(render_scale_3d),
		w_query_r(render_origin_3d),

		// required render components
		w_query_r(render_layer),
		w_query_n(hidden),

		// required shape components
		w_query_r(cylinder),
		w_query_r(color),

		// optional shape components
		w_query_o(shape_diameter_top),
		w_query_o(shape_diameter_bottom),
		w_query_o(shape_segments),
		w_query_o(shape_outline_color),
	),
{
	float height = *w_query_get(cylinder);
	int segments = *w_query_get_opt_or_default(shape_segments);
	float diameter_top = *w_query_get_opt_or_default(shape_diameter_top);
	float diameter_bottom = *w_query_get_opt_or_default(shape_diameter_bottom);

	w_color8 shape_color = *w_query_get(color);
	w_color8 outline_color = *w_query_get_opt_or_default(shape_outline_color);

	float start_rad = 0 * W_DEG2RAD;
	float end_rad = 360 * W_DEG2RAD;
	
	// calculate and create verts buffer
	int verts_count = 0;
	// add to verts_count calculation
	verts_count += W_CYLINDER_SIDES_VERTS_COUNT(segments);
	if (diameter_top > 0) verts_count += W_CYLINDER_CAP_VERTS_COUNT(segments);
	if (diameter_bottom > 0) verts_count += W_CYLINDER_CAP_VERTS_COUNT(segments);
	w_vec3 *verts = w_ecs_frame_malloc(world, verts_count * sizeof(w_vec3));

	int outline_verts_count = 0;
	if (diameter_top > 0) outline_verts_count += W_CIRCLE_OUTLINE_VERTS_COUNT(segments, true);
	if (diameter_bottom > 0) outline_verts_count += W_CIRCLE_OUTLINE_VERTS_COUNT(segments, true);
	w_vec3 *outline_verts = w_ecs_frame_malloc(world, outline_verts_count * sizeof(w_vec3));

	int verts_offset = 0;
	int outline_verts_offset = 0;

	// generate top and bottom circles if required
	// top
	if (diameter_top > 0) {
		verts_offset += w_rendering_shape_generate_circle_verts(verts, verts_offset, segments, start_rad, end_rad, ((w_vec3){0.0f, height * 0.5, 0.0f}), true);

		outline_verts_offset += w_rendering_shape_generate_circle_outline_verts(outline_verts, outline_verts_offset, segments, start_rad, end_rad, ((w_vec3){0.0f, height * 0.5, 0.0f}));

		// apply diameter offsets
		w_rendering_shape_apply_vert_scale(verts, verts_offset - W_CIRCLE_VERTS_COUNT(segments), verts_offset, ((w_vec3){diameter_top, 1.0f, diameter_top}));
		w_rendering_shape_apply_vert_scale(outline_verts, outline_verts_offset - W_CIRCLE_OUTLINE_VERTS_COUNT(segments, true), outline_verts_offset, ((w_vec3){diameter_top, 1.0f, diameter_top}));
	}

	// sides
	// generate sides (after caps)
	verts_offset += w_rendering_shape_generate_cylinder_sides_verts(verts, verts_offset, segments, diameter_top, diameter_bottom, height * 0.5f);

	// bottom
	if (diameter_bottom > 0) {
		verts_offset += w_rendering_shape_generate_circle_verts(verts, verts_offset, segments, start_rad, end_rad, ((w_vec3){0.0f, -(height * 0.5), 0.0f}), false);

		outline_verts_offset += w_rendering_shape_generate_circle_outline_verts(outline_verts, outline_verts_offset, segments, start_rad, end_rad, ((w_vec3){0.0f, -(height * 0.5), 0.0f}));

		// apply diameter offsets
		w_rendering_shape_apply_vert_scale(verts, verts_offset - W_CIRCLE_VERTS_COUNT(segments), verts_offset, ((w_vec3){diameter_bottom, 1.0f, diameter_bottom}));
		w_rendering_shape_apply_vert_scale(outline_verts, outline_verts_offset - W_CIRCLE_OUTLINE_VERTS_COUNT(segments, true), outline_verts_offset, ((w_vec3){diameter_bottom, 1.0f, diameter_bottom}));
	}

	// prepare draw command
	struct w_rendering_cmd_draw_verts cmd = {0};

	cmd.position = *w_query_get(render_position_3d);
	cmd.rotation = *w_query_get(render_rotation_3d);
	cmd.scale = *w_query_get(render_scale_3d);
	cmd.origin = *w_query_get(render_origin_3d);
	cmd.scale = w_vec3_mul(cmd.scale, ((w_vec3){ 1.0f * w_shape_vert_scale.x, 1.0f * w_shape_vert_scale.y, 1.0f * w_shape_vert_scale.z}));

	if (shape_color.a > 0)
	{
		cmd.color = shape_color;
		cmd.verts = verts;
		cmd.verts_length = verts_count;
		cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_TRIANGLES;

		w_rendering_dispatch_render_cmd(
			W_RENDERING_CMD_DRAW_VERTS, 
			w_ecs_render_layer(*w_query_get(render_layer)), 
			&cmd
		);
	}

	if (outline_color.a > 0)
	{
		cmd.color = outline_color;
		cmd.verts = outline_verts;
		cmd.verts_length = outline_verts_count;
		cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_LINES;

		w_rendering_dispatch_render_cmd(
			W_RENDERING_CMD_DRAW_VERTS, 
			w_ecs_render_layer(*w_query_get(render_layer)), 
			&cmd
		);
	}
});

w_ecs_system(
	w_rendering_shapes_procedural_dispatch_draw_polygon,
	WM_RENDER_PHASE_PRE_WORLD,
	w_query(
		// transform
		w_query_r(render_position_3d),
		w_query_r(render_rotation_3d),
		w_query_r(render_scale_3d),
		w_query_r(render_origin_3d),

		// required render components
		w_query_r(render_layer),
		w_query_n(hidden),

		// required shape components
		w_query_r(polygon),
		w_query_r(color),

		// optional shape components
		w_query_o(shape_outline_color),
	),
{
	// polygon relies on the circle generator with limited exposed components
	float diameter = 1.0f;
	float start_rad = 0;
	float end_rad = 360 * W_DEG2RAD;
	int segments = *w_query_get(polygon);

	// ensure flat side is lined up X bottom
	float rotation_offset = (W_PI / segments) + (90 * W_DEG2RAD);
	start_rad = rotation_offset;
	end_rad = rotation_offset + 2 * W_PI;

	w_color8 shape_color = *w_query_get(color);
	w_color8 outline_color = *w_query_get_opt_or_default(shape_outline_color);

	// set default verts arrays and lengths
	w_vec3 *circle_verts = NULL;
	int circle_verts_count = 0;

	w_vec3 *circle_outline_verts = NULL;
	int circle_outline_verts_count = 0;

	// generate verts array for the shape
	if (shape_color.a > 0)
	{
		circle_verts_count = W_CIRCLE_VERTS_COUNT(segments);
		circle_verts = w_ecs_frame_malloc(world, circle_verts_count * sizeof(w_vec3));
		circle_verts_count = w_rendering_shape_generate_circle_verts(circle_verts, 0, segments, start_rad, end_rad, ((w_vec3){0,0,0}), true);
	}

	// generate verts array for just the outline as vec3 lines  
	if (outline_color.a > 0)
	{
    	// arc segments + 2 radial lines for partial circles
    	circle_outline_verts_count = W_CIRCLE_OUTLINE_VERTS_COUNT(segments, false);
    	circle_outline_verts = w_ecs_frame_malloc(world, circle_outline_verts_count * sizeof(w_vec3));
    	
    	// count is set and respects full/not full circle so extra memory doesn't matter
		circle_outline_verts_count = w_rendering_shape_generate_circle_outline_verts(circle_outline_verts, 0, segments, start_rad, end_rad, ((w_vec3){0,0,0}));
    }

	// prepare draw command
	struct w_rendering_cmd_draw_verts cmd = {0};

	cmd.position = *w_query_get(render_position_3d);
	cmd.rotation = *w_query_get(render_rotation_3d);
	cmd.scale = *w_query_get(render_scale_3d);
	cmd.origin = *w_query_get(render_origin_3d);
	cmd.scale = w_vec3_mul(cmd.scale, ((w_vec3){ diameter * w_shape_vert_scale.x, diameter * w_shape_vert_scale.y, diameter * w_shape_vert_scale.z}));

	if (shape_color.a > 0)
	{
		cmd.color = shape_color;
		cmd.verts = circle_verts;
		cmd.verts_length = circle_verts_count;
		cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_TRIANGLES;

		w_rendering_dispatch_render_cmd(
			W_RENDERING_CMD_DRAW_VERTS, 
			w_ecs_render_layer(*w_query_get(render_layer)), 
			&cmd
		);
	}

	if (outline_color.a > 0)
	{
		cmd.color = outline_color;
		cmd.verts = circle_outline_verts;
		cmd.verts_length = circle_outline_verts_count;
		cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_LINES;

		w_rendering_dispatch_render_cmd(
			W_RENDERING_CMD_DRAW_VERTS, 
			w_ecs_render_layer(*w_query_get(render_layer)), 
			&cmd
		);
	}
});

#endif /* WHISKER_RENDERING_SHAPES_PROCEDURAL_SYSTEMS_H */

