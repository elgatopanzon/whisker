/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_hook_registry
 * @created     : Thursday Mar 05, 2026 16:01:09 CST
 * @description : register functions as hooks grouped by ID
 */

#include "whisker_std.h"
#include "whisker_array.h"

#ifndef WHISKER_HOOK_REGISTRY_H
#define WHISKER_HOOK_REGISTRY_H

#ifndef W_HOOK_REGISTRY_GROUP_REALLOC_BLOCK_SIZE
#define W_HOOK_REGISTRY_GROUP_REALLOC_BLOCK_SIZE 256
#endif /* ifndef W_HOOK_REGISTRY_GROUP_REALLOC_BLOCK_SIZE */

#ifndef W_HOOK_REGISTRY_ENTRY_REALLOC_BLOCK_SIZE
#define W_HOOK_REGISTRY_ENTRY_REALLOC_BLOCK_SIZE 512
#endif /* ifndef W_HOOK_REGISTRY_ENTRY_REALLOC_BLOCK_SIZE */

typedef void (*w_hook_fn)(void *ctx, void *data);

struct w_hook_entry 
{
	w_hook_fn hook_fn;
	bool enabled;
};

struct w_hook_group 
{
	w_array_declare(struct w_hook_entry, hooks);
};

struct w_hook_registry 
{
	w_array_declare(struct w_hook_group, hook_groups);
};

// init hook registry
void w_hook_registry_init(struct w_hook_registry *registry);
// free hook registry hook groups and hooks
void w_hook_registry_free(struct w_hook_registry *registry);

// register a good function to a hook group
size_t w_hook_registry_register_hook(struct w_hook_registry *registry, uint hook_group, w_hook_fn hook_fn);
// execute hooks in the given hook group
void w_hook_registry_run_hooks(struct w_hook_registry *registry, uint hook_group, void *ctx, void *data);
// execute hooks starting from a given index (for migration skipping)
void w_hook_registry_run_hooks_from_index(struct w_hook_registry *registry, uint hook_group, size_t start_index, void *ctx, void *data);

// get a hook entry with the given group and ID
struct w_hook_entry *w_hook_registry_get_hook_entry(struct w_hook_registry *registry, uint hook_group, uint hook_id);

#endif /* WHISKER_HOOK_REGISTRY_H */

