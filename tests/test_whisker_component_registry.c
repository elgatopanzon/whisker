/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_component_registry
 * @created     : Tuesday Mar 03, 2026 16:58:27 CST
 * @description : tests for whisker_component_registry component storage
 */

#include "whisker_std.h"

#include "whisker_component_registry.h"
#include "whisker_entity_registry.h"
#include "whisker_string_table.h"
#include "whisker_arena.h"
#include "whisker_hash_xxhash64.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_arena g_arena;
static struct w_string_table g_string_table;
static struct w_entity_registry g_entities;
static struct w_component_registry g_registry;

static void component_registry_setup(void)
{
	w_arena_init(&g_arena, 64 * 1024);
	w_string_table_init(&g_string_table, &g_arena,
		WHISKER_STRING_TABLE_REALLOC_SIZE,
		WHISKER_STRING_TABLE_BUCKETS_SIZE,
		w_xxhash64_hash);
	w_entity_registry_init(&g_entities, &g_string_table);
	w_component_registry_init(&g_registry, &g_arena, &g_entities);
}

static void component_registry_teardown(void)
{
	w_component_registry_free(&g_registry);
	w_entity_registry_free(&g_entities);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}

/* helper: request a component type entity ID */
static w_entity_id new_type_id(void)
{
	return w_entity_request(&g_entities);
}

/* helper: request an entity ID */
static w_entity_id new_entity_id(void)
{
	return w_entity_request(&g_entities);
}


/*****************************
*  registry_init             *
*****************************/

START_TEST(test_init_entities_stored)
{
	ck_assert_ptr_eq(g_registry.entities, &g_entities);
}
END_TEST

START_TEST(test_init_arena_stored)
{
	ck_assert_ptr_eq(g_registry.arena, &g_arena);
}
END_TEST

START_TEST(test_init_entries_length_zero)
{
	ck_assert_int_eq(g_registry.entries_length, 0);
}
END_TEST


/*****************************
*  set / get                 *
*****************************/

START_TEST(test_set_returns_nonnull)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	int val = 42;

	void *ptr = w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &val, sizeof(int));
	ck_assert_ptr_nonnull(ptr);
}
END_TEST

START_TEST(test_set_get_int_value)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	int val = 1234;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &val, sizeof(int));

	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_nonnull(got);
	ck_assert_int_eq(*(int *)got, 1234);
}
END_TEST

START_TEST(test_set_get_float_value)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	float val = 3.14f;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_float, type_id, entity_id, &val, sizeof(float));

	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_nonnull(got);
	ck_assert_float_eq_tol(*(float *)got, 3.14f, 1e-6f);
}
END_TEST

START_TEST(test_set_get_uint64_value)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	uint64_t val = 0xDEADBEEFCAFEBABEULL;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_uint64_t, type_id, entity_id, &val, sizeof(uint64_t));

	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_nonnull(got);
	ck_assert_uint_eq(*(uint64_t *)got, 0xDEADBEEFCAFEBABEULL);
}
END_TEST

START_TEST(test_set_get_bool_false)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	bool val = false;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_bool, type_id, entity_id, &val, sizeof(bool));

	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_nonnull(got);
	ck_assert_int_eq(*(bool *)got, false);
}
END_TEST

START_TEST(test_set_get_struct_value)
{
	/* store a struct using void type slot */
	struct { int x; int y; float z; } comp = { 10, 20, 1.5f };

	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();

	w_component_set_(&g_registry, W_COMPONENT_TYPE_uint8_t, type_id, entity_id, &comp, sizeof(comp));

	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_nonnull(got);

	struct { int x; int y; float z; } *result = got;
	ck_assert_int_eq(result->x, 10);
	ck_assert_int_eq(result->y, 20);
	ck_assert_float_eq_tol(result->z, 1.5f, 1e-6f);
}
END_TEST

START_TEST(test_get_missing_type_returns_null)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();

	/* never set, should return NULL */
	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_null(got);
}
END_TEST

START_TEST(test_get_missing_entity_returns_null)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_a = new_entity_id();
	w_entity_id entity_b = new_entity_id();
	int val = 5;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_a, &val, sizeof(int));

	/* entity_b never had component set */
	void *got = w_component_get_(&g_registry, type_id, entity_b);
	ck_assert_ptr_null(got);
}
END_TEST


/*****************************
*  has                       *
*****************************/

START_TEST(test_has_false_before_set)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();

	ck_assert_int_eq(w_component_has_(&g_registry, type_id, entity_id), false);
}
END_TEST

START_TEST(test_has_true_after_set)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	int val = 1;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &val, sizeof(int));

	ck_assert_int_eq(w_component_has_(&g_registry, type_id, entity_id), true);
}
END_TEST

START_TEST(test_has_false_missing_type)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();

	/* type never registered, has_ should not crash and return false */
	ck_assert_int_eq(w_component_has_(&g_registry, type_id, entity_id), false);
}
END_TEST

START_TEST(test_has_false_after_remove)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	int val = 7;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &val, sizeof(int));
	ck_assert_int_eq(w_component_has_(&g_registry, type_id, entity_id), true);

	w_component_remove_(&g_registry, type_id, entity_id);
	ck_assert_int_eq(w_component_has_(&g_registry, type_id, entity_id), false);
}
END_TEST


/*****************************
*  remove                    *
*****************************/

START_TEST(test_remove_clears_has)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	int val = 99;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &val, sizeof(int));
	w_component_remove_(&g_registry, type_id, entity_id);

	ck_assert_int_eq(w_component_has_(&g_registry, type_id, entity_id), false);
}
END_TEST

START_TEST(test_remove_makes_get_return_null)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	int val = 55;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &val, sizeof(int));
	w_component_remove_(&g_registry, type_id, entity_id);

	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_null(got);
}
END_TEST

START_TEST(test_remove_missing_type_noop)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();

	/* should not crash */
	w_component_remove_(&g_registry, type_id, entity_id);
}
END_TEST

START_TEST(test_remove_does_not_affect_other_entities)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_a = new_entity_id();
	w_entity_id entity_b = new_entity_id();
	int val_a = 10, val_b = 20;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_a, &val_a, sizeof(int));
	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_b, &val_b, sizeof(int));

	w_component_remove_(&g_registry, type_id, entity_a);

	ck_assert_int_eq(w_component_has_(&g_registry, type_id, entity_a), false);
	ck_assert_int_eq(w_component_has_(&g_registry, type_id, entity_b), true);

	void *got_b = w_component_get_(&g_registry, type_id, entity_b);
	ck_assert_ptr_nonnull(got_b);
	ck_assert_int_eq(*(int *)got_b, 20);
}
END_TEST

START_TEST(test_set_after_remove_works)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	int val = 11;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &val, sizeof(int));
	w_component_remove_(&g_registry, type_id, entity_id);

	int val2 = 22;
	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &val2, sizeof(int));

	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_nonnull(got);
	ck_assert_int_eq(*(int *)got, 22);
}
END_TEST


/*****************************
*  overwrite                 *
*****************************/

START_TEST(test_set_twice_overwrites_value)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	int val1 = 100;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &val1, sizeof(int));

	int val2 = 200;
	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &val2, sizeof(int));

	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_nonnull(got);
	ck_assert_int_eq(*(int *)got, 200);
}
END_TEST


/*****************************
*  multiple entities         *
*****************************/

START_TEST(test_multiple_entities_isolated)
{
	w_entity_id type_id = new_type_id();
	const int count = 10;
	w_entity_id entity_ids[10];

	for (int i = 0; i < count; i++) {
		entity_ids[i] = new_entity_id();
		int val = i * 10;
		w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_ids[i], &val, sizeof(int));
	}

	for (int i = 0; i < count; i++) {
		void *got = w_component_get_(&g_registry, type_id, entity_ids[i]);
		ck_assert_ptr_nonnull(got);
		ck_assert_int_eq(*(int *)got, i * 10);
	}
}
END_TEST

START_TEST(test_multiple_entities_partial_set)
{
	w_entity_id type_id = new_type_id();
	w_entity_id e_set = new_entity_id();
	w_entity_id e_not = new_entity_id();
	int val = 5;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, e_set, &val, sizeof(int));

	ck_assert_int_eq(w_component_has_(&g_registry, type_id, e_set), true);
	ck_assert_int_eq(w_component_has_(&g_registry, type_id, e_not), false);
}
END_TEST


/*****************************
*  multiple component types  *
*****************************/

START_TEST(test_multiple_types_isolated)
{
	w_entity_id type_a = new_type_id();
	w_entity_id type_b = new_type_id();
	w_entity_id entity_id = new_entity_id();

	int val_a = 1;
	float val_b = 2.5f;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_a, entity_id, &val_a, sizeof(int));
	w_component_set_(&g_registry, W_COMPONENT_TYPE_float, type_b, entity_id, &val_b, sizeof(float));

	void *got_a = w_component_get_(&g_registry, type_a, entity_id);
	void *got_b = w_component_get_(&g_registry, type_b, entity_id);

	ck_assert_ptr_nonnull(got_a);
	ck_assert_ptr_nonnull(got_b);
	ck_assert_int_eq(*(int *)got_a, 1);
	ck_assert_float_eq_tol(*(float *)got_b, 2.5f, 1e-6f);
}
END_TEST

START_TEST(test_remove_one_type_leaves_other)
{
	w_entity_id type_a = new_type_id();
	w_entity_id type_b = new_type_id();
	w_entity_id entity_id = new_entity_id();

	int val_a = 10;
	int val_b = 20;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_a, entity_id, &val_a, sizeof(int));
	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_b, entity_id, &val_b, sizeof(int));

	w_component_remove_(&g_registry, type_a, entity_id);

	ck_assert_int_eq(w_component_has_(&g_registry, type_a, entity_id), false);
	ck_assert_int_eq(w_component_has_(&g_registry, type_b, entity_id), true);
}
END_TEST


/*****************************
*  data size integrity       *
*****************************/

START_TEST(test_data_size_int8)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	int8_t val = -127;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int8_t, type_id, entity_id, &val, sizeof(int8_t));

	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_nonnull(got);
	ck_assert_int_eq(*(int8_t *)got, -127);
}
END_TEST

START_TEST(test_data_size_int16)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	int16_t val = 30000;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int16_t, type_id, entity_id, &val, sizeof(int16_t));

	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_nonnull(got);
	ck_assert_int_eq(*(int16_t *)got, 30000);
}
END_TEST

START_TEST(test_data_size_double)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	double val = 3.141592653589793;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_double, type_id, entity_id, &val, sizeof(double));

	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_nonnull(got);
	ck_assert_double_eq_tol(*(double *)got, 3.141592653589793, 1e-15);
}
END_TEST

START_TEST(test_data_size_large_struct)
{
	/* 128-byte struct */
	unsigned char large[128];
	for (int i = 0; i < 128; i++) large[i] = (unsigned char)(i ^ 0xAB);

	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();

	w_component_set_(&g_registry, W_COMPONENT_TYPE_uint8_t, type_id, entity_id, large, sizeof(large));

	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_nonnull(got);
	ck_assert_int_eq(memcmp(got, large, sizeof(large)), 0);
}
END_TEST

START_TEST(test_data_no_cross_contamination_adjacent_entities)
{
	/* verify adjacent entity slots don't bleed into each other */
	w_entity_id type_id = new_type_id();
	w_entity_id e0 = new_entity_id();
	w_entity_id e1 = new_entity_id();

	uint64_t val0 = 0xAAAAAAAAAAAAAAAAULL;
	uint64_t val1 = 0x5555555555555555ULL;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_uint64_t, type_id, e0, &val0, sizeof(uint64_t));
	w_component_set_(&g_registry, W_COMPONENT_TYPE_uint64_t, type_id, e1, &val1, sizeof(uint64_t));

	void *got0 = w_component_get_(&g_registry, type_id, e0);
	void *got1 = w_component_get_(&g_registry, type_id, e1);

	ck_assert_uint_eq(*(uint64_t *)got0, 0xAAAAAAAAAAAAAAAAULL);
	ck_assert_uint_eq(*(uint64_t *)got1, 0x5555555555555555ULL);
}
END_TEST


/*****************************
*  entry management          *
*****************************/

START_TEST(test_has_entry_false_before_set)
{
	w_entity_id type_id = new_type_id();
	ck_assert_int_eq(w_component_registry_has_entry(&g_registry, type_id), false);
}
END_TEST

START_TEST(test_has_entry_true_after_set)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	int val = 1;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &val, sizeof(int));

	ck_assert_int_eq(w_component_registry_has_entry(&g_registry, type_id), true);
}
END_TEST

START_TEST(test_get_entry_null_before_set)
{
	w_entity_id type_id = new_type_id();
	struct w_component_entry *entry = w_component_registry_get_entry(&g_registry, type_id);
	ck_assert_ptr_null(entry);
}
END_TEST

START_TEST(test_get_entry_nonnull_after_set)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	int val = 1;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &val, sizeof(int));

	struct w_component_entry *entry = w_component_registry_get_entry(&g_registry, type_id);
	ck_assert_ptr_nonnull(entry);
}
END_TEST

START_TEST(test_entry_type_size_stored_correctly)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	int val = 1;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &val, sizeof(int));

	struct w_component_entry *entry = w_component_registry_get_entry(&g_registry, type_id);
	ck_assert_ptr_nonnull(entry);
	ck_assert_uint_eq(entry->type_size, sizeof(int));
}
END_TEST

START_TEST(test_entry_type_id_stored_correctly)
{
	w_entity_id type_id = new_type_id();
	w_entity_id entity_id = new_entity_id();
	float val = 0.0f;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_float, type_id, entity_id, &val, sizeof(float));

	struct w_component_entry *entry = w_component_registry_get_entry(&g_registry, type_id);
	ck_assert_ptr_nonnull(entry);
	ck_assert_uint_eq(entry->type_id, W_COMPONENT_TYPE_float);
}
END_TEST


/*****************************
*  named components          *
*****************************/

START_TEST(test_get_id_creates_on_first_call)
{
	w_entity_id id = w_component_get_id(&g_registry, (char *)"Position");
	ck_assert_int_ne(id, W_ENTITY_INVALID);
}
END_TEST

START_TEST(test_get_id_same_id_on_second_call)
{
	w_entity_id id1 = w_component_get_id(&g_registry, (char *)"Velocity");
	w_entity_id id2 = w_component_get_id(&g_registry, (char *)"Velocity");
	ck_assert_int_eq(id1, id2);
}
END_TEST

START_TEST(test_get_id_different_names_different_ids)
{
	w_entity_id id_pos = w_component_get_id(&g_registry, (char *)"Position");
	w_entity_id id_vel = w_component_get_id(&g_registry, (char *)"Velocity");
	ck_assert_int_ne(id_pos, id_vel);
}
END_TEST

START_TEST(test_get_name_returns_name)
{
	w_entity_id id = w_component_get_id(&g_registry, (char *)"Health");
	char *name = w_component_get_name(&g_registry, id);
	ck_assert_ptr_nonnull(name);
	ck_assert_str_eq(name, "Health");
}
END_TEST

START_TEST(test_get_name_out_of_range_returns_null)
{
	/* no entities registered, id 9999 is out of range */
	char *name = w_component_get_name(&g_registry, 9999);
	ck_assert_ptr_null(name);
}
END_TEST

START_TEST(test_get_name_unnamed_entity_returns_null)
{
	/* request an anonymous entity from the entity registry */
	w_entity_id anon = w_entity_request(&g_entities);
	/* should have no name */
	char *name = w_component_get_name(&g_registry, anon);
	ck_assert_ptr_null(name);
}
END_TEST

START_TEST(test_named_component_set_and_get)
{
	w_entity_id type_id = w_component_get_id(&g_registry, (char *)"Score");
	w_entity_id entity_id = new_entity_id();
	int score = 9001;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, entity_id, &score, sizeof(int));

	void *got = w_component_get_(&g_registry, type_id, entity_id);
	ck_assert_ptr_nonnull(got);
	ck_assert_int_eq(*(int *)got, 9001);
}
END_TEST

START_TEST(test_named_component_lookup_consistent)
{
	/* set via name-derived id, get via second name lookup */
	w_entity_id type_id_set = w_component_get_id(&g_registry, (char *)"Mass");
	w_entity_id entity_id = new_entity_id();
	float mass = 1.5f;

	w_component_set_(&g_registry, W_COMPONENT_TYPE_float, type_id_set, entity_id, &mass, sizeof(float));

	w_entity_id type_id_get = w_component_get_id(&g_registry, (char *)"Mass");
	ck_assert_int_eq(type_id_set, type_id_get);

	void *got = w_component_get_(&g_registry, type_id_get, entity_id);
	ck_assert_ptr_nonnull(got);
	ck_assert_float_eq_tol(*(float *)got, 1.5f, 1e-6f);
}
END_TEST


/*****************************
*  realloc growth            *
*****************************/

START_TEST(test_high_entity_id_triggers_realloc)
{
	w_entity_id type_id = new_type_id();

	/* allocate many entity IDs to get a high index */
	w_entity_id high_entity = W_ENTITY_INVALID;
	for (int i = 0; i < 200; i++) {
		high_entity = w_entity_request(&g_entities);
	}
	ck_assert_int_ne(high_entity, W_ENTITY_INVALID);

	int val = 777;
	w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, high_entity, &val, sizeof(int));

	void *got = w_component_get_(&g_registry, type_id, high_entity);
	ck_assert_ptr_nonnull(got);
	ck_assert_int_eq(*(int *)got, 777);
}
END_TEST

START_TEST(test_many_entities_data_integrity)
{
	w_entity_id type_id = new_type_id();
	const int count = 150;
	w_entity_id ids[150];

	for (int i = 0; i < count; i++) {
		ids[i] = new_entity_id();
		int val = i;
		w_component_set_(&g_registry, W_COMPONENT_TYPE_int, type_id, ids[i], &val, sizeof(int));
	}

	for (int i = 0; i < count; i++) {
		void *got = w_component_get_(&g_registry, type_id, ids[i]);
		ck_assert_ptr_nonnull(got);
		ck_assert_int_eq(*(int *)got, i);
	}
}
END_TEST


/*****************************
*  registry_free             *
*****************************/

START_TEST(test_free_empty_registry)
{
	struct w_arena a;
	struct w_string_table st;
	struct w_entity_registry er;
	struct w_component_registry cr;

	w_arena_init(&a, 64 * 1024);
	w_string_table_init(&st, &a, WHISKER_STRING_TABLE_REALLOC_SIZE, WHISKER_STRING_TABLE_BUCKETS_SIZE, w_xxhash64_hash);
	w_entity_registry_init(&er, &st);
	w_component_registry_init(&cr, &a, &er);

	w_component_registry_free(&cr);

	ck_assert_ptr_null(cr.entries);
	ck_assert_ptr_null(cr.entities);
	ck_assert_ptr_null(cr.arena);
	ck_assert_int_eq(cr.entries_length, 0);

	w_entity_registry_free(&er);
	w_string_table_free(&st);
	w_arena_free(&a);
}
END_TEST

START_TEST(test_free_with_components)
{
	struct w_arena a;
	struct w_string_table st;
	struct w_entity_registry er;
	struct w_component_registry cr;

	w_arena_init(&a, 64 * 1024);
	w_string_table_init(&st, &a, WHISKER_STRING_TABLE_REALLOC_SIZE, WHISKER_STRING_TABLE_BUCKETS_SIZE, w_xxhash64_hash);
	w_entity_registry_init(&er, &st);
	w_component_registry_init(&cr, &a, &er);

	for (int i = 0; i < 5; i++) {
		w_entity_id type_id = w_entity_request(&er);
		w_entity_id entity_id = w_entity_request(&er);
		int val = i;
		w_component_set_(&cr, W_COMPONENT_TYPE_int, type_id, entity_id, &val, sizeof(int));
	}

	w_component_registry_free(&cr);

	ck_assert_ptr_null(cr.entries);
	ck_assert_int_eq(cr.entries_length, 0);

	w_entity_registry_free(&er);
	w_string_table_free(&st);
	w_arena_free(&a);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_component_registry_suite(void)
{
	Suite *s = suite_create("whisker_component_registry");

	TCase *tc_init = tcase_create("registry_init");
	tcase_add_checked_fixture(tc_init, component_registry_setup, component_registry_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_entities_stored);
	tcase_add_test(tc_init, test_init_arena_stored);
	tcase_add_test(tc_init, test_init_entries_length_zero);
	suite_add_tcase(s, tc_init);

	TCase *tc_set_get = tcase_create("set_get");
	tcase_add_checked_fixture(tc_set_get, component_registry_setup, component_registry_teardown);
	tcase_set_timeout(tc_set_get, 10);
	tcase_add_test(tc_set_get, test_set_returns_nonnull);
	tcase_add_test(tc_set_get, test_set_get_int_value);
	tcase_add_test(tc_set_get, test_set_get_float_value);
	tcase_add_test(tc_set_get, test_set_get_uint64_value);
	tcase_add_test(tc_set_get, test_set_get_bool_false);
	tcase_add_test(tc_set_get, test_set_get_struct_value);
	tcase_add_test(tc_set_get, test_get_missing_type_returns_null);
	tcase_add_test(tc_set_get, test_get_missing_entity_returns_null);
	suite_add_tcase(s, tc_set_get);

	TCase *tc_has = tcase_create("has");
	tcase_add_checked_fixture(tc_has, component_registry_setup, component_registry_teardown);
	tcase_set_timeout(tc_has, 10);
	tcase_add_test(tc_has, test_has_false_before_set);
	tcase_add_test(tc_has, test_has_true_after_set);
	tcase_add_test(tc_has, test_has_false_missing_type);
	tcase_add_test(tc_has, test_has_false_after_remove);
	suite_add_tcase(s, tc_has);

	TCase *tc_remove = tcase_create("remove");
	tcase_add_checked_fixture(tc_remove, component_registry_setup, component_registry_teardown);
	tcase_set_timeout(tc_remove, 10);
	tcase_add_test(tc_remove, test_remove_clears_has);
	tcase_add_test(tc_remove, test_remove_makes_get_return_null);
	tcase_add_test(tc_remove, test_remove_missing_type_noop);
	tcase_add_test(tc_remove, test_remove_does_not_affect_other_entities);
	tcase_add_test(tc_remove, test_set_after_remove_works);
	suite_add_tcase(s, tc_remove);

	TCase *tc_overwrite = tcase_create("overwrite");
	tcase_add_checked_fixture(tc_overwrite, component_registry_setup, component_registry_teardown);
	tcase_set_timeout(tc_overwrite, 10);
	tcase_add_test(tc_overwrite, test_set_twice_overwrites_value);
	suite_add_tcase(s, tc_overwrite);

	TCase *tc_multi_entity = tcase_create("multiple_entities");
	tcase_add_checked_fixture(tc_multi_entity, component_registry_setup, component_registry_teardown);
	tcase_set_timeout(tc_multi_entity, 10);
	tcase_add_test(tc_multi_entity, test_multiple_entities_isolated);
	tcase_add_test(tc_multi_entity, test_multiple_entities_partial_set);
	suite_add_tcase(s, tc_multi_entity);

	TCase *tc_multi_type = tcase_create("multiple_types");
	tcase_add_checked_fixture(tc_multi_type, component_registry_setup, component_registry_teardown);
	tcase_set_timeout(tc_multi_type, 10);
	tcase_add_test(tc_multi_type, test_multiple_types_isolated);
	tcase_add_test(tc_multi_type, test_remove_one_type_leaves_other);
	suite_add_tcase(s, tc_multi_type);

	TCase *tc_sizes = tcase_create("data_sizes");
	tcase_add_checked_fixture(tc_sizes, component_registry_setup, component_registry_teardown);
	tcase_set_timeout(tc_sizes, 10);
	tcase_add_test(tc_sizes, test_data_size_int8);
	tcase_add_test(tc_sizes, test_data_size_int16);
	tcase_add_test(tc_sizes, test_data_size_double);
	tcase_add_test(tc_sizes, test_data_size_large_struct);
	tcase_add_test(tc_sizes, test_data_no_cross_contamination_adjacent_entities);
	suite_add_tcase(s, tc_sizes);

	TCase *tc_entry = tcase_create("entry_management");
	tcase_add_checked_fixture(tc_entry, component_registry_setup, component_registry_teardown);
	tcase_set_timeout(tc_entry, 10);
	tcase_add_test(tc_entry, test_has_entry_false_before_set);
	tcase_add_test(tc_entry, test_has_entry_true_after_set);
	tcase_add_test(tc_entry, test_get_entry_null_before_set);
	tcase_add_test(tc_entry, test_get_entry_nonnull_after_set);
	tcase_add_test(tc_entry, test_entry_type_size_stored_correctly);
	tcase_add_test(tc_entry, test_entry_type_id_stored_correctly);
	suite_add_tcase(s, tc_entry);

	TCase *tc_named = tcase_create("named_components");
	tcase_add_checked_fixture(tc_named, component_registry_setup, component_registry_teardown);
	tcase_set_timeout(tc_named, 10);
	tcase_add_test(tc_named, test_get_id_creates_on_first_call);
	tcase_add_test(tc_named, test_get_id_same_id_on_second_call);
	tcase_add_test(tc_named, test_get_id_different_names_different_ids);
	tcase_add_test(tc_named, test_get_name_returns_name);
	tcase_add_test(tc_named, test_get_name_out_of_range_returns_null);
	tcase_add_test(tc_named, test_get_name_unnamed_entity_returns_null);
	tcase_add_test(tc_named, test_named_component_set_and_get);
	tcase_add_test(tc_named, test_named_component_lookup_consistent);
	suite_add_tcase(s, tc_named);

	TCase *tc_realloc = tcase_create("realloc_growth");
	tcase_add_checked_fixture(tc_realloc, component_registry_setup, component_registry_teardown);
	tcase_set_timeout(tc_realloc, 10);
	tcase_add_test(tc_realloc, test_high_entity_id_triggers_realloc);
	tcase_add_test(tc_realloc, test_many_entities_data_integrity);
	suite_add_tcase(s, tc_realloc);

	TCase *tc_free = tcase_create("registry_free");
	tcase_set_timeout(tc_free, 10);
	tcase_add_test(tc_free, test_free_empty_registry);
	tcase_add_test(tc_free, test_free_with_components);
	suite_add_tcase(s, tc_free);

	return s;
}

int main(void)
{
	Suite *s = whisker_component_registry_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
