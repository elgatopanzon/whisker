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

// ensure a tag ID array is large enough for comp_id, fill new slots with W_ENTITY_INVALID
static void ensure_tag_array_(w_entity_id **arr, _Atomic size_t *arr_size, _Atomic size_t *arr_length, w_entity_id comp_id)
{
	size_t needed = (size_t)comp_id + 1;
	if (needed <= *arr_length) return;

	size_t old_len = *arr_length;
	w_array_ensure_alloc_block_size(*arr, needed, WM_COMPONENT_EVENTS_TAG_ARRAY_BLOCK_SIZE);
	for (size_t i = old_len; i < needed; i++)
		(*arr)[i] = W_ENTITY_INVALID;
	*arr_length = needed;
}

// get or lazy-register an event tag ID for a component
static w_entity_id get_or_create_tag_id_(
	struct wm_component_events_registry *reg,
	struct w_ecs_world *world,
	w_entity_id comp_id,
	w_entity_id *tag_array,
	const char *suffix)
{
	if (tag_array[comp_id] != W_ENTITY_INVALID)
		return tag_array[comp_id];

	// need component name to build tag name
	char *comp_name = w_ecs_get_component_name(world, comp_id);
	if (!comp_name) return W_ENTITY_INVALID;

	char tag_name[256];
	snprintf(tag_name, sizeof(tag_name), "%s%s", comp_name, suffix);

	w_entity_id tag_id = w_ecs_get_component_by_name(world, tag_name);
	tag_array[comp_id] = tag_id;
	return tag_id;
}

// add an event tag to an entity and create a cleanup entity
static void add_event_tag_(struct w_ecs_world *world, w_entity_id entity, w_entity_id tag_id)
{
	// set the event tag on the target entity
	w_ecs_set_tag(world, tag_id, entity);

	// create cleanup entity with the owner+tag pair
	w_entity_id cleanup = w_ecs_request_entity(world);
	w_pack32x2 pair = { .left = entity, .right = tag_id };
	w_ecs_set_str(world, w_pack32x2, WM_COMPONENT_EVENTS_CLEANUP, cleanup, &pair);
}

// check if a comp_id is in the allow list
static bool is_tracked_(struct wm_component_events_registry *reg, w_entity_id comp_id)
{
	bool *result = NULL;
	w_hashmap_t_get(&reg->allow_list, comp_id, result);
	return result != NULL;
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

	// check allow list
	if (!is_tracked_(reg, p->type_entity_id)) return;

	// ensure tag arrays are sized
	ensure_tag_array_(&reg->added_tag_ids, &reg->added_tag_ids_size, &reg->added_tag_ids_length, p->type_entity_id);
	ensure_tag_array_(&reg->changed_tag_ids, &reg->changed_tag_ids_size, &reg->changed_tag_ids_length, p->type_entity_id);

	bool had = w_ecs_has_component_(world, p->type_entity_id, p->entity_id);

	if (!had)
	{
		// component being added for first time
		w_entity_id tag = get_or_create_tag_id_(reg, world, p->type_entity_id, reg->added_tag_ids, WM_COMPONENT_EVENTS_ADDED_SUFFIX);
		if (tag != W_ENTITY_INVALID)
			add_event_tag_(world, p->entity_id, tag);
	}
	else
	{
		// component already exists, check if value changed
		void *existing = w_ecs_get_component_(world, p->type_entity_id, p->entity_id);
		if (existing && p->data_size > 0 && memcmp(existing, new_data, p->data_size) != 0)
		{
			w_entity_id tag = get_or_create_tag_id_(reg, world, p->type_entity_id, reg->changed_tag_ids, WM_COMPONENT_EVENTS_CHANGED_SUFFIX);
			if (tag != W_ENTITY_INVALID)
				add_event_tag_(world, p->entity_id, tag);
		}
	}
}

static void pre_remove_hook_(void *world_, void *data_)
{
	struct w_ecs_world *world = world_;
	struct w_component_action_payload *p = data_;

	struct wm_component_events_registry *reg = wm_component_events_get_registry(world);
	if (!reg) return;

	// check allow list
	if (!is_tracked_(reg, p->type_entity_id)) return;

	// ensure tag array is sized
	ensure_tag_array_(&reg->removed_tag_ids, &reg->removed_tag_ids_size, &reg->removed_tag_ids_length, p->type_entity_id);

	w_entity_id tag = get_or_create_tag_id_(reg, world, p->type_entity_id, reg->removed_tag_ids, WM_COMPONENT_EVENTS_REMOVED_SUFFIX);
	if (tag != W_ENTITY_INVALID)
		add_event_tag_(world, p->entity_id, tag);
}



/*****************************
*  module API                *
*****************************/

void wm_component_events_init(struct w_ecs_world *world)
{
	struct wm_component_events_registry *reg =
		w_arena_malloc(world->arena, sizeof(*reg));
	memset(reg, 0, sizeof(*reg));
	reg->arena = world->arena;

	w_hashmap_t_init(&reg->allow_list, world->arena,
		WM_COMPONENT_EVENTS_ALLOW_MAP_BUCKET_COUNT, w_xxhash64_hash, NULL);

	w_array_init_t(reg->added_tag_ids, WM_COMPONENT_EVENTS_TAG_ARRAY_BLOCK_SIZE);
	reg->added_tag_ids_length = 0;
	w_array_init_t(reg->changed_tag_ids, WM_COMPONENT_EVENTS_TAG_ARRAY_BLOCK_SIZE);
	reg->changed_tag_ids_length = 0;
	w_array_init_t(reg->removed_tag_ids, WM_COMPONENT_EVENTS_TAG_ARRAY_BLOCK_SIZE);
	reg->removed_tag_ids_length = 0;

	w_ecs_singleton_set(world, "component_events", reg);

	// register global PRE hooks
	w_ecs_register_component_pre_set_hook(world, pre_set_hook_);
	w_ecs_register_component_pre_remove_hook(world, pre_remove_hook_);

	// register cleanup system
	wm_component_events_cleanup_system_register(world);
}

void wm_component_events_free(struct w_ecs_world *world)
{
	struct wm_component_events_registry *reg = wm_component_events_get_registry(world);
	if (!reg) return;

	w_hashmap_t_free(&reg->allow_list);
	free_null(reg->added_tag_ids);
	free_null(reg->changed_tag_ids);
	free_null(reg->removed_tag_ids);
}

void wm_component_events_register(struct w_ecs_world *world, w_entity_id comp_id)
{
	struct wm_component_events_registry *reg = wm_component_events_get_registry(world);
	if (!reg) return;

	// already registered, skip
	if (is_tracked_(reg, comp_id)) return;

	w_hashmap_t_set(&reg->allow_list, comp_id, true);
}

struct wm_component_events_registry *wm_component_events_get_registry(struct w_ecs_world *world)
{
	return w_ecs_singleton_get(world, "component_events");
}
