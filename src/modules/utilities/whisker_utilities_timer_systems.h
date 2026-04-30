/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_timer_systems
 * @created     : Tuesday Apr 28, 2026 14:03:17 CST
 * @description : 
 */

#include "whisker_utilities_timer.h"

#ifndef WHISKER_UTILITIES_TIMER_SYSTEMS_H
#define WHISKER_UTILITIES_TIMER_SYSTEMS_H

// main timer update system
w_ecs_system(
	wm_utils_timer_update_system,
	WM_PHASE_POST,
		w_query(
			w_query_w(timer_elapsed),
			w_query_w(timer_flags),
			w_query_r(timer_duration),
			w_query_r(timer_owner_entity),
			w_query_r(timer_finished_comp_id),
		)
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

#endif /* WHISKER_UTILITIES_TIMER_SYSTEMS_H */

