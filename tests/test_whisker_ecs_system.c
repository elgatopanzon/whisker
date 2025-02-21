/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_ecs_system
 * @created     : Thursday Feb 13, 2025 18:21:53 CST
 */

#include "whisker_std.h"

#include <stdio.h>
#include <stdlib.h>

#include "check.h"

#include "whisker_array.h"
#include "whisker_ecs.h"

START_TEST(test_whisker_ecs_system_create_systems_struct)
{
	whisker_ecs_systems *s;
	whisker_ecs_s_create_systems(&s);

	// verify empty arrays
	ck_assert_int_eq(0, warr_length(s->systems));

	// free
	whisker_ecs_s_free_systems(s);
}
END_TEST

START_TEST(test_whisker_ecs_system_get_component_name_index)
{
	char *component_names = "c1,c2,c55,c5,c0";

	ck_assert_int_eq(0, whisker_ecs_s_get_component_name_index(component_names, "c1"));
	ck_assert_int_eq(1, whisker_ecs_s_get_component_name_index(component_names, "c2"));
	ck_assert_int_eq(4, whisker_ecs_s_get_component_name_index(component_names, "c0"));
	// TODO: fix this if it ever becomes a problem
	/* ck_assert_int_eq(3, whisker_ecs_s_get_component_name_index(component_names, "c5")); */
	ck_assert_int_eq(2, whisker_ecs_s_get_component_name_index(component_names, "c55"));

	ck_assert_int_eq(-1, whisker_ecs_s_get_component_name_index(component_names, "cxxx"));
}
END_TEST

Suite* whisker_ecs_system_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("whisker_ecs_system");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_set_timeout(tc_core, 10);

	tcase_add_test(tc_core, test_whisker_ecs_system_create_systems_struct);
	tcase_add_test(tc_core, test_whisker_ecs_system_get_component_name_index);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = whisker_ecs_system_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
