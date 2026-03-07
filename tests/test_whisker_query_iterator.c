/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_query_iterator
 * @created     : Friday Mar 06, 2026 20:09:42 CST
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
	float scale;
} Scale;


/*****************************
*  fixture                   *
*****************************/

static struct w_arena g_arena;
static struct w_string_table g_string_table;
static struct w_ecs_world g_world;

static void query_iterator_setup(void)
{
	w_arena_init(&g_arena, 256 * 1024);
	w_string_table_init(&g_string_table, &g_arena,
		WHISKER_STRING_TABLE_REALLOC_SIZE,
		WHISKER_STRING_TABLE_BUCKETS_SIZE,
		w_xxhash64_hash);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);
}

static void query_iterator_teardown(void)
{
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}

// helper: set component on entity using world API
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

static void set_scale(w_entity_id entity, float scale)
{
	Scale s = {scale};
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_float,
		w_ecs_get_component_by_name(&g_world, "scale"), entity, &s, sizeof(Scale));
}


/*****************************
*  basic single component    *
*****************************/

START_TEST(test_single_component_iterates_all_entities)
{
	// create 5 entities with position
	for (int i = 0; i < 5; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, (float)(i * 10));
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	float sum_x = 0;
	w_query_for_each(&g_world, "read position", {
		Position *pos = w_itor_get(Position);
		sum_x += pos->x;
		count++;
	});

	ck_assert_int_eq(count, 5);
	ck_assert_float_eq(sum_x, 0 + 1 + 2 + 3 + 4);
}
END_TEST

START_TEST(test_single_component_write_modifies_data)
{
	// create 3 entities with position
	w_entity_id entities[3];
	for (int i = 0; i < 3; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);
		set_position(entities[i], 1.0f, 1.0f);
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "write position");
	w_query_rebuild_cache(&g_world.queries, q);

	// double all positions
	w_query_for_each(&g_world, "write position", {
		Position *pos = w_itor_get(Position);
		pos->x *= 2.0f;
		pos->y *= 2.0f;
	});

	// verify changes persisted
	for (int i = 0; i < 3; i++)
	{
		Position *pos = w_ecs_get_component_(&g_world,
			w_ecs_get_component_by_name(&g_world, "position"), entities[i]);
		ck_assert_float_eq(pos->x, 2.0f);
		ck_assert_float_eq(pos->y, 2.0f);
	}
}
END_TEST


/*****************************
*  multiple components       *
*****************************/

START_TEST(test_multiple_components_read)
{
	// create entities with both position and velocity
	for (int i = 0; i < 4; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		set_velocity(e, (float)(i * 2), 0);
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position, read velocity");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	float sum_x = 0;
	float sum_vx = 0;
	w_query_for_each(&g_world, "read position, read velocity", {
		Position *pos = w_itor_get(Position);
		Velocity *vel = w_itor_get(Velocity);
		sum_x += pos->x;
		sum_vx += vel->vx;
		count++;
	});

	ck_assert_int_eq(count, 4);
	ck_assert_float_eq(sum_x, 0 + 1 + 2 + 3);
	ck_assert_float_eq(sum_vx, 0 + 2 + 4 + 6);
}
END_TEST

START_TEST(test_multiple_components_mixed_access)
{
	// create entities with position (read) and velocity (write)
	w_entity_id entities[3];
	for (int i = 0; i < 3; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);
		set_position(entities[i], (float)i, 0);
		set_velocity(entities[i], 0, 0);
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position, write velocity");
	w_query_rebuild_cache(&g_world.queries, q);

	// use position to set velocity
	w_query_for_each(&g_world, "read position, write velocity", {
		Position *pos = w_itor_get(Position);
		Velocity *vel = w_itor_get(Velocity);
		vel->vx = pos->x * 10.0f;
	});

	// verify
	for (int i = 0; i < 3; i++)
	{
		Velocity *vel = w_ecs_get_component_(&g_world,
			w_ecs_get_component_by_name(&g_world, "velocity"), entities[i]);
		ck_assert_float_eq(vel->vx, (float)(i * 10));
	}
}
END_TEST

START_TEST(test_three_components)
{
	// create entities with position, velocity, and health
	for (int i = 0; i < 3; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		set_velocity(e, (float)i, 0);
		set_health(e, i * 10, 100);
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position, read velocity, read health");
	w_query_rebuild_cache(&g_world.queries, q);

	int total_health = 0;
	w_query_for_each(&g_world, "read position, read velocity, read health", {
		Position *pos = w_itor_get(Position);
		Velocity *vel = w_itor_get(Velocity);
		Health *h = w_itor_get(Health);
		(void)pos;
		(void)vel;
		total_health += h->health;
	});

	ck_assert_int_eq(total_health, 0 + 10 + 20);
}
END_TEST


/*****************************
*  optional absent           *
*****************************/

START_TEST(test_optional_returns_null_when_absent)
{
	// create entities: some with velocity, some without
	for (int i = 0; i < 5; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		// only even entities get velocity
		if (i % 2 == 0)
		{
			set_velocity(e, (float)i, 0);
		}
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position, optional velocity");
	w_query_rebuild_cache(&g_world.queries, q);

	int with_velocity = 0;
	int without_velocity = 0;
	w_query_for_each(&g_world, "read position, optional velocity", {
		Position *pos = w_itor_get(Position);
		(void)pos;
		Velocity *vel = w_itor_get_optional(Velocity);
		if (vel)
			with_velocity++;
		else
			without_velocity++;
	});

	// entities 0, 2, 4 have velocity
	ck_assert_int_eq(with_velocity, 3);
	// entities 1, 3 don't
	ck_assert_int_eq(without_velocity, 2);
}
END_TEST

START_TEST(test_optional_all_absent)
{
	// create entities with position only
	for (int i = 0; i < 4; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
	}

	// rebuild cache - note: optional still requires component registered
	// so we need to register velocity component
	w_ecs_get_component_by_name(&g_world, "velocity");

	struct w_query *q = w_ecs_get_query(&g_world, "read position, optional velocity");
	w_query_rebuild_cache(&g_world.queries, q);

	int null_count = 0;
	w_query_for_each(&g_world, "read position, optional velocity", {
		Position *pos = w_itor_get(Position);
		(void)pos;
		Velocity *vel = w_itor_get_optional(Velocity);
		if (!vel)
			null_count++;
	});

	ck_assert_int_eq(null_count, 4);
}
END_TEST


/*****************************
*  optional present          *
*****************************/

START_TEST(test_optional_returns_data_when_present)
{
	// create entities with both position and velocity
	for (int i = 0; i < 3; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		set_velocity(e, (float)(i * 100), 0);
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position, optional velocity");
	w_query_rebuild_cache(&g_world.queries, q);

	float sum_vx = 0;
	w_query_for_each(&g_world, "read position, optional velocity", {
		Position *pos = w_itor_get(Position);
		(void)pos;
		Velocity *vel = w_itor_get_optional(Velocity);
		if (vel)
			sum_vx += vel->vx;
	});

	ck_assert_float_eq(sum_vx, 0 + 100 + 200);
}
END_TEST

START_TEST(test_optional_can_write_when_present)
{
	// create entities with both position and velocity
	w_entity_id entities[2];
	for (int i = 0; i < 2; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);
		set_position(entities[i], (float)i, 0);
		set_velocity(entities[i], 1.0f, 1.0f);
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position, optional velocity");
	w_query_rebuild_cache(&g_world.queries, q);

	// modify velocity through optional
	w_query_for_each(&g_world, "read position, optional velocity", {
		Position *pos = w_itor_get(Position);
		Velocity *vel = w_itor_get_optional(Velocity);
		if (vel)
			vel->vx = pos->x * 50.0f;
	});

	// verify
	for (int i = 0; i < 2; i++)
	{
		Velocity *vel = w_ecs_get_component_(&g_world,
			w_ecs_get_component_by_name(&g_world, "velocity"), entities[i]);
		ck_assert_float_eq(vel->vx, (float)(i * 50));
	}
}
END_TEST


/*****************************
*  'has' terms skipping      *
*****************************/

START_TEST(test_has_term_skipped_by_iterator)
{
	// 'has' terms should be skipped when iterating
	// create entities with position and health, query with 'has health' at end
	for (int i = 0; i < 3; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		set_health(e, 100, 100);
	}

	// rebuild cache - 'has' parses to W_QUERY_ACCESS_NONE
	struct w_query *q = w_ecs_get_query(&g_world, "read position, has health");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	float sum_x = 0;
	w_query_for_each(&g_world, "read position, has health", {
		Position *pos = w_itor_get(Position);
		sum_x += pos->x;
		count++;
		// should NOT call w_itor_get for 'has' term - it has no accessor
	});

	ck_assert_int_eq(count, 3);
	ck_assert_float_eq(sum_x, 0 + 1 + 2);
}
END_TEST

START_TEST(test_has_term_in_middle_skipped)
{
	// 'has' term in middle of query should be skipped
	for (int i = 0; i < 3; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		set_health(e, 50, 100);
		set_velocity(e, (float)(i * 2), 0);
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position, has health, read velocity");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	float sum_vx = 0;
	w_query_for_each(&g_world, "read position, has health, read velocity", {
		Position *pos = w_itor_get(Position);
		(void)pos;
		// 'has health' is skipped by w_itor_get_optional logic
		// but w_itor_get doesn't skip, so we need to use optional for mid-has
		Velocity *vel = w_itor_get_optional(Velocity);
		if (vel)
			sum_vx += vel->vx;
		count++;
	});

	ck_assert_int_eq(count, 3);
	ck_assert_float_eq(sum_vx, 0 + 2 + 4);
}
END_TEST


/*****************************
*  dense slices              *
*****************************/

START_TEST(test_dense_slice_iteration)
{
	// pre-register component so it gets entity ID 0, then data entities start at 1
	w_ecs_get_component_by_name(&g_world, "position");

	// create 20 contiguous entities (>= MIN_SLICE of 16)
	w_entity_id entities[20];
	for (int i = 0; i < 20; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);
		set_position(entities[i], (float)i, (float)(i * 10));
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position");
	w_query_rebuild_cache(&g_world.queries, q);

	// verify we have dense slice (entities 1-20 are contiguous)
	ck_assert_int_eq(q->archetype_slices_dense_length, 1);
	ck_assert_int_eq(q->archetype_slices_sparse_length, 0);

	int count = 0;
	float sum_x = 0;
	w_query_for_each(&g_world, "read position", {
		Position *pos = w_itor_get(Position);
		sum_x += pos->x;
		count++;
	});

	ck_assert_int_eq(count, 20);
	// sum of 0..19 = 190
	ck_assert_float_eq(sum_x, 190.0f);
}
END_TEST

START_TEST(test_dense_slice_preserves_entity_id)
{
	// verify entity_id in iterator matches actual entity
	w_entity_id entities[16];
	for (int i = 0; i < 16; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);
		set_position(entities[i], (float)entities[i], 0);  // store entity ID as x
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position");
	w_query_rebuild_cache(&g_world.queries, q);

	int mismatches = 0;
	w_query_for_each(&g_world, "read position", {
		Position *pos = w_itor_get(Position);
		if ((w_entity_id)pos->x != itor.entity_id)
			mismatches++;
	});

	ck_assert_int_eq(mismatches, 0);
}
END_TEST


/*****************************
*  sparse slices             *
*****************************/

START_TEST(test_sparse_slice_iteration)
{
	// create non-contiguous entities (gap > 1)
	w_entity_id entities[4];
	for (int i = 0; i < 4; i++)
	{
		// request multiple to create gaps
		entities[i] = w_ecs_request_entity(&g_world);
		w_ecs_request_entity(&g_world);  // gap
		w_ecs_request_entity(&g_world);  // gap
		set_position(entities[i], (float)i, 0);
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position");
	w_query_rebuild_cache(&g_world.queries, q);

	// should have only sparse slices (4 isolated entities < MIN_SLICE)
	ck_assert_int_eq(q->archetype_slices_dense_length, 0);
	ck_assert_int_gt(q->archetype_slices_sparse_length, 0);

	int count = 0;
	float sum_x = 0;
	w_query_for_each(&g_world, "read position", {
		Position *pos = w_itor_get(Position);
		sum_x += pos->x;
		count++;
	});

	ck_assert_int_eq(count, 4);
	ck_assert_float_eq(sum_x, 0 + 1 + 2 + 3);
}
END_TEST

START_TEST(test_sparse_single_entities)
{
	// create widely spaced entities
	w_entity_id e1 = 0;
	w_entity_id e2 = 0;
	w_entity_id e3 = 0;

	// create gaps by requesting many entities
	for (int i = 0; i < 100; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		if (i == 0) { e1 = e; set_position(e1, 1.0f, 0); }
		if (i == 50) { e2 = e; set_position(e2, 2.0f, 0); }
		if (i == 99) { e3 = e; set_position(e3, 3.0f, 0); }
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	float sum_x = 0;
	w_query_for_each(&g_world, "read position", {
		Position *pos = w_itor_get(Position);
		sum_x += pos->x;
		count++;
	});

	ck_assert_int_eq(count, 3);
	ck_assert_float_eq(sum_x, 1.0f + 2.0f + 3.0f);
}
END_TEST


/*****************************
*  mixed dense/sparse        *
*****************************/

START_TEST(test_mixed_dense_sparse_iteration)
{
	// create dense run of 20, then sparse entities
	for (int i = 0; i < 20; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, 1.0f, 0);  // dense, all x=1
	}

	// create gap
	for (int i = 0; i < 50; i++)
	{
		w_ecs_request_entity(&g_world);
	}

	// create sparse entities
	for (int i = 0; i < 5; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, 10.0f, 0);  // sparse, all x=10
		// create gaps
		w_ecs_request_entity(&g_world);
		w_ecs_request_entity(&g_world);
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position");
	w_query_rebuild_cache(&g_world.queries, q);

	// should have both dense and sparse
	ck_assert_int_eq(q->archetype_slices_dense_length, 1);
	ck_assert_int_gt(q->archetype_slices_sparse_length, 0);

	int count = 0;
	float sum_x = 0;
	w_query_for_each(&g_world, "read position", {
		Position *pos = w_itor_get(Position);
		sum_x += pos->x;
		count++;
	});

	ck_assert_int_eq(count, 25);
	// 20 * 1.0 + 5 * 10.0 = 70
	ck_assert_float_eq(sum_x, 70.0f);
}
END_TEST

START_TEST(test_dense_then_sparse_order)
{
	// verify dense slices iterate before sparse
	// dense entities get x < 100, sparse get x >= 100

	// create dense run
	for (int i = 0; i < 16; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);  // 0-15
	}

	// create gap
	for (int i = 0; i < 100; i++)
	{
		w_ecs_request_entity(&g_world);
	}

	// create sparse
	for (int i = 0; i < 3; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, 100.0f + i, 0);  // 100-102
		w_ecs_request_entity(&g_world);  // gap
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position");
	w_query_rebuild_cache(&g_world.queries, q);

	// track when we hit sparse
	bool in_sparse = false;
	bool dense_before_sparse = true;
	w_query_for_each(&g_world, "read position", {
		Position *pos = w_itor_get(Position);
		if (pos->x >= 100.0f)
			in_sparse = true;
		else if (in_sparse)
			dense_before_sparse = false;  // found dense after sparse
	});

	ck_assert(dense_before_sparse);
}
END_TEST


/*****************************
*  empty results             *
*****************************/

START_TEST(test_empty_no_entities)
{
	// no entities at all, just register component
	w_ecs_get_component_by_name(&g_world, "position");

	struct w_query *q = w_ecs_get_query(&g_world, "read position");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	w_query_for_each(&g_world, "read position", {
		Position *pos = w_itor_get(Position);
		(void)pos;
		count++;
	});

	ck_assert_int_eq(count, 0);
}
END_TEST

START_TEST(test_empty_no_matching_entities)
{
	// create entities with position only, query for position + velocity
	for (int i = 0; i < 5; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
	}

	// register velocity but don't set it on any entity
	w_ecs_get_component_by_name(&g_world, "velocity");

	struct w_query *q = w_ecs_get_query(&g_world, "read position, read velocity");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	w_query_for_each(&g_world, "read position, read velocity", {
		Position *pos = w_itor_get(Position);
		Velocity *vel = w_itor_get(Velocity);
		(void)pos;
		(void)vel;
		count++;
	});

	ck_assert_int_eq(count, 0);
}
END_TEST

START_TEST(test_empty_slices_arrays)
{
	// verify slice arrays are empty for no-match query
	w_ecs_get_component_by_name(&g_world, "position");

	struct w_query *q = w_ecs_get_query(&g_world, "read position");
	w_query_rebuild_cache(&g_world.queries, q);

	ck_assert_int_eq(q->archetype_slices_dense_length, 0);
	ck_assert_int_eq(q->archetype_slices_sparse_length, 0);
}
END_TEST


/*****************************
*  cursor reset              *
*****************************/

START_TEST(test_cursor_resets_per_entity)
{
	// verify cursor resets to 0 for each entity
	for (int i = 0; i < 3; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, (float)i, 0);
		set_velocity(e, (float)(i * 10), 0);
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position, read velocity");
	w_query_rebuild_cache(&g_world.queries, q);

	int correct_order = 0;
	w_query_for_each(&g_world, "read position, read velocity", {
		// cursor should be 0 at start of each entity
		// after getting position, should be 1
		Position *pos = w_itor_get(Position);
		// cursor now 1, get velocity
		Velocity *vel = w_itor_get(Velocity);
		// verify we got correct data (not swapped due to wrong cursor)
		if (pos && vel && vel->vx == pos->x * 10.0f)
			correct_order++;
	});

	ck_assert_int_eq(correct_order, 3);
}
END_TEST

START_TEST(test_cursor_with_multiple_terms)
{
	// 4 components, verify all get correct data
	for (int i = 0; i < 2; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, 1.0f, 0);
		set_velocity(e, 2.0f, 0);
		set_health(e, 3, 100);
		set_scale(e, 4.0f);
	}

	// rebuild cache
	struct w_query *q = w_ecs_get_query(&g_world, "read position, read velocity, read health, read scale");
	w_query_rebuild_cache(&g_world.queries, q);

	int correct = 0;
	w_query_for_each(&g_world, "read position, read velocity, read health, read scale", {
		Position *pos = w_itor_get(Position);
		Velocity *vel = w_itor_get(Velocity);
		Health *h = w_itor_get(Health);
		Scale *s = w_itor_get(Scale);

		if (pos->x == 1.0f && vel->vx == 2.0f && h->health == 3 && s->scale == 4.0f)
			correct++;
	});

	ck_assert_int_eq(correct, 2);
}
END_TEST


/*****************************
*  edge cases                *
*****************************/

START_TEST(test_query_caching_static)
{
	// verify static query caching works (same query called multiple times)
	for (int i = 0; i < 3; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, 1.0f, 0);
	}

	struct w_query *q = w_ecs_get_query(&g_world, "read position");
	w_query_rebuild_cache(&g_world.queries, q);

	// call multiple times
	int total = 0;
	for (int round = 0; round < 3; round++)
	{
		w_query_for_each(&g_world, "read position", {
			Position *pos = w_itor_get(Position);
			(void)pos;
			total++;
		});
	}

	// 3 entities * 3 rounds = 9
	ck_assert_int_eq(total, 9);
}
END_TEST

START_TEST(test_entity_id_accessible)
{
	// verify itor.entity_id is accessible in loop
	w_entity_id entities[3];
	for (int i = 0; i < 3; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);
		set_position(entities[i], (float)entities[i], 0);
	}

	struct w_query *q = w_ecs_get_query(&g_world, "read position");
	w_query_rebuild_cache(&g_world.queries, q);

	int matched = 0;
	w_query_for_each(&g_world, "read position", {
		Position *pos = w_itor_get(Position);
		// we stored entity ID as x, verify it matches itor.entity_id
		if ((w_entity_id)pos->x == itor.entity_id)
			matched++;
	});

	ck_assert_int_eq(matched, 3);
}
END_TEST

START_TEST(test_large_entity_count)
{
	// stress test with many entities
	int n = 1000;
	for (int i = 0; i < n; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);
		set_position(e, 1.0f, 0);
	}

	struct w_query *q = w_ecs_get_query(&g_world, "read position");
	w_query_rebuild_cache(&g_world.queries, q);

	int count = 0;
	w_query_for_each(&g_world, "read position", {
		Position *pos = w_itor_get(Position);
		(void)pos;
		count++;
	});

	ck_assert_int_eq(count, n);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_query_iterator_suite(void)
{
	Suite *s = suite_create("whisker_query_iterator");

	TCase *tc_single = tcase_create("single_component");
	tcase_add_checked_fixture(tc_single, query_iterator_setup, query_iterator_teardown);
	tcase_set_timeout(tc_single, 10);
	tcase_add_test(tc_single, test_single_component_iterates_all_entities);
	tcase_add_test(tc_single, test_single_component_write_modifies_data);
	suite_add_tcase(s, tc_single);

	TCase *tc_multi = tcase_create("multiple_components");
	tcase_add_checked_fixture(tc_multi, query_iterator_setup, query_iterator_teardown);
	tcase_set_timeout(tc_multi, 10);
	tcase_add_test(tc_multi, test_multiple_components_read);
	tcase_add_test(tc_multi, test_multiple_components_mixed_access);
	tcase_add_test(tc_multi, test_three_components);
	suite_add_tcase(s, tc_multi);

	TCase *tc_opt_absent = tcase_create("optional_absent");
	tcase_add_checked_fixture(tc_opt_absent, query_iterator_setup, query_iterator_teardown);
	tcase_set_timeout(tc_opt_absent, 10);
	tcase_add_test(tc_opt_absent, test_optional_returns_null_when_absent);
	tcase_add_test(tc_opt_absent, test_optional_all_absent);
	suite_add_tcase(s, tc_opt_absent);

	TCase *tc_opt_present = tcase_create("optional_present");
	tcase_add_checked_fixture(tc_opt_present, query_iterator_setup, query_iterator_teardown);
	tcase_set_timeout(tc_opt_present, 10);
	tcase_add_test(tc_opt_present, test_optional_returns_data_when_present);
	tcase_add_test(tc_opt_present, test_optional_can_write_when_present);
	suite_add_tcase(s, tc_opt_present);

	TCase *tc_has = tcase_create("has_terms");
	tcase_add_checked_fixture(tc_has, query_iterator_setup, query_iterator_teardown);
	tcase_set_timeout(tc_has, 10);
	tcase_add_test(tc_has, test_has_term_skipped_by_iterator);
	tcase_add_test(tc_has, test_has_term_in_middle_skipped);
	suite_add_tcase(s, tc_has);

	TCase *tc_dense = tcase_create("dense_slices");
	tcase_add_checked_fixture(tc_dense, query_iterator_setup, query_iterator_teardown);
	tcase_set_timeout(tc_dense, 10);
	tcase_add_test(tc_dense, test_dense_slice_iteration);
	tcase_add_test(tc_dense, test_dense_slice_preserves_entity_id);
	suite_add_tcase(s, tc_dense);

	TCase *tc_sparse = tcase_create("sparse_slices");
	tcase_add_checked_fixture(tc_sparse, query_iterator_setup, query_iterator_teardown);
	tcase_set_timeout(tc_sparse, 10);
	tcase_add_test(tc_sparse, test_sparse_slice_iteration);
	tcase_add_test(tc_sparse, test_sparse_single_entities);
	suite_add_tcase(s, tc_sparse);

	TCase *tc_mixed = tcase_create("mixed_slices");
	tcase_add_checked_fixture(tc_mixed, query_iterator_setup, query_iterator_teardown);
	tcase_set_timeout(tc_mixed, 10);
	tcase_add_test(tc_mixed, test_mixed_dense_sparse_iteration);
	tcase_add_test(tc_mixed, test_dense_then_sparse_order);
	suite_add_tcase(s, tc_mixed);

	TCase *tc_empty = tcase_create("empty_results");
	tcase_add_checked_fixture(tc_empty, query_iterator_setup, query_iterator_teardown);
	tcase_set_timeout(tc_empty, 10);
	tcase_add_test(tc_empty, test_empty_no_entities);
	tcase_add_test(tc_empty, test_empty_no_matching_entities);
	tcase_add_test(tc_empty, test_empty_slices_arrays);
	suite_add_tcase(s, tc_empty);

	TCase *tc_cursor = tcase_create("cursor_reset");
	tcase_add_checked_fixture(tc_cursor, query_iterator_setup, query_iterator_teardown);
	tcase_set_timeout(tc_cursor, 10);
	tcase_add_test(tc_cursor, test_cursor_resets_per_entity);
	tcase_add_test(tc_cursor, test_cursor_with_multiple_terms);
	suite_add_tcase(s, tc_cursor);

	TCase *tc_edge = tcase_create("edge_cases");
	tcase_add_checked_fixture(tc_edge, query_iterator_setup, query_iterator_teardown);
	tcase_set_timeout(tc_edge, 30);
	tcase_add_test(tc_edge, test_query_caching_static);
	tcase_add_test(tc_edge, test_entity_id_accessible);
	tcase_add_test(tc_edge, test_large_entity_count);
	suite_add_tcase(s, tc_edge);

	return s;
}

// test runner
int main(void)
{
	Suite *s = whisker_query_iterator_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
