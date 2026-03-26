/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_oscillator
 * @created     : Wednesday Mar 26, 2026 11:25:00 CST
 * @description : ECS oscillator utility wrapping standalone oscillator module
 */

#include "whisker_std.h"
#include "whisker.h"
#include "whisker_oscillator.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_UTILITIES_OSCILLATOR_H
#define WHISKER_UTILITIES_OSCILLATOR_H

// oscillator components
// period in seconds for one cycle
#define W_OSCILLATOR_COMPONENT_PERIOD "oscillator_period"
// current phase [0..1)
#define W_OSCILLATOR_COMPONENT_PHASE "oscillator_phase"
// output scale factor
#define W_OSCILLATOR_COMPONENT_AMPLITUDE "oscillator_amplitude"
// baseline offset
#define W_OSCILLATOR_COMPONENT_OFFSET "oscillator_offset"
// static phase offset [0..1)
#define W_OSCILLATOR_COMPONENT_PHASE_SHIFT "oscillator_phase_shift"
// waveform type enum
#define W_OSCILLATOR_COMPONENT_TYPE "oscillator_type"
// points to the owner entity
#define W_OSCILLATOR_COMPONENT_OWNER_ENTITY "oscillator_owner_entity"

// owner oscillator component suffixes
// stores computed value on the owner
#define W_OSCILLATOR_COMPONENT_VALUE "_oscillator_value"
// points to oscillator entity
#define W_OSCILLATOR_COMPONENT_OSCILLATOR_ENTITY "_oscillator_entity"
// pre-registered component ID for value component on owner
#define W_OSCILLATOR_COMPONENT_VALUE_COMP_ID "oscillator_value_comp_id"
// extra parameter for special waveform types (duty_cycle, step_count, exponent)
#define W_OSCILLATOR_COMPONENT_EXTRA_PARAM "oscillator_extra_param"

// helper macro to build waveform enum from short name
#define W_OSCILLATOR_TYPE(type) W_WAVEFORM_##type

// create full oscillator component name
#define W_OSCILLATOR_QUERY(name, component) name component

static inline w_entity_id w_oscillator_create(struct w_ecs_world *world, w_entity_id owner, const char *name, w_waveform_type type, float period, float amplitude, float offset, float phase_shift, float extra_param) {
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

// convenience macros for standard waveform types (extra_param unused, pass 0)
#define w_oscillator_create_sine(world, owner, name, period, amplitude, offset, phase_shift) \
	w_oscillator_create(world, owner, name, W_WAVEFORM_SINE, period, amplitude, offset, phase_shift, 0.0f)

#define w_oscillator_create_triangle(world, owner, name, period, amplitude, offset, phase_shift) \
	w_oscillator_create(world, owner, name, W_WAVEFORM_TRIANGLE, period, amplitude, offset, phase_shift, 0.0f)

#define w_oscillator_create_square(world, owner, name, period, amplitude, offset, phase_shift) \
	w_oscillator_create(world, owner, name, W_WAVEFORM_SQUARE, period, amplitude, offset, phase_shift, 0.0f)

#define w_oscillator_create_sawtooth(world, owner, name, period, amplitude, offset, phase_shift) \
	w_oscillator_create(world, owner, name, W_WAVEFORM_SAWTOOTH, period, amplitude, offset, phase_shift, 0.0f)

#define w_oscillator_create_inverse_sawtooth(world, owner, name, period, amplitude, offset, phase_shift) \
	w_oscillator_create(world, owner, name, W_WAVEFORM_INVERSE_SAWTOOTH, period, amplitude, offset, phase_shift, 0.0f)

#define w_oscillator_create_bounce(world, owner, name, period, amplitude, offset, phase_shift) \
	w_oscillator_create(world, owner, name, W_WAVEFORM_BOUNCE, period, amplitude, offset, phase_shift, 0.0f)

#define w_oscillator_create_smooth_step(world, owner, name, period, amplitude, offset, phase_shift) \
	w_oscillator_create(world, owner, name, W_WAVEFORM_SMOOTH_STEP, period, amplitude, offset, phase_shift, 0.0f)

#define w_oscillator_create_rectified_sine(world, owner, name, period, amplitude, offset, phase_shift) \
	w_oscillator_create(world, owner, name, W_WAVEFORM_RECTIFIED_SINE, period, amplitude, offset, phase_shift, 0.0f)

#define w_oscillator_create_noise(world, owner, name, period, amplitude, offset, phase_shift) \
	w_oscillator_create(world, owner, name, W_WAVEFORM_NOISE, period, amplitude, offset, phase_shift, 0.0f)

#define w_oscillator_create_fixed_exponential(world, owner, name, period, amplitude, offset, phase_shift) \
	w_oscillator_create(world, owner, name, W_WAVEFORM_FIXED_EXPONENTIAL, period, amplitude, offset, phase_shift, 0.0f)

// specialized macros for waveform types with extra parameters
#define w_oscillator_create_pwm(world, owner, name, period, amplitude, offset, phase_shift, duty_cycle) \
	w_oscillator_create(world, owner, name, W_WAVEFORM_PULSE, period, amplitude, offset, phase_shift, duty_cycle)

#define w_oscillator_create_steps(world, owner, name, period, amplitude, offset, phase_shift, step_count) \
	w_oscillator_create(world, owner, name, W_WAVEFORM_STEPS, period, amplitude, offset, phase_shift, (float)(step_count))

#define w_oscillator_create_variable_exponential(world, owner, name, period, amplitude, offset, phase_shift, exponent) \
	w_oscillator_create(world, owner, name, W_WAVEFORM_VARIABLE_EXPONENTIAL, period, amplitude, offset, phase_shift, exponent)


// get current oscillator value for an owner by name
static inline float w_oscillator_get_value(struct w_ecs_world *world, w_entity_id owner, const char *name) {
	char value_name[128];
	snprintf(value_name, sizeof(value_name), "%s" W_OSCILLATOR_COMPONENT_VALUE, name);
	float *value = w_ecs_get_str(world, float, value_name, owner);
	return value ? *value : 0.0f;
}

// main oscillator update system
w_ecs_system(
	wm_utils_oscillator_update_system,
	WM_PHASE_POST,
		w_query_write(W_OSCILLATOR_COMPONENT_PHASE)
		w_query_read(W_OSCILLATOR_COMPONENT_PERIOD)
		w_query_read(W_OSCILLATOR_COMPONENT_AMPLITUDE)
		w_query_read(W_OSCILLATOR_COMPONENT_OFFSET)
		w_query_read(W_OSCILLATOR_COMPONENT_PHASE_SHIFT)
		w_query_read(W_OSCILLATOR_COMPONENT_TYPE)
		w_query_read(W_OSCILLATOR_COMPONENT_OWNER_ENTITY)
		w_query_read(W_OSCILLATOR_COMPONENT_VALUE_COMP_ID)
		w_query_read(W_OSCILLATOR_COMPONENT_EXTRA_PARAM)
	,
{
	float *phase = w_itor_get_write(float);
	float period = w_itor_get_read(float);
	float amplitude = w_itor_get_read(float);
	float offset = w_itor_get_read(float);
	float phase_shift = w_itor_get_read(float);
	w_waveform_type type = w_itor_get_read(int);
	w_entity_id owner = w_itor_get_read(w_entity_id);
	w_entity_id value_comp_id = w_itor_get_read(w_entity_id);
	float extra_param = w_itor_get_read(float);

	// skip if period is invalid
	if (period <= 0.0f) continue;

	// update phase using standalone function
	w_oscillator_update_phase(phase, delta_time, period);

	// compute effective phase with phase shift
	float effective_phase = w_oscillator_effective_phase(*phase, phase_shift);

	// compute value using standalone function with extra param support
	float value = w_oscillator_compute_ext(type, effective_phase, amplitude, offset, extra_param);

	// update owner value component using pre-registered component ID
	if (w_entity_is_valid(value_comp_id) && w_entity_is_valid(owner)) {
		w_ecs_set(world, float, value_comp_id, owner, &value);
	}
});

#endif /* WHISKER_UTILITIES_OSCILLATOR_H */
