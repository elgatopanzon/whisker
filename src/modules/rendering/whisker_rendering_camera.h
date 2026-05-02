/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_camera
 * @created     : Tuesday Mar 31, 2026 11:15:41 CST
 * @description : 3D native camera rendering base module
 */

#include "whisker.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_RENDERING_CAMERA_H
#define WHISKER_RENDERING_CAMERA_H


/******************
*  camera state  *
******************/
// global camera module resource holds active camera entity

#include "modules/whisker_module_ids.h"

/* index 4: camera state (rendering/camera sub-module) */
#define WM_RENDERING_CAMERA_STATE_RESOURCE_ID WM_MODULE_RESOURCE_ID(RENDERING, 4)
#define w_rendering_get_camera_state(w) w_ecs_get_module_resource(w, WM_RENDERING_CAMERA_STATE_RESOURCE_ID)

enum W_RENDERING_CAMERA_PROJECTION
{
	W_RENDERING_CAMERA_PROJECTION_PERSPECTIVE,
	W_RENDERING_CAMERA_PROJECTION_ORTHOGRAPHIC,
};

struct w_rendering_camera_state 
{
	bool active;
	w_entity_id camera_entity_id;

	w_vec3 camera_position;
	w_vec3 camera_target;
	w_vec3 camera_up;
	float camera_near_clip;
	float camera_far_clip;
	float camera_fov_deg;
	enum W_RENDERING_CAMERA_PROJECTION camera_projection;
};

/***********************
*  camera components  *
***********************/
// these component names go onto each camera entity
// the global camera state points to the camera entity to use

// FOV Y in degrees
w_ecs_define_component(float, camera_fov_deg, 45.0f);

// up vector
w_ecs_define_component(w_vec3, camera_up, 0.0f, 1.0f, 0.0f);

// target vector
w_ecs_define_component(w_vec3, camera_target, 0.0f, 1.0f, 0.0f);

// near and far planes e.g. 0.1 / 1000
w_ecs_define_component(float, camera_near_clip, 0.01f);
w_ecs_define_component(float, camera_far_clip, 1000.0f);

// projection type 0 = perspective 1 = orthographic
w_ecs_define_component(int, camera_projection, W_RENDERING_CAMERA_PROJECTION_PERSPECTIVE);


/********************
*  camera tag      *
********************/
// tag to mark an entity as a valid camera
// entities must have this tag to be eligible as active camera

w_ecs_define_tag(camera);


/*************************
*  camera request tags  *
*************************/
// request tags to change active camera
// the tags are added to the camera entities and processed

w_ecs_define_tag(req_active_camera);

#endif /* WHISKER_RENDERING_CAMERA_H */

