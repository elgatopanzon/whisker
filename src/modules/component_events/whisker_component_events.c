/**
 * @author      : ElGatoPanzon
 * @file        : whisker_component_events
 * @created     : Tuesday Mar 24, 2026 21:21:46 CST
 * @description : Track added/changed/removed events on ECS components
 */

#include "whisker_component_events.h"


/*****************************
*  internal helpers          *
*****************************/

// ensure event_tags array is large enough for comp_id
static void ensure_event_tags_(struct wm_component_events_registry *reg, w_entity_id comp_id)
{
	size_t needed = (size_t)comp_id + 1;
	if (needed <= reg->event_tags_length) return;

	size_t old_len = reg->event_tags_length;
	w_array_ensure_alloc_block_size(reg->event_tags, needed, WM_COMPONENT_EVENTS_TAG_ARRAY_BLOCK_SIZE);
	for (size_t i = old_len; i < needed; i++)
	{
		reg->event_tags[i].added = W_ENTITY_INVALID;
		reg->event_tags[i].changed = W_ENTITY_INVALID;
		reg->event_tags[i].removed = W_ENTITY_INVALID;
		reg->event_tags[i].initialized = false;
	}
	reg->event_tags_length = needed;
}

// lazy-init all 3 tag IDs for a component
static void ensure_tags_initialized_(
	struct wm_component_events_registry *reg,
	struct w_ecs_world *world,
	w_entity_id comp_id)
{
	ensure_event_tags_(reg, comp_id);
	if (reg->event_tags[comp_id].initialized) return;

	char *comp_name = w_ecs_get_component_name(world, comp_id);
	if (!comp_name) return;

	char tag_name[256];

	snprintf(tag_name, sizeof(tag_name), "%s%s", comp_name, WM_COMPONENT_EVENTS_ADDED_SUFFIX);
	reg->event_tags[comp_id].added = w_ecs_get_component_by_name(world, tag_name);

	snprintf(tag_name, sizeof(tag_name), "%s%s", comp_name, WM_COMPONENT_EVENTS_CHANGED_SUFFIX);
	reg->event_tags[comp_id].changed = w_ecs_get_component_by_name(world, tag_name);

	snprintf(tag_name, sizeof(tag_name), "%s%s", comp_name, WM_COMPONENT_EVENTS_REMOVED_SUFFIX);
	reg->event_tags[comp_id].removed = w_ecs_get_component_by_name(world, tag_name);

	reg->event_tags[comp_id].initialized = true;
}

/*****************************
*  hook callbacks            *
*****************************/

static void pre_set_hook_(void *world_, void *data_)
{
	struct w_ecs_world *world = world_;
	uint8_t *payload = data_;
	struct w_component_action_payload *p = (struct w_component_action_payload *)payload;
	void *new_data = payload + sizeof(*p);

	struct wm_component_events_registry *reg = wm_component_events_get_registry(world);
	if (!reg) return;

	// always use safe has - unsafe macros don't check sparse bitset bounds
	bool had = w_component_has_(&world->components, p->type_entity_id, p->entity_id);

	// get entry for fast data access after confirming component exists
	struct w_component_entry *entry = had
		? w_component_registry_get_entry(&world->components, p->type_entity_id)
		: NULL;

	if (!had)
	{
		// component being added for first time
		wm_component_events_add_tag_(world, p->entity_id, reg->event_tags[p->type_entity_id].added, reg);
	}
	else
	{
		// component already exists, check if value changed
		void *existing = NULL;
		if (entry) existing = w_component_get_entry(entry, p->entity_id, void);
		else existing = w_ecs_get_component_(world, p->type_entity_id, p->entity_id);

		if (existing && p->data_size > 0 && memcmp(existing, new_data, p->data_size) != 0)
		{
			wm_component_events_add_tag_(world, p->entity_id, reg->event_tags[p->type_entity_id].changed, reg);
		}
	}
}

static void pre_remove_hook_(void *world_, void *data_)
{
	struct w_ecs_world *world = world_;
	struct w_component_action_payload *p = data_;

	struct wm_component_events_registry *reg = wm_component_events_get_registry(world);
	if (!reg) return;

	wm_component_events_add_tag_(world, p->entity_id, reg->event_tags[p->type_entity_id].removed, reg);
}



/*****************************
*  module API                *
*****************************/

void wm_component_events_init(struct w_ecs_world *world)
{
	// guard: don't re-init if registry already exists
	if (wm_component_events_get_registry(world)) return;

	struct wm_component_events_registry *reg =
		w_arena_malloc(world->arena, sizeof(*reg));
	memset(reg, 0, sizeof(*reg));
	reg->arena = world->arena;
	reg->world = world;

	w_array_init_t(reg->event_tags, WM_COMPONENT_EVENTS_TAG_ARRAY_BLOCK_SIZE);
	reg->event_tags_length = 0;

	w_array_init_t(reg->removal_buffer, WM_COMPONENT_EVENTS_REMOVAL_BUFFER_BLOCK_SIZE);
	reg->removal_buffer_length = 0;

	w_ecs_set_module_resource(world, WM_COMPONENT_EVENTS_MODULE_RESOURCE_ID, reg);

	// register cleanup hook to run at beginning of each update
	component_events_cleanup_register(world);
}

void wm_component_events_free(struct w_ecs_world *world)
{
	struct wm_component_events_registry *reg = wm_component_events_get_registry(world);
	if (!reg) return;

	free_null(reg->event_tags);
	free_null(reg->removal_buffer);
}

void wm_component_events_register(struct w_ecs_world *world, w_entity_id comp_id)
{
	// eagerly init all 3 tag IDs at registration time
	struct wm_component_events_registry *reg = wm_component_events_get_registry(world);
	if (!reg) return;

	// check if already tracked via initialized flag
	ensure_event_tags_(reg, comp_id);
	if (reg->event_tags[comp_id].initialized) return;

	ensure_tags_initialized_(reg, world, comp_id);

	// register per-component hooks
	w_ecs_register_component_id_pre_set_hook(world, comp_id, pre_set_hook_);
	w_ecs_register_component_id_pre_remove_hook(world, comp_id, pre_remove_hook_);
}

struct wm_component_events_registry *wm_component_events_get_registry(struct w_ecs_world *world)
{
	return w_ecs_get_module_resource(world, WM_COMPONENT_EVENTS_MODULE_RESOURCE_ID);
}
