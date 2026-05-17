/**
 * @author      : ElGatoPanzon
 * @file        : whisker_managed_alloc
 * @created     : Saturday May 16, 2026 18:59:41 CST
 * @description : Whisker module to handle managed memory allocations via slab arenas
 */

#include "whisker_managed_alloc.h"
#include "whisker_managed_alloc_hooks.h"

void wm_managed_alloc_init(struct w_ecs_world *world)
{
	struct wm_managed_alloc_registry *registry = w_mem_xcalloc_t(1, *registry);

	w_array_init_t(registry->slab_arenas, WM_MANAGED_ALLOC_SLAB_ARENA_REALLOC_BLOCK_SIZE);

	w_ecs_set_module_resource(world, WM_MANAGED_ALLOC_REGISTRY_RESOURCE_ID, registry);
}

void wm_managed_alloc_free(struct w_ecs_world *world)
{
	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(world);

	// free all used slabs
	for (size_t i = 0; i < registry->slab_arenas_length; ++i)
	{
		if (registry->slab_arenas[i].slabs != NULL)
		{
			w_slab_arena_free(&registry->slab_arenas[i]);
		}
	}

	free(registry->slab_arenas);
	free(registry);
	w_ecs_clear_module_resource(world, WM_MANAGED_ALLOC_REGISTRY_RESOURCE_ID);
}


static bool wm_managed_alloc_ensure_slab_arena_valid(struct wm_managed_alloc_registry *registry, w_entity_id component)
{
	w_array_ensure_alloc_block_size(
		registry->slab_arenas,
		component + 1,
		WM_MANAGED_ALLOC_SLAB_ARENA_REALLOC_BLOCK_SIZE
	);

	// if slabs on this slot is null then its new
	if (registry->slab_arenas[component].slabs == NULL)
	{
		// init the slab arena
		w_slab_arena_init(&registry->slab_arenas[component]);

		// update length to track this slot
		if (component >= registry->slab_arenas_length)
		{
			registry->slab_arenas_length = component + 1;
		}

		return true;
	}

	return false;
}

size_t wm_managed_alloc_malloc_(struct w_ecs_world *world, struct wm_managed_alloc_registry *registry, w_entity_id component, size_t size)
{
	// ensure slab arena exists for this component
	bool is_new_slab_arena = wm_managed_alloc_ensure_slab_arena_valid(registry, component);

	// if its new we need to register the hook for this component
	if (is_new_slab_arena)
	{
		w_ecs_register_component_id_pre_remove_hook(world, component, wm_managed_alloc_cleanup_stale_handles);
	}

	// allocate a handle
	return w_slab_arena_malloc(&registry->slab_arenas[component], size);
}
