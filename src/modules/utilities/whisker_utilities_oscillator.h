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
w_ecs_define_component(float, oscillator_period, 0.0f);
// current phase [0..1) - double for accumulation precision
w_ecs_define_component(double, oscillator_phase, 0.0);
// output scale factor
w_ecs_define_component(float, oscillator_amplitude, 0.0f);
// baseline offset
w_ecs_define_component(float, oscillator_offset, 0.0f);
// static phase offset [0..1)
w_ecs_define_component(float, oscillator_phase_shift, 0.0f);
// waveform type enum
w_ecs_define_component(int, oscillator_type, 0);
// points to the owner entity
w_ecs_define_component(w_entity_id, oscillator_owner_entity, W_ENTITY_INVALID);
// pre-registered component ID for value component on owner
w_ecs_define_component(w_entity_id, oscillator_value_comp_id, W_ENTITY_INVALID);
// extra parameter for special waveform types (duty_cycle, step_count, exponent)
w_ecs_define_component(float, oscillator_extra_param, 0.0f);
// computed oscillator value (stored on both oscillator entity and owner with prefix)
w_ecs_define_component(float, oscillator_value, 0.0f);
// points to oscillator entity (stored on owner with prefix)
w_ecs_define_component(w_entity_id, oscillator_entity, W_ENTITY_INVALID);

// helper macro to build waveform enum from short name
#define W_OSCILLATOR_TYPE(type) W_WAVEFORM_##type

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
