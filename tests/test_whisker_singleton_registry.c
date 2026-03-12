/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_singleton_registry
 * @created     : Thursday Mar 12, 2026 11:48:23 CST
 * @description : tests for whisker_singleton_registry.h singleton storage
 */

#include "whisker_std.h"

#include "whisker_singleton_registry.h"
#include "whisker_arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_arena g_arena;
static struct w_singleton_registry g_registry;

static void singleton_registry_setup(void)
{
	w_arena_init(&g_arena, 64 * 1024);
	w_singleton_registry_init(&g_registry, &g_arena);
}

static void singleton_registry_teardown(void)
{
	w_singleton_registry_free(&g_registry);
	w_arena_free(&g_arena);
}


/*****************************
*  registry_init             *
*****************************/

START_TEST(test_init_arena_stored)
{
	ck_assert_ptr_eq(g_registry.arena, &g_arena);
}
END_TEST

START_TEST(test_init_total_entries_zero)
{
	ck_assert_int_eq(g_registry.map.total_entries, 0);
}
END_TEST

START_TEST(test_init_buckets_allocated)
{
	ck_assert_ptr_nonnull(g_registry.map.buckets);
}
END_TEST


/*****************************
*  set / get                 *
*****************************/

START_TEST(test_set_and_get_returns_same_pointer)
{
	int data = 42;
	w_singleton_registry_set(&g_registry, (char *)"health", &data);

	void *got = w_singleton_registry_get(&g_registry, (char *)"health");
	ck_assert_ptr_eq(got, &data);
}
END_TEST

START_TEST(test_get_missing_returns_null)
{
	void *got = w_singleton_registry_get(&g_registry, (char *)"missing");
	ck_assert_ptr_null(got);
}
END_TEST

START_TEST(test_set_overwrites_existing)
{
	int a = 1, b = 2;
	w_singleton_registry_set(&g_registry, (char *)"score", &a);
	w_singleton_registry_set(&g_registry, (char *)"score", &b);

	void *got = w_singleton_registry_get(&g_registry, (char *)"score");
	ck_assert_ptr_eq(got, &b);
}
END_TEST

START_TEST(test_multiple_singletons_isolated)
{
	int x = 10, y = 20;
	w_singleton_registry_set(&g_registry, (char *)"x", &x);
	w_singleton_registry_set(&g_registry, (char *)"y", &y);

	void *got_x = w_singleton_registry_get(&g_registry, (char *)"x");
	void *got_y = w_singleton_registry_get(&g_registry, (char *)"y");

	ck_assert_ptr_eq(got_x, &x);
	ck_assert_ptr_eq(got_y, &y);
}
END_TEST

START_TEST(test_get_value_via_pointer)
{
	float val = 3.14f;
	w_singleton_registry_set(&g_registry, (char *)"pi", &val);

	float *got = w_singleton_registry_get(&g_registry, (char *)"pi");
	ck_assert_ptr_nonnull(got);
	ck_assert_float_eq_tol(*got, 3.14f, 1e-6f);
}
END_TEST


/*****************************
*  has                       *
*****************************/

START_TEST(test_has_false_before_set)
{
	ck_assert_int_eq(w_singleton_registry_has(&g_registry, (char *)"thing"), false);
}
END_TEST

START_TEST(test_has_true_after_set)
{
	int data = 0;
	w_singleton_registry_set(&g_registry, (char *)"thing", &data);
	ck_assert_int_eq(w_singleton_registry_has(&g_registry, (char *)"thing"), true);
}
END_TEST

START_TEST(test_has_false_after_remove)
{
	int data = 0;
	w_singleton_registry_set(&g_registry, (char *)"thing", &data);
	w_singleton_registry_remove(&g_registry, (char *)"thing");
	ck_assert_int_eq(w_singleton_registry_has(&g_registry, (char *)"thing"), false);
}
END_TEST


/*****************************
*  remove                    *
*****************************/

START_TEST(test_remove_returns_true_when_exists)
{
	int data = 0;
	w_singleton_registry_set(&g_registry, (char *)"target", &data);
	bool removed = w_singleton_registry_remove(&g_registry, (char *)"target");
	ck_assert_int_eq(removed, true);
}
END_TEST

START_TEST(test_remove_returns_false_when_missing)
{
	bool removed = w_singleton_registry_remove(&g_registry, (char *)"ghost");
	ck_assert_int_eq(removed, false);
}
END_TEST

START_TEST(test_remove_makes_get_return_null)
{
	int data = 5;
	w_singleton_registry_set(&g_registry, (char *)"temp", &data);
	w_singleton_registry_remove(&g_registry, (char *)"temp");

	void *got = w_singleton_registry_get(&g_registry, (char *)"temp");
	ck_assert_ptr_null(got);
}
END_TEST

START_TEST(test_remove_does_not_affect_others)
{
	int a = 1, b = 2;
	w_singleton_registry_set(&g_registry, (char *)"a", &a);
	w_singleton_registry_set(&g_registry, (char *)"b", &b);

	w_singleton_registry_remove(&g_registry, (char *)"a");

	ck_assert_int_eq(w_singleton_registry_has(&g_registry, (char *)"a"), false);
	ck_assert_ptr_eq(w_singleton_registry_get(&g_registry, (char *)"b"), &b);
}
END_TEST

START_TEST(test_set_after_remove_works)
{
	int first = 1, second = 2;
	w_singleton_registry_set(&g_registry, (char *)"val", &first);
	w_singleton_registry_remove(&g_registry, (char *)"val");
	w_singleton_registry_set(&g_registry, (char *)"val", &second);

	void *got = w_singleton_registry_get(&g_registry, (char *)"val");
	ck_assert_ptr_eq(got, &second);
}
END_TEST


/*****************************
*  free                      *
*****************************/

START_TEST(test_free_empty_registry)
{
	struct w_arena a;
	struct w_singleton_registry r;

	w_arena_init(&a, 4096);
	w_singleton_registry_init(&r, &a);
	w_singleton_registry_free(&r);

	ck_assert_ptr_null(r.arena);

	w_arena_free(&a);
}
END_TEST

START_TEST(test_free_with_entries)
{
	struct w_arena a;
	struct w_singleton_registry r;

	w_arena_init(&a, 4096);
	w_singleton_registry_init(&r, &a);

	int x = 1, y = 2, z = 3;
	w_singleton_registry_set(&r, (char *)"x", &x);
	w_singleton_registry_set(&r, (char *)"y", &y);
	w_singleton_registry_set(&r, (char *)"z", &z);

	w_singleton_registry_free(&r);
	ck_assert_ptr_null(r.arena);

	w_arena_free(&a);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_singleton_registry_suite(void)
{
	Suite *s = suite_create("whisker_singleton_registry");

	TCase *tc_init = tcase_create("registry_init");
	tcase_add_checked_fixture(tc_init, singleton_registry_setup, singleton_registry_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_arena_stored);
	tcase_add_test(tc_init, test_init_total_entries_zero);
	tcase_add_test(tc_init, test_init_buckets_allocated);
	suite_add_tcase(s, tc_init);

	TCase *tc_set_get = tcase_create("set_get");
	tcase_add_checked_fixture(tc_set_get, singleton_registry_setup, singleton_registry_teardown);
	tcase_set_timeout(tc_set_get, 10);
	tcase_add_test(tc_set_get, test_set_and_get_returns_same_pointer);
	tcase_add_test(tc_set_get, test_get_missing_returns_null);
	tcase_add_test(tc_set_get, test_set_overwrites_existing);
	tcase_add_test(tc_set_get, test_multiple_singletons_isolated);
	tcase_add_test(tc_set_get, test_get_value_via_pointer);
	suite_add_tcase(s, tc_set_get);

	TCase *tc_has = tcase_create("has");
	tcase_add_checked_fixture(tc_has, singleton_registry_setup, singleton_registry_teardown);
	tcase_set_timeout(tc_has, 10);
	tcase_add_test(tc_has, test_has_false_before_set);
	tcase_add_test(tc_has, test_has_true_after_set);
	tcase_add_test(tc_has, test_has_false_after_remove);
	suite_add_tcase(s, tc_has);

	TCase *tc_remove = tcase_create("remove");
	tcase_add_checked_fixture(tc_remove, singleton_registry_setup, singleton_registry_teardown);
	tcase_set_timeout(tc_remove, 10);
	tcase_add_test(tc_remove, test_remove_returns_true_when_exists);
	tcase_add_test(tc_remove, test_remove_returns_false_when_missing);
	tcase_add_test(tc_remove, test_remove_makes_get_return_null);
	tcase_add_test(tc_remove, test_remove_does_not_affect_others);
	tcase_add_test(tc_remove, test_set_after_remove_works);
	suite_add_tcase(s, tc_remove);

	TCase *tc_free = tcase_create("registry_free");
	tcase_set_timeout(tc_free, 10);
	tcase_add_test(tc_free, test_free_empty_registry);
	tcase_add_test(tc_free, test_free_with_entries);
	suite_add_tcase(s, tc_free);

	return s;
}

int main(void)
{
	Suite *s = whisker_singleton_registry_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
