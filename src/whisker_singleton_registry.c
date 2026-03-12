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
	w_hashmap_init(&registry->map, arena, W_SINGLETON_REGISTRY_BUCKET_COUNT, sizeof(void *), w_xxhash64_hash, w_hashmap_eq_default);
}

void w_singleton_registry_free(struct w_singleton_registry *registry)
{
	w_hashmap_free(&registry->map);
	registry->arena = NULL;
}

void w_singleton_registry_set(struct w_singleton_registry *registry, char *name, void *ptr)
{
	w_hashmap_set(&registry->map, name, strlen(name), &ptr);
}

void *w_singleton_registry_get(struct w_singleton_registry *registry, char *name)
{
	void **slot = w_hashmap_get(&registry->map, name, strlen(name));
	return slot ? *slot : NULL;
}

bool w_singleton_registry_has(struct w_singleton_registry *registry, char *name)
{
	return w_hashmap_get(&registry->map, name, strlen(name)) != NULL;
}

bool w_singleton_registry_remove(struct w_singleton_registry *registry, char *name)
{
	return w_hashmap_remove(&registry->map, name, strlen(name));
}
