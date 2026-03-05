/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_system_registry
 * @created     : Wednesday Mar 04, 2026 19:53:18 CST
 */

#include "whisker_std.h"

#include "whisker_system_registry.h"


void w_system_registry_init(struct w_system_registry *registry)
{
	registry->systems_length = 0;

	w_array_init_t(registry->systems, 16);
}
void w_system_registry_free(struct w_system_registry *registry)
{
	free_null(registry->systems);
	registry->systems_length = 0;
}


size_t w_system_register_system(struct w_system_registry *registry, struct w_system *system)
{
	w_array_ensure_alloc_block_size(
		registry->systems,
		registry->systems_length + 1,
		16
	);

	system->enabled = true;
	system->last_update_ticks = 0;

	registry->systems[registry->systems_length] = *system;

	return registry->systems_length++;
}

void w_system_set_system_state(struct w_system_registry *registry, size_t system_id, bool state)
{
	if (registry->systems_length <= system_id) return;

	w_system_get_system_entry(registry, system_id)->enabled = state;
}

struct w_system *w_system_get_system_entry(struct w_system_registry *registry, size_t system_id)
{
	if (registry->systems_length <= system_id) return NULL;

	return &registry->systems[system_id];
}
