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
#include "whisker_rendering_components.h"
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
		w_query_o(shape_verts_handle),
		w_query_o(shape_verts_hash),
		w_query_o(shape_verts_count),
	),
{
	w_vec2 grid_size = *w_query_get(grid);
	float thickness_ = *w_query_get(thickness);

	struct w_rendering_cmd_draw_verts cmd = {0};

	cmd.position = *w_query_get(render_position_3d);
	cmd.rotation = *w_query_get(render_rotation_3d);
	cmd.scale = *w_query_get(render_scale_3d);
	cmd.origin = *w_query_get(render_origin_3d);
	cmd.color = *w_query_get(color);
	cmd.scale = w_vec3_mul(cmd.scale, w_shape_vert_scale);

	cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_LINES;

	w_vec3 *grid_verts = NULL;
	int total_verts = 0;

	uint64_t hash = w_hash(grid_size.x, grid_size.y, thickness_);

	// use cached allocation for verts
	if (hash == *w_query_get_opt_or_default(shape_verts_hash))
	{
		grid_verts = wm_managed_alloc_resolve_handle(world, shape_verts_handle, entity);
		total_verts = *w_query_get_opt_or_default(shape_verts_count);
	}
	// generate if hash changed
	else
	{
		debug_printf("regen grid for %d", entity);
		int cells_x = grid_size.x;
		int cells_z = grid_size.y;


		// decide number of lines from thickness
		int thickness_lines = (thickness_) * 3;
		thickness_ /= 1000; // convert to smaller width (1.0 = 0.001)

		float line_offset = thickness_ / (thickness_lines - 1);
		float half_thickness = thickness_ / 2.0f;

		// calculate total verts: 2 per line * thickness_lines sub-lines per grid line
		int total_grid_lines = (cells_x + 1) + (cells_z + 1);
		total_verts = total_grid_lines * thickness_lines * 2;

		// allocate from frame arena
		grid_verts = wm_managed_alloc_malloc(world, shape_verts_handle, entity, total_verts * sizeof(w_vec3));
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

		// set hash and vert count
		w_set_value(entity, shape_verts_count, total_verts);
		w_set_value(entity, shape_verts_hash, hash);
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
		w_query_o(angle_start),
		w_query_o(angle_end),
		w_query_o(segments),
		w_query_o(outline_color),
		w_query_o(shape_verts_handle),
		w_query_o(shape_verts_hash),
		w_query_o(shape_verts_count),
	),
{
	float diameter = *w_query_get(circle);
	float start_rad = *w_query_get_opt_or_default(angle_start) * W_DEG2RAD;
	float end_rad = *w_query_get_opt_or_default(angle_end) * W_DEG2RAD;
	int seg = *w_query_get_opt_or_default(segments);

	w_color8 shape_color = *w_query_get(color);
	w_color8 outline_col = *w_query_get_opt_or_default(outline_color);

	// first ensure the params are valid for generation
	if (!w_rendering_shape_sanitize_circle_params(&diameter, &start_rad, &end_rad, &seg))
    	continue;  // degenerate, skip

	// set default verts arrays and lengths
	w_vec3 *circle_verts = NULL;
	int circle_verts_count = 0;

	w_vec3 *circle_outline_verts = NULL;
	int circle_outline_verts_count = 0;

	bool is_full_circle = (end_rad - start_rad) >= (2.0f * W_PI - 0.001f);
	uint64_t hash = w_hash(diameter, start_rad, end_rad, seg);

	// use cached allocation for verts
	if (hash == *w_query_get_opt_or_default(shape_verts_hash))
	{
		circle_verts = wm_managed_alloc_resolve_handle(world, shape_verts_handle, entity);
		circle_verts_count = *w_query_get_opt_or_default(shape_verts_count);
		circle_outline_verts_count = W_SHAPE_CIRCLE_OUTLINE_VERTS_COUNT(seg, is_full_circle);
		circle_outline_verts = circle_verts + circle_verts_count;
	}
	// generate if hash changed
	else
	{
		debug_printf("regen circle for %d", entity);
		circle_verts_count = W_SHAPE_CIRCLE_VERTS_COUNT(seg);
		circle_outline_verts_count = W_SHAPE_CIRCLE_OUTLINE_VERTS_COUNT(seg, false);
		int total_verts = circle_verts_count + circle_outline_verts_count;

		circle_verts = wm_managed_alloc_malloc(world, shape_verts_handle, entity, total_verts * sizeof(w_vec3));
		circle_verts_count = w_rendering_shape_generate_circle_verts(circle_verts, 0, seg, start_rad, end_rad, ((w_vec3){0,0,0}), true);

		circle_outline_verts = circle_verts + circle_verts_count;
		circle_outline_verts_count = w_rendering_shape_generate_circle_outline_verts(circle_outline_verts, 0, seg, start_rad, end_rad, ((w_vec3){0,0,0}));

		// set hash and vert count
		w_set_value(entity, shape_verts_count, circle_verts_count);
		w_set_value(entity, shape_verts_hash, hash);
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

	if (outline_col.a > 0)
	{
		cmd.color = outline_col;
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
		w_query_o(diameter_top),
		w_query_o(diameter_bottom),
		w_query_o(segments),
		w_query_o(outline_color),
		w_query_o(shape_verts_handle),
		w_query_o(shape_verts_hash),
		w_query_o(shape_verts_count),
	),
{
	float height = *w_query_get(cylinder);
	int seg = *w_query_get_opt_or_default(segments);
	float dia_top = *w_query_get_opt_or_default(diameter_top);
	float dia_bottom = *w_query_get_opt_or_default(diameter_bottom);

	w_color8 shape_color = *w_query_get(color);
	w_color8 outline_col = *w_query_get_opt_or_default(outline_color);

	float start_rad = 0 * W_DEG2RAD;
	float end_rad = 360 * W_DEG2RAD;

	w_vec3 *verts = NULL;
	int verts_count = 0;
	w_vec3 *outline_verts = NULL;
	int outline_verts_count = 0;

	uint64_t hash = w_hash(height, seg, dia_top, dia_bottom);

	// use cached allocation for verts
	if (hash == *w_query_get_opt_or_default(shape_verts_hash))
	{
		verts = wm_managed_alloc_resolve_handle(world, shape_verts_handle, entity);
		verts_count = *w_query_get_opt_or_default(shape_verts_count);
		if (dia_top > 0) outline_verts_count += W_SHAPE_CIRCLE_OUTLINE_VERTS_COUNT(seg, true);
		if (dia_bottom > 0) outline_verts_count += W_SHAPE_CIRCLE_OUTLINE_VERTS_COUNT(seg, true);
		outline_verts = verts + verts_count;
	}
	// generate if hash changed
	else
	{
		debug_printf("regen cylinder for %d", entity);
		// calculate and create verts buffer
		verts_count += W_SHAPE_CYLINDER_SIDES_VERTS_COUNT(seg);
		if (dia_top > 0) verts_count += W_SHAPE_CYLINDER_CAP_VERTS_COUNT(seg);
		if (dia_bottom > 0) verts_count += W_SHAPE_CYLINDER_CAP_VERTS_COUNT(seg);

		if (dia_top > 0) outline_verts_count += W_SHAPE_CIRCLE_OUTLINE_VERTS_COUNT(seg, true);
		if (dia_bottom > 0) outline_verts_count += W_SHAPE_CIRCLE_OUTLINE_VERTS_COUNT(seg, true);

		int total_verts = verts_count + outline_verts_count;
		verts = wm_managed_alloc_malloc(world, shape_verts_handle, entity, total_verts * sizeof(w_vec3));
		outline_verts = verts + verts_count;

		int verts_offset = 0;
		int outline_verts_offset = 0;

		// generate top and bottom circles if required
		// top
		if (dia_top > 0) {
			verts_offset += w_rendering_shape_generate_circle_verts(verts, verts_offset, seg, start_rad, end_rad, ((w_vec3){0.0f, height * 0.5, 0.0f}), true);

			outline_verts_offset += w_rendering_shape_generate_circle_outline_verts(outline_verts, outline_verts_offset, seg, start_rad, end_rad, ((w_vec3){0.0f, height * 0.5, 0.0f}));

			// apply diameter offsets
			w_rendering_shape_apply_vert_scale(verts, verts_offset - W_SHAPE_CIRCLE_VERTS_COUNT(seg), verts_offset, ((w_vec3){dia_top, 1.0f, dia_top}));
			w_rendering_shape_apply_vert_scale(outline_verts, outline_verts_offset - W_SHAPE_CIRCLE_OUTLINE_VERTS_COUNT(seg, true), outline_verts_offset, ((w_vec3){dia_top, 1.0f, dia_top}));
		}

		// sides
		verts_offset += w_rendering_shape_generate_cylinder_sides_verts(verts, verts_offset, seg, dia_top, dia_bottom, height * 0.5f);

		// bottom
		if (dia_bottom > 0) {
			verts_offset += w_rendering_shape_generate_circle_verts(verts, verts_offset, seg, start_rad, end_rad, ((w_vec3){0.0f, -(height * 0.5), 0.0f}), false);

			outline_verts_offset += w_rendering_shape_generate_circle_outline_verts(outline_verts, outline_verts_offset, seg, start_rad, end_rad, ((w_vec3){0.0f, -(height * 0.5), 0.0f}));

			// apply diameter offsets
			w_rendering_shape_apply_vert_scale(verts, verts_offset - W_SHAPE_CIRCLE_VERTS_COUNT(seg), verts_offset, ((w_vec3){dia_bottom, 1.0f, dia_bottom}));
			w_rendering_shape_apply_vert_scale(outline_verts, outline_verts_offset - W_SHAPE_CIRCLE_OUTLINE_VERTS_COUNT(seg, true), outline_verts_offset, ((w_vec3){dia_bottom, 1.0f, dia_bottom}));
		}

		// set hash and vert count
		w_set_value(entity, shape_verts_count, verts_count);
		w_set_value(entity, shape_verts_hash, hash);
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

	if (outline_col.a > 0)
	{
		cmd.color = outline_col;
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
		w_query_o(outline_color),
		w_query_o(shape_verts_handle),
		w_query_o(shape_verts_hash),
		w_query_o(shape_verts_count),
	),
{
	// polygon relies on the circle generator with limited exposed components
	float diameter = 1.0f;
	int segments = *w_query_get(polygon);

	// ensure flat side is lined up X bottom
	float rotation_offset = (W_PI / segments) + (90 * W_DEG2RAD);
	float start_rad = rotation_offset;
	float end_rad = rotation_offset + 2 * W_PI;

	w_color8 shape_color = *w_query_get(color);
	w_color8 outline_col = *w_query_get_opt_or_default(outline_color);

	// set default verts arrays and lengths
	w_vec3 *circle_verts = NULL;
	int circle_verts_count = 0;

	w_vec3 *circle_outline_verts = NULL;
	int circle_outline_verts_count = 0;

	uint64_t hash = w_hash(segments);

	// use cached allocation for verts
	if (hash == *w_query_get_opt_or_default(shape_verts_hash))
	{
		circle_verts = wm_managed_alloc_resolve_handle(world, shape_verts_handle, entity);
		circle_verts_count = *w_query_get_opt_or_default(shape_verts_count);
		circle_outline_verts_count = W_SHAPE_CIRCLE_OUTLINE_VERTS_COUNT(segments, true);
		circle_outline_verts = circle_verts + circle_verts_count;
	}
	// generate if hash changed
	else
	{
		debug_printf("regen polygon for %d", entity);
		circle_verts_count = W_SHAPE_CIRCLE_VERTS_COUNT(segments);
		circle_outline_verts_count = W_SHAPE_CIRCLE_OUTLINE_VERTS_COUNT(segments, false);
		int total_verts = circle_verts_count + circle_outline_verts_count;

		circle_verts = wm_managed_alloc_malloc(world, shape_verts_handle, entity, total_verts * sizeof(w_vec3));
		circle_verts_count = w_rendering_shape_generate_circle_verts(circle_verts, 0, segments, start_rad, end_rad, ((w_vec3){0,0,0}), true);

		circle_outline_verts = circle_verts + circle_verts_count;
		circle_outline_verts_count = w_rendering_shape_generate_circle_outline_verts(circle_outline_verts, 0, segments, start_rad, end_rad, ((w_vec3){0,0,0}));

		// set hash and vert count
		w_set_value(entity, shape_verts_count, circle_verts_count);
		w_set_value(entity, shape_verts_hash, hash);
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

	if (outline_col.a > 0)
	{
		cmd.color = outline_col;
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
	w_rendering_shapes_procedural_dispatch_draw_sphere,
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
		w_query_r(sphere),
		w_query_r(color),

		// optional shape components
		w_query_o(angle_start),
		w_query_o(angle_end),
		w_query_o(segments),
		w_query_o(rings),
		w_query_o(shape_verts_handle),
		w_query_o(shape_verts_hash),
		w_query_o(shape_verts_count),
	),
{
	float diameter = *w_query_get(sphere);
	float start_rad = *w_query_get_opt_or_default(angle_start) * W_DEG2RAD;
	float end_rad = *w_query_get_opt_or_default(angle_end) * W_DEG2RAD;
	int seg = *w_query_get_opt_or_default(segments);
	int ring = *w_query_get_opt_or_default(rings);

	w_color8 shape_color = *w_query_get(color);

	w_vec3 *verts = NULL;
	int verts_count = 0;

	uint64_t hash = w_hash(diameter, start_rad, end_rad, seg, ring);

	// use cached allocation for verts
	if (hash == *w_query_get_opt_or_default(shape_verts_hash))
	{
		verts = wm_managed_alloc_resolve_handle(world, shape_verts_handle, entity);
		verts_count = *w_query_get_opt_or_default(shape_verts_count);
	}
	// generate if hash changed
	else
	{
		debug_printf("regen sphere for %d", entity);
		verts_count = W_SHAPE_SPHERE_VERTS_COUNT(seg, ring);
		verts = wm_managed_alloc_malloc(world, shape_verts_handle, entity, verts_count * sizeof(w_vec3));
		verts_count = w_rendering_shape_generate_sphere_verts(verts, 0, seg, ring, diameter, start_rad, end_rad);

		// set hash and vert count
		w_set_value(entity, shape_verts_count, verts_count);
		w_set_value(entity, shape_verts_hash, hash);
	}

	// prepare draw command
	struct w_rendering_cmd_draw_verts cmd = {0};

	cmd.position = *w_query_get(render_position_3d);
	cmd.rotation = *w_query_get(render_rotation_3d);
	cmd.scale = *w_query_get(render_scale_3d);
	cmd.origin = *w_query_get(render_origin_3d);
	cmd.scale = w_vec3_mul(cmd.scale, w_shape_vert_scale);

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
});

w_ecs_system(
	w_rendering_shapes_procedural_dispatch_draw_capsule,
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
		w_query_r(capsule),
		w_query_r(color),

		// optional shape components
		w_query_o(diameter),
		w_query_o(segments),
		w_query_o(rings),
		w_query_o(shape_verts_handle),
		w_query_o(shape_verts_hash),
		w_query_o(shape_verts_count),
	),
{
	float length = *w_query_get_opt_or_default(capsule);
	float dia = *w_query_get_opt_or_default(diameter);
	int seg = *w_query_get_opt_or_default(segments);
	int ring = *w_query_get_opt_or_default(rings);

	w_color8 shape_color = *w_query_get(color);

	w_vec3 *verts = NULL;
	int verts_count = 0;

	uint64_t hash = w_hash(length, dia, seg, ring);

	// use cached allocation for verts
	if (hash == *w_query_get_opt_or_default(shape_verts_hash))
	{
		verts = wm_managed_alloc_resolve_handle(world, shape_verts_handle, entity);
		verts_count = *w_query_get_opt_or_default(shape_verts_count);
	}
	// generate if hash changed
	else
	{
		debug_printf("regen capsule for %d", entity);
		verts_count = W_SHAPE_CAPSULE_VERTS_COUNT(seg, ring);
		verts = wm_managed_alloc_malloc(world, shape_verts_handle, entity, verts_count * sizeof(w_vec3));
		verts_count = w_rendering_shape_generate_capsule_verts(verts, 0, seg, ring, dia, length * 0.5f);

		// set hash and vert count
		w_set_value(entity, shape_verts_count, verts_count);
		w_set_value(entity, shape_verts_hash, hash);
	}

	// prepare draw command
	struct w_rendering_cmd_draw_verts cmd = {0};

	cmd.position = *w_query_get(render_position_3d);
	cmd.rotation = *w_query_get(render_rotation_3d);
	cmd.scale = *w_query_get(render_scale_3d);
	cmd.origin = *w_query_get(render_origin_3d);
	cmd.scale = w_vec3_mul(cmd.scale, w_shape_vert_scale);

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
});

w_ecs_system(
	w_rendering_shapes_procedural_dispatch_draw_hexahedron,
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

		// required shape component (hexahedron)
		w_query_r(hexahedron),
		w_query_r(color),

		// optional shape components
		w_query_o(outline_color),
		w_query_o(shape_verts_handle),
		w_query_o(shape_verts_hash),
		w_query_o(shape_verts_count),
	),
{
	w_tricell8 hex = *w_query_get(hexahedron);
	w_color8 shape_color = *w_query_get(color);
	w_color8 outline_col = *w_query_get_opt_or_default(outline_color);

	// set verts arrays and counts
	w_vec3 *hexahedron_verts = NULL;
	int hexahedron_verts_count = 0;

	w_vec3 *hexahedron_outline_verts = NULL;
	int hexahedron_outline_verts_count = 0;

	uint64_t hash = w_xxhash64(hex);

	// use cached allocation for verts
	if (hash == *w_query_get_opt_or_default(shape_verts_hash))
	{
		hexahedron_verts = wm_managed_alloc_resolve_handle(world, shape_verts_handle, entity);
		hexahedron_verts_count = W_SHAPE_HEXAHEDRON_VERTS_COUNT;
		hexahedron_outline_verts_count = W_SHAPE_HEXAHEDRON_OUTLINE_VERTS_COUNT;
		hexahedron_outline_verts = hexahedron_verts + hexahedron_verts_count;
	}
	// generate if hash changed
	else
	{
		debug_printf("regen hexahedron for %d", entity);
		hexahedron_verts_count = W_SHAPE_HEXAHEDRON_VERTS_COUNT;
		hexahedron_outline_verts_count = W_SHAPE_HEXAHEDRON_OUTLINE_VERTS_COUNT;
		int total_verts = hexahedron_verts_count + hexahedron_outline_verts_count;

		hexahedron_verts = wm_managed_alloc_malloc(world, shape_verts_handle, entity, total_verts * sizeof(w_vec3));
		w_rendering_shape_generate_hexahedron_verts(hexahedron_verts, &hex);

		hexahedron_outline_verts = hexahedron_verts + hexahedron_verts_count;
		w_rendering_shape_generate_hexahedron_outline_verts(hexahedron_outline_verts, &hex);

		// set hash and vert count
		w_set_value(entity, shape_verts_count, hexahedron_verts_count);
		w_set_value(entity, shape_verts_hash, hash);
	}

	// prepare draw command
	struct w_rendering_cmd_draw_verts cmd = {0};

	cmd.position = *w_query_get(render_position_3d);
	cmd.rotation = *w_query_get(render_rotation_3d);
	cmd.scale = *w_query_get(render_scale_3d);
	cmd.origin = *w_query_get(render_origin_3d);

	if (shape_color.a > 0)
	{
		cmd.color = shape_color;
		cmd.verts = hexahedron_verts;
		cmd.verts_length = hexahedron_verts_count;
		cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_TRIANGLES;

		w_rendering_dispatch_render_cmd(
			W_RENDERING_CMD_DRAW_VERTS,
			w_ecs_render_layer(*w_query_get(render_layer)),
			&cmd
		);
	}

	if (outline_col.a > 0)
	{
		cmd.color = outline_col;
		cmd.verts = hexahedron_outline_verts;
		cmd.verts_length = hexahedron_outline_verts_count;
		cmd.draw_mode = W_RENDERING_DRAW_VERT_MODE_LINES;

		w_rendering_dispatch_render_cmd(
			W_RENDERING_CMD_DRAW_VERTS,
			w_ecs_render_layer(*w_query_get(render_layer)),
			&cmd
		);
	}
});


#endif /* WHISKER_RENDERING_SHAPES_PROCEDURAL_SYSTEMS_H */

