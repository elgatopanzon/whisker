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
	w_ecs_set_str(world, float, W_OSCILLATOR_COMPONENT_PERIOD, oscillator, &period);
	w_ecs_set_str(world, float, W_OSCILLATOR_COMPONENT_PHASE, oscillator, &(float){0});
	w_ecs_set_str(world, float, W_OSCILLATOR_COMPONENT_AMPLITUDE, oscillator, &amplitude);
	w_ecs_set_str(world, float, W_OSCILLATOR_COMPONENT_OFFSET, oscillator, &offset);
	w_ecs_set_str(world, float, W_OSCILLATOR_COMPONENT_PHASE_SHIFT, oscillator, &phase_shift);
	int type_int = (int)type;
	w_ecs_set_str(world, int, W_OSCILLATOR_COMPONENT_TYPE, oscillator, &type_int);
	w_ecs_set_str(world, w_entity_id, W_OSCILLATOR_COMPONENT_OWNER_ENTITY, oscillator, &owner);
	w_ecs_set_str(world, float, W_OSCILLATOR_COMPONENT_EXTRA_PARAM, oscillator, &extra_param);

	// owner reference: "{name}_oscillator_entity"
	char oscillator_entity_name[128];
	snprintf(oscillator_entity_name, sizeof(oscillator_entity_name), "%s" W_OSCILLATOR_COMPONENT_OSCILLATOR_ENTITY, name);
	w_ecs_set_str(world, w_entity_id, oscillator_entity_name, owner, &oscillator);

	// build "{name}_oscillator_value" and get its component ID for fast path
	char value_name[128];
	snprintf(value_name, sizeof(value_name), "%s" W_OSCILLATOR_COMPONENT_VALUE, name);
	w_entity_id value_comp_id = w_ecs_get_component_by_name(world, value_name);
	w_ecs_set_str(world, w_entity_id, W_OSCILLATOR_COMPONENT_VALUE_COMP_ID, oscillator, &value_comp_id);

	// owner value component: "{name}_oscillator_value" (initialized to offset)
	w_ecs_set_str(world, float, value_name, owner, &offset);

	return oscillator;
}

float wm_utils_oscillator_get_value(struct w_ecs_world *world, w_entity_id owner, const char *name) {
	char value_name[128];
	snprintf(value_name, sizeof(value_name), "%s" W_OSCILLATOR_COMPONENT_VALUE, name);
	float *value = w_ecs_get_str(world, float, value_name, owner);
	return value ? *value : 0.0f;
}
