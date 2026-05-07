/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_transform_modifier_systems
 * @created     : Thursday May 07, 2026 12:37:51 CST
 * @description : 
 */

#include "whisker_math.h"
#include "whisker_components.h"
#include "whisker_rendering.h"
#include "whisker_rendering_camera.h"
#include "whisker_rendering_commands.h"

#ifndef WHISKER_RENDERING_TRANSFORM_MODIFIER_SYSTEMS_H
#define WHISKER_RENDERING_TRANSFORM_MODIFIER_SYSTEMS_H

// the billboard implementation supports 2 modes:
// - Y only
// - All
// its a simple rotation modification pipeline
w_ecs_system(
    w_rendering_transform_mod_billboard_sync,
    WM_RENDER_PHASE_POST_SYNC,
    w_query(
        w_query_r(render_billboard),
        w_query_r(render_position_3d),
        w_query_w(render_rotation_3d)
    ),
{
	struct w_rendering_camera_state *cam = w_rendering_get_camera_state(world);
    w_vec3 *pos = w_query_get(render_position_3d);
    w_quat *rot = w_query_get(render_rotation_3d);
    enum W_RENDERING_BILLBOARD billboard_mode = *w_query_get(render_billboard);
    
    // y billboard: only rotate around y to face camera
    if (billboard_mode == W_RENDERING_BILLBOARD_Y)
    {
        float angle = atan2f(cam->camera_position.x - pos->x, 
                             cam->camera_position.z - pos->z);
        *rot = w_quat_mul(*rot, w_quat_rotation_y(angle));
    }
    // full billboard: rotate around all axes to face camera
    else if (billboard_mode == W_RENDERING_BILLBOARD_ALL)
    {
        w_vec3 look_dir;
        look_dir.x = cam->camera_position.x - pos->x;
        look_dir.y = cam->camera_position.y - pos->y;
        look_dir.z = cam->camera_position.z - pos->z;
        look_dir = w_vec3_normalize(look_dir);

        *rot = w_quat_mul(*rot, w_quat_look_rotation(look_dir, cam->camera_up));
    }
});


// the render_scale_screen will keep the scale so it has the same size
// regardless of camera distance
w_ecs_system(
    w_rendering_transform_mod_scale_screen_sync,
    WM_RENDER_PHASE_POST_SYNC,
    w_query(
        w_query_h(render_scale_screen),
        w_query_r(render_position_3d),
        w_query_w(render_scale_3d)
    ),
{
    struct w_rendering_camera_state *cam = w_rendering_get_camera_state(world);
    struct w_rendering_render_config *config = w_rendering_get_render_config(world);

    w_vec3 *pos = w_query_get(render_position_3d);
    w_vec3 *scale = w_query_get(render_scale_3d);

    float distance = w_vec3_length(w_vec3_sub(*pos, cam->camera_position));
    float fov_rad = cam->camera_fov_deg * W_DEG2RAD * 0.5f;
    float scale_factor = distance * 2.0f * tanf(fov_rad) / (float)config->render_resolution.y;

    // scale based on scale factor and world pixel scale
    scale->x *= scale_factor * W_RENDERING_WORLD_PIXEL_SCALE;
    scale->y *= scale_factor * W_RENDERING_WORLD_PIXEL_SCALE;
    scale->z *= scale_factor * W_RENDERING_WORLD_PIXEL_SCALE;
});

#endif /* WHISKER_RENDERING_TRANSFORM_MODIFIER_SYSTEMS_H */

