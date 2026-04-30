/**
 * @author      : ElGatoPanzon
 * @file        : whisker_oscillator
 * @created     : Thursday Mar 26, 2026 14:15:00 CST
 * @description : Standalone header-only oscillator with pure stateless functions
 */

#include "whisker_std.h"
#include "whisker_random.h"

#ifndef WHISKER_OSCILLATOR_H
#define WHISKER_OSCILLATOR_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// waveform types
typedef enum w_waveform_type {
	W_WAVEFORM_SINE,
	W_WAVEFORM_TRIANGLE,
	W_WAVEFORM_SQUARE,
	W_WAVEFORM_SAWTOOTH,
	W_WAVEFORM_INVERSE_SAWTOOTH,
	W_WAVEFORM_BOUNCE,
	W_WAVEFORM_SMOOTH_STEP,
	W_WAVEFORM_RECTIFIED_SINE,
	W_WAVEFORM_NOISE,
	W_WAVEFORM_FIXED_EXPONENTIAL,
	// new waveform types
	W_WAVEFORM_PULSE,              // PWM with duty_cycle param
	W_WAVEFORM_STEPS,              // quantized with step_count param
	W_WAVEFORM_VARIABLE_EXPONENTIAL // exponential with exponent param
} w_waveform_type;

// convenience macro: W_WAVEFORM(SINE) expands to W_WAVEFORM_SINE
#define W_WAVEFORM(x) W_WAVEFORM_##x

// compute effective phase with phase shift, normalized to [0..1)
static inline float w_oscillator_effective_phase(double phase, float phase_shift) {
	double effective = fmod(phase + (double)phase_shift, 1.0);
	if (effective < 0.0) effective += 1.0;
	return (float)effective;
}

// update phase by delta_time / period, returns new phase normalized to [0..1)
// phase is updated in place
static inline void w_oscillator_update_phase(double *phase, float delta_time, float period) {
	if (period <= 0.0f) return;
	*phase += (double)delta_time / (double)period;
	*phase = fmod(*phase, 1.0);
	if (*phase < 0.0) *phase += 1.0;
}

// sine waveform: smooth periodic oscillation
static inline float w_oscillator_sine(float phase, float amplitude, float offset) {
	return offset + amplitude * sinf(phase * 2.0f * (float)M_PI);
}

// triangle waveform: linear rise and fall
static inline float w_oscillator_triangle(float phase, float amplitude, float offset) {
	return offset + amplitude * (1.0f - 4.0f * fabsf(phase - 0.5f));
}

// square waveform: 50% duty cycle, +1 first half, -1 second half
static inline float w_oscillator_square(float phase, float amplitude, float offset) {
	return offset + amplitude * (phase < 0.5f ? 1.0f : -1.0f);
}

// sawtooth waveform: linear rise from -1 to +1
static inline float w_oscillator_sawtooth(float phase, float amplitude, float offset) {
	return offset + amplitude * (2.0f * phase - 1.0f);
}

// inverse sawtooth waveform: linear fall from +1 to -1
static inline float w_oscillator_inverse_sawtooth(float phase, float amplitude, float offset) {
	return offset + amplitude * (1.0f - 2.0f * phase);
}

// bounce waveform: absolute sine, always positive (0 to 1 range)
static inline float w_oscillator_bounce(float phase, float amplitude, float offset) {
	return offset + amplitude * fabsf(sinf(phase * (float)M_PI));
}

// smooth step waveform: hermite interpolation mapped to [-1..1]
static inline float w_oscillator_smooth_step(float phase, float amplitude, float offset) {
	float t = phase;
	return offset + amplitude * (2.0f * (t * t * (3.0f - 2.0f * t)) - 1.0f);
}

// rectified sine waveform: full-wave rectified sine mapped to [-1..1]
static inline float w_oscillator_rectified_sine(float phase, float amplitude, float offset) {
	return offset + amplitude * (2.0f * fabsf(sinf(phase * 2.0f * (float)M_PI)) - 1.0f);
}

// noise waveform: random value each sample
static inline float w_oscillator_noise(float amplitude, float offset) {
	return offset + amplitude * (2.0f * w_rand_float() - 1.0f);
}

// fixed exponential waveform: exponential curve with k=3 curvature
static inline float w_oscillator_fixed_exponential(float phase, float amplitude, float offset) {
	float t = phase;
	return offset + amplitude * (2.0f * ((expf(3.0f * t) - 1.0f) / (expf(3.0f) - 1.0f)) - 1.0f);
}

// pulse/PWM waveform: variable duty cycle, +1 when phase < duty_cycle, else -1
// duty_cycle should be in [0..1]
static inline float w_oscillator_pulse(float phase, float amplitude, float offset, float duty_cycle) {
	return offset + amplitude * (phase < duty_cycle ? 1.0f : -1.0f);
}

// steps/quantized waveform: staircase with step_count levels
// step_count should be >= 1
static inline float w_oscillator_steps(float phase, float amplitude, float offset, int step_count) {
	if (step_count < 1) step_count = 1;
	// quantize phase to step_count levels, map to [-1..1]
	int step = (int)(phase * (float)step_count);
	if (step >= step_count) step = step_count - 1;
	float normalized = (2.0f * (float)step / (float)(step_count - 1)) - 1.0f;
	// handle edge case of 1 step
	if (step_count == 1) normalized = 0.0f;
	return offset + amplitude * normalized;
}

// variable exponential waveform: exponential curve with configurable exponent
// exponent controls curvature (higher = steeper curve)
static inline float w_oscillator_variable_exponential(float phase, float amplitude, float offset, float exponent) {
	if (exponent == 0.0f) {
		// linear fallback when exponent is zero
		return offset + amplitude * (2.0f * phase - 1.0f);
	}
	float t = phase;
	float denom = expf(exponent) - 1.0f;
	if (fabsf(denom) < 1e-6f) {
		// near-linear when exponent is very small
		return offset + amplitude * (2.0f * t - 1.0f);
	}
	return offset + amplitude * (2.0f * ((expf(exponent * t) - 1.0f) / denom) - 1.0f);
}

// generic waveform computation for basic types (no extra params)
// for types requiring extra params (PULSE, STEPS, VARIABLE_EXPONENTIAL), returns offset
static inline float w_oscillator_compute(w_waveform_type type, float phase, float amplitude, float offset) {
	switch (type) {
		case W_WAVEFORM_SINE:
			return w_oscillator_sine(phase, amplitude, offset);
		case W_WAVEFORM_TRIANGLE:
			return w_oscillator_triangle(phase, amplitude, offset);
		case W_WAVEFORM_SQUARE:
			return w_oscillator_square(phase, amplitude, offset);
		case W_WAVEFORM_SAWTOOTH:
			return w_oscillator_sawtooth(phase, amplitude, offset);
		case W_WAVEFORM_INVERSE_SAWTOOTH:
			return w_oscillator_inverse_sawtooth(phase, amplitude, offset);
		case W_WAVEFORM_BOUNCE:
			return w_oscillator_bounce(phase, amplitude, offset);
		case W_WAVEFORM_SMOOTH_STEP:
			return w_oscillator_smooth_step(phase, amplitude, offset);
		case W_WAVEFORM_RECTIFIED_SINE:
			return w_oscillator_rectified_sine(phase, amplitude, offset);
		case W_WAVEFORM_NOISE:
			return w_oscillator_noise(amplitude, offset);
		case W_WAVEFORM_FIXED_EXPONENTIAL:
			return w_oscillator_fixed_exponential(phase, amplitude, offset);
		case W_WAVEFORM_PULSE:
			// default 50% duty for pulse when called without extra param
			return w_oscillator_pulse(phase, amplitude, offset, 0.5f);
		case W_WAVEFORM_STEPS:
			// default 4 steps when called without extra param
			return w_oscillator_steps(phase, amplitude, offset, 4);
		case W_WAVEFORM_VARIABLE_EXPONENTIAL:
			// default k=3 when called without extra param
			return w_oscillator_variable_exponential(phase, amplitude, offset, 3.0f);
		default:
			return offset;
	}
}

// generic waveform computation with extra parameter for extended types
// extra_param usage:
//   PULSE: duty_cycle [0..1]
//   STEPS: step_count (cast to int)
//   VARIABLE_EXPONENTIAL: exponent
static inline float w_oscillator_compute_ext(w_waveform_type type, float phase, float amplitude, float offset, float extra_param) {
	switch (type) {
		case W_WAVEFORM_PULSE:
			return w_oscillator_pulse(phase, amplitude, offset, extra_param);
		case W_WAVEFORM_STEPS:
			return w_oscillator_steps(phase, amplitude, offset, (int)extra_param);
		case W_WAVEFORM_VARIABLE_EXPONENTIAL:
			return w_oscillator_variable_exponential(phase, amplitude, offset, extra_param);
		default:
			// for non-extended types, ignore extra_param
			return w_oscillator_compute(type, phase, amplitude, offset);
	}
}

#endif /* WHISKER_OSCILLATOR_H */
