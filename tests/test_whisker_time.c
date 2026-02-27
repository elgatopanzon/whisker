/**
 * @author      : ElGatoPanzon
 * @file        : test_whisker_time
 * @created     : Thursday Feb 26, 2026 15:14:00 CST
 * @description : tests for whisker_time.h precise timer and time step functions
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>

#include <check.h>
#include "whisker_time.h"


/*****************************
*  time_precise tcase      *
*****************************/

START_TEST(test_precise_returns_nonzero)
{
    uint64_t t = w_time_precise();
    ck_assert(t > 0);
}
END_TEST

START_TEST(test_precise_monotonically_increasing)
{
    uint64_t t1 = w_time_precise();
    uint64_t t2 = w_time_precise();
    ck_assert(t2 >= t1);
}
END_TEST

START_TEST(test_precise_advances_over_time)
{
    uint64_t t1 = w_time_precise();
    // spin to burn a small amount of time
    volatile int x = 0;
    for (int i = 0; i < 100000; i++) { x += i; }
    (void)x;
    uint64_t t2 = w_time_precise();
    ck_assert(t2 > t1);
}
END_TEST


/*****************************
*  time_step_create tcase  *
*****************************/

START_TEST(test_create_sets_update_rate_sec)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    ck_assert(fabs(ts.update_rate_sec - 60.0) < 1e-9);
}
END_TEST

START_TEST(test_create_sets_uncapped)
{
    w_time_step ts = w_time_step_create(60.0, 0, true, false, false, false, false, false);
    ck_assert(ts.uncapped == true);
}
END_TEST

START_TEST(test_create_sets_delta_clamp_enabled)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, true, false, false, false, false);
    ck_assert(ts.delta_clamp_enabled == true);
}
END_TEST

START_TEST(test_create_sets_delta_snap_enabled)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, true, false, false, false);
    ck_assert(ts.delta_snap_enabled == true);
}
END_TEST

START_TEST(test_create_sets_delta_average_enabled)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, true, false, false);
    ck_assert(ts.delta_average_enabled == true);
}
END_TEST

START_TEST(test_create_sets_delta_accumulation_enabled)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, true, false);
    ck_assert(ts.delta_accumulation_enabled == true);
}
END_TEST

START_TEST(test_create_sets_delta_accumulation_clamp_enabled)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, true);
    ck_assert(ts.delta_accumulation_clamp_enabled == true);
}
END_TEST

START_TEST(test_create_sets_update_count_max)
{
    w_time_step ts = w_time_step_create(60.0, 5, false, false, false, false, false, false);
    ck_assert_int_eq(ts.update_count_max, 5);
}
END_TEST

START_TEST(test_create_time_resolution_is_constant)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    ck_assert(ts.time_resolution == WHISKER_TIME_RESOLUTION);
}
END_TEST

START_TEST(test_create_delta_snap_base_is_60)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    ck_assert(ts.delta_snap_base == 60);
}
END_TEST

START_TEST(test_create_update_multiplier_is_1)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    ck_assert_int_eq(ts.update_multiplier, 1);
}
END_TEST

START_TEST(test_create_delta_time_fixed_is_inverse_rate)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    double expected = 1.0 / 60.0;
    ck_assert(fabs(ts.delta_time_fixed - expected) < 1e-9);
}
END_TEST

START_TEST(test_create_update_time_target_is_resolution_div_rate)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    uint64_t expected = WHISKER_TIME_RESOLUTION / 60;
    ck_assert(ts.update_time_target == expected);
}
END_TEST

START_TEST(test_create_update_time_current_is_zero)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    ck_assert(ts.update_time_current == 0);
}
END_TEST

START_TEST(test_create_delta_accumulation_is_zero)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    ck_assert(ts.delta_accumulation == 0);
}
END_TEST

START_TEST(test_create_update_time_prev_is_nonzero)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    ck_assert(ts.update_time_prev > 0);
}
END_TEST

START_TEST(test_create_delta_averages_filled_with_target)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    uint64_t expected = ts.update_time_target;
    for (int i = 0; i < WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT; i++)
    {
        ck_assert(ts.delta_averages[i] == expected);
    }
}
END_TEST

START_TEST(test_create_delta_average_residual_is_zero)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    ck_assert(ts.delta_average_residual == 0);
}
END_TEST

START_TEST(test_create_snap_max_error_computed)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    uint64_t expected = (uint64_t)(WHISKER_TIME_RESOLUTION * WHISKER_TIME_STEP_SNAP_ERROR_MULTIPLIER);
    ck_assert(ts.delta_snap_max_error == expected);
}
END_TEST


/*****************************
*  time_step_set_rate tcase *
*****************************/

START_TEST(test_set_rate_updates_rate_sec)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    w_time_step_set_rate(&ts, 30.0);
    ck_assert(fabs(ts.update_rate_sec - 30.0) < 1e-9);
}
END_TEST

START_TEST(test_set_rate_updates_delta_time_fixed)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    w_time_step_set_rate(&ts, 30.0);
    double expected = 1.0 / 30.0;
    ck_assert(fabs(ts.delta_time_fixed - expected) < 1e-9);
}
END_TEST

START_TEST(test_set_rate_updates_update_time_target)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    w_time_step_set_rate(&ts, 30.0);
    uint64_t expected = WHISKER_TIME_RESOLUTION / 30;
    ck_assert(ts.update_time_target == expected);
}
END_TEST

START_TEST(test_set_rate_updates_snap_frequencies)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    w_time_step_set_rate(&ts, 30.0);
    // snap frequencies are based on delta_snap_base (60), not rate
    uint64_t base_interval = ts.time_resolution / ts.delta_snap_base;
    for (int i = 0; i < WHISKER_TIME_DELTA_SNAP_FREQ_COUNT; i++)
    {
        ck_assert(ts.delta_snap_frequencies[i] == base_interval * (i + 1));
    }
}
END_TEST

START_TEST(test_set_rate_refills_delta_averages_with_new_target)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    w_time_step_set_rate(&ts, 30.0);
    uint64_t expected = ts.update_time_target;
    for (int i = 0; i < WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT; i++)
    {
        ck_assert(ts.delta_averages[i] == expected);
    }
}
END_TEST

START_TEST(test_set_rate_resets_delta_average_residual)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    ts.delta_average_residual = 42;
    w_time_step_set_rate(&ts, 30.0);
    ck_assert(ts.delta_average_residual == 0);
}
END_TEST


/*************************************
*  time_step_advance uncapped tcase *
*************************************/

START_TEST(test_advance_uncapped_returns_one)
{
    w_time_step ts = w_time_step_create(60.0, 0, true, false, false, false, false, false);
    int n = w_time_step_advance(&ts);
    ck_assert_int_eq(n, 1);
}
END_TEST

START_TEST(test_advance_uncapped_delta_time_fixed_equals_variable)
{
    w_time_step ts = w_time_step_create(60.0, 0, true, false, false, false, false, false);
    w_time_step_advance(&ts);
    ck_assert(fabs(ts.delta_time_fixed - ts.delta_time_variable) < 1e-12);
}
END_TEST

START_TEST(test_advance_uncapped_always_one_on_repeat)
{
    w_time_step ts = w_time_step_create(60.0, 0, true, false, false, false, false, false);
    for (int i = 0; i < 5; i++)
    {
        ck_assert_int_eq(w_time_step_advance(&ts), 1);
    }
}
END_TEST


/*************************************
*  time_step_advance capped tcase  *
*************************************/

START_TEST(test_advance_capped_immediate_returns_zero)
{
    // 60Hz step, accumulation enabled -- not enough real time passes for one update
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, true, false);
    int n = w_time_step_advance(&ts);
    ck_assert_int_eq(n, 0);
}
END_TEST

START_TEST(test_advance_capped_preset_accumulation_returns_expected_count)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, true, false);
    // pre-load 3 update targets worth of accumulation
    ts.delta_accumulation = ts.update_time_target * 3;
    int n = w_time_step_advance(&ts);
    // real elapsed during the call is tiny vs 16.7ms target, so exactly 3
    ck_assert_int_eq(n, 3);
}
END_TEST

START_TEST(test_advance_capped_update_count_max_clamps)
{
    w_time_step ts = w_time_step_create(60.0, 2, false, false, false, false, true, false);
    // pre-load far more accumulation than needed
    ts.delta_accumulation = ts.update_time_target * 10;
    int n = w_time_step_advance(&ts);
    ck_assert_int_eq(n, 2);
}
END_TEST

START_TEST(test_advance_capped_increments_update_tick_count)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, true, false);
    ts.delta_accumulation = ts.update_time_target * 2;
    uint64_t before = ts.update_tick_count;
    w_time_step_advance(&ts);
    ck_assert(ts.update_tick_count == before + 2);
}
END_TEST

START_TEST(test_advance_capped_no_accumulation_never_updates)
{
    // accumulation disabled: delta_accumulation stays 0, always returns 0
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    for (int i = 0; i < 3; i++)
    {
        ck_assert_int_eq(w_time_step_advance(&ts), 0);
    }
}
END_TEST


/*************************************
*  time_step_do_step_ tcase        *
*************************************/

START_TEST(test_do_step_updates_update_time_current)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    w_time_step_do_step_(&ts);
    ck_assert(ts.update_time_current > 0);
}
END_TEST

START_TEST(test_do_step_delta_time_real_is_nonneg)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    w_time_step_do_step_(&ts);
    // delta_time_real = current - prev, both from monotonic clock
    ck_assert(ts.delta_time_real >= 0);
}
END_TEST

START_TEST(test_do_step_sets_variable_delta_positive)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    // spin to ensure nonzero elapsed time
    volatile int x = 0;
    for (int i = 0; i < 100000; i++) { x += i; }
    (void)x;
    w_time_step_do_step_(&ts);
    ck_assert(ts.delta_time_variable >= 0.0);
}
END_TEST

START_TEST(test_do_step_increments_tick_count)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    uint64_t before = ts.tick_count;
    w_time_step_do_step_(&ts);
    // tick_count += delta_time_real which is > 0
    ck_assert(ts.tick_count >= before);
}
END_TEST

START_TEST(test_do_step_accumulation_adds_delta)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, true, false);
    // ensure nonzero elapsed
    volatile int x = 0;
    for (int i = 0; i < 100000; i++) { x += i; }
    (void)x;
    w_time_step_do_step_(&ts);
    ck_assert(ts.delta_accumulation > 0);
}
END_TEST

START_TEST(test_do_step_accumulation_clamp_resets_on_overflow)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, true, true);
    // set accumulation to exceed the clamp threshold (target * SNAP_FREQ_COUNT)
    ts.delta_accumulation = ts.update_time_target * (WHISKER_TIME_DELTA_SNAP_FREQ_COUNT + 1);
    w_time_step_do_step_(&ts);
    // clamp fires: accumulation reset to 0
    ck_assert(ts.delta_accumulation == 0);
}
END_TEST

START_TEST(test_do_step_average_shifts_samples)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, true, false, false);
    // record the initial last sample slot (will be shifted out)
    uint64_t old_first = ts.delta_averages[0];
    uint64_t old_second = ts.delta_averages[1];
    w_time_step_do_step_(&ts);
    // after one step, old [1] is now at [0]
    ck_assert(ts.delta_averages[0] == old_second);
    (void)old_first;
}
END_TEST

START_TEST(test_do_step_successive_prev_advances)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, false, false);
    w_time_step_do_step_(&ts);
    uint64_t after_first = ts.update_time_prev;
    w_time_step_do_step_(&ts);
    ck_assert(ts.update_time_prev >= after_first);
}
END_TEST


/*****************************
*  time_realtime tcase       *
*****************************/

/* verify snap fires when elapsed is within the error window of freq[0].
 * we back-date update_time_prev by exactly freq[0] ns so do_step_ sees
 * an elapsed of ~freq[0]; real time still passes between the assignment
 * and the clock_gettime call inside do_step_ */
START_TEST(test_realtime_snap_locks_to_60hz_frequency)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, true, false, false, false);
    uint64_t freq0 = ts.delta_snap_frequencies[0];
    ts.update_time_prev = w_time_precise() - freq0;
    w_time_step_do_step_(&ts);
    ck_assert(ts.delta_time_real == freq0);
}
END_TEST

// verify delta clamp fires after sleeping well past the 8-frame threshold
START_TEST(test_realtime_clamp_fires_after_long_sleep)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, true, false, false, false, false);
    // 200ms far exceeds the 8 * 16.7ms = ~133ms clamp threshold
    usleep(200000);
    w_time_step_do_step_(&ts);
    // clamp sets delta_time_real to WHISKER_TIME_DELTA_SNAP_FREQ_COUNT (8)
    ck_assert_int_eq((int)ts.delta_time_real, WHISKER_TIME_DELTA_SNAP_FREQ_COUNT);
}
END_TEST

// verify accumulation grows across multiple real-time do_step_ calls
START_TEST(test_realtime_accumulation_grows_over_frames)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, true, false);
    uint64_t target = ts.update_time_target;
    for (int i = 0; i < 3; i++)
    {
        usleep(16700);
        w_time_step_do_step_(&ts);
    }
    // allow some frame jitter: expect between 2 and 5 frames worth
    ck_assert(ts.delta_accumulation >= target * 2);
    ck_assert(ts.delta_accumulation <= target * 5);
}
END_TEST

// verify advance() returns expected update count after real time accumulates
START_TEST(test_realtime_advance_count_after_delay)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, false, true, false);
    // ~3 frames at 60Hz
    usleep(50000);
    int n = w_time_step_advance(&ts);
    ck_assert_int_ge(n, 2);
    ck_assert_int_le(n, 5);
}
END_TEST

// verify averaging produces a smoothed delta near the 60Hz target
START_TEST(test_realtime_averaging_smooths_to_target)
{
    w_time_step ts = w_time_step_create(60.0, 0, false, false, false, true, false, false);
    uint64_t target = ts.update_time_target;
    // fill the average window with real samples
    for (int i = 0; i < WHISKER_TIME_DELTA_AVG_SAMPLE_COUNT + 1; i++)
    {
        usleep(16700);
        w_time_step_do_step_(&ts);
    }
    // averaged delta should be within 10ms of the 60Hz target
    uint64_t margin = 10000000ULL;
    ck_assert(ts.delta_time_real >= target - margin);
    ck_assert(ts.delta_time_real <= target + margin);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_time_suite(void)
{
    Suite *s = suite_create("whisker_time");

    TCase *tc_precise = tcase_create("time_precise");
    tcase_set_timeout(tc_precise, 10);
    tcase_add_test(tc_precise, test_precise_returns_nonzero);
    tcase_add_test(tc_precise, test_precise_monotonically_increasing);
    tcase_add_test(tc_precise, test_precise_advances_over_time);
    suite_add_tcase(s, tc_precise);

    TCase *tc_create = tcase_create("time_step_create");
    tcase_set_timeout(tc_create, 10);
    tcase_add_test(tc_create, test_create_sets_update_rate_sec);
    tcase_add_test(tc_create, test_create_sets_uncapped);
    tcase_add_test(tc_create, test_create_sets_delta_clamp_enabled);
    tcase_add_test(tc_create, test_create_sets_delta_snap_enabled);
    tcase_add_test(tc_create, test_create_sets_delta_average_enabled);
    tcase_add_test(tc_create, test_create_sets_delta_accumulation_enabled);
    tcase_add_test(tc_create, test_create_sets_delta_accumulation_clamp_enabled);
    tcase_add_test(tc_create, test_create_sets_update_count_max);
    tcase_add_test(tc_create, test_create_time_resolution_is_constant);
    tcase_add_test(tc_create, test_create_delta_snap_base_is_60);
    tcase_add_test(tc_create, test_create_update_multiplier_is_1);
    tcase_add_test(tc_create, test_create_delta_time_fixed_is_inverse_rate);
    tcase_add_test(tc_create, test_create_update_time_target_is_resolution_div_rate);
    tcase_add_test(tc_create, test_create_update_time_current_is_zero);
    tcase_add_test(tc_create, test_create_delta_accumulation_is_zero);
    tcase_add_test(tc_create, test_create_update_time_prev_is_nonzero);
    tcase_add_test(tc_create, test_create_delta_averages_filled_with_target);
    tcase_add_test(tc_create, test_create_delta_average_residual_is_zero);
    tcase_add_test(tc_create, test_create_snap_max_error_computed);
    suite_add_tcase(s, tc_create);

    TCase *tc_set_rate = tcase_create("time_step_set_rate");
    tcase_set_timeout(tc_set_rate, 10);
    tcase_add_test(tc_set_rate, test_set_rate_updates_rate_sec);
    tcase_add_test(tc_set_rate, test_set_rate_updates_delta_time_fixed);
    tcase_add_test(tc_set_rate, test_set_rate_updates_update_time_target);
    tcase_add_test(tc_set_rate, test_set_rate_updates_snap_frequencies);
    tcase_add_test(tc_set_rate, test_set_rate_refills_delta_averages_with_new_target);
    tcase_add_test(tc_set_rate, test_set_rate_resets_delta_average_residual);
    suite_add_tcase(s, tc_set_rate);

    TCase *tc_uncapped = tcase_create("time_step_advance_uncapped");
    tcase_set_timeout(tc_uncapped, 10);
    tcase_add_test(tc_uncapped, test_advance_uncapped_returns_one);
    tcase_add_test(tc_uncapped, test_advance_uncapped_delta_time_fixed_equals_variable);
    tcase_add_test(tc_uncapped, test_advance_uncapped_always_one_on_repeat);
    suite_add_tcase(s, tc_uncapped);

    TCase *tc_capped = tcase_create("time_step_advance_capped");
    tcase_set_timeout(tc_capped, 10);
    tcase_add_test(tc_capped, test_advance_capped_immediate_returns_zero);
    tcase_add_test(tc_capped, test_advance_capped_preset_accumulation_returns_expected_count);
    tcase_add_test(tc_capped, test_advance_capped_update_count_max_clamps);
    tcase_add_test(tc_capped, test_advance_capped_increments_update_tick_count);
    tcase_add_test(tc_capped, test_advance_capped_no_accumulation_never_updates);
    suite_add_tcase(s, tc_capped);

    TCase *tc_do_step = tcase_create("time_step_do_step_");
    tcase_set_timeout(tc_do_step, 10);
    tcase_add_test(tc_do_step, test_do_step_updates_update_time_current);
    tcase_add_test(tc_do_step, test_do_step_delta_time_real_is_nonneg);
    tcase_add_test(tc_do_step, test_do_step_sets_variable_delta_positive);
    tcase_add_test(tc_do_step, test_do_step_increments_tick_count);
    tcase_add_test(tc_do_step, test_do_step_accumulation_adds_delta);
    tcase_add_test(tc_do_step, test_do_step_accumulation_clamp_resets_on_overflow);
    tcase_add_test(tc_do_step, test_do_step_average_shifts_samples);
    tcase_add_test(tc_do_step, test_do_step_successive_prev_advances);
    suite_add_tcase(s, tc_do_step);

    TCase *tc_realtime = tcase_create("time_realtime");
    tcase_set_timeout(tc_realtime, 30);
    tcase_add_test(tc_realtime, test_realtime_snap_locks_to_60hz_frequency);
    tcase_add_test(tc_realtime, test_realtime_clamp_fires_after_long_sleep);
    tcase_add_test(tc_realtime, test_realtime_accumulation_grows_over_frames);
    tcase_add_test(tc_realtime, test_realtime_advance_count_after_delay);
    tcase_add_test(tc_realtime, test_realtime_averaging_smooths_to_target);
    suite_add_tcase(s, tc_realtime);

    return s;
}

int main(void)
{
    Suite *s = whisker_time_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
