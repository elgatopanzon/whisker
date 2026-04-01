/**
 * @author      : ElGatoPanzon
 * @file        : whisker_events
 * @created     : Wednesday Apr 01, 2026 16:56:23 CST
 * @description : Fire-and-forget event components with automatic cleanup
 */

#include "whisker_events.h"

void wm_events_init(struct w_ecs_world *world)
{
	// guard: don't re-init if state already exists
	if (wm_events_get_state(world)) return;

	struct wm_events_state *state = w_arena_malloc(world->arena, sizeof(*state));
	memset(state, 0, sizeof(*state));
	state->arena = world->arena;

	w_array_init_t(state->removal_buffer, WM_EVENTS_REMOVAL_BUFFER_BLOCK_SIZE);
	state->removal_buffer_length = 0;

	w_ecs_set_module_resource(world, WM_EVENTS_MODULE_RESOURCE_ID, state);

	// register cleanup hook
	wm_events_cleanup_register(world);
}

void wm_events_free(struct w_ecs_world *world)
{
	struct wm_events_state *state = wm_events_get_state(world);
	if (!state) return;

	free_null(state->removal_buffer);
}

struct wm_events_state *wm_events_get_state(struct w_ecs_world *world)
{
	return w_ecs_get_module_resource(world, WM_EVENTS_MODULE_RESOURCE_ID);
}
