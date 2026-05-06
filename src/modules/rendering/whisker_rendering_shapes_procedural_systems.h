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
	WM_PHASE_PRE_RENDER,
	w_query(
		w_query_r(grid),
		w_query_r(thickness),
		w_query_r(color),
		w_query_r(position_3d),
		w_query_r(rotation_3d),
		w_query_r(scale_3d),
		w_query_r(origin_3d),
		w_query_r(render_layer),
		w_query_n(hidden),
	),
{
	struct w_rendering_cmd_draw_verts cmd = {0};

	cmd.position = *w_query_get(position_3d);
	cmd.rotation = *w_query_get(rotation_3d);
	cmd.scale = *w_query_get(scale_3d);
	cmd.origin = *w_query_get(origin_3d);
	cmd.color = *w_query_get(color);

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

	// vertical lines (span Z) - normalized -0.5 to 0.5
	for (int i = 0; i <= cells_x; i++) {
    	float x_base = (float)i / cells_x - 0.5f;
    	for (int t = 0; t < thickness_lines; t++) {
        	float x = x_base - half_thickness + t * line_offset;
        	grid_verts[v++] = ((w_vec3){x, 0, -0.5f});
        	grid_verts[v++] = ((w_vec3){x, 0, 0.5f});
    	}
	}

	// horizontal lines (span X) - normalized -0.5 to 0.5
	for (int i = 0; i <= cells_z; i++) {
    	float z_base = (float)i / cells_z - 0.5f;
    	for (int t = 0; t < thickness_lines; t++) {
        	float z = z_base - half_thickness + t * line_offset;
        	grid_verts[v++] = ((w_vec3){-0.5f, 0, z});
        	grid_verts[v++] = ((w_vec3){0.5f, 0, z});
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

#endif /* WHISKER_RENDERING_SHAPES_PROCEDURAL_SYSTEMS_H */

