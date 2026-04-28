/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_utilities_oscillator_systems
 * @created     : Tuesday Apr 28, 2026 14:05:34 CST
 * @description : 
 */

#include "whisker_utilities_oscillator.h"

#ifndef WHISKER_UTILITIES_OSCILLATOR_SYSTEMS_H
#define WHISKER_UTILITIES_OSCILLATOR_SYSTEMS_H

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

#endif /* WHISKER_UTILITIES_OSCILLATOR_SYSTEMS_H */

