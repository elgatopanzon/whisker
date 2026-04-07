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

/* index 3: camera state (rendering/camera sub-module) */
#define WM_RENDERING_CAMERA_STATE_RESOURCE_ID WM_MODULE_RESOURCE_ID(RENDERING, 3)

struct w_rendering_camera_state 
{
	bool active;
	w_entity_id camera_entity_id;
};


/***********************
*  camera components  *
***********************/
// these component names go onto each camera entity
// the global camera state points to the camera entity to use

// FOV Y in degrees
#define W_RENDERING_CAMERA_COMPONENT_FOV "camera_fov_deg"

// near and far planes e.g. 0.1 / 1000
#define W_RENDERING_CAMERA_COMPONENT_NEAR_CLIP "camera_near_clip"
#define W_RENDERING_CAMERA_COMPONENT_FAR_CLIP "camera_far_clip"

// projection type 0 = perspective 1 = orthographic
#define W_RENDERING_CAMERA_COMPONENT_PROJECTION "camera_projection"


/********************
*  camera tag      *
********************/
// tag to mark an entity as a valid camera
// entities must have this tag to be eligible as active camera

#define W_RENDERING_CAMERA_TAG "camera_t"


/*************************
*  camera request tags  *
*************************/
// request tags to change active camera
// the tags are added to the camera entities and processed

#define W_RENDERING_CAMERA_REQUEST_ACTIVATE_CAMERA "request_active_camera_t"

#endif /* WHISKER_RENDERING_CAMERA_H */

