/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_timer
 * @created     : Tuesday Mar 24, 2026 12:20:09 CST
 * @description : ECS timer utility wrapping standalone timer module
 */

#include "whisker_std.h"
#include "whisker.h"
#include "whisker_timer.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_UTILITIES_TIMER_H
#define WHISKER_UTILITIES_TIMER_H

// timer components
// current elapsed time
#define W_TIMER_COMPONENT_ELAPSED "timer_elapsed"
// target duration for this timer until its completed
#define W_TIMER_COMPONENT_DURATION "timer_duration"
// timer state flags (loop, oneshot, paused, finished)
#define W_TIMER_COMPONENT_FLAGS "timer_flags"
// points to the owner entity
#define W_TIMER_COMPONENT_OWNER_ENTITY "timer_owner_entity"
// component ID of the constructed string "NAME_finished" for use on the owner
#define W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID "timer_finished_comp_id"

// owner timer component suffixes
// set on the owner as a tag component when this timer is finished
#define W_TIMER_COMPONENT_FINISHED "_finished_tag"
// points to timer entity
#define W_TIMER_COMPONENT_TIMER_ENTITY "_timer_entity"

// create full timer component name
#define W_TIMER_QUERY(name, component) name component

static inline w_entity_id w_timer_create(struct w_ecs_world *world, w_entity_id owner, const char *name, double duration, bool loop, bool oneshot) {
	// create timer entity with name
	char timer_name[128];
	char hash[17];
	w_rand_chars(hash, 16);
	snprintf(timer_name, sizeof(timer_name), "%s_%s", name, hash);
	w_entity_id timer = w_ecs_request_entity_with_name(world, timer_name);

	// initialize timer state using standalone functions
	double elapsed = 0.0;
	int flags = W_TIMER_FLAG_NONE;
	if (loop) flags |= W_TIMER_FLAG_LOOP;
	if (oneshot) flags |= W_TIMER_FLAG_ONESHOT;

	// timer components
	w_ecs_set_str(world, double, W_TIMER_COMPONENT_ELAPSED, timer, &elapsed);
	w_ecs_set_str(world, double, W_TIMER_COMPONENT_DURATION, timer, &duration);
	w_ecs_set_str(world, int, W_TIMER_COMPONENT_FLAGS, timer, &flags);
	w_ecs_set_str(world, w_entity_id, W_TIMER_COMPONENT_OWNER_ENTITY, timer, &owner);

	// build "{name}_finished_tag" and get its component ID
	char finished_name[128];
	snprintf(finished_name, sizeof(finished_name), "%s" W_TIMER_COMPONENT_FINISHED, name);
	w_entity_id finished_comp_id = w_ecs_get_component_by_name(world, finished_name);
	w_ecs_set_str(world, w_entity_id, W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID, timer, &finished_comp_id);

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
		w_query_write(W_TIMER_COMPONENT_FLAGS)
		w_query_read(W_TIMER_COMPONENT_DURATION)
		w_query_read(W_TIMER_COMPONENT_OWNER_ENTITY)
		w_query_read(W_TIMER_COMPONENT_TIMER_FINISHED_COMP_ID)
	,
{
	double *elapsed = w_itor_get_write(double);
	w_timer_flags *flags = (w_timer_flags *)w_itor_get_write(int);
	double duration = w_itor_get_read(double);
	w_entity_id owner = w_itor_get_read(w_entity_id);
	w_entity_id finished_comp_id = w_itor_get_read(w_entity_id);

	// skip paused timers
	if (w_timer_is_paused(*flags)) continue;

	// update timer using standalone function
	bool just_finished = w_timer_update(elapsed, duration, flags, delta_time);

	// handle completion
	if (just_finished && w_entity_is_valid(finished_comp_id) && w_entity_is_valid(owner))
	{
		// stage 1: set finished tag on owner if not present
		if (!w_ecs_has_tag(world, finished_comp_id, owner))
		{
			w_ecs_set_tag(world, finished_comp_id, owner);
		}
		// stage 2: tag exists, remove it and run reset/destroy logic
		else
		{
			w_ecs_remove_tag(world, finished_comp_id, owner);

			if (w_timer_is_loop(*flags))
			{
				// wrap elapsed time and clear finished flag for next cycle
				w_timer_wrap(elapsed, duration);
				w_timer_clear_finished(flags);
			}
			else if (w_timer_is_oneshot(*flags))
			{
				// destroy one-shot timers
				w_ecs_return_entity(world, itor.entity_id);
			}
			else
			{
				// pause inert timers (neither loop nor oneshot)
				w_timer_pause(flags);
			}
		}
	}
	// handle loop timer already in finished state from previous frame
	else if (w_timer_is_finished(*flags) && w_entity_is_valid(finished_comp_id) && w_entity_is_valid(owner))
	{
		// same completion logic for when finished flag was set previously
		if (!w_ecs_has_tag(world, finished_comp_id, owner))
		{
			w_ecs_set_tag(world, finished_comp_id, owner);
		}
		else
		{
			w_ecs_remove_tag(world, finished_comp_id, owner);

			if (w_timer_is_loop(*flags))
			{
				w_timer_wrap(elapsed, duration);
				w_timer_clear_finished(flags);
			}
			else if (w_timer_is_oneshot(*flags))
			{
				w_ecs_return_entity(world, itor.entity_id);
			}
			else
			{
				w_timer_pause(flags);
			}
		}
	}
});

#endif /* WHISKER_UTILITIES_TIMER_H */
