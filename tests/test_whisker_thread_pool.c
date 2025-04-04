/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_thread_pool
 * @created     : Thursday Mar 13, 2025 19:42:46 CST
 */

#include "whisker_std.h"

#include "whisker_thread_pool.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>

START_TEST(test_whisker_thread_pool_create_and_destroy)
{
	// create thread pool using default thread count
	w_thread_pool *tp = w_thread_pool_create_and_init(0, "unit_test_1");

	// threads should be alive
	ck_assert_int_eq(w_thread_pool_system_core_count(), tp->thread_count);
	
	// destroy and wait for pool to stop
	w_thread_pool_free_all(tp);
}
END_TEST


void whisker_thread_pool_test_work_func(void *arg, w_thread_pool_context *context)
{
	double *val = arg;
	*val += 1;
}

START_TEST(test_whisker_thread_pool_work_test)
{
	// create thread pool using default thread count
	w_thread_pool *tp = w_thread_pool_create_and_init(0, "unit_test_2");

	// create work items
	int work_item_count = 100;
	double values[work_item_count] = {};	
	double values_expected[work_item_count] = {};	
	for (int i = 0; i < work_item_count; ++i)
	{
		values[i] = i * 100;
		values_expected[i] = values[i] + 1;

		w_thread_pool_queue_work(tp, whisker_thread_pool_test_work_func, &values[i]);
	}

	// wait for pool to stop all work
	w_thread_pool_wait_work(tp);

	// check all work items values
	for (int i = 0; i < work_item_count; ++i)
	{
		ck_assert_double_eq(values_expected[i], values[i]);
	}

	// stop all threads and free pool
	w_thread_pool_free_all(tp);
}
END_TEST

struct whisker_thread_pool_pinned_test
{
	size_t count;
	double values[100];
	size_t values_length;
};

void whisker_thread_pool_test_work_func_pinned(void *arg, w_thread_pool_context *t)
{
	struct whisker_thread_pool_pinned_test *pinned = arg;

	for (int i = 0; i < pinned->count; ++i)
	{
		bool process = (t->thread_id == i % t->thread_pool->thread_max);
		if (process)
		{
			pinned->values[i] += 1;
		}
	}
}

START_TEST(test_whisker_thread_pool_pinned_work_test)
{
	// create thread pool using default thread count
	w_thread_pool *tp = w_thread_pool_create_and_init(0, "unit_test_3");

	struct whisker_thread_pool_pinned_test pinned = {0};
	pinned.count = 100;

	// create work items
	double values_expected[pinned.count] = {};	
	for (int i = 0; i < pinned.count; ++i)
	{
		pinned.values[i] = i * 100;
		values_expected[i] = pinned.values[i] + 2;
	}

	w_thread_pool_queue_work_all(tp, whisker_thread_pool_test_work_func_pinned, &pinned);
	w_thread_pool_wait_work(tp);

	w_thread_pool_queue_work_all(tp, whisker_thread_pool_test_work_func_pinned, &pinned);
	w_thread_pool_wait_work(tp);

	// check all work items values
	for (int i = 0; i < pinned.count; ++i)
	{
		ck_assert_double_eq(values_expected[i], pinned.values[i]);
	}

	// stop all threads and free pool
	w_thread_pool_free_all(tp);
}
END_TEST


Suite* whisker_thread_pool_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("whisker_thread_pool");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_set_timeout(tc_core, 10);

	tcase_add_test(tc_core, test_whisker_thread_pool_create_and_destroy);
	tcase_add_test(tc_core, test_whisker_thread_pool_work_test);
	tcase_add_test(tc_core, test_whisker_thread_pool_pinned_work_test);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = whisker_thread_pool_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
