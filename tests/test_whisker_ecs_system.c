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
	whisker_ecs_system s = {};
	wdict_create(&s.component_name_index, int, 0);

	ck_assert_int_eq(0, whisker_ecs_s_get_component_name_index(&s, component_names, "c1"));
	ck_assert_int_eq(1, whisker_ecs_s_get_component_name_index(&s, component_names, "c2"));
	ck_assert_int_eq(4, whisker_ecs_s_get_component_name_index(&s, component_names, "c0"));
	// TODO: fix this if it ever becomes a problem
	/* ck_assert_int_eq(3, whisker_ecs_s_get_component_name_index(component_names, "c5")); */
	ck_assert_int_eq(2, whisker_ecs_s_get_component_name_index(&s, component_names, "c55"));
	// run again, it should fetch from the trie cache
	ck_assert_int_eq(2, whisker_ecs_s_get_component_name_index(&s, component_names, "c55"));

	// TODO: not strictly required, but this needs to be fixed
	/* ck_assert_int_eq(-1, whisker_ecs_s_get_component_name_index(&s, component_names, "cxxx")); */

	whisker_ecs_s_free_system(&s);
}
END_TEST

START_TEST(test_whisker_ecs_system_get_iterator_and_iterate)
{
	// create entities, components and systems holders
	whisker_ecs_systems *s;
	whisker_ecs_s_create_systems(&s);
	whisker_ecs_components *c;
	whisker_ecs_c_create_components(&c);
	whisker_ecs_entities *e;
	whisker_ecs_e_create_entities(&e);

	// create and register a system
	whisker_ecs_system sy = {
		.entities = e,
		.components = c,
	};
	whisker_ecs_system *sys = whisker_ecs_s_register_system(s, c, sy);

	// create some entities with component names
	whisker_ecs_e_create_named(e, "comp1");
	whisker_ecs_e_create_named(e, "comp2");
	whisker_ecs_e_create_named(e, "comp3");
	whisker_ecs_e_create_named(e, "comp4");
	whisker_ecs_e_create_named(e, "comp5");

	// create some components
	whisker_ecs_entity_id comp1 = whisker_ecs_e_create_named(e, "comp1");
	whisker_ecs_entity_id comp2 = whisker_ecs_e_create_named(e, "comp2");
	whisker_ecs_entity_id comp3 = whisker_ecs_e_create_named(e, "comp3");
	whisker_ecs_entity_id comp4 = whisker_ecs_e_create_named(e, "comp4");
	whisker_ecs_entity_id comp5 = whisker_ecs_e_create_named(e, "comp5");

	int val1 = 4345;
	whisker_ecs_c_set_component(c, comp2, sizeof(int), whisker_ecs_e_id(12), &val1);
	int val2 = 98798;
	whisker_ecs_c_set_component(c, comp5, sizeof(int), whisker_ecs_e_id(54), &val2);

	// request an iterator with the created components
	whisker_ecs_iterator *itor = whisker_ecs_s_get_iterator(sys, 0, "comp1,comp2,comp3", "comp4,comp5");

	// get component array using cached IDs
	whisker_sparse_set *comp2_ss;
	whisker_ecs_c_get_component_array(c, itor->component_ids->arr[1], &comp2_ss);
	whisker_sparse_set *comp5_ss;
	whisker_ecs_c_get_component_array(c, itor->component_ids->arr[4], &comp5_ss);

	// validate values
	int val1_obtained = *(int*)wss_get(comp2_ss, 12);
	int val2_obtained = *(int*)wss_get(comp5_ss, 54);

	ck_assert_int_eq(4345, val1_obtained);
	ck_assert_int_eq(98798, val2_obtained);

	// set some more components and do a demo iteration
	whisker_ecs_c_set_component(c, comp1, sizeof(int), whisker_ecs_e_id(10), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp2, sizeof(int), whisker_ecs_e_id(10), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp3, sizeof(int), whisker_ecs_e_id(10), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp4, sizeof(int), whisker_ecs_e_id(10), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp5, sizeof(int), whisker_ecs_e_id(10), &(int){ 123 });

	whisker_ecs_c_set_component(c, comp1, sizeof(int), whisker_ecs_e_id(11), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp2, sizeof(int), whisker_ecs_e_id(11), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp3, sizeof(int), whisker_ecs_e_id(11), &(int){ 123 });
	/* whisker_ecs_c_set_component(c, comp4, sizeof(int), whisker_ecs_e_id(11), &(int){ 123 }); */
	whisker_ecs_c_set_component(c, comp5, sizeof(int), whisker_ecs_e_id(11), &(int){ 123 });

	whisker_ecs_c_set_component(c, comp1, sizeof(int), whisker_ecs_e_id(15), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp2, sizeof(int), whisker_ecs_e_id(15), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp3, sizeof(int), whisker_ecs_e_id(15), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp4, sizeof(int), whisker_ecs_e_id(15), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp5, sizeof(int), whisker_ecs_e_id(15), &(int){ 123 });

	whisker_ecs_c_set_component(c, comp1, sizeof(int), whisker_ecs_e_id(16), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp2, sizeof(int), whisker_ecs_e_id(16), &(int){ 123 });
	/* whisker_ecs_c_set_component(c, comp3, sizeof(int), whisker_ecs_e_id(16), &(int){ 123 }); */
	/* whisker_ecs_c_set_component(c, comp4, sizeof(int), whisker_ecs_e_id(16), &(int){ 123 }); */
	/* whisker_ecs_c_set_component(c, comp5, sizeof(int), whisker_ecs_e_id(16), &(int){ 123 }); */

	whisker_ecs_c_set_component(c, comp1, sizeof(int), whisker_ecs_e_id(19), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp2, sizeof(int), whisker_ecs_e_id(19), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp3, sizeof(int), whisker_ecs_e_id(19), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp4, sizeof(int), whisker_ecs_e_id(19), &(int){ 123 });
	whisker_ecs_c_set_component(c, comp5, sizeof(int), whisker_ecs_e_id(19), &(int){ 123 });

	whisker_ecs_c_set_component(c, comp5, sizeof(int), whisker_ecs_e_id(20), &(int){ 123 });

	// get the iterator again
	itor = whisker_ecs_s_get_iterator(sys, 0, "comp1,comp2,comp3", "comp4,comp5");

	whisker_ecs_entity_id expected_entities[] = {10,15,19};
	while (whisker_ecs_s_iterate(sys, itor)) 
	{
		printf("itor test: entity %zu\n", itor->entity_id);
		ck_assert_uint_eq(expected_entities[itor->cursor].index, itor->entity_id.index);
	}
	printf("itor test: iteration ended\n");

	// test iterator reset
	itor = whisker_ecs_s_get_iterator(sys, 0, "comp1,comp2,comp3", "comp4,comp5");

	while (whisker_ecs_s_iterate(sys, itor)) 
	{
		ck_assert_uint_eq(expected_entities[itor->cursor].index, itor->entity_id.index);
	}


	whisker_ecs_e_free_entities(e);
	whisker_ecs_c_free_components(c);
	whisker_ecs_s_free_systems(s);
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
