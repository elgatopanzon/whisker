/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_components
 * @created     : Thursday Apr 30, 2026 18:15:40 CST
 * @description : common set of generic components
 */

#include "whisker.h"

#ifndef WHISKER_COMPONENTS_H
#define WHISKER_COMPONENTS_H

/******************
*  generic tags  *
******************/
w_ecs_define_tag(active);
w_ecs_define_tag(inactive);
w_ecs_define_tag(visible);
w_ecs_define_tag(hidden);
w_ecs_define_tag(dirty);


/************************
*  generic components  *
************************/

// string is a char* pointer
w_ecs_define_component(char_ptr, string, NULL);
w_ecs_define_component(void_ptr, ptr, NULL);
w_ecs_define_component(int, string_id, W_STRING_TABLE_INVALID_ID);


/***********************
*  visual components  *
***********************/
w_ecs_define_component(w_color8, color, 255, 255, 255, 255);
w_ecs_define_component(w_color, color_f, 1.0f, 1.0f, 1.0f, 1.0f);
w_ecs_define_component(w_color8, outline_color, 0, 0, 0, 0);
w_ecs_define_component(float, alpha, 1.0f);
w_ecs_define_component(float, thickness, 1.0f);


/*************************
*  geometry components  *
*************************/
w_ecs_define_component(float, angle_start, 0);
w_ecs_define_component(float, angle_end, 360); // full circle
w_ecs_define_component(float, diameter, 1.0f);
w_ecs_define_component(float, diameter_top, 1.0f); // top diameter
w_ecs_define_component(float, diameter_bottom, 1.0f); // bottom diameter
w_ecs_define_component(int, segments, 32);
w_ecs_define_component(int, rings, 16);


/**********************
*  shape components  *
**********************/
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
w_ecs_define_component(float, sphere, 1.0f); // diameter
w_ecs_define_component(float, capsule, 1.0f); // length
// defaults to a cube
w_ecs_define_component(w_tricell8, hexahedron, 
	.c000 = {-1.0f, -1.0f, -1.0f},
    .c100 = { 1.0f, -1.0f, -1.0f},
    .c010 = {-1.0f,  1.0f, -1.0f},
    .c110 = { 1.0f,  1.0f, -1.0f},
    .c001 = {-1.0f, -1.0f,  1.0f},
    .c101 = { 1.0f, -1.0f,  1.0f},
    .c011 = {-1.0f,  1.0f,  1.0f},
    .c111 = { 1.0f,  1.0f,  1.0f},
);

/**************************
*  hierarchy components  *
**************************/
w_ecs_define_component(w_entity_id, parent, W_ENTITY_INVALID);
w_ecs_define_component(int, layer, 0);
w_ecs_define_component(int, depth, 0);
w_ecs_define_component(int, priority, 0);


/**************************
*  transform components  *
**************************/
// 3D
w_ecs_define_component(w_vec3, position_3d, 0.0f, 0.0f, 0.0f);
w_ecs_define_component(w_quat, rotation_3d, 0.0f, 0.0f, 0.0f, 1.0f);
w_ecs_define_component(w_vec3, scale_3d, 1.0f, 1.0f, 1.0f);
w_ecs_define_component(w_vec3, origin_3d, 0.0f, 0.0f, 0.0f);

// 2D
w_ecs_define_component(w_vec2, position_2d, 0.0f, 0.0f);
w_ecs_define_component(float, rotation_2d, 0.0f);
w_ecs_define_component(w_vec2, scale_2d, 1.0f, 1.0f);
w_ecs_define_component(w_vec2, origin_2d, 0.0f, 0.0f);


/************************
*  physics components  *
************************/
// general
w_ecs_define_component(float, mass, 0.0f);

// 3D
w_ecs_define_component(w_vec3, velocity_3d, 0.0f, 0.0f, 0.0f);
w_ecs_define_component(w_vec3, acceleration_3d, 0.0f, 0.0f, 0.0f);
w_ecs_define_component(w_vec3, angular_velocity_3d, 0.0f, 0.0f, 0.0f);

// 2D
w_ecs_define_component(w_vec2, velocity_2d, 0.0f, 0.0f);
w_ecs_define_component(w_vec2, acceleration_2d, 0.0f, 0.0f);
w_ecs_define_component(float, angular_velocity_2d, 0.0f);


/****************************
*  render copy components  *
****************************/
// render components are copies of base authorative components at render time
// 3D
w_ecs_define_component(w_vec3, render_position_3d, 0.0f, 0.0f, 0.0f);
w_ecs_define_component(w_quat, render_rotation_3d, 0.0f, 0.0f, 0.0f, 1.0f);
w_ecs_define_component(w_vec3, render_scale_3d, 1.0f, 1.0f, 1.0f);
w_ecs_define_component(w_vec3, render_origin_3d, 0.0f, 0.0f, 0.0f);

// 2D
w_ecs_define_component(w_vec2, render_position_2d, 0.0f, 0.0f);
w_ecs_define_component(float, render_rotation_2d, 0.0f);
w_ecs_define_component(w_vec2, render_scale_2d, 1.0f, 1.0f);
w_ecs_define_component(w_vec2, render_origin_2d, 0.0f, 0.0f);

#endif /* WHISKER_COMPONENTS_H */

