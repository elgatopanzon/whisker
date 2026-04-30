/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_random_systems
 * @created     : Tuesday Apr 28, 2026 14:12:33 CST
 * @description : 
 */

#include "whisker_utilities_random.h"

#ifndef WHISKER_UTILITIES_RANDOM_SYSTEMS_H
#define WHISKER_UTILITIES_RANDOM_SYSTEMS_H

// main random update system
w_ecs_system(
	wm_utils_random_update_system,
	WM_PHASE_POST,
	w_query(
		w_query_w(random_state),
		w_query_w(random_increment),
		w_query_w(random_output_long),
		w_query_w(random_output_double),
		w_query_r(random_range_min),
		w_query_r(random_range_max),
		w_query_r(random_owner_entity),
		w_query_r(random_output_long_comp_id),
		w_query_r(random_output_double_comp_id),
	)
	,
{
	// auto-init if state == 0
	uint64_t *state = w_query_get(random_state);
	uint64_t *increment = w_query_get(random_increment);

	if (*state == 0) {
		uint64_t seed = (uint64_t)entity ^ w_time_precise();
		uint64_t stream = (uint64_t)entity;
		w_pcg_init(state, increment, seed, stream);
	}

	// generate new values
	int64_t range_min = *w_query_get(random_range_min);
	int64_t range_max = *w_query_get(random_range_max);
	int64_t *output_long = w_query_get(random_output_long);
	double *output_double = w_query_get(random_output_double);

	*output_long = w_pcg_next_int64(state, *increment, range_min, range_max);
	*output_double = w_pcg_next_double(state, *increment);

	// update owner value components using pre-registered component IDs
	w_entity_id owner = *w_query_get(random_owner_entity);
	w_entity_id value_long_comp_id = *w_query_get(random_output_long_comp_id);
	w_entity_id value_double_comp_id = *w_query_get(random_output_double_comp_id);

	if (w_entity_is_valid(value_long_comp_id) && w_entity_is_valid(owner)) {
		w_set_id(owner, random_output_long, value_long_comp_id, output_long);
	}
	if (w_entity_is_valid(value_double_comp_id) && w_entity_is_valid(owner)) {
		w_ecs_set(world, double, value_double_comp_id, owner, output_double);
		w_set_id(owner, random_output_double, value_double_comp_id, output_double);
	}
});

#endif /* WHISKER_UTILITIES_RANDOM_SYSTEMS_H */

