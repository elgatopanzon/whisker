/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_oscillator
 * @created     : Tuesday Apr 28, 2026 14:06:44 CST
 */

#include "whisker_std.h"

#include "whisker_utilities_oscillator.h"

w_entity_id wm_utils_oscillator_create(struct w_ecs_world *world, w_entity_id owner, const char *name, w_waveform_type type, float period, float amplitude, float offset, float phase_shift, float extra_param) {
	// create oscillator entity with name
	char oscillator_name[128];
	char hash[17];
	w_rand_chars(hash, 16);
	snprintf(oscillator_name, sizeof(oscillator_name), "%s_%s", name, hash);
	w_entity_id oscillator = w_ecs_request_entity_with_name(world, oscillator_name);

	// oscillator components
	double phase = 0.0;
	int type_int = (int)type;

	w_set(oscillator, oscillator_period, &period);
	w_set(oscillator, oscillator_phase, &phase);
	w_set(oscillator, oscillator_amplitude, &amplitude);
	w_set(oscillator, oscillator_offset, &offset);
	w_set(oscillator, oscillator_phase_shift, &phase_shift);
	w_set(oscillator, oscillator_type, &type_int);
	w_set(oscillator, oscillator_owner_entity, &owner);
	w_set(oscillator, oscillator_extra_param, &extra_param);

	// owner reference: "{name}_oscillator_entity"
	w_set_g(owner, oscillator_entity, name, &oscillator);

	// build "{name}_oscillator_value" component ID for fast path
	w_set_value(oscillator, oscillator_value_comp_id, w_gid(oscillator_value, name));

	// owner value component: "{name}_oscillator_value" (initialized to offset)
	w_set_g(owner, oscillator_value, name, &offset);

	// oscillator entity also stores current value (initialized to offset)
	w_set(oscillator, oscillator_value, &offset);

	return oscillator;
}

float wm_utils_oscillator_get_value(struct w_ecs_world *world, w_entity_id owner, const char *name) {
	float *value = w_get_g(owner, oscillator_value, name);
	return value ? *value : 0.0f;
}
