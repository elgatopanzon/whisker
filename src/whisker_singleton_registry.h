/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_singleton_registry
 * @created     : Thursday Mar 12, 2026 11:48:23 CST
 * @description : string-keyed void* singleton storage registry
 */

#include "whisker_std.h"
#include "whisker_arena.h"
#include "whisker_hashmap.h"
#include "whisker_hash_xxhash64.h"

#ifndef WHISKER_SINGLETON_REGISTRY_H
#define WHISKER_SINGLETON_REGISTRY_H

#ifndef W_SINGLETON_REGISTRY_BUCKET_COUNT
#define W_SINGLETON_REGISTRY_BUCKET_COUNT 64
#endif /* ifndef W_SINGLETON_REGISTRY_BUCKET_COUNT */

// stores named void* pointers as global singletons
struct w_singleton_registry
{
	struct w_arena *arena;
	struct w_hashmap map;
};

// init a singleton registry using the given arena for bucket storage
void w_singleton_registry_init(struct w_singleton_registry *registry, struct w_arena *arena);

// free all singleton entries and map storage
void w_singleton_registry_free(struct w_singleton_registry *registry);

// set a singleton pointer by name
void w_singleton_registry_set(struct w_singleton_registry *registry, char *name, void *ptr);

// get a singleton pointer by name, returns NULL if not found
void *w_singleton_registry_get(struct w_singleton_registry *registry, char *name);

// check if a singleton exists by name
bool w_singleton_registry_has(struct w_singleton_registry *registry, char *name);

// remove a singleton by name, returns true if it existed
bool w_singleton_registry_remove(struct w_singleton_registry *registry, char *name);

#endif /* WHISKER_SINGLETON_REGISTRY_H */
