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

w_entity_id wm_utils_oscillator_create(struct w_ecs_world *world, w_entity_id owner, const char *name, w_waveform_type type, float period, float amplitude, float offset, float phase_shift, float extra_param);

// convenience macros for standard waveform types (extra_param unused, pass 0)
#define wm_utils_oscillator_create_sine(world, owner, name, period, amplitude, offset, phase_shift) \
	wm_utils_oscillator_create(world, owner, name, W_WAVEFORM_SINE, period, amplitude, offset, phase_shift, 0.0f)

#define wm_utils_oscillator_create_triangle(world, owner, name, period, amplitude, offset, phase_shift) \
	wm_utils_oscillator_create(world, owner, name, W_WAVEFORM_TRIANGLE, period, amplitude, offset, phase_shift, 0.0f)

#define wm_utils_oscillator_create_square(world, owner, name, period, amplitude, offset, phase_shift) \
	wm_utils_oscillator_create(world, owner, name, W_WAVEFORM_SQUARE, period, amplitude, offset, phase_shift, 0.0f)

#define wm_utils_oscillator_create_sawtooth(world, owner, name, period, amplitude, offset, phase_shift) \
	wm_utils_oscillator_create(world, owner, name, W_WAVEFORM_SAWTOOTH, period, amplitude, offset, phase_shift, 0.0f)

#define wm_utils_oscillator_create_inverse_sawtooth(world, owner, name, period, amplitude, offset, phase_shift) \
	wm_utils_oscillator_create(world, owner, name, W_WAVEFORM_INVERSE_SAWTOOTH, period, amplitude, offset, phase_shift, 0.0f)

#define wm_utils_oscillator_create_bounce(world, owner, name, period, amplitude, offset, phase_shift) \
	wm_utils_oscillator_create(world, owner, name, W_WAVEFORM_BOUNCE, period, amplitude, offset, phase_shift, 0.0f)

#define wm_utils_oscillator_create_smooth_step(world, owner, name, period, amplitude, offset, phase_shift) \
	wm_utils_oscillator_create(world, owner, name, W_WAVEFORM_SMOOTH_STEP, period, amplitude, offset, phase_shift, 0.0f)

#define wm_utils_oscillator_create_rectified_sine(world, owner, name, period, amplitude, offset, phase_shift) \
	wm_utils_oscillator_create(world, owner, name, W_WAVEFORM_RECTIFIED_SINE, period, amplitude, offset, phase_shift, 0.0f)

#define wm_utils_oscillator_create_noise(world, owner, name, period, amplitude, offset, phase_shift) \
	wm_utils_oscillator_create(world, owner, name, W_WAVEFORM_NOISE, period, amplitude, offset, phase_shift, 0.0f)

#define wm_utils_oscillator_create_fixed_exponential(world, owner, name, period, amplitude, offset, phase_shift) \
	wm_utils_oscillator_create(world, owner, name, W_WAVEFORM_FIXED_EXPONENTIAL, period, amplitude, offset, phase_shift, 0.0f)

// specialized macros for waveform types with extra parameters
#define wm_utils_oscillator_create_pwm(world, owner, name, period, amplitude, offset, phase_shift, duty_cycle) \
	wm_utils_oscillator_create(world, owner, name, W_WAVEFORM_PULSE, period, amplitude, offset, phase_shift, duty_cycle)

#define wm_utils_oscillator_create_steps(world, owner, name, period, amplitude, offset, phase_shift, step_count) \
	wm_utils_oscillator_create(world, owner, name, W_WAVEFORM_STEPS, period, amplitude, offset, phase_shift, (float)(step_count))

#define wm_utils_oscillator_create_variable_exponential(world, owner, name, period, amplitude, offset, phase_shift, exponent) \
	wm_utils_oscillator_create(world, owner, name, W_WAVEFORM_VARIABLE_EXPONENTIAL, period, amplitude, offset, phase_shift, exponent)


// get current oscillator value for an owner by name
float wm_utils_oscillator_get_value(struct w_ecs_world *world, w_entity_id owner, const char *name);

#endif /* WHISKER_UTILITIES_OSCILLATOR_H */
