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
		w_query_write(W_RANDOM_COMPONENT_STATE)
		w_query_write(W_RANDOM_COMPONENT_INCREMENT)
		w_query_write(W_RANDOM_COMPONENT_OUTPUT_LONG)
		w_query_write(W_RANDOM_COMPONENT_OUTPUT_DOUBLE)
		w_query_read(W_RANDOM_COMPONENT_RANGE_MIN)
		w_query_read(W_RANDOM_COMPONENT_RANGE_MAX)
		w_query_read(W_RANDOM_COMPONENT_OWNER_ENTITY)
		w_query_read(W_RANDOM_COMPONENT_VALUE_LONG_COMP_ID)
		w_query_read(W_RANDOM_COMPONENT_VALUE_DOUBLE_COMP_ID)
	,
{
	uint64_t *state = w_itor_get_write(uint64_t);
	uint64_t *increment = w_itor_get_write(uint64_t);
	int64_t *output_long = w_itor_get_write(int64_t);
	double *output_double = w_itor_get_write(double);
	int64_t range_min = w_itor_get_read(int64_t);
	int64_t range_max = w_itor_get_read(int64_t);
	w_entity_id owner = w_itor_get_read(w_entity_id);
	w_entity_id value_long_comp_id = w_itor_get_read(w_entity_id);
	w_entity_id value_double_comp_id = w_itor_get_read(w_entity_id);

	// auto-init if state == 0
	if (*state == 0) {
		uint64_t seed = (uint64_t)itor.entity_id ^ w_time_precise();
		uint64_t stream = (uint64_t)itor.entity_id;
		w_pcg_init(state, increment, seed, stream);
	}

	// generate new values
	*output_long = w_pcg_next_int64(state, *increment, range_min, range_max);
	*output_double = w_pcg_next_double(state, *increment);

	// update owner value components using pre-registered component IDs
	if (w_entity_is_valid(value_long_comp_id) && w_entity_is_valid(owner)) {
		w_ecs_set(world, int64_t, value_long_comp_id, owner, output_long);
	}
	if (w_entity_is_valid(value_double_comp_id) && w_entity_is_valid(owner)) {
		w_ecs_set(world, double, value_double_comp_id, owner, output_double);
	}
});

#endif /* WHISKER_UTILITIES_RANDOM_SYSTEMS_H */

