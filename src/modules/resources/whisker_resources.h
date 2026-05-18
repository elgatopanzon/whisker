/**
 * @author      : ElGatoPanzon
 * @file        : whisker_resources
 * @created     : Sunday May 17, 2026 15:49:12 CST
 * @description : Resources module providing file loading with cold/hot states
 */

#ifndef WHISKER_RESOURCES_H
#define WHISKER_RESOURCES_H

#include "whisker.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/events/whisker_events.h"

enum WM_RESOURCES_FILE_INVALID 
{ 
	WM_RESOURCES_FILE_INVALID_UNKNOWN,
	WM_RESOURCES_FILE_INVALID_FILE_PATH,
	WM_RESOURCES_FILE_INVALID_NOT_FOUND,
	WM_RESOURCES_FILE_INVALID_NOT_READABLE,
	WM_RESOURCES_FILE_INVALID_NO_SIZE,
	WM_RESOURCES_FILE_INVALID_LOAD_FAILED,
};

/****************
*  components  *
****************/
// holds the virtual path to this resource as a string table ID
w_ecs_define_component(w_string_table_id, resource_file_path_string_id, W_STRING_TABLE_INVALID_ID);

// during enumeration this component will be set to the size in bytes of the
// resource (if its found)
w_ecs_define_component(uint64_t, resource_file_size_bytes, 0);

// simple tag indicating resource file is valid
w_ecs_define_tag(resource_file_valid);

// error component, indicates that the resource file is invalid with custom ID
w_ecs_define_component(int, resource_file_invalid, -1);

// used for hashing the original file (hot reload resources)
w_ecs_define_component(uint64_t, resource_file_hash, UINT64_MAX);

// optional local hash, can be a combination of other components
w_ecs_define_component(uint64_t, resource_hash, UINT64_MAX);

// optional group name this resource belongs to as a string table ID
w_ecs_define_component(w_string_table_id, resource_group_string_id, W_STRING_TABLE_INVALID_ID);


/**************************
*  lifecycle components  *
**************************/
// setting these tags indicates we want the resource hot or cold
w_ecs_define_event(req_resource_hot);
w_ecs_define_event(req_resource_cold);

// once loaded and hot, resources get this tag
w_ecs_define_tag(resource_hot);

// data alloc handle of this resources loaded data
w_ecs_define_component(uint64_t, resource_hot_data_handle, UINT64_MAX);

// final data size of loaded data in managed alloc
w_ecs_define_component(uint64_t, resource_hot_data_size, 0);


/************************
*  functions & macros  *
************************/

#define wm_resources_request_hot(entity) w_set_tag(entity, req_resource_hot, true)
#define wm_resources_request_cold(entity) w_set_tag(entity, req_resource_cold, true)

// initialize the resources module
void wm_resources_init(struct w_ecs_world *world);

// cleanup the resources module
void wm_resources_free(struct w_ecs_world *world);

#endif /* WHISKER_RESOURCES_H */
