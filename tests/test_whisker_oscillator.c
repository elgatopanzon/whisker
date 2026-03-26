/**
 * @author      : ElGatoPanzon
 * @file        : test_whisker_oscillator
 * @created     : Thursday Mar 26, 2026 14:15:00 CST
 * @description : Tests for standalone whisker_oscillator module
 */

#include "whisker_std.h"
#include "whisker_oscillator.h"

#include <check.h>


/*****************************
*  phase update tcase        *
*****************************/

START_TEST(test_phase_update_advances)
{
	float phase = 0.0f;
	w_oscillator_update_phase(&phase, 0.5f, 1.0f);
	ck_assert_float_eq_tol(phase, 0.5f, 0.0001f);
}
END_TEST

START_TEST(test_phase_update_wraps)
{
	float phase = 0.9f;
	w_oscillator_update_phase(&phase, 0.2f, 1.0f);
	ck_assert_float_eq_tol(phase, 0.1f, 0.0001f);
}
END_TEST

START_TEST(test_phase_update_zero_period)
{
	float phase = 0.5f;
	w_oscillator_update_phase(&phase, 0.1f, 0.0f);
	// should not change with zero period
	ck_assert_float_eq_tol(phase, 0.5f, 0.0001f);
}
END_TEST

START_TEST(test_phase_update_negative_period)
{
	float phase = 0.5f;
	w_oscillator_update_phase(&phase, 0.1f, -1.0f);
	// should not change with negative period
	ck_assert_float_eq_tol(phase, 0.5f, 0.0001f);
}
END_TEST

START_TEST(test_effective_phase_basic)
{
	float eff = w_oscillator_effective_phase(0.25f, 0.0f);
	ck_assert_float_eq_tol(eff, 0.25f, 0.0001f);
}
END_TEST

START_TEST(test_effective_phase_with_shift)
{
	float eff = w_oscillator_effective_phase(0.25f, 0.5f);
	ck_assert_float_eq_tol(eff, 0.75f, 0.0001f);
}
END_TEST

START_TEST(test_effective_phase_wraps)
{
	float eff = w_oscillator_effective_phase(0.75f, 0.5f);
	ck_assert_float_eq_tol(eff, 0.25f, 0.0001f);
}
END_TEST


/*****************************
*  sine waveform tcase       *
*****************************/

START_TEST(test_sine_at_zero)
{
	float val = w_oscillator_sine(0.0f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 0.0f, 0.0001f);
}
END_TEST

START_TEST(test_sine_at_quarter)
{
	float val = w_oscillator_sine(0.25f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_sine_at_half)
{
	float val = w_oscillator_sine(0.5f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 0.0f, 0.0001f);
}
END_TEST

START_TEST(test_sine_at_three_quarter)
{
	float val = w_oscillator_sine(0.75f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST

START_TEST(test_sine_with_amplitude)
{
	float val = w_oscillator_sine(0.25f, 2.0f, 0.0f);
	ck_assert_float_eq_tol(val, 2.0f, 0.0001f);
}
END_TEST

START_TEST(test_sine_with_offset)
{
	float val = w_oscillator_sine(0.25f, 1.0f, 5.0f);
	ck_assert_float_eq_tol(val, 6.0f, 0.0001f);
}
END_TEST


/*****************************
*  triangle waveform tcase   *
*****************************/

START_TEST(test_triangle_at_zero)
{
	float val = w_oscillator_triangle(0.0f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST

START_TEST(test_triangle_at_half)
{
	float val = w_oscillator_triangle(0.5f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_triangle_at_one)
{
	float val = w_oscillator_triangle(1.0f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST


/*****************************
*  square waveform tcase     *
*****************************/

START_TEST(test_square_first_half)
{
	float val = w_oscillator_square(0.25f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_square_second_half)
{
	float val = w_oscillator_square(0.75f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST

START_TEST(test_square_at_boundary)
{
	float val = w_oscillator_square(0.5f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST


/*****************************
*  sawtooth waveform tcase   *
*****************************/

START_TEST(test_sawtooth_at_zero)
{
	float val = w_oscillator_sawtooth(0.0f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST

START_TEST(test_sawtooth_at_half)
{
	float val = w_oscillator_sawtooth(0.5f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 0.0f, 0.0001f);
}
END_TEST

START_TEST(test_sawtooth_at_one)
{
	float val = w_oscillator_sawtooth(1.0f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST


/*****************************
*  inverse sawtooth tcase    *
*****************************/

START_TEST(test_inverse_sawtooth_at_zero)
{
	float val = w_oscillator_inverse_sawtooth(0.0f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_inverse_sawtooth_at_half)
{
	float val = w_oscillator_inverse_sawtooth(0.5f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 0.0f, 0.0001f);
}
END_TEST

START_TEST(test_inverse_sawtooth_at_one)
{
	float val = w_oscillator_inverse_sawtooth(1.0f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST


/*****************************
*  bounce waveform tcase     *
*****************************/

START_TEST(test_bounce_at_zero)
{
	float val = w_oscillator_bounce(0.0f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 0.0f, 0.0001f);
}
END_TEST

START_TEST(test_bounce_at_half)
{
	float val = w_oscillator_bounce(0.5f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_bounce_always_positive)
{
	// bounce should always be >= 0
	for (int i = 0; i <= 10; i++) {
		float phase = (float)i / 10.0f;
		float val = w_oscillator_bounce(phase, 1.0f, 0.0f);
		ck_assert_float_ge(val, 0.0f);
	}
}
END_TEST


/*****************************
*  smooth step tcase         *
*****************************/

START_TEST(test_smooth_step_at_zero)
{
	float val = w_oscillator_smooth_step(0.0f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST

START_TEST(test_smooth_step_at_one)
{
	float val = w_oscillator_smooth_step(1.0f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_smooth_step_at_half)
{
	float val = w_oscillator_smooth_step(0.5f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 0.0f, 0.0001f);
}
END_TEST


/*****************************
*  rectified sine tcase      *
*****************************/

START_TEST(test_rectified_sine_at_zero)
{
	float val = w_oscillator_rectified_sine(0.0f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST

START_TEST(test_rectified_sine_at_quarter)
{
	float val = w_oscillator_rectified_sine(0.25f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST


/*****************************
*  noise waveform tcase      *
*****************************/

START_TEST(test_noise_in_range)
{
	// noise should be in [-amplitude + offset, amplitude + offset]
	for (int i = 0; i < 100; i++) {
		float val = w_oscillator_noise(1.0f, 0.0f);
		ck_assert_float_ge(val, -1.0f);
		ck_assert_float_le(val, 1.0f);
	}
}
END_TEST

START_TEST(test_noise_with_offset)
{
	for (int i = 0; i < 100; i++) {
		float val = w_oscillator_noise(1.0f, 5.0f);
		ck_assert_float_ge(val, 4.0f);
		ck_assert_float_le(val, 6.0f);
	}
}
END_TEST


/*****************************
*  fixed exponential tcase   *
*****************************/

START_TEST(test_fixed_exp_at_zero)
{
	float val = w_oscillator_fixed_exponential(0.0f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST

START_TEST(test_fixed_exp_at_one)
{
	float val = w_oscillator_fixed_exponential(1.0f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_fixed_exp_monotonic)
{
	// exponential should be monotonically increasing
	float prev = w_oscillator_fixed_exponential(0.0f, 1.0f, 0.0f);
	for (int i = 1; i <= 10; i++) {
		float phase = (float)i / 10.0f;
		float val = w_oscillator_fixed_exponential(phase, 1.0f, 0.0f);
		ck_assert_float_gt(val, prev);
		prev = val;
	}
}
END_TEST


/*****************************
*  pulse/PWM waveform tcase  *
*****************************/

START_TEST(test_pulse_50_duty)
{
	// 50% duty same as square
	float val1 = w_oscillator_pulse(0.25f, 1.0f, 0.0f, 0.5f);
	ck_assert_float_eq_tol(val1, 1.0f, 0.0001f);
	float val2 = w_oscillator_pulse(0.75f, 1.0f, 0.0f, 0.5f);
	ck_assert_float_eq_tol(val2, -1.0f, 0.0001f);
}
END_TEST

START_TEST(test_pulse_25_duty)
{
	// 25% duty: high for first quarter
	float val1 = w_oscillator_pulse(0.1f, 1.0f, 0.0f, 0.25f);
	ck_assert_float_eq_tol(val1, 1.0f, 0.0001f);
	float val2 = w_oscillator_pulse(0.3f, 1.0f, 0.0f, 0.25f);
	ck_assert_float_eq_tol(val2, -1.0f, 0.0001f);
}
END_TEST

START_TEST(test_pulse_75_duty)
{
	// 75% duty: high for first three quarters
	float val1 = w_oscillator_pulse(0.5f, 1.0f, 0.0f, 0.75f);
	ck_assert_float_eq_tol(val1, 1.0f, 0.0001f);
	float val2 = w_oscillator_pulse(0.8f, 1.0f, 0.0f, 0.75f);
	ck_assert_float_eq_tol(val2, -1.0f, 0.0001f);
}
END_TEST


/*****************************
*  steps waveform tcase      *
*****************************/

START_TEST(test_steps_2_levels)
{
	// 2 steps: -1 and +1
	float val1 = w_oscillator_steps(0.25f, 1.0f, 0.0f, 2);
	ck_assert_float_eq_tol(val1, -1.0f, 0.0001f);
	float val2 = w_oscillator_steps(0.75f, 1.0f, 0.0f, 2);
	ck_assert_float_eq_tol(val2, 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_steps_4_levels)
{
	// 4 steps: -1, -0.333, +0.333, +1
	float val = w_oscillator_steps(0.0f, 1.0f, 0.0f, 4);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST

START_TEST(test_steps_1_level)
{
	// 1 step should return 0
	float val = w_oscillator_steps(0.5f, 1.0f, 0.0f, 1);
	ck_assert_float_eq_tol(val, 0.0f, 0.0001f);
}
END_TEST


/*****************************
*  variable exp tcase        *
*****************************/

START_TEST(test_var_exp_at_zero)
{
	float val = w_oscillator_variable_exponential(0.0f, 1.0f, 0.0f, 3.0f);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST

START_TEST(test_var_exp_at_one)
{
	float val = w_oscillator_variable_exponential(1.0f, 1.0f, 0.0f, 3.0f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_var_exp_zero_exponent)
{
	// zero exponent should be linear
	float val = w_oscillator_variable_exponential(0.5f, 1.0f, 0.0f, 0.0f);
	ck_assert_float_eq_tol(val, 0.0f, 0.0001f);
}
END_TEST

START_TEST(test_var_exp_high_exponent)
{
	// higher exponent = steeper curve, midpoint should be below linear
	float val = w_oscillator_variable_exponential(0.5f, 1.0f, 0.0f, 5.0f);
	ck_assert_float_lt(val, 0.0f);
}
END_TEST


/*****************************
*  generic compute tcase     *
*****************************/

START_TEST(test_compute_dispatches_sine)
{
	float val = w_oscillator_compute(W_WAVEFORM_SINE, 0.25f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_compute_dispatches_square)
{
	float val = w_oscillator_compute(W_WAVEFORM_SQUARE, 0.25f, 1.0f, 0.0f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_compute_ext_pulse)
{
	float val = w_oscillator_compute_ext(W_WAVEFORM_PULSE, 0.1f, 1.0f, 0.0f, 0.25f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_compute_ext_steps)
{
	float val = w_oscillator_compute_ext(W_WAVEFORM_STEPS, 0.0f, 1.0f, 0.0f, 4.0f);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST

START_TEST(test_compute_ext_var_exp)
{
	float val = w_oscillator_compute_ext(W_WAVEFORM_VARIABLE_EXPONENTIAL, 0.0f, 1.0f, 0.0f, 3.0f);
	ck_assert_float_eq_tol(val, -1.0f, 0.0001f);
}
END_TEST

START_TEST(test_compute_ext_fallback)
{
	// non-extended type ignores extra param
	float val = w_oscillator_compute_ext(W_WAVEFORM_SINE, 0.25f, 1.0f, 0.0f, 999.0f);
	ck_assert_float_eq_tol(val, 1.0f, 0.0001f);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_oscillator_suite(void)
{
	Suite *s = suite_create("whisker_oscillator");

	TCase *tc_phase = tcase_create("phase_update");
	tcase_set_timeout(tc_phase, 10);
	tcase_add_test(tc_phase, test_phase_update_advances);
	tcase_add_test(tc_phase, test_phase_update_wraps);
	tcase_add_test(tc_phase, test_phase_update_zero_period);
	tcase_add_test(tc_phase, test_phase_update_negative_period);
	tcase_add_test(tc_phase, test_effective_phase_basic);
	tcase_add_test(tc_phase, test_effective_phase_with_shift);
	tcase_add_test(tc_phase, test_effective_phase_wraps);
	suite_add_tcase(s, tc_phase);

	TCase *tc_sine = tcase_create("sine");
	tcase_set_timeout(tc_sine, 10);
	tcase_add_test(tc_sine, test_sine_at_zero);
	tcase_add_test(tc_sine, test_sine_at_quarter);
	tcase_add_test(tc_sine, test_sine_at_half);
	tcase_add_test(tc_sine, test_sine_at_three_quarter);
	tcase_add_test(tc_sine, test_sine_with_amplitude);
	tcase_add_test(tc_sine, test_sine_with_offset);
	suite_add_tcase(s, tc_sine);

	TCase *tc_triangle = tcase_create("triangle");
	tcase_set_timeout(tc_triangle, 10);
	tcase_add_test(tc_triangle, test_triangle_at_zero);
	tcase_add_test(tc_triangle, test_triangle_at_half);
	tcase_add_test(tc_triangle, test_triangle_at_one);
	suite_add_tcase(s, tc_triangle);

	TCase *tc_square = tcase_create("square");
	tcase_set_timeout(tc_square, 10);
	tcase_add_test(tc_square, test_square_first_half);
	tcase_add_test(tc_square, test_square_second_half);
	tcase_add_test(tc_square, test_square_at_boundary);
	suite_add_tcase(s, tc_square);

	TCase *tc_sawtooth = tcase_create("sawtooth");
	tcase_set_timeout(tc_sawtooth, 10);
	tcase_add_test(tc_sawtooth, test_sawtooth_at_zero);
	tcase_add_test(tc_sawtooth, test_sawtooth_at_half);
	tcase_add_test(tc_sawtooth, test_sawtooth_at_one);
	suite_add_tcase(s, tc_sawtooth);

	TCase *tc_inv_saw = tcase_create("inverse_sawtooth");
	tcase_set_timeout(tc_inv_saw, 10);
	tcase_add_test(tc_inv_saw, test_inverse_sawtooth_at_zero);
	tcase_add_test(tc_inv_saw, test_inverse_sawtooth_at_half);
	tcase_add_test(tc_inv_saw, test_inverse_sawtooth_at_one);
	suite_add_tcase(s, tc_inv_saw);

	TCase *tc_bounce = tcase_create("bounce");
	tcase_set_timeout(tc_bounce, 10);
	tcase_add_test(tc_bounce, test_bounce_at_zero);
	tcase_add_test(tc_bounce, test_bounce_at_half);
	tcase_add_test(tc_bounce, test_bounce_always_positive);
	suite_add_tcase(s, tc_bounce);

	TCase *tc_smooth = tcase_create("smooth_step");
	tcase_set_timeout(tc_smooth, 10);
	tcase_add_test(tc_smooth, test_smooth_step_at_zero);
	tcase_add_test(tc_smooth, test_smooth_step_at_one);
	tcase_add_test(tc_smooth, test_smooth_step_at_half);
	suite_add_tcase(s, tc_smooth);

	TCase *tc_rect = tcase_create("rectified_sine");
	tcase_set_timeout(tc_rect, 10);
	tcase_add_test(tc_rect, test_rectified_sine_at_zero);
	tcase_add_test(tc_rect, test_rectified_sine_at_quarter);
	suite_add_tcase(s, tc_rect);

	TCase *tc_noise = tcase_create("noise");
	tcase_set_timeout(tc_noise, 10);
	tcase_add_test(tc_noise, test_noise_in_range);
	tcase_add_test(tc_noise, test_noise_with_offset);
	suite_add_tcase(s, tc_noise);

	TCase *tc_fixed_exp = tcase_create("fixed_exponential");
	tcase_set_timeout(tc_fixed_exp, 10);
	tcase_add_test(tc_fixed_exp, test_fixed_exp_at_zero);
	tcase_add_test(tc_fixed_exp, test_fixed_exp_at_one);
	tcase_add_test(tc_fixed_exp, test_fixed_exp_monotonic);
	suite_add_tcase(s, tc_fixed_exp);

	TCase *tc_pulse = tcase_create("pulse");
	tcase_set_timeout(tc_pulse, 10);
	tcase_add_test(tc_pulse, test_pulse_50_duty);
	tcase_add_test(tc_pulse, test_pulse_25_duty);
	tcase_add_test(tc_pulse, test_pulse_75_duty);
	suite_add_tcase(s, tc_pulse);

	TCase *tc_steps = tcase_create("steps");
	tcase_set_timeout(tc_steps, 10);
	tcase_add_test(tc_steps, test_steps_2_levels);
	tcase_add_test(tc_steps, test_steps_4_levels);
	tcase_add_test(tc_steps, test_steps_1_level);
	suite_add_tcase(s, tc_steps);

	TCase *tc_var_exp = tcase_create("variable_exponential");
	tcase_set_timeout(tc_var_exp, 10);
	tcase_add_test(tc_var_exp, test_var_exp_at_zero);
	tcase_add_test(tc_var_exp, test_var_exp_at_one);
	tcase_add_test(tc_var_exp, test_var_exp_zero_exponent);
	tcase_add_test(tc_var_exp, test_var_exp_high_exponent);
	suite_add_tcase(s, tc_var_exp);

	TCase *tc_compute = tcase_create("generic_compute");
	tcase_set_timeout(tc_compute, 10);
	tcase_add_test(tc_compute, test_compute_dispatches_sine);
	tcase_add_test(tc_compute, test_compute_dispatches_square);
	tcase_add_test(tc_compute, test_compute_ext_pulse);
	tcase_add_test(tc_compute, test_compute_ext_steps);
	tcase_add_test(tc_compute, test_compute_ext_var_exp);
	tcase_add_test(tc_compute, test_compute_ext_fallback);
	suite_add_tcase(s, tc_compute);

	return s;
}

int main(void)
{
	Suite *s = whisker_oscillator_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
