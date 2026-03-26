/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_random
 * @created     : Wednesday Mar 26, 2026 15:57:00 CST
 * @description : ECS random utility wrapping standalone PCG random module
 */

#include "whisker_std.h"
#include "whisker.h"
#include "whisker_random.h"
#include "whisker_time.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_UTILITIES_RANDOM_H
#define WHISKER_UTILITIES_RANDOM_H

// random components
// PCG state
#define W_RANDOM_COMPONENT_STATE "random_state"
// PCG increment
#define W_RANDOM_COMPONENT_INCREMENT "random_increment"
// min value for int64 output
#define W_RANDOM_COMPONENT_RANGE_MIN "random_range_min"
// max value for int64 output
#define W_RANDOM_COMPONENT_RANGE_MAX "random_range_max"
// last generated int64 value
#define W_RANDOM_COMPONENT_OUTPUT_LONG "random_output_long"
// last generated double value [0.0, 1.0)
#define W_RANDOM_COMPONENT_OUTPUT_DOUBLE "random_output_double"
// points to the owner entity
#define W_RANDOM_COMPONENT_OWNER_ENTITY "random_owner_entity"

// owner random component suffixes
// stores int64 value on the owner
#define W_RANDOM_COMPONENT_VALUE_LONG "_random_value_long"
// stores double value on the owner
#define W_RANDOM_COMPONENT_VALUE_DOUBLE "_random_value_double"
// points to random entity
#define W_RANDOM_COMPONENT_RANDOM_ENTITY "_random_entity"
// pre-registered component ID for long value on owner
#define W_RANDOM_COMPONENT_VALUE_LONG_COMP_ID "random_value_long_comp_id"
// pre-registered component ID for double value on owner
#define W_RANDOM_COMPONENT_VALUE_DOUBLE_COMP_ID "random_value_double_comp_id"

// create full random component name
#define W_RANDOM_QUERY(name, component) name component

static inline w_entity_id w_random_create(struct w_ecs_world *world, w_entity_id owner, const char *name, int64_t range_min, int64_t range_max) {
	// create random entity with name
	char random_name[128];
	char hash[17];
	w_rand_chars(hash, 16);
	snprintf(random_name, sizeof(random_name), "%s_%s", name, hash);
	w_entity_id random = w_ecs_request_entity_with_name(world, random_name);

	// random components - state/increment initialized to 0, auto-init in system
	uint64_t state = 0;
	uint64_t increment = 0;
	int64_t output_long = 0;
	double output_double = 0.0;

	w_ecs_set_str(world, uint64_t, W_RANDOM_COMPONENT_STATE, random, &state);
	w_ecs_set_str(world, uint64_t, W_RANDOM_COMPONENT_INCREMENT, random, &increment);
	w_ecs_set_str(world, int64_t, W_RANDOM_COMPONENT_RANGE_MIN, random, &range_min);
	w_ecs_set_str(world, int64_t, W_RANDOM_COMPONENT_RANGE_MAX, random, &range_max);
	w_ecs_set_str(world, int64_t, W_RANDOM_COMPONENT_OUTPUT_LONG, random, &output_long);
	w_ecs_set_str(world, double, W_RANDOM_COMPONENT_OUTPUT_DOUBLE, random, &output_double);
	w_ecs_set_str(world, w_entity_id, W_RANDOM_COMPONENT_OWNER_ENTITY, random, &owner);

	// owner reference: "{name}_random_entity"
	char random_entity_name[128];
	snprintf(random_entity_name, sizeof(random_entity_name), "%s" W_RANDOM_COMPONENT_RANDOM_ENTITY, name);
	w_ecs_set_str(world, w_entity_id, random_entity_name, owner, &random);

	// build "{name}_random_value_long" and get its component ID for fast path
	char value_long_name[128];
	snprintf(value_long_name, sizeof(value_long_name), "%s" W_RANDOM_COMPONENT_VALUE_LONG, name);
	w_entity_id value_long_comp_id = w_ecs_get_component_by_name(world, value_long_name);
	w_ecs_set_str(world, w_entity_id, W_RANDOM_COMPONENT_VALUE_LONG_COMP_ID, random, &value_long_comp_id);

	// build "{name}_random_value_double" and get its component ID for fast path
	char value_double_name[128];
	snprintf(value_double_name, sizeof(value_double_name), "%s" W_RANDOM_COMPONENT_VALUE_DOUBLE, name);
	w_entity_id value_double_comp_id = w_ecs_get_component_by_name(world, value_double_name);
	w_ecs_set_str(world, w_entity_id, W_RANDOM_COMPONENT_VALUE_DOUBLE_COMP_ID, random, &value_double_comp_id);

	// owner value components (initialized to 0)
	w_ecs_set_str(world, int64_t, value_long_name, owner, &output_long);
	w_ecs_set_str(world, double, value_double_name, owner, &output_double);

	return random;
}

// get current random int64 value for an owner by name
static inline int64_t w_random_get_long(struct w_ecs_world *world, w_entity_id owner, const char *name) {
	char value_name[128];
	snprintf(value_name, sizeof(value_name), "%s" W_RANDOM_COMPONENT_VALUE_LONG, name);
	int64_t *value = w_ecs_get_str(world, int64_t, value_name, owner);
	return value ? *value : 0;
}

// get current random double value for an owner by name
static inline double w_random_get_double(struct w_ecs_world *world, w_entity_id owner, const char *name) {
	char value_name[128];
	snprintf(value_name, sizeof(value_name), "%s" W_RANDOM_COMPONENT_VALUE_DOUBLE, name);
	double *value = w_ecs_get_str(world, double, value_name, owner);
	return value ? *value : 0.0;
}

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

#endif /* WHISKER_UTILITIES_RANDOM_H */
