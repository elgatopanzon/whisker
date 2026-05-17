/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_managed_alloc_hooks
 * @created     : Saturday May 16, 2026 19:22:53 CST
 * @description : hooks for the managed alloc module
 */

#include "whisker_managed_alloc.h"

#ifndef WHISKER_MANAGED_ALLOC_HOOKS_H
#define WHISKER_MANAGED_ALLOC_HOOKS_H

// NOTE: type here is -1 because we wont be using the manual registration
// function - we register the same hook for each managed component
w_ecs_component_id_pre_remove_hook(wm_managed_alloc_cleanup_stale_handles, -1, {
	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(world);
	if (registry == NULL) return;

	w_entity_id entity = payload->entity_id;
	w_entity_id component = payload->type_entity_id;

	// get this component to get the handle
	size_t *handle = w_ecs_get_component_(world, component, entity);

	w_slab_arena_free_handle(&registry->slab_arenas[component], *handle);
});

#endif /* WHISKER_MANAGED_ALLOC_HOOKS_H */

