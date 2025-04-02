/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_module_resource
 * @created     : Wednesday Apr 02, 2025 08:54:02 CST
 */

#include "whisker_std.h"

#include "whisker_ecs_module_resource.h"

void wm_resource_init(struct w_ecs *ecs)
{

}

w_entity_id wm_resource_create(struct w_world *world, char *name, void *resource_ptr)
{
	w_entity_id e = wm_resource_get_entity(world, name);
	struct wm_resource_component resource = {.resource_ptr = resource_ptr};
	w_set_named_component(world, MACRO_STR(WM_RESOURCE_COMPONENT_NAME), sizeof(resource), e, &resource);
	return e;
}

void *wm_resource(struct w_world *world, char *name)
{
	struct wm_resource_component *resource = wm_resource_get_component(world, name);
	if (resource)
	{
		return resource->resource_ptr;
	}

	return NULL;
}

w_entity_id wm_resource_get_entity(struct w_world *world, char *name)
{
	return w_create_named_entity(world, name);
}

void *wm_resource_get_component(struct w_world *world, char *name)
{
	struct wm_resource_component *resource = w_get_named_component(world, MACRO_STR(WM_RESOURCE_COMPONENT_NAME), wm_resource_get_entity(world, name));

	return resource;
}

void wm_resource_destroy(struct w_world *world, char *name)
{
	w_remove_named_component(world, MACRO_STR(WM_RESOURCE_COMPONENT_NAME), wm_resource_get_entity(world, name));
}
