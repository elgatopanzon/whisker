/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_components
 * @created     : Wednesday May 13, 2026 16:29:47 CST
 * @description : components used by the rendering module
 */

#include "whisker.h"

#ifndef WHISKER_RENDERING_COMPONENTS_H
#define WHISKER_RENDERING_COMPONENTS_H

/*************************
*  components and tags  *
*************************/

// phase as render layer
w_ecs_define_component(int, render_layer, 0);

// enable billboard on Y or all axies
enum W_RENDERING_BILLBOARD
{
	W_RENDERING_BILLBOARD_Y,
	W_RENDERING_BILLBOARD_ALL,
};
w_ecs_define_component(int, render_billboard, W_RENDERING_BILLBOARD_Y);

// render at screen scale
w_ecs_define_tag(render_scale_screen);


/*********************
*  text components  *
*********************/
enum W_RENDERING_TEXT_ALIGN
{
	W_RENDERING_TEXT_ALIGN_LEFT,
	W_RENDERING_TEXT_ALIGN_CENTER,
	W_RENDERING_TEXT_ALIGN_RIGHT,
};

w_ecs_define_component(int, font_size, 10);
w_ecs_define_component(int, font_spacing, 1);
w_ecs_define_component(float, font_line_height, 0.0f);
w_ecs_define_component(w_color8, font_color, W_COLOR8_BLACK.r, W_COLOR8_BLACK.g, W_COLOR8_BLACK.b, W_COLOR8_BLACK.a);
w_ecs_define_component(w_vec2, font_shadow, 0.0f, 0.0f);
w_ecs_define_component(w_color8, font_shadow_color, 0.0f, 0.0f, 0.0f, 0.0f);
w_ecs_define_component(int, font_alignment, W_RENDERING_TEXT_ALIGN_LEFT);
w_ecs_define_component(float, font_max_width, 0);

#endif /* WHISKER_RENDERING_COMPONENTS_H */

