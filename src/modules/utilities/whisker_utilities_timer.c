/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_timer
 * @created     : Tuesday Apr 28, 2026 13:57:06 CST
 */

#include "whisker_std.h"

#include "whisker_utilities_timer.h"

w_entity_id wm_utils_timer_create(struct w_ecs_world *world, w_entity_id owner, const char *name, double duration, bool loop, bool oneshot) {
	// create timer entity with name
	char timer_name[128];
	char hash[17];
	w_rand_chars(hash, 16);
	snprintf(timer_name, sizeof(timer_name), "%s_%s", name, hash);
	w_entity_id timer = w_request_named(timer_name);

	// initialize timer state
	double elapsed = 0.0;
	int flags = W_TIMER_FLAG_NONE;
	if (loop) flags |= W_TIMER_FLAG_LOOP;
	if (oneshot) flags |= W_TIMER_FLAG_ONESHOT;

	// set timer components
	w_set(timer, timer_elapsed, &elapsed);
	w_set(timer, timer_duration, &duration);
	w_set(timer, timer_flags, &flags);
	w_set(timer, timer_owner_entity, &owner);

	// set "{name}_timer_finished" then set the ID 
	w_set_value(timer, timer_finished_comp_id, w_gid(timer_finished, name));

	// owner reference: "{name}_timer_entity"
	w_set_g(owner, timer_entity, name, &timer);

	return timer;
}
