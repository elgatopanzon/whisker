/**
 * @author      : ElGatoPanzon
 * @file        : benchmark_whisker_module_component_events
 * @created     : Tuesday Mar 24, 2026 22:00:34 CST
 * @description : benchmarks for component_events module overhead on set/remove
 */

#include "ubench.h"
#include "whisker_ecs_world.h"
#include "modules/component_events/whisker_component_events.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


// ============================================================================
// constants
// ============================================================================

#define BENCH_ARENA_SIZE (256 * 1024 * 1024)
#define BENCH_COUNT 1000000

// component name for the float component under test
#define BENCH_COMP_NAME "bench_float"


// ============================================================================
// bench_set: 3 tests for component set overhead
// plain / module_no_track / module_tracked
// ============================================================================

struct bench_set
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_ecs_world world;
	w_entity_id comp_id;
	float data;
};

UBENCH_F_SETUP(bench_set)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_ecs_world_init(&ubench_fixture->world, &ubench_fixture->string_table, &ubench_fixture->arena);
	ubench_fixture->world.buffering_enabled = false;
	ubench_fixture->comp_id = w_ecs_get_component_by_name(&ubench_fixture->world, BENCH_COMP_NAME);
	ubench_fixture->data = 42.0f;
}

UBENCH_F_TEARDOWN(bench_set)
{
	w_ecs_world_free(&ubench_fixture->world);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_set, plain)
{
	for (w_entity_id i = 0; i < BENCH_COUNT; i++)
	{
		void *p = w_ecs_set_component_(&ubench_fixture->world, W_COMPONENT_TYPE_float,
			ubench_fixture->comp_id, i, &ubench_fixture->data, sizeof(float));
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_EX_F(bench_set, module_no_track)
{
	wm_component_events_init(&ubench_fixture->world);
	UBENCH_DO_BENCHMARK()
	{
		for (w_entity_id i = 0; i < BENCH_COUNT; i++)
		{
			void *p = w_ecs_set_component_(&ubench_fixture->world, W_COMPONENT_TYPE_float,
				ubench_fixture->comp_id, i, &ubench_fixture->data, sizeof(float));
			UBENCH_DO_NOTHING(p);
		}
	}
	wm_component_events_free(&ubench_fixture->world);
}

UBENCH_EX_F(bench_set, module_tracked)
{
	wm_component_events_init(&ubench_fixture->world);
	wm_component_events_register(&ubench_fixture->world, ubench_fixture->comp_id);
	UBENCH_DO_BENCHMARK()
	{
		for (w_entity_id i = 0; i < BENCH_COUNT; i++)
		{
			void *p = w_ecs_set_component_(&ubench_fixture->world, W_COMPONENT_TYPE_float,
				ubench_fixture->comp_id, i, &ubench_fixture->data, sizeof(float));
			UBENCH_DO_NOTHING(p);
		}
	}
	wm_component_events_free(&ubench_fixture->world);
}


// ============================================================================
// bench_remove: 3 tests for component remove overhead
// plain / module_no_track / module_tracked
// pre-populated with components in setup
// ============================================================================

struct bench_remove
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_ecs_world world;
	w_entity_id comp_id;
	float data;
};

UBENCH_F_SETUP(bench_remove)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_ecs_world_init(&ubench_fixture->world, &ubench_fixture->string_table, &ubench_fixture->arena);
	ubench_fixture->world.buffering_enabled = false;
	ubench_fixture->comp_id = w_ecs_get_component_by_name(&ubench_fixture->world, BENCH_COMP_NAME);
	ubench_fixture->data = 42.0f;

	for (w_entity_id i = 0; i < BENCH_COUNT; i++)
	{
		w_ecs_set_component_(&ubench_fixture->world, W_COMPONENT_TYPE_float,
			ubench_fixture->comp_id, i, &ubench_fixture->data, sizeof(float));
	}
}

UBENCH_F_TEARDOWN(bench_remove)
{
	w_ecs_world_free(&ubench_fixture->world);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_remove, plain)
{
	for (w_entity_id i = 0; i < BENCH_COUNT; i++)
	{
		w_ecs_remove_component_(&ubench_fixture->world, ubench_fixture->comp_id, i);
	}
}

UBENCH_EX_F(bench_remove, module_no_track)
{
	wm_component_events_init(&ubench_fixture->world);
	UBENCH_DO_BENCHMARK()
	{
		for (w_entity_id i = 0; i < BENCH_COUNT; i++)
		{
			w_ecs_remove_component_(&ubench_fixture->world, ubench_fixture->comp_id, i);
		}
	}
	wm_component_events_free(&ubench_fixture->world);
}

UBENCH_EX_F(bench_remove, module_tracked)
{
	wm_component_events_init(&ubench_fixture->world);
	wm_component_events_register(&ubench_fixture->world, ubench_fixture->comp_id);
	UBENCH_DO_BENCHMARK()
	{
		for (w_entity_id i = 0; i < BENCH_COUNT; i++)
		{
			w_ecs_remove_component_(&ubench_fixture->world, ubench_fixture->comp_id, i);
		}
	}
	wm_component_events_free(&ubench_fixture->world);
}


// ============================================================================
// bench_changed: 3 tests for component changed event overhead
// plain / module_no_track / module_tracked
// pre-populated with components in setup to measure "changed" not "added"
// ============================================================================

struct bench_changed
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_ecs_world world;
	w_entity_id comp_id;
	float data;
	float data_changed;
};

UBENCH_F_SETUP(bench_changed)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_ecs_world_init(&ubench_fixture->world, &ubench_fixture->string_table, &ubench_fixture->arena);
	ubench_fixture->world.buffering_enabled = false;
	ubench_fixture->comp_id = w_ecs_get_component_by_name(&ubench_fixture->world, BENCH_COMP_NAME);
	ubench_fixture->data = 42.0f;
	ubench_fixture->data_changed = 99.0f;

	for (w_entity_id i = 0; i < BENCH_COUNT; i++)
	{
		w_ecs_set_component_(&ubench_fixture->world, W_COMPONENT_TYPE_float,
			ubench_fixture->comp_id, i, &ubench_fixture->data, sizeof(float));
	}
}

UBENCH_F_TEARDOWN(bench_changed)
{
	w_ecs_world_free(&ubench_fixture->world);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_changed, plain)
{
	for (w_entity_id i = 0; i < BENCH_COUNT; i++)
	{
		void *p = w_ecs_set_component_(&ubench_fixture->world, W_COMPONENT_TYPE_float,
			ubench_fixture->comp_id, i, &ubench_fixture->data_changed, sizeof(float));
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_EX_F(bench_changed, module_no_track)
{
	wm_component_events_init(&ubench_fixture->world);
	UBENCH_DO_BENCHMARK()
	{
		for (w_entity_id i = 0; i < BENCH_COUNT; i++)
		{
			void *p = w_ecs_set_component_(&ubench_fixture->world, W_COMPONENT_TYPE_float,
				ubench_fixture->comp_id, i, &ubench_fixture->data_changed, sizeof(float));
			UBENCH_DO_NOTHING(p);
		}
	}
	wm_component_events_free(&ubench_fixture->world);
}

UBENCH_EX_F(bench_changed, module_tracked)
{
	wm_component_events_init(&ubench_fixture->world);
	wm_component_events_register(&ubench_fixture->world, ubench_fixture->comp_id);
	UBENCH_DO_BENCHMARK()
	{
		for (w_entity_id i = 0; i < BENCH_COUNT; i++)
		{
			void *p = w_ecs_set_component_(&ubench_fixture->world, W_COMPONENT_TYPE_float,
				ubench_fixture->comp_id, i, &ubench_fixture->data_changed, sizeof(float));
			UBENCH_DO_NOTHING(p);
		}
	}
	wm_component_events_free(&ubench_fixture->world);
}


UBENCH_MAIN();
