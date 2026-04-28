/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_random
 * @created     : Tuesday Apr 28, 2026 14:12:41 CST
 */

#include "whisker_utilities_random.h"

w_entity_id wm_utils_random_create(struct w_ecs_world *world, w_entity_id owner, const char *name, int64_t range_min, int64_t range_max) {
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
int64_t wm_utils_random_get_long(struct w_ecs_world *world, w_entity_id owner, const char *name) {
	char value_name[128];
	snprintf(value_name, sizeof(value_name), "%s" W_RANDOM_COMPONENT_VALUE_LONG, name);
	int64_t *value = w_ecs_get_str(world, int64_t, value_name, owner);
	return value ? *value : 0;
}

// get current random double value for an owner by name
double wm_utils_random_get_double(struct w_ecs_world *world, w_entity_id owner, const char *name) {
	char value_name[128];
	snprintf(value_name, sizeof(value_name), "%s" W_RANDOM_COMPONENT_VALUE_DOUBLE, name);
	double *value = w_ecs_get_str(world, double, value_name, owner);
	return value ? *value : 0.0;
}
