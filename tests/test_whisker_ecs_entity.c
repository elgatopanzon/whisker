/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_ecs_entity
 * @created     : Thursday Feb 13, 2025 18:20:21 CST
 */

#include "whisker_std.h"

#include <stdio.h>
#include <stdlib.h>

#include "check.h"

#include "whisker_array.h"
#include "whisker_ecs.h"

START_TEST(test_whisker_ecs_entity_create_entities_struct)
{
	whisker_ecs_entities *entities = whisker_ecs_create_and_init_entities_container_();

	// verify empty arrays
	// note: starts with 1, since it contains entity 0
	ck_assert_int_eq(0, entities->entities_length);
	ck_assert_int_eq(0, entities->destroyed_entities_length);
	ck_assert_ptr_ne(NULL, entities->entity_names);

	// free
	whisker_ecs_free_entities_all_(entities);
}
END_TEST

START_TEST(test_whisker_ecs_entity_create_destroy_and_recycle)
{
	// create entities instance
	whisker_ecs_entities *entities = whisker_ecs_create_and_init_entities_container_();

	// create some entities
	whisker_ecs_entity_id e1 = whisker_ecs_entity_api_create_unsafe_(entities);
	whisker_ecs_entity_id e2 = whisker_ecs_entity_api_create_unsafe_(entities);
	whisker_ecs_entity_id e3 = whisker_ecs_entity_api_create_unsafe_(entities);

	// validate entity count
	// note: since we added 3 and 0 already exists, the length is 4
	ck_assert_uint_eq(3, entities->entities_length);
	ck_assert_uint_eq(3, whisker_ecs_alive_entity_count(entities));
	
	// validate returned indexes
	ck_assert_uint_eq(0, e1.index);
	ck_assert_uint_eq(1, e2.index);
	ck_assert_uint_eq(2, e3.index);
	ck_assert_uint_eq(0, e1.version);
	ck_assert_uint_eq(0, e2.version);
	ck_assert_uint_eq(0, e3.version);

	// validate indexes with obtained entity pointers
	ck_assert_uint_eq(0, whisker_ecs_get_entity(entities, e1)->id.index);
	ck_assert_uint_eq(1, whisker_ecs_get_entity(entities, e2)->id.index);
	ck_assert_uint_eq(2, whisker_ecs_get_entity(entities, e3)->id.index);

	// make sure entities are alive
	ck_assert_int_eq(true, whisker_ecs_is_entity_alive(entities, e1));
	ck_assert_int_eq(true, whisker_ecs_is_entity_alive(entities, e2));
	ck_assert_int_eq(true, whisker_ecs_is_entity_alive(entities, e3));

	// destroy e1 and e3 and check they are dead
	whisker_ecs_destroy_entity_(entities, e1);
	whisker_ecs_destroy_entity_(entities, e3);
	ck_assert_int_eq(false, whisker_ecs_is_entity_alive(entities, e1));
	ck_assert_int_eq(false, whisker_ecs_is_entity_alive(entities, e3));

	// verify the versions
	ck_assert_uint_eq(0, e1.version);
	ck_assert_uint_eq(1, whisker_ecs_get_entity(entities, e1)->id.version);
	ck_assert_uint_eq(0, e3.version);
	ck_assert_uint_eq(1, whisker_ecs_get_entity(entities, e3)->id.version);

	// create a new entity (it should recycle 3 with version 1 first)
	whisker_ecs_entity_id e4 = whisker_ecs_entity_api_create_unsafe_(entities);
	ck_assert_uint_eq(2, e4.index);
	ck_assert_uint_eq(1, e4.version);

	// free entities
	whisker_ecs_free_entities_all_(entities);
}
END_TEST

START_TEST(test_whisker_ecs_create_and_set_entity_name)
{
	whisker_ecs_entities *entities = whisker_ecs_create_and_init_entities_container_();

	// create some named entities
	whisker_ecs_entity_id e1 = whisker_ecs_entity_api_create_named_(entities, "e1");
	whisker_ecs_entity_id e2 = whisker_ecs_entity_api_create_named_(entities, "e2");
	whisker_ecs_entity_id e3 = whisker_ecs_entity_api_create_named_(entities, "e3");

	// get entity struct by name
	whisker_ecs_entity *e1_fetched = whisker_ecs_get_named_entity(entities, "e1");
	whisker_ecs_entity *e2_fetched = whisker_ecs_get_named_entity(entities, "e2");
	whisker_ecs_entity *e3_fetched = whisker_ecs_get_named_entity(entities, "e3");

	// validate returned indexes
	ck_assert_uint_eq(0, e1_fetched->id.index);
	ck_assert_uint_eq(1, e2_fetched->id.index);
	ck_assert_uint_eq(2, e3_fetched->id.index);

	// validate returned names
	ck_assert_str_eq("e1", e1_fetched->name);
	ck_assert_str_eq("e2", e2_fetched->name);
	ck_assert_str_eq("e3", e3_fetched->name);

	// create an entity with the same name, to get the existing entity
	whisker_ecs_entity_id e4 = whisker_ecs_entity_api_create_named_(entities, "e3");
	ck_assert_uint_eq(2, e4.index);

	// destroy an entity, validate the key no longer works
	whisker_ecs_entity *e4_fetched = whisker_ecs_get_named_entity(entities, "e3");
	whisker_ecs_destroy_entity_(entities, e4);
	ck_assert(e4_fetched->name == NULL);
	ck_assert(whisker_ecs_get_named_entity(entities, "e3") == NULL);

	// free
	whisker_ecs_free_entities_all_(entities);
}
END_TEST

START_TEST(test_whisker_ecs_entity_named_entities_to_id)
{
	// create entities list
	whisker_ecs_entities *en = whisker_ecs_create_and_init_entities_container_();

	// create archetype from named entities
	struct whisker_ecs_entity_id_array *a1 = whisker_ecs_array_from_named_entities(en, "test1,test2");
	ck_assert_int_eq(2, a1->arr_length);

	// verify the entity IDs created
	int expected[] = {0, 1};
	for (int i = 0; i < 2; ++i)
	{
		ck_assert_uint_eq(expected[i], a1->arr[i].index);
	}

	// create the archetype from same named entities
	struct whisker_ecs_entity_id_array *a2 = whisker_ecs_array_from_named_entities(en, "test1,test2");
	ck_assert_int_eq(2, a2->arr_length);

	// verify the entity ids are the same
	for (int i = 0; i < 2; ++i)
	{
		ck_assert_uint_eq(expected[i], a2->arr[i].index);
	}

	// create some new named entities
	whisker_ecs_entity_id e3 = whisker_ecs_entity_api_create_named_(en, "test3");
	whisker_ecs_entity_id e4 = whisker_ecs_entity_api_create_named_(en, "test4");

	// create new archetype from the named entities
	struct whisker_ecs_entity_id_array *a3 = whisker_ecs_array_from_named_entities(en, "test3,test4,test1");
	ck_assert_int_eq(3, a3->arr_length);

	// verify the entity IDs created
	int expected_2[] = {2, 3, 0};
	for (int i = 0; i < 3; ++i)
	{
		ck_assert_uint_eq(expected_2[i], a3->arr[i].index);
	}

	// free
	whisker_ecs_free_entities_all_(en);
	free(a1->arr);
	free(a2->arr);
	free(a3->arr);
	free(a1);
	free(a2);
	free(a3);
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
	tcase_add_test(tc_core, test_whisker_ecs_create_and_set_entity_name);
	tcase_add_test(tc_core, test_whisker_ecs_entity_named_entities_to_id);

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
