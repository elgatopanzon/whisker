/**
 * @author      : ElGatoPanzon
 * @file        : benchmark_whisker_component_registry
 * @created     : 2026-03-03
 * @description : benchmarks for component registry set/get/remove/has operations
 */

#include "ubench.h"
#include "whisker_component_registry.h"
#include "whisker_entity_registry.h"
#include "whisker_string_table.h"
#include "whisker_arena.h"
#include "whisker_hashmap.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


// ============================================================================
// constants
// ============================================================================

#define BENCH_ARENA_SIZE       (16 * 1024 * 1024)
#define BENCH_ARENA_SIZE_LARGE (64 * 1024 * 1024)
#define BENCH_ARENA_SIZE_HUGE  (256 * 1024 * 1024)

#define BENCH_COUNT_1K    1000
#define BENCH_COUNT_10K   10000
#define BENCH_COUNT_100K  100000
#define BENCH_COUNT_1M    1000000

#define COMPONENT_SIZE_16B 16

// 16-byte component struct for benchmarks
struct bench_component_16b
{
	uint64_t a;
	uint64_t b;
};


// ============================================================================
// scale benchmarks 1k: set, get, has, remove
// ============================================================================

struct bench_scale_1k
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_entity_registry entities;
	struct w_component_registry registry;
	w_entity_id type_id;
	struct bench_component_16b data;
};

UBENCH_F_SETUP(bench_scale_1k)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_entity_registry_init(&ubench_fixture->entities, &ubench_fixture->string_table);
	w_component_registry_init(&ubench_fixture->registry, &ubench_fixture->arena, &ubench_fixture->entities);
	ubench_fixture->type_id = w_entity_request(&ubench_fixture->entities);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
	// pre-populate for get/has/remove benchmarks (set will overwrite)
	for (w_entity_id i = 0; i < BENCH_COUNT_1K; i++)
	{
		w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
	}
}

UBENCH_F_TEARDOWN(bench_scale_1k)
{
	w_component_registry_free(&ubench_fixture->registry);
	w_entity_registry_free(&ubench_fixture->entities);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_scale_1k, set_1k_16b)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_1K; i++)
	{
		void *p = w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_scale_1k, get_1k_16b)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_1K; i++)
	{
		void *p = w_component_get_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_scale_1k, has_1k_16b)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_1K; i++)
	{
		bool r = w_component_has_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(&r);
	}
}

UBENCH_F(bench_scale_1k, remove_1k_16b)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_1K; i++)
	{
		w_component_remove_(&ubench_fixture->registry, ubench_fixture->type_id, i);
	}
}


// ============================================================================
// scale benchmarks 10k: set, get, has, remove
// ============================================================================

struct bench_scale_10k
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_entity_registry entities;
	struct w_component_registry registry;
	w_entity_id type_id;
	struct bench_component_16b data;
};

UBENCH_F_SETUP(bench_scale_10k)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_entity_registry_init(&ubench_fixture->entities, &ubench_fixture->string_table);
	w_component_registry_init(&ubench_fixture->registry, &ubench_fixture->arena, &ubench_fixture->entities);
	ubench_fixture->type_id = w_entity_request(&ubench_fixture->entities);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
	// pre-populate for get/has/remove benchmarks (set will overwrite)
	for (w_entity_id i = 0; i < BENCH_COUNT_10K; i++)
	{
		w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
	}
}

UBENCH_F_TEARDOWN(bench_scale_10k)
{
	w_component_registry_free(&ubench_fixture->registry);
	w_entity_registry_free(&ubench_fixture->entities);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_scale_10k, set_10k_16b)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_10K; i++)
	{
		void *p = w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_scale_10k, get_10k_16b)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_10K; i++)
	{
		void *p = w_component_get_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_scale_10k, has_10k_16b)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_10K; i++)
	{
		bool r = w_component_has_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(&r);
	}
}

UBENCH_F(bench_scale_10k, remove_10k_16b)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_10K; i++)
	{
		w_component_remove_(&ubench_fixture->registry, ubench_fixture->type_id, i);
	}
}


// ============================================================================
// scale benchmarks 100k: set, get, has, remove
// ============================================================================

struct bench_scale_100k
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_entity_registry entities;
	struct w_component_registry registry;
	w_entity_id type_id;
	struct bench_component_16b data;
};

UBENCH_F_SETUP(bench_scale_100k)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_LARGE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_entity_registry_init(&ubench_fixture->entities, &ubench_fixture->string_table);
	w_component_registry_init(&ubench_fixture->registry, &ubench_fixture->arena, &ubench_fixture->entities);
	ubench_fixture->type_id = w_entity_request(&ubench_fixture->entities);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
	// pre-populate for get/has/remove benchmarks (set will overwrite)
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
	}
}

UBENCH_F_TEARDOWN(bench_scale_100k)
{
	w_component_registry_free(&ubench_fixture->registry);
	w_entity_registry_free(&ubench_fixture->entities);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_scale_100k, set_100k_16b)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		void *p = w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_scale_100k, get_100k_16b)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		void *p = w_component_get_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_scale_100k, has_100k_16b)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		bool r = w_component_has_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(&r);
	}
}

UBENCH_F(bench_scale_100k, remove_100k_16b)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		w_component_remove_(&ubench_fixture->registry, ubench_fixture->type_id, i);
	}
}


// ============================================================================
// single-op benchmarks: entity type ID access (func, func_unsafe, direct)
// ============================================================================

struct bench_single_typeid
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_entity_registry entities;
	struct w_component_registry registry;
	w_entity_id type_id;
	struct bench_component_16b data;
	struct w_component_entry *cached_entry;
};

UBENCH_F_SETUP(bench_single_typeid)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_entity_registry_init(&ubench_fixture->entities, &ubench_fixture->string_table);
	w_component_registry_init(&ubench_fixture->registry, &ubench_fixture->arena, &ubench_fixture->entities);
	ubench_fixture->type_id = w_entity_request(&ubench_fixture->entities);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
	// pre-set one component so entry exists
	w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, 0, &ubench_fixture->data, COMPONENT_SIZE_16B);
	// cache entry pointer for macro benchmarks
	ubench_fixture->cached_entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
}

UBENCH_F_TEARDOWN(bench_single_typeid)
{
	w_component_registry_free(&ubench_fixture->registry);
	w_entity_registry_free(&ubench_fixture->entities);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_single_typeid, set_typeid_func)
{
	void *p = w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, 1, &ubench_fixture->data, COMPONENT_SIZE_16B);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_single_typeid, get_typeid_func)
{
	void *p = w_component_get_(&ubench_fixture->registry, ubench_fixture->type_id, 0);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_single_typeid, has_typeid_func)
{
	bool r = w_component_has_(&ubench_fixture->registry, ubench_fixture->type_id, 0);
	UBENCH_DO_NOTHING(&r);
}

UBENCH_F(bench_single_typeid, set_typeid_func_unsafe)
{
	void *p = w_component_set_unsafe_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, 1, &ubench_fixture->data, COMPONENT_SIZE_16B);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_single_typeid, get_typeid_func_unsafe)
{
	void *p = w_component_get_unsafe_(&ubench_fixture->registry, ubench_fixture->type_id, 0);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_single_typeid, has_typeid_func_unsafe)
{
	bool r = w_component_has_unsafe_(&ubench_fixture->registry, ubench_fixture->type_id, 0);
	UBENCH_DO_NOTHING(&r);
}

UBENCH_F(bench_single_typeid, get_typeid_direct)
{
	struct w_component_entry *entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
	void *p = &entry->data[0 * entry->type_size];
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_single_typeid, set_typeid_direct)
{
	struct w_component_entry *entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
	memcpy(entry->data + (COMPONENT_SIZE_16B * 1), &ubench_fixture->data, COMPONENT_SIZE_16B);
	void *p = entry->data + (COMPONENT_SIZE_16B * 1);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_single_typeid, set_typeid_macro_unsafe)
{
	w_component_set_entry(ubench_fixture->cached_entry, 1, &ubench_fixture->data, struct bench_component_16b);
	void *p = w_component_get_entry(ubench_fixture->cached_entry, 1, struct bench_component_16b);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_single_typeid, get_typeid_macro_unsafe)
{
	void *p = w_component_get_entry(ubench_fixture->cached_entry, 0, struct bench_component_16b);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_single_typeid, has_typeid_macro_unsafe)
{
	bool r = w_component_has_entry(ubench_fixture->cached_entry, 0);
	UBENCH_DO_NOTHING(&r);
}


// ============================================================================
// single-op benchmarks: string name access (lookup vs create)
// ============================================================================

struct bench_single_str
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_entity_registry entities;
	struct w_component_registry registry;
	struct bench_component_16b data;
	int counter;
	char name_buf[32];
};

UBENCH_F_SETUP(bench_single_str)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_entity_registry_init(&ubench_fixture->entities, &ubench_fixture->string_table);
	w_component_registry_init(&ubench_fixture->registry, &ubench_fixture->arena, &ubench_fixture->entities);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
	ubench_fixture->counter = 0;
	// pre-register the component name for lookup tests
	w_entity_id tid = w_component_get_id(&ubench_fixture->registry, "Position");
	w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, tid, 0, &ubench_fixture->data, COMPONENT_SIZE_16B);
}

UBENCH_F_TEARDOWN(bench_single_str)
{
	w_component_registry_free(&ubench_fixture->registry);
	w_entity_registry_free(&ubench_fixture->entities);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_single_str, get_id_str)
{
	w_entity_id id = w_component_get_id(&ubench_fixture->registry, "Position");
	UBENCH_DO_NOTHING(&id);
}

UBENCH_F(bench_single_str, set_str)
{
	w_entity_id tid = w_component_get_id(&ubench_fixture->registry, "Position");
	void *p = w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, tid, 1, &ubench_fixture->data, COMPONENT_SIZE_16B);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_single_str, get_str)
{
	w_entity_id tid = w_component_get_id(&ubench_fixture->registry, "Position");
	void *p = w_component_get_(&ubench_fixture->registry, tid, 0);
	UBENCH_DO_NOTHING(p);
}

UBENCH_F(bench_single_str, has_str)
{
	w_entity_id tid = w_component_get_id(&ubench_fixture->registry, "Position");
	bool r = w_component_has_(&ubench_fixture->registry, tid, 0);
	UBENCH_DO_NOTHING(&r);
}

UBENCH_F(bench_single_str, get_id_str_create)
{
	// each iteration creates a new name
	snprintf(ubench_fixture->name_buf, sizeof(ubench_fixture->name_buf), "Component_%d", ubench_fixture->counter++);
	w_entity_id id = w_component_get_id(&ubench_fixture->registry, ubench_fixture->name_buf);
	UBENCH_DO_NOTHING(&id);
}


// ============================================================================
// bulk 100k set benchmarks: typeid, str, direct, unsafe variants
// ============================================================================

struct bench_bulk_100k_set
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_entity_registry entities;
	struct w_component_registry registry;
	w_entity_id type_id;
	struct bench_component_16b data;
	struct w_component_entry *cached_entry;
};

UBENCH_F_SETUP(bench_bulk_100k_set)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_LARGE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_entity_registry_init(&ubench_fixture->entities, &ubench_fixture->string_table);
	w_component_registry_init(&ubench_fixture->registry, &ubench_fixture->arena, &ubench_fixture->entities);
	ubench_fixture->type_id = w_entity_request(&ubench_fixture->entities);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
	// pre-populate so entry exists for direct/macro access
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
	}
	ubench_fixture->cached_entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
}

UBENCH_F_TEARDOWN(bench_bulk_100k_set)
{
	w_component_registry_free(&ubench_fixture->registry);
	w_entity_registry_free(&ubench_fixture->entities);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_bulk_100k_set, set_100k_typeid)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		void *p = w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_bulk_100k_set, set_100k_typeid_direct)
{
	struct w_component_entry *entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
	for (size_t i = 0; i < BENCH_COUNT_100K; i++)
	{
		void *data = entry->data + (i * entry->type_size);
		memcpy(data, &ubench_fixture->data, COMPONENT_SIZE_16B);
		UBENCH_DO_NOTHING(data);
	}
}

UBENCH_F(bench_bulk_100k_set, set_100k_typeid_func_unsafe)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		void *p = w_component_set_unsafe_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_bulk_100k_set, set_100k_typeid_macro_unsafe)
{
	for (size_t i = 0; i < BENCH_COUNT_100K; i++)
	{
		w_component_set_entry(ubench_fixture->cached_entry, i, &ubench_fixture->data, struct bench_component_16b);
		void *p = w_component_get_entry(ubench_fixture->cached_entry, i, struct bench_component_16b);
		UBENCH_DO_NOTHING(p);
	}
}


// ============================================================================
// bulk 100k get benchmarks: typeid, str, direct, unsafe variants
// ============================================================================

struct bench_bulk_100k_get
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_entity_registry entities;
	struct w_component_registry registry;
	w_entity_id type_id;
	struct bench_component_16b data;
	struct w_component_entry *cached_entry;
};

UBENCH_F_SETUP(bench_bulk_100k_get)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_LARGE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_entity_registry_init(&ubench_fixture->entities, &ubench_fixture->string_table);
	w_component_registry_init(&ubench_fixture->registry, &ubench_fixture->arena, &ubench_fixture->entities);
	ubench_fixture->type_id = w_entity_request(&ubench_fixture->entities);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
	// pre-populate for get benchmarks
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
	}
	ubench_fixture->cached_entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
}

UBENCH_F_TEARDOWN(bench_bulk_100k_get)
{
	w_component_registry_free(&ubench_fixture->registry);
	w_entity_registry_free(&ubench_fixture->entities);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_bulk_100k_get, get_100k_typeid)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		void *p = w_component_get_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_bulk_100k_get, get_100k_typeid_direct)
{
	struct w_component_entry *entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
	for (size_t i = 0; i < BENCH_COUNT_100K; i++)
	{
		void *p = entry->data + (i * entry->type_size);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_bulk_100k_get, get_100k_typeid_func_unsafe)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		void *p = w_component_get_unsafe_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_bulk_100k_get, get_100k_typeid_macro_unsafe)
{
	for (size_t i = 0; i < BENCH_COUNT_100K; i++)
	{
		void *p = w_component_get_entry(ubench_fixture->cached_entry, i, struct bench_component_16b);
		UBENCH_DO_NOTHING(p);
	}
}


// ============================================================================
// bulk 100k has benchmarks: typeid, func_unsafe, macro_unsafe
// ============================================================================

struct bench_bulk_100k_has
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_entity_registry entities;
	struct w_component_registry registry;
	w_entity_id type_id;
	struct bench_component_16b data;
	struct w_component_entry *cached_entry;
};

UBENCH_F_SETUP(bench_bulk_100k_has)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_LARGE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_entity_registry_init(&ubench_fixture->entities, &ubench_fixture->string_table);
	w_component_registry_init(&ubench_fixture->registry, &ubench_fixture->arena, &ubench_fixture->entities);
	ubench_fixture->type_id = w_entity_request(&ubench_fixture->entities);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
	// pre-populate for has benchmarks
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
	}
	ubench_fixture->cached_entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
}

UBENCH_F_TEARDOWN(bench_bulk_100k_has)
{
	w_component_registry_free(&ubench_fixture->registry);
	w_entity_registry_free(&ubench_fixture->entities);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_bulk_100k_has, has_100k_typeid)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		bool r = w_component_has_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(&r);
	}
}

UBENCH_F(bench_bulk_100k_has, has_100k_typeid_func_unsafe)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		bool r = w_component_has_unsafe_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(&r);
	}
}

UBENCH_F(bench_bulk_100k_has, has_100k_typeid_macro_unsafe)
{
	for (size_t i = 0; i < BENCH_COUNT_100K; i++)
	{
		bool r = w_component_has_entry(ubench_fixture->cached_entry, i);
		UBENCH_DO_NOTHING(&r);
	}
}


// ============================================================================
// bulk 100k remove benchmarks: typeid, macro_unsafe
// ============================================================================

struct bench_bulk_100k_remove
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_entity_registry entities;
	struct w_component_registry registry;
	w_entity_id type_id;
	struct bench_component_16b data;
	struct w_component_entry *cached_entry;
};

UBENCH_F_SETUP(bench_bulk_100k_remove)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_LARGE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_entity_registry_init(&ubench_fixture->entities, &ubench_fixture->string_table);
	w_component_registry_init(&ubench_fixture->registry, &ubench_fixture->arena, &ubench_fixture->entities);
	ubench_fixture->type_id = w_entity_request(&ubench_fixture->entities);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
	// pre-populate for remove benchmarks
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
	}
	ubench_fixture->cached_entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
}

UBENCH_F_TEARDOWN(bench_bulk_100k_remove)
{
	w_component_registry_free(&ubench_fixture->registry);
	w_entity_registry_free(&ubench_fixture->entities);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_bulk_100k_remove, remove_100k_typeid)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_100K; i++)
	{
		w_component_remove_(&ubench_fixture->registry, ubench_fixture->type_id, i);
	}
}

UBENCH_F(bench_bulk_100k_remove, remove_100k_typeid_macro_unsafe)
{
	for (size_t i = 0; i < BENCH_COUNT_100K; i++)
	{
		w_component_remove_entry(ubench_fixture->cached_entry, i);
	}
}


// ============================================================================
// bulk 1m set benchmarks: typeid, str, direct, unsafe variants
// ============================================================================

struct bench_bulk_1m_set
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_entity_registry entities;
	struct w_component_registry registry;
	w_entity_id type_id;
	struct bench_component_16b data;
	struct w_component_entry *cached_entry;
};

UBENCH_F_SETUP(bench_bulk_1m_set)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_HUGE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_entity_registry_init(&ubench_fixture->entities, &ubench_fixture->string_table);
	w_component_registry_init(&ubench_fixture->registry, &ubench_fixture->arena, &ubench_fixture->entities);
	ubench_fixture->type_id = w_entity_request(&ubench_fixture->entities);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
	// pre-populate so entry exists for direct/macro access
	for (w_entity_id i = 0; i < BENCH_COUNT_1M; i++)
	{
		w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
	}
	ubench_fixture->cached_entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
}

UBENCH_F_TEARDOWN(bench_bulk_1m_set)
{
	w_component_registry_free(&ubench_fixture->registry);
	w_entity_registry_free(&ubench_fixture->entities);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_bulk_1m_set, set_1m_typeid)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_1M; i++)
	{
		void *p = w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_bulk_1m_set, set_1m_typeid_direct)
{
	struct w_component_entry *entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
	for (size_t i = 0; i < BENCH_COUNT_1M; i++)
	{
		void *data = entry->data + (i * entry->type_size);
		memcpy(data, &ubench_fixture->data, COMPONENT_SIZE_16B);
		UBENCH_DO_NOTHING(data);
	}
}

UBENCH_F(bench_bulk_1m_set, set_1m_typeid_func_unsafe)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_1M; i++)
	{
		void *p = w_component_set_unsafe_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_bulk_1m_set, set_1m_typeid_macro_unsafe)
{
	for (size_t i = 0; i < BENCH_COUNT_1M; i++)
	{
		w_component_set_entry(ubench_fixture->cached_entry, i, &ubench_fixture->data, struct bench_component_16b);
		void *p = w_component_get_entry(ubench_fixture->cached_entry, i, struct bench_component_16b);
		UBENCH_DO_NOTHING(p);
	}
}


// ============================================================================
// bulk 1m get benchmarks: typeid, str, direct, unsafe variants
// ============================================================================

struct bench_bulk_1m_get
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_entity_registry entities;
	struct w_component_registry registry;
	w_entity_id type_id;
	struct bench_component_16b data;
	struct w_component_entry *cached_entry;
};

UBENCH_F_SETUP(bench_bulk_1m_get)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_HUGE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_entity_registry_init(&ubench_fixture->entities, &ubench_fixture->string_table);
	w_component_registry_init(&ubench_fixture->registry, &ubench_fixture->arena, &ubench_fixture->entities);
	ubench_fixture->type_id = w_entity_request(&ubench_fixture->entities);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
	// pre-populate for get benchmarks
	for (w_entity_id i = 0; i < BENCH_COUNT_1M; i++)
	{
		w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
	}
	ubench_fixture->cached_entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
}

UBENCH_F_TEARDOWN(bench_bulk_1m_get)
{
	w_component_registry_free(&ubench_fixture->registry);
	w_entity_registry_free(&ubench_fixture->entities);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_bulk_1m_get, get_1m_typeid)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_1M; i++)
	{
		void *p = w_component_get_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_bulk_1m_get, get_1m_typeid_direct)
{
	struct w_component_entry *entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
	for (size_t i = 0; i < BENCH_COUNT_1M; i++)
	{
		void *p = entry->data + (i * entry->type_size);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_bulk_1m_get, get_1m_typeid_func_unsafe)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_1M; i++)
	{
		void *p = w_component_get_unsafe_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(p);
	}
}

UBENCH_F(bench_bulk_1m_get, get_1m_typeid_macro_unsafe)
{
	for (size_t i = 0; i < BENCH_COUNT_1M; i++)
	{
		void *p = w_component_get_entry(ubench_fixture->cached_entry, i, struct bench_component_16b);
		UBENCH_DO_NOTHING(p);
	}
}


// ============================================================================
// bulk 1m has benchmarks: typeid, func_unsafe, macro_unsafe
// ============================================================================

struct bench_bulk_1m_has
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_entity_registry entities;
	struct w_component_registry registry;
	w_entity_id type_id;
	struct bench_component_16b data;
	struct w_component_entry *cached_entry;
};

UBENCH_F_SETUP(bench_bulk_1m_has)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_HUGE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_entity_registry_init(&ubench_fixture->entities, &ubench_fixture->string_table);
	w_component_registry_init(&ubench_fixture->registry, &ubench_fixture->arena, &ubench_fixture->entities);
	ubench_fixture->type_id = w_entity_request(&ubench_fixture->entities);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
	// pre-populate for has benchmarks
	for (w_entity_id i = 0; i < BENCH_COUNT_1M; i++)
	{
		w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
	}
	ubench_fixture->cached_entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
}

UBENCH_F_TEARDOWN(bench_bulk_1m_has)
{
	w_component_registry_free(&ubench_fixture->registry);
	w_entity_registry_free(&ubench_fixture->entities);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_bulk_1m_has, has_1m_typeid)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_1M; i++)
	{
		bool r = w_component_has_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(&r);
	}
}

UBENCH_F(bench_bulk_1m_has, has_1m_typeid_func_unsafe)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_1M; i++)
	{
		bool r = w_component_has_unsafe_(&ubench_fixture->registry, ubench_fixture->type_id, i);
		UBENCH_DO_NOTHING(&r);
	}
}

UBENCH_F(bench_bulk_1m_has, has_1m_typeid_macro_unsafe)
{
	for (size_t i = 0; i < BENCH_COUNT_1M; i++)
	{
		bool r = w_component_has_entry(ubench_fixture->cached_entry, i);
		UBENCH_DO_NOTHING(&r);
	}
}


// ============================================================================
// bulk 1m remove benchmarks: typeid, macro_unsafe
// ============================================================================

struct bench_bulk_1m_remove
{
	struct w_arena arena;
	struct w_string_table string_table;
	struct w_entity_registry entities;
	struct w_component_registry registry;
	w_entity_id type_id;
	struct bench_component_16b data;
	struct w_component_entry *cached_entry;
};

UBENCH_F_SETUP(bench_bulk_1m_remove)
{
	w_arena_init(&ubench_fixture->arena, BENCH_ARENA_SIZE_HUGE);
	w_string_table_init(&ubench_fixture->string_table, &ubench_fixture->arena, 4096, 4096, w_hashmap_hash_str);
	w_entity_registry_init(&ubench_fixture->entities, &ubench_fixture->string_table);
	w_component_registry_init(&ubench_fixture->registry, &ubench_fixture->arena, &ubench_fixture->entities);
	ubench_fixture->type_id = w_entity_request(&ubench_fixture->entities);
	ubench_fixture->data.a = 123;
	ubench_fixture->data.b = 456;
	// pre-populate for remove benchmarks
	for (w_entity_id i = 0; i < BENCH_COUNT_1M; i++)
	{
		w_component_set_(&ubench_fixture->registry, W_COMPONENT_TYPE_uint8_t, ubench_fixture->type_id, i, &ubench_fixture->data, COMPONENT_SIZE_16B);
	}
	ubench_fixture->cached_entry = &ubench_fixture->registry.entries[ubench_fixture->type_id];
}

UBENCH_F_TEARDOWN(bench_bulk_1m_remove)
{
	w_component_registry_free(&ubench_fixture->registry);
	w_entity_registry_free(&ubench_fixture->entities);
	w_string_table_free(&ubench_fixture->string_table);
	w_arena_free(&ubench_fixture->arena);
}

UBENCH_F(bench_bulk_1m_remove, remove_1m_typeid)
{
	for (w_entity_id i = 0; i < BENCH_COUNT_1M; i++)
	{
		w_component_remove_(&ubench_fixture->registry, ubench_fixture->type_id, i);
	}
}

UBENCH_F(bench_bulk_1m_remove, remove_1m_typeid_macro_unsafe)
{
	for (size_t i = 0; i < BENCH_COUNT_1M; i++)
	{
		w_component_remove_entry(ubench_fixture->cached_entry, i);
	}
}


UBENCH_MAIN();
