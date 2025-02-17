/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_ecs_component
 * @created     : Thursday Feb 13, 2025 18:21:12 CST
 */

#include "whisker_std.h"

#include <stdio.h>
#include <stdlib.h>

#include "check.h"

#include "whisker_array.h"
#include "whisker_ecs.h"

START_TEST(test_whisker_ecs_component_create_components_struct)
{
	whisker_ecs_components *c;
	whisker_ecs_c_create_components(&c);

	// verify empty arrays
	ck_assert_int_eq(0, warr_length(c->components));

	// free
	whisker_ecs_c_free_components(c);
}
END_TEST

START_TEST(test_whisker_ecs_component_get_component_array)
{
	whisker_ecs_components *c;
	whisker_ecs_c_create_components(&c);

	// create component array for aritrary IDs
	uint64_t *uint_component_array;
	whisker_ecs_entity_id uint_id = {.id = 123};
	whisker_ecs_c_get_component_array(c, uint_id, sizeof(uint64_t), (void**)&uint_component_array);

	// free
	whisker_ecs_c_free_components(c);
}
END_TEST

Suite* whisker_ecs_component_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("whisker_ecs_component");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_set_timeout(tc_core, 10);

	tcase_add_test(tc_core, test_whisker_ecs_component_create_components_struct);
	tcase_add_test(tc_core, test_whisker_ecs_component_get_component_array);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = whisker_ecs_component_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

