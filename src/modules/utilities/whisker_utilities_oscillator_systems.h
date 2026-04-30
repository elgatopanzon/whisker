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
	w_query(
		w_query_w(oscillator_phase),
		w_query_w(oscillator_value),
		w_query_r(oscillator_period),
		w_query_r(oscillator_amplitude),
		w_query_r(oscillator_offset),
		w_query_r(oscillator_phase_shift),
		w_query_r(oscillator_type),
		w_query_r(oscillator_owner_entity),
		w_query_r(oscillator_value_comp_id),
		w_query_r(oscillator_extra_param),
	)
	,
{
	double *phase = w_query_get(oscillator_phase);
	float period = *w_query_get(oscillator_period);
	float amplitude = *w_query_get(oscillator_amplitude);
	float offset = *w_query_get(oscillator_offset);
	float phase_shift = *w_query_get(oscillator_phase_shift);
	w_waveform_type type = *w_query_get(oscillator_type);
	w_entity_id owner = *w_query_get(oscillator_owner_entity);
	w_entity_id value_comp_id = *w_query_get(oscillator_value_comp_id);
	float extra_param = *w_query_get(oscillator_extra_param);

	// skip if period is invalid
	if (period <= 0.0f) continue;

	// update phase using standalone function
	w_oscillator_update_phase(phase, delta_time, period);

	// compute effective phase with phase shift
	float effective_phase = w_oscillator_effective_phase(*phase, phase_shift);

	// compute value using standalone function with extra param support
	float value = w_oscillator_compute_ext(type, effective_phase, amplitude, offset, extra_param);

	// update oscillator entity value component
	float *osc_value = w_query_get(oscillator_value);
	*osc_value = value;

	// update owner value component using pre-registered component ID
	if (w_entity_is_valid(value_comp_id) && w_entity_is_valid(owner)) {
		w_set_id(owner, oscillator_value, value_comp_id, &value);
	}
});

#endif /* WHISKER_UTILITIES_OSCILLATOR_SYSTEMS_H */

