/**
 * @author      : ElGatoPanzon
 * @file        : test_managed_alloc
 * @created     : Friday May 16, 2026 20:15:00 CST
 * @description : tests for managed_alloc module
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"
#include "modules/managed_alloc/whisker_managed_alloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>


/*****************************
*  managed component defs    *
*****************************/

w_ecs_define_managed_component(managed_buffer);
w_ecs_define_managed_component(managed_string);
w_ecs_define_managed_component(managed_data);


/*****************************
*  fixture                   *
*****************************/

static struct w_ecs_world g_world;
static struct w_string_table g_string_table;
static struct w_arena g_arena;

static void managed_alloc_setup(void)
{
	w_arena_init(&g_arena, 64 * 1024);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);

	wm_managed_alloc_init(&g_world);
}

static void managed_alloc_teardown(void)
{
	wm_managed_alloc_free(&g_world);
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}


/*****************************
*  basic allocation          *
*****************************/

START_TEST(test_malloc_returns_valid_pointer)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	void *ptr = wm_managed_alloc_malloc(&g_world, managed_buffer, e, 1024);

	ck_assert_ptr_nonnull(ptr);
}
END_TEST

START_TEST(test_malloc_memory_is_writable)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	char *ptr = wm_managed_alloc_malloc(&g_world, managed_buffer, e, 256);
	ck_assert_ptr_nonnull(ptr);

	// write and verify
	memset(ptr, 0xAB, 256);
	for (int i = 0; i < 256; i++)
	{
		ck_assert_uint_eq((unsigned char)ptr[i], 0xAB);
	}
}
END_TEST

START_TEST(test_malloc_sets_handle_on_entity)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	wm_managed_alloc_malloc(&g_world, managed_buffer, e, 512);

	// the managed_buffer component should exist on the entity
	ck_assert(managed_buffer_exists(&g_world, e));
}
END_TEST


/*****************************
*  multiple components       *
*****************************/

START_TEST(test_multiple_component_types_same_entity)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	char *buf = wm_managed_alloc_malloc(&g_world, managed_buffer, e, 1024);
	char *str = wm_managed_alloc_malloc(&g_world, managed_string, e, 256);

	ck_assert_ptr_nonnull(buf);
	ck_assert_ptr_nonnull(str);
	ck_assert_ptr_ne(buf, str);

	// write different data to verify independence
	memset(buf, 'B', 1024);
	memset(str, 'S', 256);

	ck_assert_uint_eq((unsigned char)buf[0], 'B');
	ck_assert_uint_eq((unsigned char)str[0], 'S');
}
END_TEST

START_TEST(test_same_component_type_different_entities)
{
	w_entity_id e1 = w_ecs_request_entity(&g_world);
	w_entity_id e2 = w_ecs_request_entity(&g_world);

	char *ptr1 = wm_managed_alloc_malloc(&g_world, managed_buffer, e1, 512);
	char *ptr2 = wm_managed_alloc_malloc(&g_world, managed_buffer, e2, 512);

	ck_assert_ptr_nonnull(ptr1);
	ck_assert_ptr_nonnull(ptr2);
	ck_assert_ptr_ne(ptr1, ptr2);

	// verify independence
	memset(ptr1, 'X', 512);
	memset(ptr2, 'Y', 512);

	ck_assert_uint_eq((unsigned char)ptr1[0], 'X');
	ck_assert_uint_eq((unsigned char)ptr2[0], 'Y');
}
END_TEST


/*****************************
*  auto cleanup on remove    *
*****************************/

START_TEST(test_cleanup_on_component_remove)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	wm_managed_alloc_malloc(&g_world, managed_buffer, e, 1024);

	ck_assert(managed_buffer_exists(&g_world, e));

	// remove the component - should trigger cleanup hook
	managed_buffer_remove(&g_world, e);

	ck_assert(!managed_buffer_exists(&g_world, e));

	// entity should still be valid
	ck_assert(w_ecs_is_valid_entity(e));
}
END_TEST

START_TEST(test_cleanup_on_entity_destroy)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	wm_managed_alloc_malloc(&g_world, managed_buffer, e, 1024);
	wm_managed_alloc_malloc(&g_world, managed_string, e, 256);

	ck_assert(managed_buffer_exists(&g_world, e));
	ck_assert(managed_string_exists(&g_world, e));

	// return entity - should trigger cleanup for all managed components
	w_ecs_return_entity(&g_world, e);
}
END_TEST

START_TEST(test_cleanup_multiple_entities)
{
	w_entity_id e1 = w_ecs_request_entity(&g_world);
	w_entity_id e2 = w_ecs_request_entity(&g_world);
	w_entity_id e3 = w_ecs_request_entity(&g_world);

	wm_managed_alloc_malloc(&g_world, managed_buffer, e1, 1024);
	wm_managed_alloc_malloc(&g_world, managed_buffer, e2, 512);
	wm_managed_alloc_malloc(&g_world, managed_buffer, e3, 256);

	// remove component from middle entity
	managed_buffer_remove(&g_world, e2);

	// e1 and e3 should still have their components
	ck_assert(managed_buffer_exists(&g_world, e1));
	ck_assert(!managed_buffer_exists(&g_world, e2));
	ck_assert(managed_buffer_exists(&g_world, e3));
}
END_TEST


/*****************************
*  varying sizes             *
*****************************/

START_TEST(test_small_allocation)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	char *ptr = wm_managed_alloc_malloc(&g_world, managed_buffer, e, 8);
	ck_assert_ptr_nonnull(ptr);

	memset(ptr, 'A', 8);
	ck_assert_uint_eq((unsigned char)ptr[7], 'A');
}
END_TEST

START_TEST(test_large_allocation)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	// 1MB allocation
	size_t size = 1024 * 1024;
	char *ptr = wm_managed_alloc_malloc(&g_world, managed_buffer, e, size);
	ck_assert_ptr_nonnull(ptr);

	// write at start and end
	ptr[0] = 'S';
	ptr[size - 1] = 'E';

	ck_assert_uint_eq((unsigned char)ptr[0], 'S');
	ck_assert_uint_eq((unsigned char)ptr[size - 1], 'E');
}
END_TEST


/*****************************
*  stress tests              *
*****************************/

START_TEST(test_many_entities_same_component)
{
	int count = 100;
	w_entity_id *entities = malloc(count * sizeof(w_entity_id));

	for (int i = 0; i < count; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);
		char *ptr = wm_managed_alloc_malloc(&g_world, managed_buffer, entities[i], 128);
		ck_assert_ptr_nonnull(ptr);
		memset(ptr, (char)i, 128);
	}

	// verify all have components
	for (int i = 0; i < count; i++)
	{
		ck_assert(managed_buffer_exists(&g_world, entities[i]));
	}

	// remove half
	for (int i = 0; i < count; i += 2)
	{
		managed_buffer_remove(&g_world, entities[i]);
	}

	// verify correct ones removed
	for (int i = 0; i < count; i++)
	{
		if (i % 2 == 0)
			ck_assert(!managed_buffer_exists(&g_world, entities[i]));
		else
			ck_assert(managed_buffer_exists(&g_world, entities[i]));
	}

	free(entities);
}
END_TEST

START_TEST(test_mixed_component_types_many_entities)
{
	int count = 50;

	for (int i = 0; i < count; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);

		wm_managed_alloc_malloc(&g_world, managed_buffer, e, 256);
		wm_managed_alloc_malloc(&g_world, managed_string, e, 64);
		wm_managed_alloc_malloc(&g_world, managed_data, e, 512);

		ck_assert(managed_buffer_exists(&g_world, e));
		ck_assert(managed_string_exists(&g_world, e));
		ck_assert(managed_data_exists(&g_world, e));
	}
}
END_TEST


/*****************************
*  hook verification         *
*****************************/

START_TEST(test_hook_frees_handle_on_remove)
{
	// verify the cleanup hook actually frees the slab arena handle
	w_entity_id e = w_ecs_request_entity(&g_world);
	w_entity_id comp_id = managed_buffer_get_id(&g_world);

	wm_managed_alloc_malloc(&g_world, managed_buffer, e, 256);

	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(&g_world);
	struct w_slab_arena *arena = &registry->slab_arenas[comp_id];

	// before removal: 1 handle alive, 0 recycled
	size_t alive_before = w_id_pool_alive_count(&arena->handle_pool);
	size_t recycled_before = w_id_pool_recycled_count(&arena->handle_pool);
	ck_assert_uint_eq(alive_before, 1);
	ck_assert_uint_eq(recycled_before, 0);

	managed_buffer_remove(&g_world, e);

	// after removal: handle should be recycled
	size_t alive_after = w_id_pool_alive_count(&arena->handle_pool);
	size_t recycled_after = w_id_pool_recycled_count(&arena->handle_pool);
	ck_assert_uint_eq(alive_after, 0);
	ck_assert_uint_eq(recycled_after, 1);
}
END_TEST

START_TEST(test_hook_frees_handle_on_entity_destroy)
{
	w_entity_id e = w_ecs_request_entity(&g_world);
	w_entity_id comp_id = managed_buffer_get_id(&g_world);

	wm_managed_alloc_malloc(&g_world, managed_buffer, e, 128);

	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(&g_world);
	struct w_slab_arena *arena = &registry->slab_arenas[comp_id];

	ck_assert_uint_eq(w_id_pool_alive_count(&arena->handle_pool), 1);

	w_ecs_return_entity(&g_world, e);

	// handle should be freed by hook
	ck_assert_uint_eq(w_id_pool_alive_count(&arena->handle_pool), 0);
	ck_assert_uint_eq(w_id_pool_recycled_count(&arena->handle_pool), 1);
}
END_TEST

START_TEST(test_malloc_replaces_existing_handle)
{
	// verify calling malloc on entity that already has handle frees old handle
	w_entity_id e = w_ecs_request_entity(&g_world);
	w_entity_id comp_id = managed_buffer_get_id(&g_world);

	// first allocation
	void *ptr1 = wm_managed_alloc_malloc(&g_world, managed_buffer, e, 256);
	ck_assert_ptr_nonnull(ptr1);

	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(&g_world);
	struct w_slab_arena *arena = &registry->slab_arenas[comp_id];

	// after first alloc: 1 alive, 0 recycled
	ck_assert_uint_eq(w_id_pool_alive_count(&arena->handle_pool), 1);
	ck_assert_uint_eq(w_id_pool_recycled_count(&arena->handle_pool), 0);

	// second allocation on SAME entity - should free old handle first
	void *ptr2 = wm_managed_alloc_malloc(&g_world, managed_buffer, e, 512);
	ck_assert_ptr_nonnull(ptr2);

	// alive_count should still be 1 (not 2) - old was freed before new allocated
	// key invariant: only 1 alive - no leak
	ck_assert_uint_eq(w_id_pool_alive_count(&arena->handle_pool), 1);
	// total should be at most 2 (one created for first, one for second or recycled)
	ck_assert_uint_le(w_id_pool_total_count(&arena->handle_pool), 2);
}
END_TEST

START_TEST(test_hook_frees_multiple_component_types)
{
	w_entity_id e = w_ecs_request_entity(&g_world);
	w_entity_id buf_id = managed_buffer_get_id(&g_world);
	w_entity_id str_id = managed_string_get_id(&g_world);
	w_entity_id data_id = managed_data_get_id(&g_world);

	wm_managed_alloc_malloc(&g_world, managed_buffer, e, 256);
	wm_managed_alloc_malloc(&g_world, managed_string, e, 64);
	wm_managed_alloc_malloc(&g_world, managed_data, e, 512);

	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(&g_world);

	// all three arenas should have 1 handle each
	ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[buf_id].handle_pool), 1);
	ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[str_id].handle_pool), 1);
	ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[data_id].handle_pool), 1);

	// destroy entity - all hooks should fire
	w_ecs_return_entity(&g_world, e);

	// all handles should be freed
	ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[buf_id].handle_pool), 0);
	ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[str_id].handle_pool), 0);
	ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[data_id].handle_pool), 0);
}
END_TEST


/*****************************
*  resolve handle            *
*****************************/

START_TEST(test_resolve_handle_matches_malloc_pointer)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	void *ptr = wm_managed_alloc_malloc(&g_world, managed_buffer, e, 256);
	ck_assert_ptr_nonnull(ptr);

	void *resolved = wm_managed_alloc_resolve_handle(&g_world, managed_buffer, e);
	ck_assert_ptr_eq(ptr, resolved);
}
END_TEST

START_TEST(test_resolve_handle_read_write)
{
	w_entity_id e = w_ecs_request_entity(&g_world);

	char *ptr = wm_managed_alloc_malloc(&g_world, managed_buffer, e, 128);
	ck_assert_ptr_nonnull(ptr);

	/* write through malloc pointer */
	memset(ptr, 0xCD, 128);

	/* read back through resolved handle */
	char *resolved = wm_managed_alloc_resolve_handle(&g_world, managed_buffer, e);
	ck_assert_ptr_nonnull(resolved);

	for (int i = 0; i < 128; i++)
	{
		ck_assert_uint_eq((unsigned char)resolved[i], 0xCD);
	}

	/* write through resolved pointer, verify via original */
	memset(resolved, 0x42, 128);
	ck_assert_uint_eq((unsigned char)ptr[0], 0x42);
	ck_assert_uint_eq((unsigned char)ptr[127], 0x42);
}
END_TEST

START_TEST(test_resolve_handle_multiple_entities)
{
	w_entity_id e1 = w_ecs_request_entity(&g_world);
	w_entity_id e2 = w_ecs_request_entity(&g_world);

	void *ptr1 = wm_managed_alloc_malloc(&g_world, managed_buffer, e1, 64);
	void *ptr2 = wm_managed_alloc_malloc(&g_world, managed_buffer, e2, 64);

	void *resolved1 = wm_managed_alloc_resolve_handle(&g_world, managed_buffer, e1);
	void *resolved2 = wm_managed_alloc_resolve_handle(&g_world, managed_buffer, e2);

	ck_assert_ptr_eq(ptr1, resolved1);
	ck_assert_ptr_eq(ptr2, resolved2);
	ck_assert_ptr_ne(resolved1, resolved2);
}
END_TEST


/*****************************
*  add/remove cycle stress   *
*****************************/

START_TEST(test_add_remove_cycle_single_entity)
{
	// 200 cycles of add/remove on same entity
	w_entity_id e = w_ecs_request_entity(&g_world);
	w_entity_id comp_id = managed_buffer_get_id(&g_world);

	for (int i = 0; i < 200; i++)
	{
		char *ptr = wm_managed_alloc_malloc(&g_world, managed_buffer, e, 128);
		ck_assert_ptr_nonnull(ptr);
		memset(ptr, (char)i, 128);
		ck_assert(managed_buffer_exists(&g_world, e));

		managed_buffer_remove(&g_world, e);
		ck_assert(!managed_buffer_exists(&g_world, e));
	}

	// verify handles are recycling (not leaking)
	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(&g_world);
	struct w_slab_arena *arena = &registry->slab_arenas[comp_id];

	// no live handles
	ck_assert_uint_eq(w_id_pool_alive_count(&arena->handle_pool), 0);
	// total should be small due to recycling (not 200)
	ck_assert_uint_lt(w_id_pool_total_count(&arena->handle_pool), 10);
}
END_TEST

START_TEST(test_add_remove_cycle_different_entities)
{
	// 150 cycles: create entity, alloc, remove component, return entity
	w_entity_id comp_id = managed_buffer_get_id(&g_world);

	for (int i = 0; i < 150; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);

		char *ptr = wm_managed_alloc_malloc(&g_world, managed_buffer, e, 256);
		ck_assert_ptr_nonnull(ptr);
		ptr[0] = 'X';
		ptr[255] = 'Y';

		managed_buffer_remove(&g_world, e);
		w_ecs_return_entity(&g_world, e);
	}

	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(&g_world);
	struct w_slab_arena *arena = &registry->slab_arenas[comp_id];

	ck_assert_uint_eq(w_id_pool_alive_count(&arena->handle_pool), 0);
}
END_TEST

START_TEST(test_add_remove_cycle_multiple_components)
{
	// 100 cycles with all 3 component types
	for (int i = 0; i < 100; i++)
	{
		w_entity_id e = w_ecs_request_entity(&g_world);

		wm_managed_alloc_malloc(&g_world, managed_buffer, e, 64);
		wm_managed_alloc_malloc(&g_world, managed_string, e, 32);
		wm_managed_alloc_malloc(&g_world, managed_data, e, 128);

		ck_assert(managed_buffer_exists(&g_world, e));
		ck_assert(managed_string_exists(&g_world, e));
		ck_assert(managed_data_exists(&g_world, e));

		// remove one by one
		managed_buffer_remove(&g_world, e);
		managed_string_remove(&g_world, e);
		managed_data_remove(&g_world, e);

		w_ecs_return_entity(&g_world, e);
	}

	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(&g_world);
	w_entity_id buf_id = managed_buffer_get_id(&g_world);
	w_entity_id str_id = managed_string_get_id(&g_world);
	w_entity_id data_id = managed_data_get_id(&g_world);

	ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[buf_id].handle_pool), 0);
	ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[str_id].handle_pool), 0);
	ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[data_id].handle_pool), 0);
}
END_TEST


/*****************************
*  rapid removal stress      *
*****************************/

START_TEST(test_rapid_remove_many_entities)
{
	// allocate 200 entities with managed components
	int count = 200;
	w_entity_id *entities = malloc(count * sizeof(w_entity_id));

	for (int i = 0; i < count; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);
		wm_managed_alloc_malloc(&g_world, managed_buffer, entities[i], 64 + (i % 64));
	}

	w_entity_id comp_id = managed_buffer_get_id(&g_world);
	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(&g_world);
	struct w_slab_arena *arena = &registry->slab_arenas[comp_id];

	ck_assert_uint_eq(w_id_pool_alive_count(&arena->handle_pool), count);

	// rapid removal of all
	for (int i = 0; i < count; i++)
	{
		managed_buffer_remove(&g_world, entities[i]);
	}

	ck_assert_uint_eq(w_id_pool_alive_count(&arena->handle_pool), 0);
	ck_assert_uint_eq(w_id_pool_recycled_count(&arena->handle_pool), count);

	free(entities);
}
END_TEST

START_TEST(test_rapid_remove_interleaved)
{
	// interleave adds and removes to stress the hook system
	int count = 100;
	w_entity_id *entities = malloc(count * sizeof(w_entity_id));

	// allocate all
	for (int i = 0; i < count; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);
		wm_managed_alloc_malloc(&g_world, managed_buffer, entities[i], 128);
	}

	// remove odd indices, re-add even indices with new data
	for (int i = 0; i < count; i++)
	{
		if (i % 2 == 1)
		{
			managed_buffer_remove(&g_world, entities[i]);
		}
		else
		{
			// remove and re-add
			managed_buffer_remove(&g_world, entities[i]);
			wm_managed_alloc_malloc(&g_world, managed_buffer, entities[i], 256);
		}
	}

	// verify only even entities have components
	for (int i = 0; i < count; i++)
	{
		if (i % 2 == 0)
			ck_assert(managed_buffer_exists(&g_world, entities[i]));
		else
			ck_assert(!managed_buffer_exists(&g_world, entities[i]));
	}

	free(entities);
}
END_TEST

START_TEST(test_rapid_entity_destroy)
{
	// allocate entities with multiple components, then rapid destroy
	int count = 150;
	w_entity_id *entities = malloc(count * sizeof(w_entity_id));

	for (int i = 0; i < count; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);
		wm_managed_alloc_malloc(&g_world, managed_buffer, entities[i], 32);
		wm_managed_alloc_malloc(&g_world, managed_string, entities[i], 16);
		wm_managed_alloc_malloc(&g_world, managed_data, entities[i], 64);
	}

	// rapid destroy all entities
	for (int i = 0; i < count; i++)
	{
		w_ecs_return_entity(&g_world, entities[i]);
	}

	// verify all handles freed
	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(&g_world);
	w_entity_id buf_id = managed_buffer_get_id(&g_world);
	w_entity_id str_id = managed_string_get_id(&g_world);
	w_entity_id data_id = managed_data_get_id(&g_world);

	ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[buf_id].handle_pool), 0);
	ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[str_id].handle_pool), 0);
	ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[data_id].handle_pool), 0);

	free(entities);
}
END_TEST


/*****************************
*  many component types      *
*****************************/

w_ecs_define_managed_component(managed_type_a);
w_ecs_define_managed_component(managed_type_b);
w_ecs_define_managed_component(managed_type_c);
w_ecs_define_managed_component(managed_type_d);
w_ecs_define_managed_component(managed_type_e);

START_TEST(test_many_component_types_simultaneous)
{
	// 8 different managed component types on same entity
	w_entity_id e = w_ecs_request_entity(&g_world);

	wm_managed_alloc_malloc(&g_world, managed_buffer, e, 64);
	wm_managed_alloc_malloc(&g_world, managed_string, e, 32);
	wm_managed_alloc_malloc(&g_world, managed_data, e, 128);
	wm_managed_alloc_malloc(&g_world, managed_type_a, e, 256);
	wm_managed_alloc_malloc(&g_world, managed_type_b, e, 512);
	wm_managed_alloc_malloc(&g_world, managed_type_c, e, 16);
	wm_managed_alloc_malloc(&g_world, managed_type_d, e, 1024);
	wm_managed_alloc_malloc(&g_world, managed_type_e, e, 8);

	ck_assert(managed_buffer_exists(&g_world, e));
	ck_assert(managed_string_exists(&g_world, e));
	ck_assert(managed_data_exists(&g_world, e));
	ck_assert(managed_type_a_exists(&g_world, e));
	ck_assert(managed_type_b_exists(&g_world, e));
	ck_assert(managed_type_c_exists(&g_world, e));
	ck_assert(managed_type_d_exists(&g_world, e));
	ck_assert(managed_type_e_exists(&g_world, e));

	// destroy entity - all 8 hooks should fire
	w_ecs_return_entity(&g_world, e);

	// verify all slab arenas have freed their handles
	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(&g_world);

	w_entity_id ids[8] = {
		managed_buffer_get_id(&g_world),
		managed_string_get_id(&g_world),
		managed_data_get_id(&g_world),
		managed_type_a_get_id(&g_world),
		managed_type_b_get_id(&g_world),
		managed_type_c_get_id(&g_world),
		managed_type_d_get_id(&g_world),
		managed_type_e_get_id(&g_world)
	};

	for (int i = 0; i < 8; i++)
	{
		ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[ids[i]].handle_pool), 0);
	}
}
END_TEST

START_TEST(test_many_component_types_many_entities)
{
	// 50 entities each with 5 managed component types
	int count = 50;
	w_entity_id *entities = malloc(count * sizeof(w_entity_id));

	for (int i = 0; i < count; i++)
	{
		entities[i] = w_ecs_request_entity(&g_world);

		wm_managed_alloc_malloc(&g_world, managed_type_a, entities[i], 32);
		wm_managed_alloc_malloc(&g_world, managed_type_b, entities[i], 64);
		wm_managed_alloc_malloc(&g_world, managed_type_c, entities[i], 128);
		wm_managed_alloc_malloc(&g_world, managed_type_d, entities[i], 256);
		wm_managed_alloc_malloc(&g_world, managed_type_e, entities[i], 512);
	}

	// verify counts
	struct wm_managed_alloc_registry *registry = wm_managed_alloc_get_registry(&g_world);
	w_entity_id a_id = managed_type_a_get_id(&g_world);
	ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[a_id].handle_pool), count);

	// destroy all
	for (int i = 0; i < count; i++)
	{
		w_ecs_return_entity(&g_world, entities[i]);
	}

	// verify all freed
	w_entity_id ids[5] = {
		managed_type_a_get_id(&g_world),
		managed_type_b_get_id(&g_world),
		managed_type_c_get_id(&g_world),
		managed_type_d_get_id(&g_world),
		managed_type_e_get_id(&g_world)
	};

	for (int i = 0; i < 5; i++)
	{
		ck_assert_uint_eq(w_id_pool_alive_count(&registry->slab_arenas[ids[i]].handle_pool), 0);
	}

	free(entities);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *managed_alloc_suite(void)
{
	Suite *s = suite_create("managed_alloc");

	TCase *tc_basic = tcase_create("basic_allocation");
	tcase_add_checked_fixture(tc_basic, managed_alloc_setup, managed_alloc_teardown);
	tcase_set_timeout(tc_basic, 10);
	tcase_add_test(tc_basic, test_malloc_returns_valid_pointer);
	tcase_add_test(tc_basic, test_malloc_memory_is_writable);
	tcase_add_test(tc_basic, test_malloc_sets_handle_on_entity);
	suite_add_tcase(s, tc_basic);

	TCase *tc_multi = tcase_create("multiple_components");
	tcase_add_checked_fixture(tc_multi, managed_alloc_setup, managed_alloc_teardown);
	tcase_set_timeout(tc_multi, 10);
	tcase_add_test(tc_multi, test_multiple_component_types_same_entity);
	tcase_add_test(tc_multi, test_same_component_type_different_entities);
	suite_add_tcase(s, tc_multi);

	TCase *tc_cleanup = tcase_create("auto_cleanup");
	tcase_add_checked_fixture(tc_cleanup, managed_alloc_setup, managed_alloc_teardown);
	tcase_set_timeout(tc_cleanup, 10);
	tcase_add_test(tc_cleanup, test_cleanup_on_component_remove);
	tcase_add_test(tc_cleanup, test_cleanup_on_entity_destroy);
	tcase_add_test(tc_cleanup, test_cleanup_multiple_entities);
	suite_add_tcase(s, tc_cleanup);

	TCase *tc_sizes = tcase_create("varying_sizes");
	tcase_add_checked_fixture(tc_sizes, managed_alloc_setup, managed_alloc_teardown);
	tcase_set_timeout(tc_sizes, 10);
	tcase_add_test(tc_sizes, test_small_allocation);
	tcase_add_test(tc_sizes, test_large_allocation);
	suite_add_tcase(s, tc_sizes);

	TCase *tc_stress = tcase_create("stress");
	tcase_add_checked_fixture(tc_stress, managed_alloc_setup, managed_alloc_teardown);
	tcase_set_timeout(tc_stress, 30);
	tcase_add_test(tc_stress, test_many_entities_same_component);
	tcase_add_test(tc_stress, test_mixed_component_types_many_entities);
	suite_add_tcase(s, tc_stress);

	TCase *tc_hooks = tcase_create("hook_verification");
	tcase_add_checked_fixture(tc_hooks, managed_alloc_setup, managed_alloc_teardown);
	tcase_set_timeout(tc_hooks, 10);
	tcase_add_test(tc_hooks, test_hook_frees_handle_on_remove);
	tcase_add_test(tc_hooks, test_hook_frees_handle_on_entity_destroy);
	tcase_add_test(tc_hooks, test_hook_frees_multiple_component_types);
	tcase_add_test(tc_hooks, test_malloc_replaces_existing_handle);
	suite_add_tcase(s, tc_hooks);

	TCase *tc_resolve = tcase_create("resolve_handle");
	tcase_add_checked_fixture(tc_resolve, managed_alloc_setup, managed_alloc_teardown);
	tcase_set_timeout(tc_resolve, 10);
	tcase_add_test(tc_resolve, test_resolve_handle_matches_malloc_pointer);
	tcase_add_test(tc_resolve, test_resolve_handle_read_write);
	tcase_add_test(tc_resolve, test_resolve_handle_multiple_entities);
	suite_add_tcase(s, tc_resolve);

	TCase *tc_cycles = tcase_create("add_remove_cycles");
	tcase_add_checked_fixture(tc_cycles, managed_alloc_setup, managed_alloc_teardown);
	tcase_set_timeout(tc_cycles, 30);
	tcase_add_test(tc_cycles, test_add_remove_cycle_single_entity);
	tcase_add_test(tc_cycles, test_add_remove_cycle_different_entities);
	tcase_add_test(tc_cycles, test_add_remove_cycle_multiple_components);
	suite_add_tcase(s, tc_cycles);

	TCase *tc_rapid = tcase_create("rapid_removal");
	tcase_add_checked_fixture(tc_rapid, managed_alloc_setup, managed_alloc_teardown);
	tcase_set_timeout(tc_rapid, 30);
	tcase_add_test(tc_rapid, test_rapid_remove_many_entities);
	tcase_add_test(tc_rapid, test_rapid_remove_interleaved);
	tcase_add_test(tc_rapid, test_rapid_entity_destroy);
	suite_add_tcase(s, tc_rapid);

	TCase *tc_many_types = tcase_create("many_component_types");
	tcase_add_checked_fixture(tc_many_types, managed_alloc_setup, managed_alloc_teardown);
	tcase_set_timeout(tc_many_types, 30);
	tcase_add_test(tc_many_types, test_many_component_types_simultaneous);
	tcase_add_test(tc_many_types, test_many_component_types_many_entities);
	suite_add_tcase(s, tc_many_types);

	return s;
}

int main(void)
{
	Suite *s = managed_alloc_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
