/**
 * @author      : ElGatoPanzon
 * @file        : whisker_rendering
 * @created     : Monday Mar 30, 2026 11:17:25 CST
 * @description : Rendering base module
 */

#ifndef WHISKER_RENDERING_H
#define WHISKER_RENDERING_H

#include "whisker.h"
#include "whisker_types.h"

#include "modules/whisker_module_ids.h"

/* index 0: display configuration */
/* index 1: render configuration */
/* index 2: render state */
/* index 3: render dispatch buffer */
#define WM_RENDERING_DISPLAY_CONFIG_RESOURCE_ID    WM_MODULE_RESOURCE_ID(RENDERING, 0)
#define WM_RENDERING_RENDER_CONFIG_RESOURCE_ID     WM_MODULE_RESOURCE_ID(RENDERING, 1)
#define WM_RENDERING_RENDER_STATE_RESOURCE_ID      WM_MODULE_RESOURCE_ID(RENDERING, 2)
#define WM_RENDERING_RENDER_DISPATCH_BUFFER_RESOURCE_ID   WM_MODULE_RESOURCE_ID(RENDERING, 3)

#define w_rendering_get_display_config(w) w_ecs_get_module_resource(w, WM_RENDERING_DISPLAY_CONFIG_RESOURCE_ID)
#define w_rendering_get_render_config(w) w_ecs_get_module_resource(w, WM_RENDERING_RENDER_CONFIG_RESOURCE_ID)
#define w_rendering_get_render_state(w) w_ecs_get_module_resource(w, WM_RENDERING_RENDER_STATE_RESOURCE_ID)
#define w_rendering_get_render_dispatch_buffer(w) w_ecs_get_module_resource(w, WM_RENDERING_RENDER_DISPATCH_BUFFER_RESOURCE_ID)

#ifndef W_RENDERING_WORLD_PIXEL_SCALE
#define W_RENDERING_WORLD_PIXEL_SCALE 100.0f;
#endif

struct w_rendering_display_config 
{
	// display resolution and monitor selection
	w_vec2i window_resolution; 
	uint8_t monitor_index;

	// display filtering options
	bool vsync;
	bool high_dpi;

	// window title and basic options
	w_string_table_id window_title_id;

	// window mode options
	bool fullscreen;
	bool borderless;
	bool resizable;
	bool maximised;
	bool minimized;

	// window appearance options
	bool always_on_top;
	bool undecorated;
	bool transparent;
	bool hidden;
	bool unfocused;
	bool mouse_passthrough;

	// window behaviour options
	bool continue_running_minimised;
	bool handle_window_close;
};

enum W_RENDERING_CULL_MODE 
{ 
	W_RENDERING_CULL_MODE_NONE, 
	W_RENDERING_CULL_MODE_BACK, 
	W_RENDERING_CULL_MODE_FRONT 
};
enum W_RENDERING_BLEND_MODE
{
	W_RENDERING_BLEND_MODE_ALPHA,
	W_RENDERING_BLEND_MODE_ADDITIVE,
	W_RENDERING_BLEND_MODE_MULTIPLY,
};
enum W_RENDERING_SCALING_MODE
{
	W_RENDERING_SCALING_STRETCH,   // fill window, ignore aspect ratio
	W_RENDERING_SCALING_FIT,       // letterbox/pillarbox, maintain aspect ratio
	W_RENDERING_SCALING_INTEGER,   // pixel-perfect integer multiples only
};
enum W_RENDERING_TEXTURE_FILTER
{
    W_RENDERING_TEXTURE_FILTER_POINT,
    W_RENDERING_TEXTURE_FILTER_BILINEAR,
    W_RENDERING_TEXTURE_FILTER_TRILINEAR,
    W_RENDERING_TEXTURE_FILTER_ANISOTROPIC,
};
struct w_rendering_render_config 
{
	// general rendering toggles
	bool enabled;
	bool handle_drawing_lifecycle;

	// resolution and aspect
	w_vec2i render_resolution;
	enum W_RENDERING_SCALING_MODE scaling_mode;

	// filtering options
	enum W_RENDERING_TEXTURE_FILTER texture_filter;
	uint8_t msaa_samples;
	uint8_t anisotropic_level;

	// color and clearing
	w_color window_clear_color;
	w_color render_clear_color;
	bool clear_enabled;

	// performance
	uint32_t target_fps;

	// rendering modes
	bool wireframe_mode;
	bool interlaced_mode;

	// color and lighting correction
	bool gamma_correction;

	// blending and culling
	enum W_RENDERING_CULL_MODE default_cull_mode;
	enum W_RENDERING_BLEND_MODE default_blend_mode;

	// interpolation
	bool interpolation;
	float interpolation_alpha;

	// depth and stencil
	bool depth_test;
	bool depth_write;
	bool scissor_test;
	bool stencil_test;
};

// active rendering state
struct w_rendering_render_state 
{
	uint32_t render_fps;
	float render_frame_time;
	w_vec2 window_resolution;

	// scaling
	w_rect render_texture_source;
	w_rect render_texture_destination;
	float scaling_factor;

	// draw
	w_vec2 draw_origin;
	float draw_rotation;
	w_color draw_tint;
};

// custom rendering phases
enum WM_RENDER_PHASE
{
	WM_RENDER_PHASE_PRE_SCALE = 100,
	WM_RENDER_PHASE_ON_SCALE = 101,
	WM_RENDER_PHASE_POST_SCALE = 102,
	WM_RENDER_PHASE_PRE_FILTER = 103,
	WM_RENDER_PHASE_ON_FILTER = 104,
	WM_RENDER_PHASE_POST_FILTER = 105,
	WM_RENDER_PHASE_PRE_DRAW = 106,
	WM_RENDER_PHASE_ON_DRAW = 107,
	WM_RENDER_PHASE_POST_DRAW = 108,

	WM_RENDER_PHASE_PRE_WORLD = 109,
	WM_RENDER_PHASE_BEGIN_WORLD = 110,
	WM_RENDER_PHASE_WORLD_BACKGROUND = 111,
	WM_RENDER_PHASE_ON_WORLD = 112,
	WM_RENDER_PHASE_WORLD_FOREGROUND = 113,
	WM_RENDER_PHASE_END_WORLD = 115,
	WM_RENDER_PHASE_POST_WORLD = 114,

	WM_RENDER_PHASE_PRE_SYNC = 116,
	WM_RENDER_PHASE_ON_SYNC = 117,
	WM_RENDER_PHASE_POST_SYNC = 118,

	WM_RENDER_PHASE_PRE_OVERLAY = 119,
	WM_RENDER_PHASE_ON_OVERLAY = 120,
	WM_RENDER_PHASE_POST_OVERLAY = 121,
};

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

// initialize the rendering module
void wm_rendering_init(struct w_ecs_world *world, struct w_rendering_display_config *display_config, struct w_rendering_render_config *rendering_config);

// cleanup the rendering module
void wm_rendering_free(struct w_ecs_world *world);

#endif /* WHISKER_RENDERING_H */
