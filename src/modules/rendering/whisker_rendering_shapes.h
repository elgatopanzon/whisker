/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_shapes
 * @created     : Thursday Apr 30, 2026 19:11:39 CST
 * @description : shape rendering base module
 */

#include "whisker.h"
#include "whisker_components.h"

#ifndef WHISKER_RENDERING_SHAPES_H
#define WHISKER_RENDERING_SHAPES_H

enum W_RENDERING_SHAPE_TYPE
{
	W_RENDERING_SHAPE_TYPE_RECT,
	W_RENDERING_SHAPE_TYPE_CUBE,
};

// verts for shapes XZ native (-1 to 1 range, scale 0.5 normalizes to 1 unit)
static const w_vec3 w_shape_vert_scale = {0.5f, 0.5f, 0.5f};
static const int w_shape_vert_range[] = {-1, 1};

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


/**********************
*  shape components  *
**********************/
w_ecs_define_component(float, thickness, 1.0f);
w_ecs_define_component(w_vec2, rectangle, 0.0f, 0.0f);
w_ecs_define_component(w_vec3, cube, 0.0f, 0.0f, 0.0f);
w_ecs_define_component(w_vec2, grid, 0.0f, 0.0f);

// color applied to outline when exists and .a > 0
// note: alpha defaults to 0 so we don't draw the outline
w_ecs_define_component(w_color8, shape_outline_color, 0, 0, 0, 0);

#endif /* WHISKER_RENDERING_SHAPES_H */

