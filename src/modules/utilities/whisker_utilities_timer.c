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
