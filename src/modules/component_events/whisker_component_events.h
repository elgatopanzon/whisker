/**
 * @author      : ElGatoPanzon
 * @file        : whisker_component_events
 * @created     : Tuesday Mar 24, 2026 21:21:46 CST
 * @description : Track added/changed/removed events on ECS components
 */

#ifndef WHISKER_COMPONENT_EVENTS_H
#define WHISKER_COMPONENT_EVENTS_H

#include "whisker.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

/*****************************
*  config                    *
*****************************/

// module resource ID for fast world lookup
#include "modules/whisker_module_ids.h"

/* index 0: the event registry singleton */
#define WM_COMPONENT_EVENTS_MODULE_RESOURCE_ID WM_MODULE_RESOURCE_ID(COMPONENT_EVENTS, 0)

#ifndef WM_COMPONENT_EVENTS_TAG_ARRAY_BLOCK_SIZE
#define WM_COMPONENT_EVENTS_TAG_ARRAY_BLOCK_SIZE 64
#endif

#ifndef WM_COMPONENT_EVENTS_REMOVAL_BUFFER_BLOCK_SIZE
#define WM_COMPONENT_EVENTS_REMOVAL_BUFFER_BLOCK_SIZE 128
#endif

/*****************************
*  component names           *
*****************************/

// event tag suffixes appended to component name
#define WM_COMPONENT_EVENTS_ADDED_SUFFIX "_added"
#define WM_COMPONENT_EVENTS_CHANGED_SUFFIX "_changed"
#define WM_COMPONENT_EVENTS_REMOVED_SUFFIX "_removed"

// query macros for changed/added/removed generics
#define w_query_changed(name) w_query_part_generic_(read, name, changed)
#define w_query_added(name) w_query_part_generic_(read, name, added)
#define w_query_removed(name) w_query_part_generic_(read, name, removed)
#define w_query_not_changed(name) w_query_part_generic_(not, name, changed)
#define w_query_not_added(name) w_query_part_generic_(not, name, added)
#define w_query_not_removed(name) w_query_part_generic_(not, name, removed)

/*****************************
*  data structures           *
*****************************/

// cached event tag IDs for a single component
struct wm_component_event_tags
{
	w_entity_id added;
	w_entity_id changed;
	w_entity_id removed;
	bool initialized;
};

// singleton registry for component events
struct wm_component_events_registry
{
	struct w_arena *arena;
	struct w_ecs_world *world;

	// event tag IDs indexed by type_entity_id (comp_id)
	// check initialized flag before accessing tag IDs
	w_array_declare(struct wm_component_event_tags, event_tags);

	// deferred removal buffer: pairs of {owner_entity, tag_component_id}
	w_array_declare(w_pack32x2, removal_buffer);
};


/*****************************
*  inline helpers            *
*****************************/

// add an event tag to an entity and queue for deferred removal
#define wm_component_events_add_tag_(world, entity, tag_id, reg) do { \
	uint8_t tag_val_ = 0; \
	w_component_set_(&(world)->components, W_COMPONENT_TYPE_uint8_t, (tag_id), (entity), &tag_val_, sizeof(tag_val_)); \
	w_array_ensure_alloc_block_size(reg->removal_buffer, reg->removal_buffer_length + 1, WM_COMPONENT_EVENTS_REMOVAL_BUFFER_BLOCK_SIZE); \
	reg->removal_buffer[reg->removal_buffer_length++] = (w_pack32x2){ .left = (entity), .right = (tag_id) }; \
} while(0)


/*****************************
*  module API                *
*****************************/

// initialize the component events module
void wm_component_events_init(struct w_ecs_world *world);

// cleanup the component events module
void wm_component_events_free(struct w_ecs_world *world);

// register a component for event tracking (opt-in)
void wm_component_events_register(struct w_ecs_world *world, w_entity_id comp_id);

// get the registry singleton
struct wm_component_events_registry *wm_component_events_get_registry(struct w_ecs_world *world);


/*****************************
*  convenience macros        *
*****************************/

#define w_ecs_enable_tracking(world, component) \
	({ \
	 	(void)sizeof(name); \
		W_ECS_SET_COMP_ID_CACHE(name); \
		wm_component_events_register(world, name##_component_id_); \
	}) \

// register component for event tracking then set its value
#define w_ecs_set_tracked_id(w, t, te, e, d) do { \
	wm_component_events_register(w, te); \
	w_ecs_set_component_(w, W_COMPONENT_TYPE##_##t, te, e, (t*)d, sizeof(t)); \
} while(0)

#define w_ecs_set_tracked_str(w, t, n, e, d) do { \
	w_entity_id _te = w_ecs_get_component_by_name(w, n); \
	wm_component_events_register(w, _te); \
	w_ecs_set_component_(w, W_COMPONENT_TYPE##_##t, _te, e, (t*)d, sizeof(t)); \
} while(0)

#endif /* WHISKER_COMPONENT_EVENTS_H */
