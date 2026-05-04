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

// verts for shapes XZ native

// rectangle
static const w_vec3 w_shape_rect_tris[] = {
    {0, 0, 0}, {0, 0, 1}, {1, 0, 0},
    {1, 0, 0}, {0, 0, 1}, {1, 0, 1},
};
#define W_SHAPE_RECT_TRIS_LEN (sizeof(w_shape_rect_tris) / sizeof(*w_shape_rect_tris))

// rectangle outline (4 edges, 8 verts)
static const w_vec3 w_shape_rect_lines[] = {
    {0, 0, 0}, {1, 0, 0},  // bottom
    {1, 0, 0}, {1, 0, 1},  // right
    {1, 0, 1}, {0, 0, 1},  // top
    {0, 0, 1}, {0, 0, 0},  // left
};
#define W_SHAPE_RECT_LINES_LEN (sizeof(w_shape_rect_lines) / sizeof(*w_shape_rect_lines))

// cube triangles (centered, -1 to 1 range, 36 verts)
static const w_vec3 w_shape_cube_tris[] = {
    // front
    {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1},
    {-1, -1,  1}, { 1,  1,  1}, {-1,  1,  1},
    // back
    { 1, -1, -1}, {-1, -1, -1}, {-1,  1, -1},
    { 1, -1, -1}, {-1,  1, -1}, { 1,  1, -1},
    // top
    {-1,  1, -1}, {-1,  1,  1}, { 1,  1,  1},
    {-1,  1, -1}, { 1,  1,  1}, { 1,  1, -1},
    // bottom
    {-1, -1, -1}, { 1, -1, -1}, { 1, -1,  1},
    {-1, -1, -1}, { 1, -1,  1}, {-1, -1,  1},
    // right
    { 1, -1, -1}, { 1,  1, -1}, { 1,  1,  1},
    { 1, -1, -1}, { 1,  1,  1}, { 1, -1,  1},
    // left
    {-1, -1,  1}, {-1,  1,  1}, {-1,  1, -1},
    {-1, -1,  1}, {-1,  1, -1}, {-1, -1, -1},
};
#define W_SHAPE_CUBE_TRIS_LEN (sizeof(w_shape_cube_tris) / sizeof(*w_shape_cube_tris))

// cube outline (12 edges, 24 verts)
static const w_vec3 w_shape_cube_lines[] = {
    // front face
    {-1, -1,  1}, { 1, -1,  1},
    { 1, -1,  1}, { 1,  1,  1},
    { 1,  1,  1}, {-1,  1,  1},
    {-1,  1,  1}, {-1, -1,  1},
    // back face
    {-1, -1, -1}, { 1, -1, -1},
    { 1, -1, -1}, { 1,  1, -1},
    { 1,  1, -1}, {-1,  1, -1},
    {-1,  1, -1}, {-1, -1, -1},
    // connecting edges
    {-1, -1,  1}, {-1, -1, -1},
    { 1, -1,  1}, { 1, -1, -1},
    { 1,  1,  1}, { 1,  1, -1},
    {-1,  1,  1}, {-1,  1, -1},
};
#define W_SHAPE_CUBE_LINES_LEN (sizeof(w_shape_cube_lines) / sizeof(*w_shape_cube_lines))

/**************************
*  shape tag components  *
**************************/
// shape type tags
w_ecs_define_tag(shape_type_cube);


/**********************
*  shape components  *
**********************/
w_ecs_define_component(w_color8, shape_outline_color, 0, 0, 0, 255);

#endif /* WHISKER_RENDERING_SHAPES_H */

