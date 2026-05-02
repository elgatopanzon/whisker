/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_commands
 * @created     : Friday May 01, 2026 12:15:54 CST
 * @description : 
 */

#include "whisker_rendering.h"
#include "whisker_rendering_camera.h"

#ifndef WHISKER_RENDERING_COMMANDS_H
#define WHISKER_RENDERING_COMMANDS_H

// the render commands are dispatched with a w_dispatch_buffer
// this rendering module is responsible for dispatching them, and an upstream rendering backend module consumes the buffer and implements each command type

enum W_RENDERING_CMD
{
	W_RENDERING_CMD_DUMMY = 0,

	/************************************
	*  1-99: render pipeline commands  *
	************************************/

	// create window and context with given resolution and title
	// payload: struct w_rendering_cmd_init_window
	W_RENDERING_CMD_INIT_WINDOW = 1,

	// close the window managed by the backend
	// no payload
	W_RENDERING_CMD_CLOSE_WINDOW = 2,

	// handle window close detection
	// no payload
	W_RENDERING_CMD_HANDLE_WINDOW_CLOSE = 3,

	// init the main framebuffer in the backend
	// no payload
	W_RENDERING_CMD_FRAMEBUFFER_MAIN_INIT = 10,

	// activate the main framebuffer before rendering
	// no payload
	W_RENDERING_CMD_FRAMEBUFFER_MAIN_ACTIVATE = 11,

	// deactivate the main framebuffer before drawing it
	// no payload
	W_RENDERING_CMD_FRAMEBUFFER_MAIN_DEACTIVATE = 12,

	// set main framebuffer filtering mode before drawing it
	// payload: struct w_rendering_cmd_framebuffer_set_filter
	W_RENDERING_CMD_FRAMEBUFFER_MAIN_SET_FILTER = 13,

	// clear background color and flush screen buffers
	// payload: struct w_rendering_cmd_clear_color
	W_RENDERING_CMD_CLEAR_COLOR = 20,

	// sync backend state (FPS, frametime, current res) into ECS
	// no payload
	W_RENDERING_CMD_RENDER_STATE_SYNC = 30,

	// begin mode for camera in current camera_state
	// payload: struct w_rendering_begin_camera_3d
	W_RENDERING_CMD_CAMERA_BEGIN_3D = 50,

	// end mode for active camera
	// no payload
	W_RENDERING_CMD_CAMERA_END_3D = 51,


	/*****************************
	*  100-109: draw lifecycle commands  *
	*****************************/

	// begin drawing to the screen
	// no payload
	W_RENDERING_CMD_DRAW_BEGIN = 100,

	// end drawing to the screen
	// no payload
	W_RENDERING_CMD_DRAW_END = 101,

	// draw the main framebuffer to the screen
	// no payload
	W_RENDERING_CMD_DRAW_MAIN_FRAMEBUFFER = 102,

	/*******************************
	*  110-199: 2D draw commands  *
	*******************************/
	

};

// dummy payload, for when theres none
static const int W_RENDERING_DUMMY_PAYLOAD = 0;
#define w_rendering_dispatch_dummy_payload (void *)&W_RENDERING_DUMMY_PAYLOAD

// helper function to push a render command
// note: implicit world to fetch the render buffer
#define w_rendering_dispatch_render_cmd_value(cmd, payload_type, ...) \
	((void)sizeof(cmd), (void)sizeof(payload_type), w_dispatch_buffer_push(w_rendering_get_render_dispatch_buffer(world), cmd, &((payload_type){__VA_ARGS__}), sizeof(payload_type)));

#define w_rendering_dispatch_render_cmd(cmd, payload) \
	((void)sizeof(cmd), w_dispatch_buffer_push(w_rendering_get_render_dispatch_buffer(world), cmd, payload, sizeof(*payload)));

#define w_rendering_dispatch_render_cmd_no_payload(cmd) \
	w_rendering_dispatch_render_cmd_value(cmd, bool, false)

/*********************
*  command structs  *
*********************/
// each command is dispatched with an enum ID and a payload struct

// init a window with desired resolution and title
// note: upstream gets flags from display_config and render_config
struct w_rendering_cmd_init_window 
{
	w_vec2i window_resolution;
	char *window_title;
};

struct w_rendering_cmd_framebuffer_set_filter 
{
	enum W_RENDERING_TEXTURE_FILTER filter_type;
};

// clear color
struct w_rendering_cmd_clear_color 
{
	w_color clear_color;
};

// begin camera 3d
struct w_rendering_cmd_camera_begin_3d 
{
	enum W_RENDERING_CAMERA_PROJECTION projection_type;

	// frustum/orthographic values
	double left, right, bottom, top;

	// Z axis near/far clip
	float near_clip, far_clip;

	// look_at view matrix
	w_mat4 view_matrix;
};

#endif /* WHISKER_RENDERING_COMMANDS_H */

