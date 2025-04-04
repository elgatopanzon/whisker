/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_ecs
 * @created     : Thursday Feb 13, 2025 18:18:52 CST
 */

#include "whisker_std.h"

#include <stdio.h>
#include <stdlib.h>

#include "check.h"

#include "whisker_array.h"
#include "whisker_ecs.h"

START_TEST(test_whisker_ecs_create)
{
	struct w_ecs *ecs = w_ecs_create();

	// verify the lengths of some of the struct's arrays
	ck_assert_int_eq(0, ecs->world->components->components_length);
	// note: includes dummy system now
	ck_assert_int_eq(1, ecs->world->systems->systems_length);
	
	w_ecs_free(ecs);
}
END_TEST

Suite* whisker_ecs_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("whisker_ecs");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_set_timeout(tc_core, 10);

	tcase_add_test(tc_core, test_whisker_ecs_create);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = whisker_ecs_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
