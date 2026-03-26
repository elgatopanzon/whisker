/**
 * @author      : ElGatoPanzon
 * @file        : test_whisker_timer
 * @created     : Thursday Mar 26, 2026 13:36:00 CST
 * @description : tests for whisker_timer.h standalone timer module
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <check.h>
#include "whisker_timer.h"


/*****************************
 *  timer_init tcase         *
 *****************************/

START_TEST(test_init_sets_duration)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 2.5, W_TIMER_FLAG_NONE);
    ck_assert(fabs(duration - 2.5) < 1e-6);
}
END_TEST

START_TEST(test_init_elapsed_is_zero)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    ck_assert(fabs(elapsed) < 1e-6);
}
END_TEST

START_TEST(test_init_flags_none)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    ck_assert_int_eq(flags, W_TIMER_FLAG_NONE);
}
END_TEST

START_TEST(test_init_flags_loop)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_LOOP);
    ck_assert(w_timer_is_loop(flags));
}
END_TEST

START_TEST(test_init_flags_oneshot)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_ONESHOT);
    ck_assert(w_timer_is_oneshot(flags));
}
END_TEST

START_TEST(test_init_flags_paused)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_PAUSED);
    ck_assert(w_timer_is_paused(flags));
}
END_TEST

START_TEST(test_init_combined_flags)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_LOOP | W_TIMER_FLAG_PAUSED);
    ck_assert(w_timer_is_loop(flags));
    ck_assert(w_timer_is_paused(flags));
}
END_TEST


/*****************************
 *  timer_update tcase       *
 *****************************/

START_TEST(test_update_increments_elapsed)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    w_timer_update(&elapsed, duration, &flags, 0.25);
    ck_assert(fabs(elapsed - 0.25) < 1e-6);
}
END_TEST

START_TEST(test_update_returns_false_before_finish)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    bool finished = w_timer_update(&elapsed, duration, &flags, 0.5);
    ck_assert(!finished);
}
END_TEST

START_TEST(test_update_returns_true_on_finish)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    bool finished = w_timer_update(&elapsed, duration, &flags, 1.0);
    ck_assert(finished);
}
END_TEST

START_TEST(test_update_returns_true_on_overshoot)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    bool finished = w_timer_update(&elapsed, duration, &flags, 1.5);
    ck_assert(finished);
}
END_TEST

START_TEST(test_update_sets_finished_flag)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    w_timer_update(&elapsed, duration, &flags, 1.0);
    ck_assert(w_timer_is_finished(flags));
}
END_TEST

START_TEST(test_update_paused_no_elapsed)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_PAUSED);
    w_timer_update(&elapsed, duration, &flags, 0.5);
    ck_assert(fabs(elapsed) < 1e-6);
}
END_TEST

START_TEST(test_update_paused_returns_false)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_PAUSED);
    bool finished = w_timer_update(&elapsed, duration, &flags, 2.0);
    ck_assert(!finished);
}
END_TEST

START_TEST(test_update_zero_duration_returns_false)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 0.0, W_TIMER_FLAG_NONE);
    bool finished = w_timer_update(&elapsed, duration, &flags, 1.0);
    ck_assert(!finished);
}
END_TEST

START_TEST(test_update_negative_duration_returns_false)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, -1.0, W_TIMER_FLAG_NONE);
    bool finished = w_timer_update(&elapsed, duration, &flags, 1.0);
    ck_assert(!finished);
}
END_TEST


/*****************************
 *  timer_loop tcase         *
 *****************************/

START_TEST(test_loop_wrap_wraps_elapsed)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_LOOP);
    w_timer_update(&elapsed, duration, &flags, 1.5);
    w_timer_clear_finished(&flags);
    w_timer_wrap(&elapsed, duration);
    // elapsed should wrap: 1.5 - 1.0 = 0.5
    ck_assert(fabs(elapsed - 0.5) < 1e-6);
}
END_TEST

START_TEST(test_loop_wrap_wraps_multiple_times)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_LOOP);
    w_timer_update(&elapsed, duration, &flags, 3.25);
    w_timer_clear_finished(&flags);
    w_timer_wrap(&elapsed, duration);
    // elapsed should wrap: 3.25 % 1.0 = 0.25
    ck_assert(fabs(elapsed - 0.25) < 1e-6);
}
END_TEST

START_TEST(test_loop_sets_finished_flag)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_LOOP);
    w_timer_update(&elapsed, duration, &flags, 1.0);
    ck_assert(w_timer_is_finished(flags));
}
END_TEST

START_TEST(test_loop_continues_after_wrap)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_LOOP);
    w_timer_update(&elapsed, duration, &flags, 1.5);
    w_timer_clear_finished(&flags);
    w_timer_wrap(&elapsed, duration);
    // elapsed is now 0.5
    bool finished = w_timer_update(&elapsed, duration, &flags, 0.6);
    // 0.5 + 0.6 = 1.1 >= 1.0, should finish again
    ck_assert(finished);
}
END_TEST


/*****************************
 *  timer_oneshot tcase      *
 *****************************/

START_TEST(test_oneshot_sets_finished)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_ONESHOT);
    w_timer_update(&elapsed, duration, &flags, 1.0);
    ck_assert(w_timer_is_finished(flags));
}
END_TEST

START_TEST(test_oneshot_flag_preserved)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_ONESHOT);
    w_timer_update(&elapsed, duration, &flags, 1.0);
    ck_assert(w_timer_is_oneshot(flags));
}
END_TEST


/*****************************
 *  timer_pause tcase        *
 *****************************/

START_TEST(test_pause_sets_flag)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    w_timer_pause(&flags);
    ck_assert(w_timer_is_paused(flags));
}
END_TEST

START_TEST(test_unpause_clears_flag)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_PAUSED);
    w_timer_unpause(&flags);
    ck_assert(!w_timer_is_paused(flags));
}
END_TEST

START_TEST(test_pause_mid_timer)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    w_timer_update(&elapsed, duration, &flags, 0.3);
    w_timer_pause(&flags);
    w_timer_update(&elapsed, duration, &flags, 0.5);
    // elapsed should remain 0.3
    ck_assert(fabs(elapsed - 0.3) < 1e-6);
}
END_TEST

START_TEST(test_unpause_resumes)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    w_timer_update(&elapsed, duration, &flags, 0.3);
    w_timer_pause(&flags);
    w_timer_update(&elapsed, duration, &flags, 0.5);
    w_timer_unpause(&flags);
    w_timer_update(&elapsed, duration, &flags, 0.2);
    // elapsed should be 0.3 + 0.2 = 0.5
    ck_assert(fabs(elapsed - 0.5) < 1e-6);
}
END_TEST


/*****************************
 *  timer_reset tcase        *
 *****************************/

START_TEST(test_reset_clears_elapsed)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    w_timer_update(&elapsed, duration, &flags, 0.5);
    w_timer_reset(&elapsed, &flags);
    ck_assert(fabs(elapsed) < 1e-6);
}
END_TEST

START_TEST(test_reset_clears_finished)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    w_timer_update(&elapsed, duration, &flags, 1.0);
    w_timer_reset(&elapsed, &flags);
    ck_assert(!w_timer_is_finished(flags));
}
END_TEST

START_TEST(test_reset_preserves_duration)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 2.5, W_TIMER_FLAG_NONE);
    w_timer_update(&elapsed, duration, &flags, 1.0);
    w_timer_reset(&elapsed, &flags);
    ck_assert(fabs(duration - 2.5) < 1e-6);
}
END_TEST

START_TEST(test_reset_preserves_loop_flag)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_LOOP);
    w_timer_update(&elapsed, duration, &flags, 1.5);
    w_timer_reset(&elapsed, &flags);
    ck_assert(w_timer_is_loop(flags));
}
END_TEST

START_TEST(test_reset_preserves_paused_flag)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_PAUSED);
    w_timer_reset(&elapsed, &flags);
    ck_assert(w_timer_is_paused(flags));
}
END_TEST


/*****************************
 *  timer_progress tcase     *
 *****************************/

START_TEST(test_progress_zero_at_start)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    double p = w_timer_progress(elapsed, duration);
    ck_assert(fabs(p) < 1e-6);
}
END_TEST

START_TEST(test_progress_half)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    w_timer_update(&elapsed, duration, &flags, 0.5);
    double p = w_timer_progress(elapsed, duration);
    ck_assert(fabs(p - 0.5) < 1e-6);
}
END_TEST

START_TEST(test_progress_capped_at_one)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    w_timer_update(&elapsed, duration, &flags, 2.0);
    double p = w_timer_progress(elapsed, duration);
    ck_assert(fabs(p - 1.0) < 1e-6);
}
END_TEST

START_TEST(test_progress_zero_duration_returns_zero)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 0.0, W_TIMER_FLAG_NONE);
    double p = w_timer_progress(elapsed, duration);
    ck_assert(fabs(p) < 1e-6);
}
END_TEST

START_TEST(test_remaining_at_start)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 2.0, W_TIMER_FLAG_NONE);
    double r = w_timer_remaining(elapsed, duration);
    ck_assert(fabs(r - 2.0) < 1e-6);
}
END_TEST

START_TEST(test_remaining_after_update)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 2.0, W_TIMER_FLAG_NONE);
    w_timer_update(&elapsed, duration, &flags, 0.5);
    double r = w_timer_remaining(elapsed, duration);
    ck_assert(fabs(r - 1.5) < 1e-6);
}
END_TEST

START_TEST(test_remaining_zero_when_finished)
{
    double elapsed, duration;
    w_timer_flags flags;
    w_timer_init(&elapsed, &duration, &flags, 1.0, W_TIMER_FLAG_NONE);
    w_timer_update(&elapsed, duration, &flags, 1.5);
    double r = w_timer_remaining(elapsed, duration);
    ck_assert(fabs(r) < 1e-6);
}
END_TEST


/*****************************
 *  suite + runner           *
 *****************************/

Suite *whisker_timer_suite(void)
{
    Suite *s = suite_create("whisker_timer");

    TCase *tc_init = tcase_create("timer_init");
    tcase_set_timeout(tc_init, 10);
    tcase_add_test(tc_init, test_init_sets_duration);
    tcase_add_test(tc_init, test_init_elapsed_is_zero);
    tcase_add_test(tc_init, test_init_flags_none);
    tcase_add_test(tc_init, test_init_flags_loop);
    tcase_add_test(tc_init, test_init_flags_oneshot);
    tcase_add_test(tc_init, test_init_flags_paused);
    tcase_add_test(tc_init, test_init_combined_flags);
    suite_add_tcase(s, tc_init);

    TCase *tc_update = tcase_create("timer_update");
    tcase_set_timeout(tc_update, 10);
    tcase_add_test(tc_update, test_update_increments_elapsed);
    tcase_add_test(tc_update, test_update_returns_false_before_finish);
    tcase_add_test(tc_update, test_update_returns_true_on_finish);
    tcase_add_test(tc_update, test_update_returns_true_on_overshoot);
    tcase_add_test(tc_update, test_update_sets_finished_flag);
    tcase_add_test(tc_update, test_update_paused_no_elapsed);
    tcase_add_test(tc_update, test_update_paused_returns_false);
    tcase_add_test(tc_update, test_update_zero_duration_returns_false);
    tcase_add_test(tc_update, test_update_negative_duration_returns_false);
    suite_add_tcase(s, tc_update);

    TCase *tc_loop = tcase_create("timer_loop");
    tcase_set_timeout(tc_loop, 10);
    tcase_add_test(tc_loop, test_loop_wrap_wraps_elapsed);
    tcase_add_test(tc_loop, test_loop_wrap_wraps_multiple_times);
    tcase_add_test(tc_loop, test_loop_sets_finished_flag);
    tcase_add_test(tc_loop, test_loop_continues_after_wrap);
    suite_add_tcase(s, tc_loop);

    TCase *tc_oneshot = tcase_create("timer_oneshot");
    tcase_set_timeout(tc_oneshot, 10);
    tcase_add_test(tc_oneshot, test_oneshot_sets_finished);
    tcase_add_test(tc_oneshot, test_oneshot_flag_preserved);
    suite_add_tcase(s, tc_oneshot);

    TCase *tc_pause = tcase_create("timer_pause");
    tcase_set_timeout(tc_pause, 10);
    tcase_add_test(tc_pause, test_pause_sets_flag);
    tcase_add_test(tc_pause, test_unpause_clears_flag);
    tcase_add_test(tc_pause, test_pause_mid_timer);
    tcase_add_test(tc_pause, test_unpause_resumes);
    suite_add_tcase(s, tc_pause);

    TCase *tc_reset = tcase_create("timer_reset");
    tcase_set_timeout(tc_reset, 10);
    tcase_add_test(tc_reset, test_reset_clears_elapsed);
    tcase_add_test(tc_reset, test_reset_clears_finished);
    tcase_add_test(tc_reset, test_reset_preserves_duration);
    tcase_add_test(tc_reset, test_reset_preserves_loop_flag);
    tcase_add_test(tc_reset, test_reset_preserves_paused_flag);
    suite_add_tcase(s, tc_reset);

    TCase *tc_progress = tcase_create("timer_progress");
    tcase_set_timeout(tc_progress, 10);
    tcase_add_test(tc_progress, test_progress_zero_at_start);
    tcase_add_test(tc_progress, test_progress_half);
    tcase_add_test(tc_progress, test_progress_capped_at_one);
    tcase_add_test(tc_progress, test_progress_zero_duration_returns_zero);
    tcase_add_test(tc_progress, test_remaining_at_start);
    tcase_add_test(tc_progress, test_remaining_after_update);
    tcase_add_test(tc_progress, test_remaining_zero_when_finished);
    suite_add_tcase(s, tc_progress);

    return s;
}

int main(void)
{
    Suite *s = whisker_timer_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
