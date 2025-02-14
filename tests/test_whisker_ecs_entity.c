/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_ecs_entity
 * @created     : Thursday Feb 13, 2025 18:20:21 CST
 */

#include "whisker_std.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>

#include "whisker_array.h"
#include "whisker_ecs.h"

START_TEST(test_whisker_ecs_entity_create_entities_struct)
{
	whisker_ecs_entities *entities;
	whisker_ecs_e_create_entities(&entities);

	// verify empty arrays
	ck_assert_int_eq(0, warr_length(entities->entities));
	ck_assert_int_eq(0, warr_length(entities->dead_entities));
	ck_assert_int_eq(0, warr_length(entities->entity_keys));

	// free
	whisker_ecs_e_free_entities(entities);
}
END_TEST

START_TEST(test_whisker_ecs_entity_create_destroy_and_recycle)
{
	// create entities instance
	whisker_ecs_entities *entities;
	whisker_ecs_e_create_entities(&entities);

	// create some entities
	wecs_e_id e1; whisker_ecs_e_create(entities, &e1);
	wecs_e_id e2; whisker_ecs_e_create(entities, &e2);
	wecs_e_id e3; whisker_ecs_e_create(entities, &e3);

	// validate entity count
	ck_assert_uint_eq(3, warr_length(entities->entities));
	ck_assert_uint_eq(3, wecs_e_count(entities));
	
	// validate returned indexes
	ck_assert_uint_eq(0, e1.index);
	ck_assert_uint_eq(1, e2.index);
	ck_assert_uint_eq(2, e3.index);

	// validate indexes with obtained entity pointers
	ck_assert_uint_eq(0, whisker_ecs_e(entities, e1)->id.index);
	ck_assert_uint_eq(1, whisker_ecs_e(entities, e2)->id.index);
	ck_assert_uint_eq(2, whisker_ecs_e(entities, e3)->id.index);

	// make sure entities are alive
	ck_assert_int_eq(true, wecs_e_is_alive(entities, e1));
	ck_assert_int_eq(true, wecs_e_is_alive(entities, e2));
	ck_assert_int_eq(true, wecs_e_is_alive(entities, e3));

	// destroy e1 and e3 and check they are dead
	whisker_ecs_e_destroy(entities, e1);
	whisker_ecs_e_destroy(entities, e3);
	ck_assert_int_eq(false, wecs_e_is_alive(entities, e1));
	ck_assert_int_eq(false, wecs_e_is_alive(entities, e3));

	// verify the versions
	ck_assert_uint_eq(0, e1.version);
	ck_assert_uint_eq(1, whisker_ecs_e(entities, e1)->id.version);
	ck_assert_uint_eq(0, e3.version);
	ck_assert_uint_eq(1, whisker_ecs_e(entities, e3)->id.version);

	// create a new entity (it should recycle 0 with version 1 first)
	wecs_e_id e4; whisker_ecs_e_create(entities, &e4);
	ck_assert_uint_eq(0, e4.index);
	ck_assert_uint_eq(1, e4.version);

	// free entities
	whisker_ecs_e_free_entities(entities);
}
END_TEST

Suite* whisker_ecs_entity_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("whisker_ecs_entity");

	/* Core test case */
	tc_core = tcase_create("Core");
	tcase_set_timeout(tc_core, 10);

	tcase_add_test(tc_core, test_whisker_ecs_entity_create_entities_struct);
	tcase_add_test(tc_core, test_whisker_ecs_entity_create_destroy_and_recycle);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = whisker_ecs_entity_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
