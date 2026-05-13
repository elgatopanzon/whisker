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

enum W_RENDERING_TEXT_ALIGN
{
	W_RENDERING_TEXT_ALIGN_LEFT,
	W_RENDERING_TEXT_ALIGN_CENTER,
	W_RENDERING_TEXT_ALIGN_RIGHT,
};


#endif /* WHISKER_RENDERING_COMPONENTS_H */

