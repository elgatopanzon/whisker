/**
 * @author      : ElGatoPanzon
 * @file        : test_whisker_query_not
 * @created     : Tuesday Mar 24, 2026 17:52:08 CST
 * @description : tests for NOT filter in query system
 */

#include "whisker_std.h"

#include "whisker_query_iterator.h"
#include "whisker_query_registry.h"
#include "whisker_component_registry.h"
#include "whisker_entity_registry.h"
#include "whisker_string_table.h"
#include "whisker_arena.h"
#include "whisker_hash_xxhash64.h"
#include "whisker_ecs_world.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>


/*****************************
*  test component types      *
*****************************/

typedef struct {
	float x;
	float y;
} Position;

typedef struct {
	float vx;
	float vy;
} Velocity;

typedef struct {
	int health;
	int max_health;
} Health;

typedef struct {
	int is_poisoned;
} Poison;

typedef struct {
	int is_frozen;
} Frozen;


/*****************************
*  fixture                   *
*****************************/

static struct w_arena g_arena;
static struct w_string_table g_string_table;
static struct w_ecs_world g_world;

static void query_not_setup(void)
{
	w_arena_init(&g_arena, 256 * 1024);
	w_string_table_init(&g_string_table, &g_arena,
		WHISKER_STRING_TABLE_REALLOC_SIZE,
		WHISKER_STRING_TABLE_BUCKETS_SIZE,
		w_xxhash64_hash);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);
}

static void query_not_teardown(void)
{
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}

// helpers
static void set_position(w_entity_id entity, float x, float y)
{
	Position p = {x, y};
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_float,
		w_ecs_get_component_by_name(&g_world, "position"), entity, &p, sizeof(Position));
}

static void set_velocity(w_entity_id entity, float vx, float vy)
{
	Velocity v = {vx, vy};
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_float,
		w_ecs_get_component_by_name(&g_world, "velocity"), entity, &v, sizeof(Velocity));
}

static void set_health(w_entity_id entity, int health, int max_health)
{
	Health h = {health, max_health};
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_int,
		w_ecs_get_component_by_name(&g_world, "health"), entity, &h, sizeof(Health));
}

static void set_poison(w_entity_id entity, int is_poisoned)
{
	Poison p = {is_poisoned};
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_int,
		w_ecs_get_component_by_name(&g_world, "poison"), entity, &p, sizeof(Poison));
}

static void set_frozen(w_entity_id entity, int is_frozen)
{
	Frozen f = {is_frozen};
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_int,
		w_ecs_get_component_by_name(&g_world, "frozen"), entity, &f, sizeof(Frozen));
}


/*****************************
*  basic NOT exclusion       *
*****************************/

START_TEST(test_not_excludes_entities_with_component)
{
	// 5 entities with position, 2 also have poison
	for (int i = 0; i < 5; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		if (i == 1 || i == 3)
			set_poison(e, 1);
	}

	struct w_query *q = w_ecs_get_query(&g_world, "read position, not poison");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	float sum_x = 0;
	w_query_for_each(&g_world, "read position, not poison", {
		Position *pos = w_itor_get(Position);
		sum_x += pos->x;
		count++;
	});

	// entities 0, 2, 4 remain (1, 3 excluded)
	ck_assert_int_eq(count, 3);
	ck_assert_float_eq(sum_x, 0 + 2 + 4);
}
END_TEST

START_TEST(test_not_excludes_all_when_all_have_component)
{
	// all entities have both position and poison
	for (int i = 0; i < 4; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		set_poison(e, 1);
	}

	struct w_query *q = w_ecs_get_query(&g_world, "read position, not poison");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	w_query_for_each(&g_world, "read position, not poison", {
		Position *pos = w_itor_get(Position);
		(void)pos;
		count++;
	});

	ck_assert_int_eq(count, 0);
}
END_TEST

START_TEST(test_not_excludes_none_when_none_have_component)
{
	// no entities have poison, only position
	for (int i = 0; i < 4; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
	}

	// register poison so query can resolve it
	w_ecs_get_component_by_name(&g_world, "poison");

	struct w_query *q = w_ecs_get_query(&g_world, "read position, not poison");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	float sum_x = 0;
	w_query_for_each(&g_world, "read position, not poison", {
		Position *pos = w_itor_get(Position);
		sum_x += pos->x;
		count++;
	});

	// all 4 remain
	ck_assert_int_eq(count, 4);
	ck_assert_float_eq(sum_x, 0 + 1 + 2 + 3);
}
END_TEST


/*****************************
*  NOT combined with write   *
*****************************/

START_TEST(test_not_with_write_access)
{
	w_entity_id entities[4];
	for (int i = 0; i < 4; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);
		set_position(entities[i], (float)i, 0);
		if (i == 0 || i == 2)
			set_poison(entities[i], 1);
	}

	struct w_query *q = w_ecs_get_query(&g_world, "write position, not poison");
	w_query_rebuild_cache(&g_world.queries, q);

	w_query_for_each(&g_world, "write position, not poison", {
		Position *pos = w_itor_get(Position);
		pos->x *= 10.0f;
	});

	// entities 1, 3 should be modified (not poisoned)
	Position *p1 = w_ecs_get_component_(&g_world,
		w_ecs_get_component_by_name(&g_world, "position"), entities[1]);
	ck_assert_float_eq(p1->x, 10.0f);

	Position *p3 = w_ecs_get_component_(&g_world,
		w_ecs_get_component_by_name(&g_world, "position"), entities[3]);
	ck_assert_float_eq(p3->x, 30.0f);

	// entities 0, 2 should be unchanged (poisoned, excluded)
	Position *p0 = w_ecs_get_component_(&g_world,
		w_ecs_get_component_by_name(&g_world, "position"), entities[0]);
	ck_assert_float_eq(p0->x, 0.0f);

	Position *p2 = w_ecs_get_component_(&g_world,
		w_ecs_get_component_by_name(&g_world, "position"), entities[2]);
	ck_assert_float_eq(p2->x, 2.0f);
}
END_TEST


/*****************************
*  NOT combined with optional*
*****************************/

START_TEST(test_not_with_optional)
{
	// entities with position, some with velocity (optional), some with poison (excluded)
	for (int i = 0; i < 6; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		if (i % 2 == 0)
			set_velocity(e, (float)(i * 10), 0);
		if (i == 2 || i == 5)
			set_poison(e, 1);
	}

	struct w_query *q = w_ecs_get_query(&g_world, "read position, optional velocity, not poison");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	int with_vel = 0;
	int without_vel = 0;
	w_query_for_each(&g_world, "read position, optional velocity, not poison", {
		Position *pos = w_itor_get(Position);
		(void)pos;
		Velocity *vel = w_itor_get_optional(Velocity);
		if (vel)
			with_vel++;
		else
			without_vel++;
		count++;
	});

	// entities: 0(pos,vel), 1(pos), 2(pos,vel,poison-EXCLUDED), 3(pos), 4(pos,vel), 5(pos,poison-EXCLUDED)
	// remaining: 0, 1, 3, 4
	ck_assert_int_eq(count, 4);
	// with velocity: 0, 4
	ck_assert_int_eq(with_vel, 2);
	// without velocity: 1, 3
	ck_assert_int_eq(without_vel, 2);
}
END_TEST


/*****************************
*  multiple NOT terms        *
*****************************/

START_TEST(test_multiple_not_terms)
{
	// entities with position, some with poison, some with frozen, some with both
	for (int i = 0; i < 6; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		if (i == 1 || i == 4)
			set_poison(e, 1);
		if (i == 2 || i == 4)
			set_frozen(e, 1);
	}

	struct w_query *q = w_ecs_get_query(&g_world, "read position, not poison, not frozen");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	float sum_x = 0;
	w_query_for_each(&g_world, "read position, not poison, not frozen", {
		Position *pos = w_itor_get(Position);
		sum_x += pos->x;
		count++;
	});

	// entity 0: no poison, no frozen -> included
	// entity 1: poison -> excluded
	// entity 2: frozen -> excluded
	// entity 3: no poison, no frozen -> included
	// entity 4: poison + frozen -> excluded
	// entity 5: no poison, no frozen -> included
	ck_assert_int_eq(count, 3);
	ck_assert_float_eq(sum_x, 0 + 3 + 5);
}
END_TEST

START_TEST(test_multiple_not_with_multiple_read)
{
	// 2 required components + 2 NOT filters
	for (int i = 0; i < 5; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		set_velocity(e, (float)(i * 10), 0);
		if (i == 0 || i == 3)
			set_poison(e, 1);
		if (i == 2)
			set_frozen(e, 1);
	}

	struct w_query *q = w_ecs_get_query(&g_world, "read position, read velocity, not poison, not frozen");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	float sum_x = 0;
	float sum_vx = 0;
	w_query_for_each(&g_world, "read position, read velocity, not poison, not frozen", {
		Position *pos = w_itor_get(Position);
		Velocity *vel = w_itor_get(Velocity);
		sum_x += pos->x;
		sum_vx += vel->vx;
		count++;
	});

	// entity 0: poison -> excluded
	// entity 1: clean -> included
	// entity 2: frozen -> excluded
	// entity 3: poison -> excluded
	// entity 4: clean -> included
	ck_assert_int_eq(count, 2);
	ck_assert_float_eq(sum_x, 1 + 4);
	ck_assert_float_eq(sum_vx, 10 + 40);
}
END_TEST


/*****************************
*  edge cases                *
*****************************/

START_TEST(test_not_on_empty_result)
{
	// no entities at all, just register components
	w_ecs_get_component_by_name(&g_world, "position");
	w_ecs_get_component_by_name(&g_world, "poison");

	struct w_query *q = w_ecs_get_query(&g_world, "read position, not poison");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	w_query_for_each(&g_world, "read position, not poison", {
		Position *pos = w_itor_get(Position);
		(void)pos;
		count++;
	});

	ck_assert_int_eq(count, 0);
}
END_TEST

START_TEST(test_not_single_entity_excluded)
{
	// only one entity, and it has the excluded component
	w_entity_id e = w_ecs_request_entity(&g_world);
	set_position(e, 42.0f, 0);
	set_poison(e, 1);

	struct w_query *q = w_ecs_get_query(&g_world, "read position, not poison");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	w_query_for_each(&g_world, "read position, not poison", {
		Position *pos = w_itor_get(Position);
		(void)pos;
		count++;
	});

	ck_assert_int_eq(count, 0);
}
END_TEST

START_TEST(test_not_single_entity_included)
{
	// only one entity, and it does NOT have the excluded component
	w_entity_id e = w_ecs_request_entity(&g_world);
	set_position(e, 42.0f, 0);

	// register poison so query resolves
	w_ecs_get_component_by_name(&g_world, "poison");

	struct w_query *q = w_ecs_get_query(&g_world, "read position, not poison");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	float sum_x = 0;
	w_query_for_each(&g_world, "read position, not poison", {
		Position *pos = w_itor_get(Position);
		sum_x += pos->x;
		count++;
	});

	ck_assert_int_eq(count, 1);
	ck_assert_float_eq(sum_x, 42.0f);
}
END_TEST

START_TEST(test_not_with_trailing_comma)
{
	// trailing comma from macro-generated queries
	for (int i = 0; i < 4; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		if (i == 1)
			set_poison(e, 1);
	}

	struct w_query *q = w_ecs_get_query(&g_world, "read position, not poison, ");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	w_query_for_each(&g_world, "read position, not poison, ", {
		Position *pos = w_itor_get(Position);
		(void)pos;
		count++;
	});

	// entity 1 excluded
	ck_assert_int_eq(count, 3);
}
END_TEST

START_TEST(test_not_with_query_macro)
{
	// test w_query_not macro produces correct strings
	for (int i = 0; i < 4; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		if (i == 2)
			set_poison(e, 1);
	}

	char *query_str = w_query_read("position") w_query_not("poison");
	struct w_query *q = w_ecs_get_query(&g_world, query_str);
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	w_query_for_each(&g_world, w_query_read("position") w_query_not("poison"), {
		Position *pos = w_itor_get(Position);
		(void)pos;
		count++;
	});

	// entity 2 excluded
	ck_assert_int_eq(count, 3);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_query_not_suite(void)
{
	Suite *s = suite_create("whisker_query_not");

	TCase *tc_basic = tcase_create("basic_not");
	tcase_add_checked_fixture(tc_basic, query_not_setup, query_not_teardown);
	tcase_set_timeout(tc_basic, 10);
	tcase_add_test(tc_basic, test_not_excludes_entities_with_component);
	tcase_add_test(tc_basic, test_not_excludes_all_when_all_have_component);
	tcase_add_test(tc_basic, test_not_excludes_none_when_none_have_component);
	suite_add_tcase(s, tc_basic);

	TCase *tc_write = tcase_create("not_with_write");
	tcase_add_checked_fixture(tc_write, query_not_setup, query_not_teardown);
	tcase_set_timeout(tc_write, 10);
	tcase_add_test(tc_write, test_not_with_write_access);
	suite_add_tcase(s, tc_write);

	TCase *tc_optional = tcase_create("not_with_optional");
	tcase_add_checked_fixture(tc_optional, query_not_setup, query_not_teardown);
	tcase_set_timeout(tc_optional, 10);
	tcase_add_test(tc_optional, test_not_with_optional);
	suite_add_tcase(s, tc_optional);

	TCase *tc_multi = tcase_create("multiple_not");
	tcase_add_checked_fixture(tc_multi, query_not_setup, query_not_teardown);
	tcase_set_timeout(tc_multi, 10);
	tcase_add_test(tc_multi, test_multiple_not_terms);
	tcase_add_test(tc_multi, test_multiple_not_with_multiple_read);
	suite_add_tcase(s, tc_multi);

	TCase *tc_edge = tcase_create("edge_cases");
	tcase_add_checked_fixture(tc_edge, query_not_setup, query_not_teardown);
	tcase_set_timeout(tc_edge, 10);
	tcase_add_test(tc_edge, test_not_on_empty_result);
	tcase_add_test(tc_edge, test_not_single_entity_excluded);
	tcase_add_test(tc_edge, test_not_single_entity_included);
	tcase_add_test(tc_edge, test_not_with_trailing_comma);
	tcase_add_test(tc_edge, test_not_with_query_macro);
	suite_add_tcase(s, tc_edge);

	return s;
}

// test runner
int main(void)
{
	Suite *s = whisker_query_not_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
