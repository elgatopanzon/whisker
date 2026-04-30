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

	w_set(random, random_state, &state);
	w_set(random, random_increment, &increment);
	w_set(random, random_range_min, &range_min);
	w_set(random, random_range_max, &range_max);
	w_set(random, random_output_long, &output_long);
	w_set(random, random_output_double, &output_double);
	w_set(random, random_owner_entity, &owner);

	// owner reference: "{name}_random_entity"
	w_set_g(owner, random_entity, name, &random);

	// build "{name}_random_value_long" and get its component ID for fast path
	w_set_value(random, random_output_long_comp_id, w_gid(random_output_long, name));

	// build "{name}_random_value_double" and get its component ID for fast path
	w_set_value(random, random_output_double_comp_id, w_gid(random_output_double, name));

	// owner value components (initialized to 0)
	w_set_g(owner, random_output_long, name, &output_long);
	w_set_g(owner, random_output_double, name, &output_double);

	return random;
}

// get current random int64 value for an owner by name
inline int64_t wm_utils_random_get_long(struct w_ecs_world *world, w_entity_id owner, const char *name) {
	int64_t *value = w_get_g(owner, random_output_long, name);
	return value ? *value : 0;
}

// get current random double value for an owner by name
inline double wm_utils_random_get_double(struct w_ecs_world *world, w_entity_id owner, const char *name) {
	double *value = w_get_g(owner, random_output_double, name);
	return value ? *value : 0.0;
}
