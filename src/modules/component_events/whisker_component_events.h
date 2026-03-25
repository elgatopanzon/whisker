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

#ifndef WM_COMPONENT_EVENTS_ALLOW_MAP_BUCKET_COUNT
#define WM_COMPONENT_EVENTS_ALLOW_MAP_BUCKET_COUNT 64
#endif

#ifndef WM_COMPONENT_EVENTS_TAG_ARRAY_BLOCK_SIZE
#define WM_COMPONENT_EVENTS_TAG_ARRAY_BLOCK_SIZE 64
#endif

/*****************************
*  component names           *
*****************************/

// cleanup component: holds w_pack32x2{owner_entity, event_tag_id}
#define WM_COMPONENT_EVENTS_CLEANUP "wm_component_events_cleanup"

// event tag suffixes appended to component name
#define WM_COMPONENT_EVENTS_ADDED_SUFFIX "_added"
#define WM_COMPONENT_EVENTS_CHANGED_SUFFIX "_changed"
#define WM_COMPONENT_EVENTS_REMOVED_SUFFIX "_removed"


/*****************************
*  data structures           *
*****************************/

// typed hashmap: w_entity_id -> bool (allow list for tracked components)
w_hashmap_t_declare(w_entity_id, bool, wm_component_events_allow_map);

// singleton registry for component events
struct wm_component_events_registry
{
	struct w_arena *arena;

	// allow list: which component IDs are being tracked
	struct wm_component_events_allow_map allow_list;

	// cached event tag IDs indexed by type_entity_id (comp_id)
	// W_ENTITY_INVALID means not yet registered
	w_array_declare(w_entity_id, added_tag_ids);
	w_array_declare(w_entity_id, changed_tag_ids);
	w_array_declare(w_entity_id, removed_tag_ids);
};


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

// register component for event tracking then set its value
#define w_ecs_set_tracked(w, t, te, e, d) do { \
	wm_component_events_register(w, te); \
	w_ecs_set_component_(w, W_COMPONENT_TYPE##_##t, te, e, (t*)d, sizeof(t)); \
} while(0)

#define w_ecs_set_tracked_str(w, t, n, e, d) do { \
	w_entity_id _te = w_ecs_get_component_by_name(w, n); \
	wm_component_events_register(w, _te); \
	w_ecs_set_component_(w, W_COMPONENT_TYPE##_##t, _te, e, (t*)d, sizeof(t)); \
} while(0)


/*****************************
*  cleanup system            *
*****************************/

w_ecs_system(
	wm_component_events_cleanup_system,
	WM_PHASE_PRE,
		w_query_read(WM_COMPONENT_EVENTS_CLEANUP)
	,
{
	w_pack32x2 pair = w_itor_get_read(w_pack32x2);
	w_entity_id owner = pair.left;
	w_entity_id event_tag = pair.right;

	// remove the event tag from the owner entity
	w_ecs_remove_tag(world, event_tag, owner);

	// destroy the cleanup entity
	w_ecs_return_entity(world, itor.entity_id);
});

#endif /* WHISKER_COMPONENT_EVENTS_H */
