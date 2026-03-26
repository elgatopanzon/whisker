/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_oscillator
 * @created     : Wednesday Mar 26, 2026 11:25:00 CST
 * @description : Oscillator utility
 */

#include "whisker_std.h"
#include "whisker.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

#ifndef WHISKER_UTILITIES_OSCILLATOR_H
#define WHISKER_UTILITIES_OSCILLATOR_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// waveform types
typedef enum wm_waveform_type {
	WM_WAVEFORM_SINE,
	WM_WAVEFORM_TRIANGLE,
	WM_WAVEFORM_SQUARE,
	WM_WAVEFORM_SAWTOOTH,
	WM_WAVEFORM_INVERSE_SAWTOOTH,
	WM_WAVEFORM_BOUNCE,
	WM_WAVEFORM_SMOOTH_STEP,
	WM_WAVEFORM_RECTIFIED_SINE,
	WM_WAVEFORM_NOISE,
	WM_WAVEFORM_FIXED_EXPONENTIAL
} wm_waveform_type;

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

// helper macro to build waveform enum from short name
#define W_OSCILLATOR_TYPE(type) WM_WAVEFORM_##type

// create full oscillator component name
#define W_OSCILLATOR_QUERY(name, component) name component

static inline w_entity_id w_oscillator_create(struct w_ecs_world *world, w_entity_id owner, const char *name, wm_waveform_type type, float period, float amplitude, float offset, float phase_shift) {
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

// compute waveform value from effective phase
static inline float w_oscillator_compute_value_(wm_waveform_type type, float effective_phase, float amplitude, float offset) {
	float t;
	switch (type) {
		case WM_WAVEFORM_SINE:
			return offset + amplitude * sinf(effective_phase * 2.0f * (float)M_PI);
		case WM_WAVEFORM_TRIANGLE:
			return offset + amplitude * (1.0f - 4.0f * fabsf(effective_phase - 0.5f));
		case WM_WAVEFORM_SQUARE:
			return offset + amplitude * (effective_phase < 0.5f ? 1.0f : -1.0f);
		case WM_WAVEFORM_SAWTOOTH:
			return offset + amplitude * (2.0f * effective_phase - 1.0f);
		case WM_WAVEFORM_INVERSE_SAWTOOTH:
			return offset + amplitude * (1.0f - 2.0f * effective_phase);
		case WM_WAVEFORM_BOUNCE:
			return offset + amplitude * fabsf(sinf(effective_phase * (float)M_PI));
		case WM_WAVEFORM_SMOOTH_STEP:
			// hermite interpolation mapped to [-1..1]
			t = effective_phase;
			return offset + amplitude * (2.0f * (t * t * (3.0f - 2.0f * t)) - 1.0f);
		case WM_WAVEFORM_RECTIFIED_SINE:
			return offset + amplitude * (2.0f * fabsf(sinf(effective_phase * 2.0f * (float)M_PI)) - 1.0f);
		case WM_WAVEFORM_NOISE:
			return offset + amplitude * (2.0f * w_rand_float() - 1.0f);
		case WM_WAVEFORM_FIXED_EXPONENTIAL:
			// exponential curve with fixed curvature (k=3)
			t = effective_phase;
			return offset + amplitude * (2.0f * ((expf(3.0f * t) - 1.0f) / (expf(3.0f) - 1.0f)) - 1.0f);
		default:
			return offset;
	}
}

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
	,
{
	float *phase = w_itor_get_write(float);
	float period = w_itor_get_read(float);
	float amplitude = w_itor_get_read(float);
	float offset = w_itor_get_read(float);
	float phase_shift = w_itor_get_read(float);
	wm_waveform_type type = w_itor_get_read(int);
	w_entity_id owner = w_itor_get_read(w_entity_id);
	w_entity_id value_comp_id = w_itor_get_read(w_entity_id);

	// skip if period is invalid
	if (period <= 0.0f) continue;

	// update phase
	*phase += delta_time / period;
	// normalize to [0..1)
	*phase = fmodf(*phase, 1.0f);
	if (*phase < 0.0f) *phase += 1.0f;

	// compute effective phase with phase shift
	float effective_phase = fmodf(*phase + phase_shift, 1.0f);
	if (effective_phase < 0.0f) effective_phase += 1.0f;

	// compute value
	float value = w_oscillator_compute_value_(type, effective_phase, amplitude, offset);

	// update owner value component using pre-registered component ID
	if (w_entity_is_valid(value_comp_id) && w_entity_is_valid(owner)) {
		w_ecs_set(world, float, value_comp_id, owner, &value);
	}
});

#endif /* WHISKER_UTILITIES_OSCILLATOR_H */
