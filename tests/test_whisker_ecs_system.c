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
	struct w_systems *s = wcreate_and_init_systems_container();

	// verify empty arrays
	ck_assert_int_eq(0, s->systems_length);

	// free
	w_free_systems_container_all(s);
}
END_TEST


START_TEST(test_whisker_ecs_system_get_iterator_and_iterate)
{
	// create entities, components and systems holders
	struct w_ecs *ecs = w_ecs_create();

	// create and register a system
	struct w_system sy = {
		.world = ecs->world,
	};
	struct w_system *sys = w_register_system(ecs, NULL, "test_phase", "test_system", 0);

	// create some entities with component names
	w_entity_api_create_named_(ecs->world, "comp1");
	w_entity_api_create_named_(ecs->world, "comp2");
	w_entity_api_create_named_(ecs->world, "comp3");
	w_entity_api_create_named_(ecs->world, "comp4");
	w_entity_api_create_named_(ecs->world, "comp5");

	// create some components
	w_entity_id comp1 = w_entity_api_create_named_(ecs->world, "comp1");
	w_entity_id comp2 = w_entity_api_create_named_(ecs->world, "comp2");
	w_entity_id comp3 = w_entity_api_create_named_(ecs->world, "comp3");
	w_entity_id comp4 = w_entity_api_create_named_(ecs->world, "comp4");
	w_entity_id comp5 = w_entity_api_create_named_(ecs->world, "comp5");
	w_entity_id comp6 = w_entity_api_create_named_(ecs->world, "comp6");

	int val1 = 4345;
	w_set_component_(ecs->world, comp2, sizeof(int), w_entity_id_from_raw(12), &val1);
	w_sort_component_array(ecs->world, comp2);
	int val2 = 98798;
	w_set_component_(ecs->world, comp5, sizeof(int), w_entity_id_from_raw(54), &val2);
	w_sort_component_array(ecs->world, comp5);
	int val3 = 321;
	w_set_component_(ecs->world, comp6, sizeof(int), w_entity_id_from_raw(88), &val3);
	w_sort_component_array(ecs->world, comp6);

	// request an iterator with the created components
	struct w_iterator *itor = w_query(&sys->thread_contexts[0], 0, "comp1,comp2,comp3", "comp4,comp5", "comp6");

	// get component array using cached IDs
	w_sparse_set *comp2_ss = w_get_component_array(ecs->world, itor->component_ids_rw[1]);
	w_sparse_set *comp5_ss = w_get_component_array(ecs->world, itor->component_ids_rw[4]);
	w_sparse_set *comp6_ss = w_get_component_array(ecs->world, itor->component_ids_opt[0]);

	// validate values
	int val1_obtained = *(int*)w_sparse_set_get(comp2_ss, 12);
	int val2_obtained = *(int*)w_sparse_set_get(comp5_ss, 54);
	int val3_obtained = *(int*)w_sparse_set_get(comp6_ss, 88);

	ck_assert_int_eq(4345, val1_obtained);
	ck_assert_int_eq(98798, val2_obtained);
	ck_assert_int_eq(321, val3_obtained);

	// set some more components and do a demo iteration
	w_set_component_(ecs->world, comp1, sizeof(int), w_entity_id_from_raw(10), &(int){ 123 });
	w_set_component_(ecs->world, comp2, sizeof(int), w_entity_id_from_raw(10), &(int){ 123 });
	w_set_component_(ecs->world, comp3, sizeof(int), w_entity_id_from_raw(10), &(int){ 123 });
	w_set_component_(ecs->world, comp4, sizeof(int), w_entity_id_from_raw(10), &(int){ 123 });
	w_set_component_(ecs->world, comp5, sizeof(int), w_entity_id_from_raw(10), &(int){ 123 });
	w_set_component_(ecs->world, comp6, sizeof(int), w_entity_id_from_raw(10), &(int){ 123 });

	w_set_component_(ecs->world, comp1, sizeof(int), w_entity_id_from_raw(11), &(int){ 123 });
	w_set_component_(ecs->world, comp2, sizeof(int), w_entity_id_from_raw(11), &(int){ 123 });
	w_set_component_(ecs->world, comp3, sizeof(int), w_entity_id_from_raw(11), &(int){ 123 });
	/* whisker_ecs_c_set_component(c, comp4, sizeof(int), whisker_ecs_e_id(11), &(int){ 123 }); */
	w_set_component_(ecs->world, comp5, sizeof(int), w_entity_id_from_raw(11), &(int){ 123 });
	w_set_component_(ecs->world, comp6, sizeof(int), w_entity_id_from_raw(11), &(int){ 123 });

	w_set_component_(ecs->world, comp1, sizeof(int), w_entity_id_from_raw(15), &(int){ 123 });
	w_set_component_(ecs->world, comp2, sizeof(int), w_entity_id_from_raw(15), &(int){ 123 });
	w_set_component_(ecs->world, comp3, sizeof(int), w_entity_id_from_raw(15), &(int){ 123 });
	w_set_component_(ecs->world, comp4, sizeof(int), w_entity_id_from_raw(15), &(int){ 123 });
	w_set_component_(ecs->world, comp5, sizeof(int), w_entity_id_from_raw(15), &(int){ 123 });

	w_set_component_(ecs->world, comp1, sizeof(int), w_entity_id_from_raw(16), &(int){ 123 });
	w_set_component_(ecs->world, comp2, sizeof(int), w_entity_id_from_raw(16), &(int){ 123 });
	/* whisker_ecs_c_set_component(c, comp3, sizeof(int), whisker_ecs_e_id(16), &(int){ 123 }); */
	/* whisker_ecs_c_set_component(c, comp4, sizeof(int), whisker_ecs_e_id(16), &(int){ 123 }); */
	/* whisker_ecs_c_set_component(c, comp5, sizeof(int), whisker_ecs_e_id(16), &(int){ 123 }); */

	w_set_component_(ecs->world, comp1, sizeof(int), w_entity_id_from_raw(19), &(int){ 123 });
	w_set_component_(ecs->world, comp2, sizeof(int), w_entity_id_from_raw(19), &(int){ 123 });
	w_set_component_(ecs->world, comp3, sizeof(int), w_entity_id_from_raw(19), &(int){ 123 });
	w_set_component_(ecs->world, comp4, sizeof(int), w_entity_id_from_raw(19), &(int){ 123 });
	w_set_component_(ecs->world, comp5, sizeof(int), w_entity_id_from_raw(19), &(int){ 123 });

	w_set_component_(ecs->world, comp5, sizeof(int), w_entity_id_from_raw(20), &(int){ 123 });

	w_sort_component_array(ecs->world, comp1);
	w_sort_component_array(ecs->world, comp2);
	w_sort_component_array(ecs->world, comp3);
	w_sort_component_array(ecs->world, comp4);
	w_sort_component_array(ecs->world, comp5);
	w_sort_component_array(ecs->world, comp6);

	// get the iterator again
	itor = w_query(&sys->thread_contexts[0], 0, "comp1,comp2,comp3", "comp4,comp5", "comp6");

	w_entity_id expected_entities[] = {10,15,19};
	while (w_iterate(itor)) 
	{
		printf("itor test: entity %zu\n", itor->entity_id);
		ck_assert_uint_eq(expected_entities[itor->cursor].index, itor->entity_id.index);
	}
	printf("itor test: iteration ended\n");

	// test iterator reset
	itor = w_query(&sys->thread_contexts[0], 0, "comp1,comp2,comp3", "comp4,comp5", "comp6");

	while (w_iterate(itor)) 
	{
		ck_assert_uint_eq(expected_entities[itor->cursor].index, itor->entity_id.index);
	}

	// get another iterator with a single component
	itor = w_query(&sys->thread_contexts[0], 1, "comp1", "", "");

	w_entity_id expected_entities_smaller[] = {10,11,15,16,19};
	while (w_iterate(itor)) 
	{
		printf("itor single test: entity %zu\n", itor->entity_id);
		ck_assert_uint_eq(expected_entities_smaller[itor->cursor].index, itor->entity_id.index);
	}
	printf("itor single test: iteration ended\n");

	w_ecs_free(ecs);
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
	tcase_add_test(tc_core, test_whisker_ecs_system_get_iterator_and_iterate);

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
