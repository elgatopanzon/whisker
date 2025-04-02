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
	struct w_entities *entities = w_create_and_init_entities_container_();

	// verify empty arrays
	// note: starts with 1, since it contains entity 0
	ck_assert_int_eq(0, entities->entities_length);
	ck_assert_int_eq(0, entities->destroyed_entities_length);
	ck_assert_ptr_ne(NULL, entities->entity_names);

	// free
	w_free_entities_all_(entities);
}
END_TEST

START_TEST(test_whisker_ecs_entity_create_destroy_and_recycle)
{
	// create entities instance
	struct w_entities *entities = w_create_and_init_entities_container_();
	struct w_world world = {.entities = entities};

	// create some entities
	w_entity_id e1 = w_entity_api_create_unsafe_(&world);
	w_entity_id e2 = w_entity_api_create_unsafe_(&world);
	w_entity_id e3 = w_entity_api_create_unsafe_(&world);

	// validate entity count
	// note: since we added 3 and 0 already exists, the length is 4
	ck_assert_uint_eq(3, entities->entities_length);
	ck_assert_uint_eq(3, w_alive_entity_count(&world));
	
	// validate returned indexes
	ck_assert_uint_eq(0, e1.index);
	ck_assert_uint_eq(1, e2.index);
	ck_assert_uint_eq(2, e3.index);
	ck_assert_uint_eq(0, e1.version);
	ck_assert_uint_eq(0, e2.version);
	ck_assert_uint_eq(0, e3.version);

	// validate indexes with obtained entity pointers
	ck_assert_uint_eq(0, w_get_entity(&world, e1)->id.index);
	ck_assert_uint_eq(1, w_get_entity(&world, e2)->id.index);
	ck_assert_uint_eq(2, w_get_entity(&world, e3)->id.index);

	// make sure entities are alive
	ck_assert_int_eq(true, w_is_entity_alive(&world, e1));
	ck_assert_int_eq(true, w_is_entity_alive(&world, e2));
	ck_assert_int_eq(true, w_is_entity_alive(&world, e3));

	// destroy e1 and e3 and check they are dead
	w_destroy_entity_non_deferred(&world, e1);
	w_destroy_entity_non_deferred(&world, e3);
	ck_assert_int_eq(false, w_is_entity_alive(&world, e1));
	ck_assert_int_eq(false, w_is_entity_alive(&world, e3));

	// verify the versions
	ck_assert_uint_eq(0, e1.version);
	ck_assert_uint_eq(1, w_get_entity(&world, e1)->id.version);
	ck_assert_uint_eq(0, e3.version);
	ck_assert_uint_eq(1, w_get_entity(&world, e3)->id.version);

	// create a new entity (it should recycle 3 with version 1 first)
	w_entity_id e4 = w_entity_api_create_unsafe_(&world);
	ck_assert_uint_eq(2, e4.index);
	ck_assert_uint_eq(1, e4.version);

	// free entities
	w_free_entities_all_(entities);
}
END_TEST

START_TEST(test_whisker_ecs_create_and_set_entity_name)
{
	struct w_entities *entities = w_create_and_init_entities_container_();
	struct w_world world = {.entities = entities};

	// create some named entities
	w_entity_id e1 = w_entity_api_create_named_(&world, "e1");
	w_entity_id e2 = w_entity_api_create_named_(&world, "e2");
	w_entity_id e3 = w_entity_api_create_named_(&world, "e3");

	// get entity struct by name
	struct w_entity *e1_fetched = w_get_named_entity(&world, "e1");
	struct w_entity *e2_fetched = w_get_named_entity(&world, "e2");
	struct w_entity *e3_fetched = w_get_named_entity(&world, "e3");

	// validate returned indexes
	ck_assert_uint_eq(0, e1_fetched->id.index);
	ck_assert_uint_eq(1, e2_fetched->id.index);
	ck_assert_uint_eq(2, e3_fetched->id.index);

	// validate returned names
	ck_assert_str_eq("e1", e1_fetched->name);
	ck_assert_str_eq("e2", e2_fetched->name);
	ck_assert_str_eq("e3", e3_fetched->name);

	// create an entity with the same name, to get the existing entity
	w_entity_id e4 = w_entity_api_create_named_(&world, "e3");
	ck_assert_uint_eq(2, e4.index);

	// destroy an entity, validate the key no longer works
	struct w_entity *e4_fetched = w_get_named_entity(&world, "e3");
	w_destroy_entity_non_deferred(&world, e4);
	ck_assert(e4_fetched->name == NULL);
	ck_assert(w_get_named_entity(&world, "e3") == NULL);

	// free
	w_free_entities_all_(entities);
}
END_TEST

START_TEST(test_whisker_ecs_entity_named_entities_to_id)
{
	// create entities list
	struct w_entities *en = w_create_and_init_entities_container_();
	struct w_world world = {.entities = en};

	// create archetype from named entities
	struct w_entity_id_arr *a1 = w_batch_create_named_entities(&world, "test1,test2");
	ck_assert_int_eq(2, a1->arr_length);

	// verify the entity IDs created
	int expected[] = {0, 1};
	for (int i = 0; i < 2; ++i)
	{
		ck_assert_uint_eq(expected[i], a1->arr[i].index);
	}

	// create the archetype from same named entities
	struct w_entity_id_arr *a2 = w_batch_create_named_entities(&world, "test1,test2");
	ck_assert_int_eq(2, a2->arr_length);

	// verify the entity ids are the same
	for (int i = 0; i < 2; ++i)
	{
		ck_assert_uint_eq(expected[i], a2->arr[i].index);
	}

	// create some new named entities
	w_entity_id e3 = w_entity_api_create_named_(&world, "test3");
	w_entity_id e4 = w_entity_api_create_named_(&world, "test4");

	// create new archetype from the named entities
	struct w_entity_id_arr *a3 = w_batch_create_named_entities(&world, "test3,test4,test1");
	ck_assert_int_eq(3, a3->arr_length);

	// verify the entity IDs created
	int expected_2[] = {2, 3, 0};
	for (int i = 0; i < 3; ++i)
	{
		ck_assert_uint_eq(expected_2[i], a3->arr[i].index);
	}

	// free
	w_free_entities_all_(en);
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
