/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_singleton_registry
 * @created     : Thursday Mar 12, 2026 11:48:23 CST
 */

#include "whisker_std.h"

#include "whisker_singleton_registry.h"

void w_singleton_registry_init(struct w_singleton_registry *registry, struct w_arena *arena)
{
	registry->arena = arena;
	w_hashmap_t_init(&registry->map, arena, W_SINGLETON_REGISTRY_BUCKET_COUNT, w_hashmap_hash_str, w_hashmap_eq_str);
}

void w_singleton_registry_free(struct w_singleton_registry *registry)
{
	w_hashmap_t_free(&registry->map);
	registry->arena = NULL;
}

void w_singleton_registry_set(struct w_singleton_registry *registry, char *name, void *ptr)
{
	w_hashmap_t_set(&registry->map, (const char *)name, ptr);
}

void *w_singleton_registry_get(struct w_singleton_registry *registry, char *name)
{
	void **slot = NULL;
	w_hashmap_t_get(&registry->map, (const char *)name, slot);
	return slot ? *slot : NULL;
}

bool w_singleton_registry_has(struct w_singleton_registry *registry, char *name)
{
	void **slot = NULL;
	w_hashmap_t_get(&registry->map, (const char *)name, slot);
	return slot != NULL;
}

bool w_singleton_registry_remove(struct w_singleton_registry *registry, char *name)
{
	bool removed;
	w_hashmap_t_remove(&registry->map, (const char *)name, removed);
	return removed;
}
