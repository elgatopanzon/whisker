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
	struct w_components *c = w_create_and_init_components_container();

	// verify empty arrays
	ck_assert_int_eq(0, c->components_length);

	// free
	w_free_components_container_all(c);
}
END_TEST

START_TEST(test_whisker_ecs_component_get_component_array)
{
	struct w_components *c = w_create_and_init_components_container();
	struct w_world world = {.components = c};

	// set a component and trigger creation of component sparse set
	w_entity_id component_id = {.id = 123};
	w_entity_id entity_id = {.id = 111};
	uint64_t component = 5454;

	// set the component
	w_set_component_(&world, component_id, sizeof(uint64_t), entity_id, &component);
	w_sort_component_array(&world, component_id);

	// get the component sparse set
	w_sparse_set *component_array = w_get_component_array(&world, component_id);

	// verify length
	ck_assert_uint_eq(1, component_array->sparse_index_length);

	// free
	w_free_components_container_all(c);
}
END_TEST

START_TEST(test_whisker_ecs_component_get_and_set)
{
	// create components and component array
	struct w_components *c = w_create_and_init_components_container();
	struct w_world world = {.components = c};
	w_entity_id component_id = {.id = 3};

	w_entity_id e1 = {.id = 0};

	// set using component set
	w_set_component_(&world, component_id, sizeof(uint64_t), e1, &(uint64_t){10});
	w_sort_component_array(&world, component_id);

	// check array
	int64_t *uint_e1 = w_get_component(&world, component_id, e1);
	ck_assert_uint_eq(10, *uint_e1);

	// check edirectly
	ck_assert_uint_eq(10, *(uint64_t*)w_get_component(&world, component_id, e1));

	// free
	w_free_components_container_all(c);
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
	tcase_add_test(tc_core, test_whisker_ecs_component_get_and_set);

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

