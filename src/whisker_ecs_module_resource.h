/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_module_resource
 * @created     : Wednesday Apr 02, 2025 08:53:53 CST
 */

#include "whisker_std.h"
#include "whisker_macros.h"
#include "whisker_ecs.h"

#ifndef WHISKER_ECS_MODULE_RESOURCE_H
#define WHISKER_ECS_MODULE_RESOURCE_H

/* resource module
*
*  resources are stored as components containing a void* pointer to a resource.
*  each individual resource is a named entity with the resource component type
*  attached to it.
*/

#define WM_RESOURCE_COMPONENT_NAME wm_resource_component

struct wm_resource_component 
{
	void *resource_ptr;
};

#define wm_resource_t(w, n, t) \
	((t*) wm_resource(w, n))

void wm_resource_init(struct w_ecs *ecs);

w_entity_id wm_resource_create(struct w_world *world, char *name, void *resource_ptr);
void *wm_resource(struct w_world *world, char *name);
w_entity_id wm_resource_get_entity(struct w_world *world, char *name);
void *wm_resource_get_component(struct w_world *world, char *name);
void wm_resource_destroy(struct w_world *world, char *name);

#endif /* WHISKER_ECS_MODULE_RESOURCE_H */

