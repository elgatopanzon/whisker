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


// ============================================================================
// create entities: world API vs direct registry
// ============================================================================

struct bench_create_entities
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_ecs_world world;
};

UBENCH_F_SETUP(bench_create_entities)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_ecs_world_init(&ubench_fixture->world, &ubench_fixture->string_table, &ubench_fixture->arena);
}

UBENCH_F_TEARDOWN(bench_create_entities)
{
	w_ecs_world_free(&ubench_fixture->world);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_create_entities, world_unbuffered)
{
	ubench_fixture->world.buffering_enabled = false;
	for (int i = 0; i < 10000; i++)
	{
		w_entity_id e = w_ecs_request_entity(&ubench_fixture->world);
		UBENCH_DO_NOTHING(&e);
	}
}

UBENCH_F(bench_create_entities, direct_registry)
{
	struct w_entity_registry *reg = &ubench_fixture->world.entities;
	for (int i = 0; i < 10000; i++)
	{
		w_entity_id e = w_entity_request(reg);
		UBENCH_DO_NOTHING(&e);
	}
}


// ============================================================================
// create named entities: world API vs direct registry
// ============================================================================

struct bench_create_named_entities
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_ecs_world world;
	char name_buf[32];
};

UBENCH_F_SETUP(bench_create_named_entities)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 131072, 131072, w_hashmap_hash_str);
	w_ecs_world_init(&ubench_fixture->world, &ubench_fixture->string_table, &ubench_fixture->arena);
}

UBENCH_F_TEARDOWN(bench_create_named_entities)
{
	w_ecs_world_free(&ubench_fixture->world);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_create_named_entities, world_buffered)
{
	ubench_fixture->world.buffering_enabled = true;
	for (int i = 0; i < 10000; i++)
	{
		snprintf(ubench_fixture->name_buf, sizeof(ubench_fixture->name_buf), "Entity_%d", i);
		w_entity_id e = w_ecs_request_entity_with_name(&ubench_fixture->world, ubench_fixture->name_buf);
		UBENCH_DO_NOTHING(&e);
	}
	w_command_buffer_flush(&ubench_fixture->world.command_buffer);
}

UBENCH_F(bench_create_named_entities, world_unbuffered)
{
	ubench_fixture->world.buffering_enabled = false;
	for (int i = 0; i < 10000; i++)
	{
		snprintf(ubench_fixture->name_buf, sizeof(ubench_fixture->name_buf), "Entity_%d", i);
		w_entity_id e = w_ecs_request_entity_with_name(&ubench_fixture->world, ubench_fixture->name_buf);
		UBENCH_DO_NOTHING(&e);
	}
}

UBENCH_F(bench_create_named_entities, direct_registry)
{
	struct w_entity_registry *reg = &ubench_fixture->world.entities;
	for (int i = 0; i < 10000; i++)
	{
		snprintf(ubench_fixture->name_buf, sizeof(ubench_fixture->name_buf), "Entity_%d", i);
		w_entity_id existing = w_entity_lookup_by_name(reg, ubench_fixture->name_buf);
		if (existing != W_ENTITY_INVALID)
		{
			UBENCH_DO_NOTHING(&existing);
			continue;
		}
		w_entity_id e = w_entity_request(reg);
		w_entity_set_name(reg, e, ubench_fixture->name_buf);
		UBENCH_DO_NOTHING(&e);
	}
}


// ============================================================================
// return entities: world API vs direct registry
// ============================================================================

struct bench_return_entities
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_ecs_world world;
	w_entity_id *entities;
};

UBENCH_F_SETUP(bench_return_entities)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_ecs_world_init(&ubench_fixture->world, &ubench_fixture->string_table, &ubench_fixture->arena);

	// pre-create 10k entities to return
	ubench_fixture->entities = malloc(10000 * sizeof(w_entity_id));
	ubench_fixture->world.buffering_enabled = false;
	for (int i = 0; i < 10000; i++)
	{
		ubench_fixture->entities[i] = w_ecs_request_entity(&ubench_fixture->world);
	}
}

UBENCH_F_TEARDOWN(bench_return_entities)
{
	free(ubench_fixture->entities);
	w_ecs_world_free(&ubench_fixture->world);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_return_entities, world_buffered)
{
	ubench_fixture->world.buffering_enabled = true;
	for (int i = 0; i < 10000; i++)
	{
		w_ecs_return_entity(&ubench_fixture->world, ubench_fixture->entities[i]);
	}
	w_command_buffer_flush(&ubench_fixture->world.command_buffer);
}

UBENCH_F(bench_return_entities, world_unbuffered)
{
	ubench_fixture->world.buffering_enabled = false;
	for (int i = 0; i < 10000; i++)
	{
		w_ecs_return_entity(&ubench_fixture->world, ubench_fixture->entities[i]);
	}
}

UBENCH_F(bench_return_entities, direct_registry)
{
	struct w_entity_registry *reg = &ubench_fixture->world.entities;
	for (int i = 0; i < 10000; i++)
	{
		w_entity_return(reg, ubench_fixture->entities[i]);
	}
}


// ============================================================================
// set components: world API vs direct registry
// ============================================================================

struct bench_component_16b
{
	uint64_t a;
	uint64_t b;
};

struct bench_set_components
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_ecs_world world;
	w_entity_id type_id;
	struct bench_component_16b data;
};

UBENCH_F_SETUP(bench_set_components)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_ecs_world_init(&ubench_fixture->world, &ubench_fixture->string_table, &ubench_fixture->arena);

	// pre-create component type
	ubench_fixture->world.buffering_enabled = false;
	ubench_fixture->type_id = w_ecs_request_entity(&ubench_fixture->world);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
}

UBENCH_F_TEARDOWN(bench_set_components)
{
	w_ecs_world_free(&ubench_fixture->world);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_set_components, world_buffered)
{
	ubench_fixture->world.buffering_enabled = true;
	for (w_entity_id i = 0; i < 10000; i++)
	{
		void *p = w_ecs_set_component_(&ubench_fixture->world, 0, ubench_fixture->type_id, i, &ubench_fixture->data, sizeof(ubench_fixture->data));
		UBENCH_DO_NOTHING(p);
	}
	w_command_buffer_flush(&ubench_fixture->world.command_buffer);
}

UBENCH_F(bench_set_components, world_unbuffered)
{
	ubench_fixture->world.buffering_enabled = false;
	for (w_entity_id i = 0; i < 10000; i++)
	{
		void *p = w_ecs_set_component_(&ubench_fixture->world, 0, ubench_fixture->type_id, i, &ubench_fixture->data, sizeof(ubench_fixture->data));
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_set_components, direct_registry)
{
	struct w_component_registry *reg = &ubench_fixture->world.components;
	for (w_entity_id i = 0; i < 10000; i++)
	{
		void *p = w_component_set_(reg, 0, ubench_fixture->type_id, i, &ubench_fixture->data, sizeof(ubench_fixture->data));
		UBENCH_DO_NOTHING(p);
	}
}


// ============================================================================
// remove components: world API vs direct registry
// ============================================================================

struct bench_remove_components
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_ecs_world world;
	w_entity_id type_id;
	struct bench_component_16b data;
};

UBENCH_F_SETUP(bench_remove_components)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_ecs_world_init(&ubench_fixture->world, &ubench_fixture->string_table, &ubench_fixture->arena);

	// pre-create component type and set 10k components
	ubench_fixture->world.buffering_enabled = false;
	ubench_fixture->type_id = w_ecs_request_entity(&ubench_fixture->world);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;

	for (w_entity_id i = 0; i < 10000; i++)
	{
		w_ecs_set_component_(&ubench_fixture->world, 0, ubench_fixture->type_id, i, &ubench_fixture->data, sizeof(ubench_fixture->data));
	}
}

UBENCH_F_TEARDOWN(bench_remove_components)
{
	w_ecs_world_free(&ubench_fixture->world);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_remove_components, world_buffered)
{
	ubench_fixture->world.buffering_enabled = true;
	for (w_entity_id i = 0; i < 10000; i++)
	{
		w_ecs_remove_component_(&ubench_fixture->world, ubench_fixture->type_id, i);
	}
	w_command_buffer_flush(&ubench_fixture->world.command_buffer);
}

UBENCH_F(bench_remove_components, world_unbuffered)
{
	ubench_fixture->world.buffering_enabled = false;
	for (w_entity_id i = 0; i < 10000; i++)
	{
		w_ecs_remove_component_(&ubench_fixture->world, ubench_fixture->type_id, i);
	}
}

UBENCH_F(bench_remove_components, direct_registry)
{
	struct w_component_registry *reg = &ubench_fixture->world.components;
	for (w_entity_id i = 0; i < 10000; i++)
	{
		w_component_remove_(reg, ubench_fixture->type_id, i);
	}
}


UBENCH_MAIN();
