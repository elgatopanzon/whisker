/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_timer
 * @created     : Tuesday Mar 24, 2026 12:20:09 CST
 * @description : Timer utility
 */

#include "whisker_std.h"
#include "whisker.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_UTILITIES_TIMER_H
#define WHISKER_UTILITIES_TIMER_H

// timer components
// current elapsed time
#define W_TIMER_COMPONENT_ELAPSED "timer_elapsed"
// target duration for this timer until its completed
#define W_TIMER_COMPONENT_DURATION "timer_duration"
// flag component, if present timer stops being processed
#define W_TIMER_COMPONENT_PAUSED "timer_paused"
// flag component, if present loops the time when finished
#define W_TIMER_COMPONENT_LOOP "timer_loop"
// flag component, if present destroys timer
#define W_TIMER_COMPONENT_ONESHOT "timer_oneshot"
// points to the owner entity
#define W_TIMER_COMPONENT_OWNER_ENTITY "timer_owner_entity"
// string name ID of this timer
#define W_TIMER_COMPONENT_TIMER_NAME_ID "timer_name_id"
// component ID of the constructed string "NAME_finished" for use on the owner
#define W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID "timer_finished_comp_id"

// owner timer component suffixes
// set on the owner as a tag component when this timer is finished
#define W_TIMER_COMPONENT_FINISHED "_finished_tag"
// points to timer entity
#define W_TIMER_COMPONENT_TIMER_ENTITY "_timer_entity"

// create full timer component name
#define W_TIMER_QUERY(name, component) name component

static inline w_entity_id w_timer_create(struct w_ecs_world *world, w_entity_id owner, const char *name, float duration, bool loop, bool oneshot) {
	// create timer entity with name
    char timer_name[128];
    uint8_t hash_bytes[8];
	w_rand_bytes(hash_bytes, 8);
	char hash[17];
	for (int i = 0; i < 8; i++)
    	snprintf(hash + i*2, 3, "%02x", hash_bytes[i]);
	hash[16] = '\0';
    snprintf(timer_name, sizeof(timer_name), "%s_%s", name, hash);
    w_entity_id timer = w_ecs_request_entity_with_name(world, timer_name);
    
    // timer components
    w_ecs_set_str(world, float, W_TIMER_COMPONENT_ELAPSED, timer, &(float){0});
    w_ecs_set_str(world, float, W_TIMER_COMPONENT_DURATION, timer, &duration);
    w_ecs_set_str(world, w_entity_id, W_TIMER_COMPONENT_OWNER_ENTITY, timer, &owner);
    
    // build "{name}_finished_tag" and get its component ID
    char finished_name[128];
    snprintf(finished_name, sizeof(finished_name), "%s" W_TIMER_COMPONENT_FINISHED, name);
    w_entity_id finished_comp_id = w_ecs_get_component_by_name(world, finished_name);
    w_ecs_set_str(world, w_entity_id, W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID, timer, &finished_comp_id);
    
    // flags
    if (loop)
        w_ecs_set_tag_str(world, W_TIMER_COMPONENT_LOOP, timer);
    if (oneshot)
        w_ecs_set_tag_str(world, W_TIMER_COMPONENT_ONESHOT, timer);
    
    // owner reference: "{name}_timer_entity"
    char timer_entity_name[128];
    snprintf(timer_entity_name, sizeof(timer_entity_name), "%s" W_TIMER_COMPONENT_TIMER_ENTITY, name);
    w_ecs_set_str(world, w_entity_id, timer_entity_name, owner, &timer);
    
    return timer;
}

// main timer update system
w_ecs_system(
	wm_utils_timer_update_system, 
	WM_PHASE_POST, 
		w_query_write(W_TIMER_COMPONENT_ELAPSED)
		w_query_read(W_TIMER_COMPONENT_DURATION)
		w_query_read(W_TIMER_COMPONENT_OWNER_ENTITY)
		w_query_read(W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID)
		w_query_optional(W_TIMER_COMPONENT_LOOP)
		w_query_optional(W_TIMER_COMPONENT_ONESHOT)
		w_query_optional(W_TIMER_COMPONENT_PAUSED)
	, 
{
	float *elapsed = w_itor_get_write(float);
	float duration = w_itor_get_read(float);
	w_entity_id owner = w_itor_get_read(w_entity_id);
	w_entity_id finished_comp_id = w_itor_get_read(w_entity_id);
	bool has_loop = (w_itor_get_optional(void) != NULL);
	bool has_oneshot = (w_itor_get_optional(void) != NULL);
	bool has_paused = (w_itor_get_optional(void) != NULL);

	if (has_paused) continue;

	// main timer update
	*elapsed += delta_time;

	if (w_entity_is_valid(finished_comp_id) && w_entity_is_valid(owner) && duration > 0.0f && *elapsed >= duration)
	{
		// stage 1: set finished tag on owner if not present
		if (!w_ecs_has_tag(world, finished_comp_id, owner))
		{
			w_ecs_set_tag(world, finished_comp_id, owner);
		}

		// stage 2: when the tag exists, remove it and run reset/destroy logic
		else
		{
			w_ecs_remove_tag(world, finished_comp_id, owner);

			// auto-loop
			if (has_loop)
				*elapsed = 0;

			// destroy one-shot timers
			else if (has_oneshot)
				w_ecs_return_entity(world, itor.entity_id);

			// pause timer
			else
				w_ecs_set_tag_str(world, W_TIMER_COMPONENT_PAUSED, itor.entity_id);
		}
	}
});

#endif /* WHISKER_UTILITIES_TIMER_H */

