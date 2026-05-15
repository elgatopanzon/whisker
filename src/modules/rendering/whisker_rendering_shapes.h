/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_shapes
 * @created     : Thursday Apr 30, 2026 19:11:39 CST
 * @description : shape rendering base module
 */

#include "whisker.h"
#include "whisker_components.h"
#include "whisker_rendering.h"

#ifndef WHISKER_RENDERING_SHAPES_H
#define WHISKER_RENDERING_SHAPES_H

enum W_RENDERING_SHAPE_TYPE
{
	W_RENDERING_SHAPE_TYPE_POINT,
	W_RENDERING_SHAPE_TYPE_LINE,
	W_RENDERING_SHAPE_TYPE_TRIANGLE,
	W_RENDERING_SHAPE_TYPE_RECT,
	W_RENDERING_SHAPE_TYPE_CIRCLE,
	W_RENDERING_SHAPE_TYPE_CUBE,
};

/*************************************
*  static shape verts and outlines  *
*************************************/
// NOTE: all verts are CCW winded

// verts for shapes XZ native (-1 to 1 range, scale 0.5 normalizes to 1 unit)
static const w_vec3 w_shape_vert_scale = {0.5f, 0.5f, 0.5f};
static const int w_shape_vert_range[] = {-1, 1};

// line (-1 to 1, centered)
static const w_vec3 w_shape_line_lines[] = {
    {-1.0f, 0, 0.0f}, // start
    {1.0f, 0, 0.0f},  // end
};
#define W_SHAPE_LINE_LINES_LEN (sizeof(w_shape_line_lines) / sizeof(*w_shape_line_lines))

// point cross
static const w_vec3 w_shape_point_cross_verts[] = {
    // horizontal
    { (1.0f / W_RENDERING_WORLD_PIXEL_SCALE), 0.0f, 0.0f},
    {-(1.0f / W_RENDERING_WORLD_PIXEL_SCALE), 0.0f, 0.0f},
    // vertical
    {0.0f, 0.0f, (1.0f / W_RENDERING_WORLD_PIXEL_SCALE)},
    {0.0f, 0.0f, -(1.0f / W_RENDERING_WORLD_PIXEL_SCALE)},
};

#define W_SHAPE_POINT_CROSS_VERTS_LEN 4  // 2 lines

// triangle (spans -1 to 1, centered)
static const w_vec3 w_shape_triangle_tris[] = {
    { 0.0f, 0.0f,  1.0f},  // top
    { 1.0f, 0.0f, -1.0f},  // bottom right
    {-1.0f, 0.0f, -1.0f},  // bottom left
};

#define W_SHAPE_TRIANGLE_TRIS_LEN 3

// rectangle
static const w_vec3 w_shape_rect_tris[] = {
    {-1.0f, 0, -1.0f}, {-1.0f, 0, 1.0f}, {1.0f, 0, -1.0f},
    {1.0f, 0, -1.0f}, {-1.0f, 0, 1.0f}, {1.0f, 0, 1.0f},
};
#define W_SHAPE_RECT_TRIS_LEN (sizeof(w_shape_rect_tris) / sizeof(*w_shape_rect_tris))

// rectangle outline (4 edges, 8 verts)
static const w_vec3 w_shape_rect_lines[] = {
    {-1.0f, 0, -1.0f}, {1.0f, 0, -1.0f},  // bottom
    {1.0f, 0, -1.0f}, {1.0f, 0, 1.0f},  // right
    {1.0f, 0, 1.0f}, {-1.0f, 0, 1.0f},  // top
    {-1.0f, 0, 1.0f}, {-1.0f, 0, -1.0f},  // left
};
#define W_SHAPE_RECT_LINES_LEN (sizeof(w_shape_rect_lines) / sizeof(*w_shape_rect_lines))

// cube triangles (centered, -1 to 1 range, 36 verts)
static const w_vec3 w_shape_cube_tris[] = {
    // front
    {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f},
    {-1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f},
    // back
    { 1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f},
    { 1.0f, -1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f},
    // top
    {-1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f},
    {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f, -1.0f},
    // bottom
    {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f,  1.0f},
    {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f,  1.0f},
    // right
    { 1.0f, -1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f,  1.0f},
    { 1.0f, -1.0f, -1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f},
    // left
    {-1.0f, -1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f, -1.0f},
    {-1.0f, -1.0f,  1.0f}, {-1.0f,  1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f},
};
#define W_SHAPE_CUBE_TRIS_LEN (sizeof(w_shape_cube_tris) / sizeof(*w_shape_cube_tris))

// cube outline (12 edges, 24 verts)
static const w_vec3 w_shape_cube_lines[] = {
    // front face
    {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f},
    { 1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f},
    { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f},
    {-1.0f,  1.0f,  1.0f}, {-1.0f, -1.0f,  1.0f},
    // back face
    {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f},
    { 1.0f, -1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f},
    { 1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f},
    {-1.0f,  1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f},
    // connecting edges
    {-1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f, -1.0f},
    { 1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f, -1.0f},
    { 1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f, -1.0f},
    {-1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f, -1.0f},
};
#define W_SHAPE_CUBE_LINES_LEN (sizeof(w_shape_cube_lines) / sizeof(*w_shape_cube_lines))


/*************************
*  shape helper macros  *
*************************/
#define W_CIRCLE_VERTS_COUNT(segments) ((segments) * 3)
#define W_CIRCLE_OUTLINE_VERTS_COUNT(segments, is_full) ((segments) * 2 + ((is_full) ? 0 : 4))
#define W_CYLINDER_CAP_VERTS_COUNT(segments)   ((segments) * 3)   // same as circle
#define W_CYLINDER_SIDES_VERTS_COUNT(segments) ((segments) * 6)   // 1 quad = 2 tris


/**********************
*  shape components  *
**********************/
// general
w_ecs_define_component(float, thickness, 1.0f);
w_ecs_define_component(float, shape_angle_start, 0);
w_ecs_define_component(float, shape_angle_end, 360); // full circle
w_ecs_define_component(float, shape_diameter_top, 1.0f); // top diameter
w_ecs_define_component(float, shape_diameter_bottom, 1.0f); // bottom diameter
w_ecs_define_component(float, shape_segments, 32);

// 2D
w_ecs_define_component(float, point, 1.0f);
w_ecs_define_component(float, line, 1.0f);
w_ecs_define_component(float, triangle, 1.0f);
w_ecs_define_component(w_vec2, rectangle, 0.0f, 0.0f);
w_ecs_define_component(float, circle, 1.0f); // diameter
w_ecs_define_component(int, polygon, 5); // N-sides
w_ecs_define_component(w_vec2, grid, 0.0f, 0.0f);

// 3D
w_ecs_define_component(w_vec3, cube, 0.0f, 0.0f, 0.0f);
w_ecs_define_component(float, cylinder, 1.0f); // height

// color applied to outline when exists and .a > 0
// note: alpha defaults to 0 so we don't draw the outline
w_ecs_define_component(w_color8, shape_outline_color, 0, 0, 0, 0);


/*********************
*  shape functions  *
*********************/

// apply an offset to a vert array
void w_rendering_shape_apply_vert_offset(w_vec3 *verts, size_t from, size_t to, w_vec3 offset);

// apply a scale to a vert array
void w_rendering_shape_apply_vert_scale(w_vec3 *verts, size_t from, size_t to, w_vec3 scale);

// sanitize incoming circle params
bool w_rendering_shape_sanitize_circle_params(float *diameter, float *start_rad, float *end_rad, int *segments);

// generate the tri verts for a circle
int w_rendering_shape_generate_circle_verts(w_vec3 *out_verts, int offset, int segments, float start_rad, float end_rad, w_vec3 vert_offset, bool face_up);

// generate the line verts for a circle outline
int w_rendering_shape_generate_circle_outline_verts(w_vec3 *out_verts, int offset, int segments, float start_rad, float end_rad, w_vec3 vert_offset);

// generate the sides of cylinder given segment count
int w_rendering_shape_generate_cylinder_sides_verts(w_vec3 *out_verts, int start_index, int segments, float diameter_top, float diameter_bottom, float half_height);

#endif /* WHISKER_RENDERING_SHAPES_H */

