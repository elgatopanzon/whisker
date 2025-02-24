/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_ecs_archetype
 * @created     : Tuesday Feb 18, 2025 10:51:59 CST
 */

#include "whisker_std.h"
#include "whisker_array.h"

#include "whisker_ecs_entity.h"
#include "whisker_ecs_archetype.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>

START_TEST(test_whisker_ecs_archetype_set_remove_and_match)
{
	// create some empty archetypes
	whisker_ecs_entity_id *a1; warr_create(whisker_ecs_entity_id, 0, &a1);
	whisker_ecs_entity_id *a2; warr_create(whisker_ecs_entity_id, 0, &a2);
	whisker_ecs_entity_id *a3; warr_create(whisker_ecs_entity_id, 0, &a3);

	// set some archetype IDs
	whisker_ecs_a_set(&a1, whisker_ecs_e_id(0));
	whisker_ecs_a_set(&a1, whisker_ecs_e_id(1));
	whisker_ecs_a_set(&a1, whisker_ecs_e_id(2));

	whisker_ecs_a_set(&a2, whisker_ecs_e_id(1));
	whisker_ecs_a_set(&a2, whisker_ecs_e_id(3));
	whisker_ecs_a_set(&a2, whisker_ecs_e_id(4));

	whisker_ecs_a_set(&a3, whisker_ecs_e_id(1));
	whisker_ecs_a_set(&a3, whisker_ecs_e_id(1));
	whisker_ecs_a_set(&a3, whisker_ecs_e_id(3));

	// create a fake system archetype to match against
	whisker_ecs_entity_id *s1; warr_create(whisker_ecs_entity_id, 0, &s1);
	whisker_ecs_a_set(&s1, whisker_ecs_e_id(1));
	whisker_ecs_a_set(&s1, whisker_ecs_e_id(3));

	// validate the matches
	ck_assert_int_eq(false, whisker_ecs_a_match(s1, a1));
	ck_assert_int_eq(true, whisker_ecs_a_match(s1, a2));
	ck_assert_int_eq(true, whisker_ecs_a_match(s1, a3));

	// free arrays
	warr_free(a1);
	warr_free(a2);
	warr_free(a3);
	warr_free(s1);
}
END_TEST

START_TEST(test_whisker_ecs_archetype_named_entities_to_archetype)
{
	// create entities list
	whisker_ecs_entities *en;
	whisker_ecs_e_create_entities(&en);

	// create archetype from named entities
	whisker_ecs_entity_id *a1 = whisker_ecs_a_from_named_entities(en, "test1,test2");
	ck_assert_int_eq(2, warr_length(a1));

	// verify the entity IDs created
	int expected[] = {0, 1};
	for (int i = 0; i < 2; ++i)
	{
		ck_assert_uint_eq(expected[i], a1[i].index);
	}

	// create the archetype from same named entities
	whisker_ecs_entity_id *a2 = whisker_ecs_a_from_named_entities(en, "test1,test2");
	ck_assert_int_eq(2, warr_length(a2));

	// verify the entity ids are the same
	for (int i = 0; i < 2; ++i)
	{
		ck_assert_uint_eq(expected[i], a2[i].index);
	}

	// create some new named entities
	whisker_ecs_entity_id e3;
	whisker_ecs_e_create_named_(en, "test3", &e3);
	whisker_ecs_entity_id e4;
	whisker_ecs_e_create_named_(en, "test4", &e4);

	// create new archetype from the named entities
	whisker_ecs_entity_id *a3 = whisker_ecs_a_from_named_entities(en, "test3,test4,test1");
	ck_assert_int_eq(3, warr_length(a3));

	// verify the entity IDs created
	int expected_2[] = {0, 2, 3};
	for (int i = 0; i < 3; ++i)
	{
		ck_assert_uint_eq(expected_2[i], a3[i].index);
	}

	// free
	whisker_ecs_e_free_entities(en);
	warr_free(a1);
	warr_free(a2);
	warr_free(a3);
}
END_TEST

Suite* whisker_ecs_archetype_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("whisker_ecs_archetype");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_set_timeout(tc_core, 10);

	tcase_add_test(tc_core, test_whisker_ecs_archetype_set_remove_and_match);
	tcase_add_test(tc_core, test_whisker_ecs_archetype_named_entities_to_archetype);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = whisker_ecs_archetype_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
