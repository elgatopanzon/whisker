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

