/**
 * @author      : ElGatoPanzon
 * @file        : benchmark_whisker_iteration
 * @created     : 2026-03-06
 * @description : benchmarks for measuring iteration system overhead
 */

#include "ubench.h"
#include "whisker_ecs_world.h"
#include "whisker_query_iterator.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>


// ============================================================================
// constants
// ============================================================================

#define BENCH_ARENA_SIZE       (64 * 1024 * 1024)
#define BENCH_UPDATE_COUNT     1000
#define BENCH_ENTITY_COUNT     1000


// ============================================================================
// component types
// ============================================================================

typedef struct {
	float x;
	float y;
} BenchPosition;

typedef struct {
	float vx;
	float vy;
} BenchVelocity;

typedef struct {
	float scale;
	float rotation;
} BenchTransform;


// accumulator to prevent optimization
static volatile double g_accumulator = 0.0;


// ============================================================================
// iteration_10_systems_3_components group
// ============================================================================

struct bench_iteration_10_systems_3_components
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_ecs_world world;

	// component IDs
	w_entity_id position_id;
	w_entity_id velocity_id;
	w_entity_id transform_id;

	// component entries for direct access
	struct w_component_entry *position_entry;
	struct w_component_entry *velocity_entry;
	struct w_component_entry *transform_entry;

	// bitset intersect cache for manual loop
	struct w_sparse_bitset_intersect_cache bitset_cache;

	// delta time for systems
	double delta_time;
};

// system work: multiply position by velocity, accumulate to transform
// repeated 10 times with different multipliers for 10 systems
static void do_system_work_(BenchPosition *pos, BenchVelocity *vel, BenchTransform *trans, double dt, int sys_id)
{
	float mult = 0.1f * (sys_id + 1);
	trans->scale += (pos->x * vel->vx + pos->y * vel->vy) * mult * (float)dt;
	trans->rotation += (pos->x - pos->y) * mult * (float)dt;
	g_accumulator += trans->scale + trans->rotation;
}


// ============================================================================
// system functions for manual_system_invocation and world_update
// ============================================================================

static void system_0_(void *ctx, double dt)
{
	struct w_ecs_world *world = ctx;
	w_query_for_each(world, "read position, read velocity, write transform", {
		BenchPosition *pos = w_itor_get(BenchPosition);
		BenchVelocity *vel = w_itor_get(BenchVelocity);
		BenchTransform *trans = w_itor_get(BenchTransform);
		do_system_work_(pos, vel, trans, dt, 0);
	});
}

static void system_1_(void *ctx, double dt)
{
	struct w_ecs_world *world = ctx;
	w_query_for_each(world, "read position, read velocity, write transform", {
		BenchPosition *pos = w_itor_get(BenchPosition);
		BenchVelocity *vel = w_itor_get(BenchVelocity);
		BenchTransform *trans = w_itor_get(BenchTransform);
		do_system_work_(pos, vel, trans, dt, 1);
	});
}

static void system_2_(void *ctx, double dt)
{
	struct w_ecs_world *world = ctx;
	w_query_for_each(world, "read position, read velocity, write transform", {
		BenchPosition *pos = w_itor_get(BenchPosition);
		BenchVelocity *vel = w_itor_get(BenchVelocity);
		BenchTransform *trans = w_itor_get(BenchTransform);
		do_system_work_(pos, vel, trans, dt, 2);
	});
}

static void system_3_(void *ctx, double dt)
{
	struct w_ecs_world *world = ctx;
	w_query_for_each(world, "read position, read velocity, write transform", {
		BenchPosition *pos = w_itor_get(BenchPosition);
		BenchVelocity *vel = w_itor_get(BenchVelocity);
		BenchTransform *trans = w_itor_get(BenchTransform);
		do_system_work_(pos, vel, trans, dt, 3);
	});
}

static void system_4_(void *ctx, double dt)
{
	struct w_ecs_world *world = ctx;
	w_query_for_each(world, "read position, read velocity, write transform", {
		BenchPosition *pos = w_itor_get(BenchPosition);
		BenchVelocity *vel = w_itor_get(BenchVelocity);
		BenchTransform *trans = w_itor_get(BenchTransform);
		do_system_work_(pos, vel, trans, dt, 4);
	});
}

static void system_5_(void *ctx, double dt)
{
	struct w_ecs_world *world = ctx;
	w_query_for_each(world, "read position, read velocity, write transform", {
		BenchPosition *pos = w_itor_get(BenchPosition);
		BenchVelocity *vel = w_itor_get(BenchVelocity);
		BenchTransform *trans = w_itor_get(BenchTransform);
		do_system_work_(pos, vel, trans, dt, 5);
	});
}

static void system_6_(void *ctx, double dt)
{
	struct w_ecs_world *world = ctx;
	w_query_for_each(world, "read position, read velocity, write transform", {
		BenchPosition *pos = w_itor_get(BenchPosition);
		BenchVelocity *vel = w_itor_get(BenchVelocity);
		BenchTransform *trans = w_itor_get(BenchTransform);
		do_system_work_(pos, vel, trans, dt, 6);
	});
}

static void system_7_(void *ctx, double dt)
{
	struct w_ecs_world *world = ctx;
	w_query_for_each(world, "read position, read velocity, write transform", {
		BenchPosition *pos = w_itor_get(BenchPosition);
		BenchVelocity *vel = w_itor_get(BenchVelocity);
		BenchTransform *trans = w_itor_get(BenchTransform);
		do_system_work_(pos, vel, trans, dt, 7);
	});
}

static void system_8_(void *ctx, double dt)
{
	struct w_ecs_world *world = ctx;
	w_query_for_each(world, "read position, read velocity, write transform", {
		BenchPosition *pos = w_itor_get(BenchPosition);
		BenchVelocity *vel = w_itor_get(BenchVelocity);
		BenchTransform *trans = w_itor_get(BenchTransform);
		do_system_work_(pos, vel, trans, dt, 8);
	});
}

static void system_9_(void *ctx, double dt)
{
	struct w_ecs_world *world = ctx;
	w_query_for_each(world, "read position, read velocity, write transform", {
		BenchPosition *pos = w_itor_get(BenchPosition);
		BenchVelocity *vel = w_itor_get(BenchVelocity);
		BenchTransform *trans = w_itor_get(BenchTransform);
		do_system_work_(pos, vel, trans, dt, 9);
	});
}

typedef void (*system_fn)(void *, double);
static system_fn g_system_fns[10] = {
	system_0_, system_1_, system_2_, system_3_, system_4_,
	system_5_, system_6_, system_7_, system_8_, system_9_
};


UBENCH_F_SETUP(bench_iteration_10_systems_3_components)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_ecs_world_init(&ubench_fixture->world, &ubench_fixture->string_table, &ubench_fixture->arena);

	ubench_fixture->world.buffering_enabled = false;
	ubench_fixture->delta_time = 1.0 / 60.0;

	// register components and get IDs
	ubench_fixture->position_id = w_ecs_get_component_by_name(&ubench_fixture->world, "position");
	ubench_fixture->velocity_id = w_ecs_get_component_by_name(&ubench_fixture->world, "velocity");
	ubench_fixture->transform_id = w_ecs_get_component_by_name(&ubench_fixture->world, "transform");

	// create entities with all 3 components
	for (int i = 0; i < BENCH_ENTITY_COUNT; i++)
	{
		w_entity_id e = w_ecs_request_entity(&ubench_fixture->world);

		BenchPosition pos = {(float)i * 0.1f, (float)i * 0.2f};
		BenchVelocity vel = {1.0f, 2.0f};
		BenchTransform trans = {1.0f, 0.0f};

		w_ecs_set_component_(&ubench_fixture->world, W_COMPONENT_TYPE_float,
			ubench_fixture->position_id, e, &pos, sizeof(BenchPosition));
		w_ecs_set_component_(&ubench_fixture->world, W_COMPONENT_TYPE_float,
			ubench_fixture->velocity_id, e, &vel, sizeof(BenchVelocity));
		w_ecs_set_component_(&ubench_fixture->world, W_COMPONENT_TYPE_float,
			ubench_fixture->transform_id, e, &trans, sizeof(BenchTransform));
	}

	// cache component entries for manual loop
	ubench_fixture->position_entry = w_ecs_get_component_entry(&ubench_fixture->world, ubench_fixture->position_id);
	ubench_fixture->velocity_entry = w_ecs_get_component_entry(&ubench_fixture->world, ubench_fixture->velocity_id);
	ubench_fixture->transform_entry = w_ecs_get_component_entry(&ubench_fixture->world, ubench_fixture->transform_id);

	// setup bitset cache for manual loop
	memset(&ubench_fixture->bitset_cache, 0, sizeof(ubench_fixture->bitset_cache));
	w_array_init_t(ubench_fixture->bitset_cache.bitsets, 3);
	ubench_fixture->bitset_cache.bitsets[0] = &ubench_fixture->position_entry->data_bitset;
	ubench_fixture->bitset_cache.bitsets[1] = &ubench_fixture->velocity_entry->data_bitset;
	ubench_fixture->bitset_cache.bitsets[2] = &ubench_fixture->transform_entry->data_bitset;
	ubench_fixture->bitset_cache.bitsets_length = 3;
	w_sparse_bitset_intersect(&ubench_fixture->bitset_cache);

	// pre-build query cache for query-based tests
	struct w_query *q = w_ecs_get_query(&ubench_fixture->world, "read position, read velocity, write transform");
	w_query_rebuild_cache(&ubench_fixture->world.queries, q);

	// register timestep for world_update test (uncapped for benchmarking)
	struct w_scheduler_time_step ts = {
		.enabled = true,
		.id = 0,
		.time_step = w_time_step_create(60.0, 1, true, false, false, false, false, false)
	};
	w_scheduler_register_time_step(&ubench_fixture->world.scheduler, &ts);

	// register 10 phases (one per system) for world_update test
	for (int i = 0; i < 10; i++)
	{
		struct w_scheduler_phase phase = {
			.enabled = true,
			.id = (size_t)i,
			.time_step_id = 0
		};
		w_scheduler_register_phase(&ubench_fixture->world.scheduler, &phase);
	}

	// register 10 systems for world_update test (one per phase)
	for (int i = 0; i < 10; i++)
	{
		struct w_system sys = {
			.phase_id = (size_t)i,
			.enabled = true,
			.update = g_system_fns[i],
			.last_update_ticks = 0,
			.update_frequency = 0
		};
		w_ecs_register_system(&ubench_fixture->world, &sys);
	}

	g_accumulator = 0.0;
}

UBENCH_F_TEARDOWN(bench_iteration_10_systems_3_components)
{
	w_sparse_bitset_intersect_free_cache(&ubench_fixture->bitset_cache);
	w_ecs_world_free(&ubench_fixture->world);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}


// ============================================================================
// CASE 1: manual_loop - direct bitset intersection + component array access
// ============================================================================

UBENCH_F(bench_iteration_10_systems_3_components, manual_loop)
{
	struct w_sparse_bitset_intersect_cache *cache = &ubench_fixture->bitset_cache;
	struct w_component_entry *pos_entry = ubench_fixture->position_entry;
	struct w_component_entry *vel_entry = ubench_fixture->velocity_entry;
	struct w_component_entry *trans_entry = ubench_fixture->transform_entry;
	double dt = ubench_fixture->delta_time;

	for (int cycle = 0; cycle < BENCH_UPDATE_COUNT; cycle++)
	{
		// simulate 10 systems worth of work
		for (int sys = 0; sys < 10; sys++)
		{
			for (size_t i = 0; i < cache->indexes_length; i++)
			{
				w_entity_id e = cache->indexes[i];
				BenchPosition *pos = w_component_get_entry(pos_entry, e, BenchPosition);
				BenchVelocity *vel = w_component_get_entry(vel_entry, e, BenchVelocity);
				BenchTransform *trans = w_component_get_entry(trans_entry, e, BenchTransform);
				do_system_work_(pos, vel, trans, dt, sys);
			}
		}
	}
	UBENCH_DO_NOTHING(&g_accumulator);
}


// ============================================================================
// CASE 2: manual_system_invocation - 10 systems using w_query_for_each
// ============================================================================

UBENCH_F(bench_iteration_10_systems_3_components, manual_system_invocation)
{
	struct w_ecs_world *world = &ubench_fixture->world;
	double dt = ubench_fixture->delta_time;

	for (int cycle = 0; cycle < BENCH_UPDATE_COUNT; cycle++)
	{
		system_0_(world, dt);
		system_1_(world, dt);
		system_2_(world, dt);
		system_3_(world, dt);
		system_4_(world, dt);
		system_5_(world, dt);
		system_6_(world, dt);
		system_7_(world, dt);
		system_8_(world, dt);
		system_9_(world, dt);
	}
	UBENCH_DO_NOTHING(&g_accumulator);
}


// ============================================================================
// CASE 3: world_update - 10 systems via w_ecs_register_system + w_ecs_update
// ============================================================================

UBENCH_F(bench_iteration_10_systems_3_components, world_update)
{
	for (int cycle = 0; cycle < BENCH_UPDATE_COUNT; cycle++)
	{
		w_ecs_update(&ubench_fixture->world);
	}
	UBENCH_DO_NOTHING(&g_accumulator);
}


UBENCH_MAIN();
