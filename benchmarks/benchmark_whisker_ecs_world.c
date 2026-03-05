/**
 * @author      : ElGatoPanzon
 * @file        : benchmark_whisker_ecs_world
 * @created     : 2026-03-05
 * @description : benchmarks for ECS world update with scheduler overhead comparisons
 */

#include "ubench.h"
#include "whisker_ecs_world.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>


// ============================================================================
// constants
// ============================================================================

#define BENCH_ARENA_SIZE       (16 * 1024 * 1024)
#define BENCH_UPDATE_COUNT     1000

// simple math workload accumulator (volatile to prevent optimization)
static volatile double g_accumulator = 0.0;

// simple math workload system function
static void system_math_workload(void *ctx, double delta_time)
{
	(void)ctx;
	g_accumulator += sin(delta_time * 1.5) + cos(delta_time * 0.7);
}


// ============================================================================
// update_1_system: single system baseline
// ============================================================================

struct bench_update_1_system
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_ecs_world world;
};

UBENCH_F_SETUP(bench_update_1_system)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_ecs_world_init(&ubench_fixture->world, &ubench_fixture->string_table, &ubench_fixture->arena);

	// register variable timestep (uncapped)
	struct w_scheduler_time_step ts = {
		.enabled = true,
		.id = 0,
		.time_step = w_time_step_create(60.0, 1, true, false, false, false, false, false)
	};
	w_scheduler_register_time_step(&ubench_fixture->world.scheduler, &ts);

	// register single phase
	struct w_scheduler_phase phase = {
		.enabled = true,
		.id = 0,
		.time_step_id = 0
	};
	w_scheduler_register_phase(&ubench_fixture->world.scheduler, &phase);

	// register single system (frequency=0 for no per-system throttling)
	struct w_system sys = {
		.phase_id = 0,
		.enabled = true,
		.update = system_math_workload,
		.last_update_ticks = 0,
		.update_frequency = 0
	};
	w_ecs_register_system(&ubench_fixture->world, &sys);

	g_accumulator = 0.0;
}

UBENCH_F_TEARDOWN(bench_update_1_system)
{
	w_ecs_world_free(&ubench_fixture->world);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_update_1_system, update_1k)
{
	for (int i = 0; i < BENCH_UPDATE_COUNT; i++)
	{
		w_ecs_update(&ubench_fixture->world);
	}
	UBENCH_DO_NOTHING(&g_accumulator);
}


// ============================================================================
// update_10_systems: scheduler overhead comparison (grouped fixture)
// ============================================================================

// helper to init world with timestep and N phases, 10 systems distributed
static void init_world_with_phases_(
	struct w_ecs_world *world,
	struct w_string_table *st,
	struct w_arena *arena,
	size_t num_phases)
{
	w_ecs_world_init(world, st, arena);

	// register variable timestep (uncapped)
	struct w_scheduler_time_step ts = {
		.enabled = true,
		.id = 0,
		.time_step = w_time_step_create(60.0, 1, true, false, false, false, false, false)
	};
	w_scheduler_register_time_step(&world->scheduler, &ts);

	// register phases
	for (size_t p = 0; p < num_phases; p++)
	{
		struct w_scheduler_phase phase = {
			.enabled = true,
			.id = p,
			.time_step_id = 0
		};
		w_scheduler_register_phase(&world->scheduler, &phase);
	}

	// register 10 systems distributed across phases
	size_t systems_per_phase = 10 / num_phases;
	for (int i = 0; i < 10; i++)
	{
		struct w_system sys = {
			.phase_id = (size_t)(i / systems_per_phase) % num_phases,
			.enabled = true,
			.update = system_math_workload,
			.last_update_ticks = 0,
			.update_frequency = 0
		};
		w_ecs_register_system(world, &sys);
	}
}

struct bench_update_10_systems
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_ecs_world world_1_phase;
	struct w_ecs_world world_2_phases;
	struct w_ecs_world world_5_phases;
	struct w_ecs_world world_10_phases;
	double delta_time;
};

UBENCH_F_SETUP(bench_update_10_systems)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE * 4);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);

	init_world_with_phases_(&ubench_fixture->world_1_phase, &ubench_fixture->string_table, &ubench_fixture->arena, 1);
	init_world_with_phases_(&ubench_fixture->world_2_phases, &ubench_fixture->string_table, &ubench_fixture->arena, 2);
	init_world_with_phases_(&ubench_fixture->world_5_phases, &ubench_fixture->string_table, &ubench_fixture->arena, 5);
	init_world_with_phases_(&ubench_fixture->world_10_phases, &ubench_fixture->string_table, &ubench_fixture->arena, 10);

	ubench_fixture->delta_time = 1.0 / 60.0;
	g_accumulator = 0.0;
}

UBENCH_F_TEARDOWN(bench_update_10_systems)
{
	w_ecs_world_free(&ubench_fixture->world_10_phases);
	w_ecs_world_free(&ubench_fixture->world_5_phases);
	w_ecs_world_free(&ubench_fixture->world_2_phases);
	w_ecs_world_free(&ubench_fixture->world_1_phase);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_update_10_systems, manual)
{
	double dt = ubench_fixture->delta_time;
	for (int i = 0; i < BENCH_UPDATE_COUNT; i++)
	{
		system_math_workload(NULL, dt);
		system_math_workload(NULL, dt);
		system_math_workload(NULL, dt);
		system_math_workload(NULL, dt);
		system_math_workload(NULL, dt);
		system_math_workload(NULL, dt);
		system_math_workload(NULL, dt);
		system_math_workload(NULL, dt);
		system_math_workload(NULL, dt);
		system_math_workload(NULL, dt);
	}
	UBENCH_DO_NOTHING(&g_accumulator);
}

UBENCH_F(bench_update_10_systems, 1_phase)
{
	for (int i = 0; i < BENCH_UPDATE_COUNT; i++)
	{
		w_ecs_update(&ubench_fixture->world_1_phase);
	}
	UBENCH_DO_NOTHING(&g_accumulator);
}

UBENCH_F(bench_update_10_systems, 2_phases)
{
	for (int i = 0; i < BENCH_UPDATE_COUNT; i++)
	{
		w_ecs_update(&ubench_fixture->world_2_phases);
	}
	UBENCH_DO_NOTHING(&g_accumulator);
}

UBENCH_F(bench_update_10_systems, 5_phases)
{
	for (int i = 0; i < BENCH_UPDATE_COUNT; i++)
	{
		w_ecs_update(&ubench_fixture->world_5_phases);
	}
	UBENCH_DO_NOTHING(&g_accumulator);
}

UBENCH_F(bench_update_10_systems, 10_phases)
{
	for (int i = 0; i < BENCH_UPDATE_COUNT; i++)
	{
		w_ecs_update(&ubench_fixture->world_10_phases);
	}
	UBENCH_DO_NOTHING(&g_accumulator);
}


UBENCH_MAIN();
