/**
 * @author      : ElGatoPanzon
 * @file        : whisker_managed_alloc
 * @created     : Saturday May 16, 2026 18:59:41 CST
 * @description : Whisker module to handle managed memory allocations via slab arenas
 */

#ifndef WHISKER_MANAGED_ALLOC_H
#define WHISKER_MANAGED_ALLOC_H

#include "whisker.h"
#include "whisker_slab_arena.h"
#include "modules/whisker_module_ids.h"

#define WM_MANAGED_ALLOC_REGISTRY_RESOURCE_ID    WM_MODULE_RESOURCE_ID(MANAGED_ALLOC, 0)
#define wm_managed_alloc_get_registry(world) w_ecs_get_module_resource(world, WM_MANAGED_ALLOC_REGISTRY_RESOURCE_ID)

#define WM_MANAGED_ALLOC_SLAB_ARENA_REALLOC_BLOCK_SIZE 16

#define w_ecs_define_managed_component(name) w_ecs_define_component(uint64_t, name);

#define WM_MANAGED_ALLOC_INVALID_HANDLE UINT64_MAX

struct wm_managed_alloc_registry 
{
	w_array_declare(struct w_slab_arena, slab_arenas);
};

// initialize the managed_alloc module
void wm_managed_alloc_init(struct w_ecs_world *world);

// cleanup the managed_alloc module
void wm_managed_alloc_free(struct w_ecs_world *world);

uint64_t wm_managed_alloc_malloc_(struct w_ecs_world *world, struct wm_managed_alloc_registry *registry, w_entity_id component, size_t size);

#define wm_managed_alloc_malloc(world, component, entity, size) \
	({ \
		(void)sizeof(component); \
		w_entity_id comp_id = component##_get_id(world); \
		struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(world); \
		if (w_ecs_has_component_(world, comp_id, entity)) { \
			uint64_t *existing = w_ecs_get_component_(world, comp_id, entity); \
			if (existing != NULL && *existing != WM_MANAGED_ALLOC_INVALID_HANDLE) { \
				w_slab_arena_free_handle(&registry->slab_arenas[comp_id], (size_t)*existing); \
			} \
		} \
		uint64_t handle = wm_managed_alloc_malloc_(world, registry, comp_id, size); \
		component##_set_value(world, entity, ((component){handle})); \
		w_slab_arena_resolve_handle(&registry->slab_arenas[comp_id], (size_t)handle); \
	})

#define wm_managed_alloc_malloc_t(world, component, entity, T) \
	wm_managed_alloc_malloc(world, component, entity, sizeof(T))

#define wm_managed_alloc_free_handle(world, component, entity) \
	do { \
		(void)sizeof(component); \
		w_entity_id comp_id = component##_get_id(world); \
		struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(world); \
		uint64_t *handle = w_ecs_get_component_(world, comp_id, entity); \
		if (handle != NULL && *handle != WM_MANAGED_ALLOC_INVALID_HANDLE) { \
			w_slab_arena_free_handle(&registry->slab_arenas[comp_id], (size_t)*handle); \
			component##_set_value(world, entity, ((component){WM_MANAGED_ALLOC_INVALID_HANDLE})); \
		} \
	} while (0)
	
#define wm_managed_alloc_resolve_handle(world, component, entity) \
	({ \
		(void)sizeof(component); \
		w_entity_id comp_id = component##_get_id(world); \
		struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(world); \
		uint64_t *handle = w_ecs_get_component_(world, comp_id, entity); \
		(handle == NULL || *handle == WM_MANAGED_ALLOC_INVALID_HANDLE) \
			? NULL \
			: w_slab_arena_resolve_handle(&registry->slab_arenas[comp_id], (size_t)*handle); \
	})

#endif /* WHISKER_MANAGED_ALLOC_H */
