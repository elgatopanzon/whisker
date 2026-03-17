/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_hook_registry
 * @created     : Thursday Mar 05, 2026 16:09:41 CST
 * @description : register functions as hooks grouped by ID
 */

#include "whisker_std.h"

#include "whisker_hook_registry.h"

void w_hook_registry_init(struct w_hook_registry *registry)
{
	w_array_init_t(registry->hook_groups, W_HOOK_REGISTRY_GROUP_REALLOC_BLOCK_SIZE);
	registry->hook_groups_length = 0;
}
void w_hook_registry_free(struct w_hook_registry *registry)
{
	for (size_t i = 0; i < registry->hook_groups_length; ++i)
	{
		if (registry->hook_groups[i].hooks)
		{
			free_null(registry->hook_groups[i].hooks);
			registry->hook_groups[i].hooks_length = 0;
		}
	}

	free_null(registry->hook_groups);
	registry->hook_groups_length = 0;
}

size_t w_hook_registry_register_hook(struct w_hook_registry *registry, uint hook_group, w_hook_fn hook_fn)
{
	w_array_ensure_alloc_block_size(
		registry->hook_groups,
		hook_group + 1,
		W_HOOK_REGISTRY_GROUP_REALLOC_BLOCK_SIZE
	);
	if (hook_group + 1 > registry->hook_groups_length)
	{
		for (size_t i = registry->hook_groups_length; i <= hook_group; ++i)
		{
			registry->hook_groups[i].hooks = NULL;
			registry->hook_groups[i].hooks_length = 0;
			registry->hook_groups[i].hooks_size = 0;
		}
		registry->hook_groups_length = hook_group + 1;
	}

	w_array_ensure_alloc_block_size(
		registry->hook_groups[hook_group].hooks,
		registry->hook_groups[hook_group].hooks_length + 1,
		W_HOOK_REGISTRY_ENTRY_REALLOC_BLOCK_SIZE
	);

	struct w_hook_entry *entry = &registry->hook_groups[hook_group].hooks[registry->hook_groups[hook_group].hooks_length++];
	entry->hook_fn = hook_fn;
	entry->enabled = true;

	return registry->hook_groups[hook_group].hooks_length - 1;
}

void w_hook_registry_run_hooks(struct w_hook_registry *registry, uint hook_group, void *ctx, void *data)
{
	if (hook_group >= registry->hook_groups_length || registry->hook_groups_length == 0)
		return;

	struct w_hook_group *group = &registry->hook_groups[hook_group];
	if (!group->hooks) return;

	for (size_t i = 0; i < group->hooks_length; ++i)
	{
		if (group->hooks[i].enabled)
		{
			group->hooks[i].hook_fn(ctx, data);
		}
	}
}

void w_hook_registry_run_hooks_from_index(struct w_hook_registry *registry, uint hook_group, size_t start_index, void *ctx, void *data)
{
	if (hook_group >= registry->hook_groups_length || registry->hook_groups_length == 0)
		return;

	struct w_hook_group *group = &registry->hook_groups[hook_group];
	if (!group->hooks) return;

	for (size_t i = start_index; i < group->hooks_length; ++i)
	{
		if (group->hooks[i].enabled)
		{
			group->hooks[i].hook_fn(ctx, data);
		}
	}
}

struct w_hook_entry *w_hook_registry_get_hook_entry(struct w_hook_registry *registry, uint hook_group, uint hook_id)
{
	if (hook_group >= registry->hook_groups_length || registry->hook_groups_length == 0)
		return NULL;

	struct w_hook_group *group = &registry->hook_groups[hook_group];
	if (!group->hooks) return NULL;

	if (hook_id >= group->hooks_length || group->hooks_length == 0)
		return NULL;

	return &group->hooks[hook_id];
}
