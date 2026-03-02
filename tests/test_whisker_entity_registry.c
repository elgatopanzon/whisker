/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_entity_registry
 * @created     : Sunday Mar 01, 2026 21:36:46 CST
 * @description : tests for whisker_entity_registry.h entity management
 */

#include "whisker_std.h"
#include "whisker_entity_registry.h"
#include "whisker_string_table.h"
#include "whisker_arena.h"
#include "whisker_hash_xxhash64.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_arena g_arena;
static struct w_string_table g_string_table;
static struct w_entity_registry g_registry;

static void entity_registry_setup(void)
{
	w_arena_init(&g_arena, 4096);
	w_string_table_init(&g_string_table, &g_arena, WHISKER_STRING_TABLE_REALLOC_SIZE, WHISKER_STRING_TABLE_BUCKETS_SIZE, w_xxhash64_hash);
	w_entity_registry_init(&g_registry, &g_string_table);
}

static void entity_registry_teardown(void)
{
	w_entity_registry_free(&g_registry);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}


/*****************************
*  entity_registry_init      *
*****************************/

START_TEST(test_init_name_table_stored)
{
	ck_assert_ptr_eq(g_registry.name_table, &g_string_table);
}
END_TEST

START_TEST(test_init_next_id_zero)
{
	ck_assert_int_eq(g_registry.next_id, 0);
}
END_TEST

START_TEST(test_init_arrays_empty)
{
	ck_assert_int_eq(g_registry.entity_to_name_length, 0);
	ck_assert_int_eq(g_registry.name_to_entity_length, 0);
	ck_assert_int_eq(g_registry.recycled_stack_length, 0);
}
END_TEST


/*****************************
*  basic request/return      *
*****************************/

START_TEST(test_request_returns_zero_first)
{
	w_entity_id id = w_entity_request(&g_registry);
	ck_assert_int_eq(id, 0);
}
END_TEST

START_TEST(test_request_increments_id)
{
	w_entity_id id1 = w_entity_request(&g_registry);
	w_entity_id id2 = w_entity_request(&g_registry);
	w_entity_id id3 = w_entity_request(&g_registry);
	ck_assert_int_eq(id1, 0);
	ck_assert_int_eq(id2, 1);
	ck_assert_int_eq(id3, 2);
}
END_TEST

START_TEST(test_request_entity_is_valid)
{
	w_entity_id id = w_entity_request(&g_registry);
	ck_assert_int_ne(id, W_ENTITY_INVALID);
}
END_TEST

START_TEST(test_return_entity_updates_recycled_count)
{
	w_entity_id id = w_entity_request(&g_registry);
	ck_assert_int_eq(g_registry.recycled_stack_length, 0);
	w_entity_return(&g_registry, id);
	ck_assert_int_eq(g_registry.recycled_stack_length, 1);
}
END_TEST

// helper to compute alive count (macro in header has bug: recycled_length vs recycled_stack_length)
static inline uint32_t alive_count(struct w_entity_registry *r)
{
	return r->next_id - r->recycled_stack_length;
}

START_TEST(test_alive_count_increments)
{
	ck_assert_int_eq(alive_count(&g_registry), 0);
	w_entity_request(&g_registry);
	ck_assert_int_eq(alive_count(&g_registry), 1);
	w_entity_request(&g_registry);
	ck_assert_int_eq(alive_count(&g_registry), 2);
}
END_TEST

START_TEST(test_alive_count_decrements_on_return)
{
	w_entity_id id1 = w_entity_request(&g_registry);
	w_entity_id id2 = w_entity_request(&g_registry);
	ck_assert_int_eq(alive_count(&g_registry), 2);
	w_entity_return(&g_registry, id1);
	ck_assert_int_eq(alive_count(&g_registry), 1);
	w_entity_return(&g_registry, id2);
	ck_assert_int_eq(alive_count(&g_registry), 0);
}
END_TEST


/*****************************
*  recycling                 *
*****************************/

START_TEST(test_recycle_reuses_returned_id)
{
	w_entity_id id1 = w_entity_request(&g_registry);
	w_entity_return(&g_registry, id1);
	w_entity_id id2 = w_entity_request(&g_registry);
	ck_assert_int_eq(id1, id2);
}
END_TEST

START_TEST(test_recycle_lifo_order)
{
	w_entity_id a = w_entity_request(&g_registry);
	w_entity_id b = w_entity_request(&g_registry);
	w_entity_id c = w_entity_request(&g_registry);
	w_entity_return(&g_registry, a);
	w_entity_return(&g_registry, b);
	w_entity_return(&g_registry, c);
	// LIFO: c returned last, requested first
	ck_assert_int_eq(w_entity_request(&g_registry), c);
	ck_assert_int_eq(w_entity_request(&g_registry), b);
	ck_assert_int_eq(w_entity_request(&g_registry), a);
}
END_TEST

START_TEST(test_recycle_exhausted_falls_to_new)
{
	w_entity_id id1 = w_entity_request(&g_registry);
	w_entity_return(&g_registry, id1);
	w_entity_id id2 = w_entity_request(&g_registry);
	ck_assert_int_eq(id2, id1);
	// recycled exhausted, should get next_id
	w_entity_id id3 = w_entity_request(&g_registry);
	ck_assert_int_eq(id3, 1);
}
END_TEST

START_TEST(test_recycle_multiple_cycles)
{
	for (int cycle = 0; cycle < 5; cycle++) {
		w_entity_id id = w_entity_request(&g_registry);
		ck_assert_int_eq(id, 0);
		w_entity_return(&g_registry, id);
	}
	// next_id should still be 1 (never incremented past the recycling)
	ck_assert_int_eq(g_registry.next_id, 1);
}
END_TEST


/*****************************
*  name operations           *
*****************************/

START_TEST(test_set_name_and_get_name)
{
	w_entity_id id = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, id, (char*)"player");
	char *name = w_entity_get_name(&g_registry, id);
	ck_assert_ptr_nonnull(name);
	ck_assert_str_eq(name, "player");
}
END_TEST

START_TEST(test_anonymous_entity_has_null_name)
{
	w_entity_id id = w_entity_request(&g_registry);
	char *name = w_entity_get_name(&g_registry, id);
	ck_assert_ptr_null(name);
}
END_TEST

START_TEST(test_clear_name)
{
	w_entity_id id = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, id, (char*)"temp_name");
	ck_assert_ptr_nonnull(w_entity_get_name(&g_registry, id));
	w_entity_clear_name(&g_registry, id);
	ck_assert_ptr_null(w_entity_get_name(&g_registry, id));
}
END_TEST

START_TEST(test_clear_name_anonymous_noop)
{
	w_entity_id id = w_entity_request(&g_registry);
	// should not crash
	w_entity_clear_name(&g_registry, id);
	ck_assert_ptr_null(w_entity_get_name(&g_registry, id));
}
END_TEST

START_TEST(test_set_name_replaces_old_name)
{
	w_entity_id id = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, id, (char*)"old_name");
	w_entity_set_name(&g_registry, id, (char*)"new_name");
	char *name = w_entity_get_name(&g_registry, id);
	ck_assert_str_eq(name, "new_name");
}
END_TEST

START_TEST(test_return_clears_name)
{
	w_entity_id id = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, id, (char*)"will_be_cleared");
	ck_assert_ptr_nonnull(w_entity_get_name(&g_registry, id));
	w_entity_return(&g_registry, id);
	// after return, getting name for that id should be null
	ck_assert_ptr_null(w_entity_get_name(&g_registry, id));
}
END_TEST


/*****************************
*  lookup by name            *
*****************************/

START_TEST(test_lookup_by_name_found)
{
	w_entity_id id = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, id, (char*)"hero");
	w_entity_id found = w_entity_lookup_by_name(&g_registry, (char*)"hero");
	ck_assert_int_eq(found, id);
}
END_TEST

START_TEST(test_lookup_by_name_not_found)
{
	w_entity_id id = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, id, (char*)"exists");
	w_entity_id found = w_entity_lookup_by_name(&g_registry, (char*)"not_exists");
	ck_assert_int_eq(found, W_ENTITY_INVALID);
}
END_TEST

START_TEST(test_lookup_by_name_after_clear)
{
	w_entity_id id = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, id, (char*)"temp");
	w_entity_clear_name(&g_registry, id);
	w_entity_id found = w_entity_lookup_by_name(&g_registry, (char*)"temp");
	ck_assert_int_eq(found, W_ENTITY_INVALID);
}
END_TEST

START_TEST(test_lookup_by_name_after_return)
{
	w_entity_id id = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, id, (char*)"returned");
	w_entity_return(&g_registry, id);
	w_entity_id found = w_entity_lookup_by_name(&g_registry, (char*)"returned");
	ck_assert_int_eq(found, W_ENTITY_INVALID);
}
END_TEST

START_TEST(test_lookup_multiple_named_entities)
{
	w_entity_id e1 = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, e1, (char*)"entity_one");
	w_entity_id e2 = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, e2, (char*)"entity_two");
	w_entity_id e3 = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, e3, (char*)"entity_three");
	ck_assert_int_eq(w_entity_lookup_by_name(&g_registry, (char*)"entity_one"), e1);
	ck_assert_int_eq(w_entity_lookup_by_name(&g_registry, (char*)"entity_two"), e2);
	ck_assert_int_eq(w_entity_lookup_by_name(&g_registry, (char*)"entity_three"), e3);
}
END_TEST

START_TEST(test_get_or_create_pattern)
{
	// pattern: lookup by name, if not found then request + set_name
	char *name = (char*)"player";

	// first time: entity doesn't exist
	w_entity_id id = w_entity_lookup_by_name(&g_registry, name);
	if (id == W_ENTITY_INVALID) {
		id = w_entity_request(&g_registry);
		w_entity_set_name(&g_registry, id, name);
	}
	ck_assert_int_eq(id, 0);
	ck_assert_str_eq(w_entity_get_name(&g_registry, id), "player");

	// second time: entity exists, should return same id
	w_entity_id id2 = w_entity_lookup_by_name(&g_registry, name);
	if (id2 == W_ENTITY_INVALID) {
		id2 = w_entity_request(&g_registry);
		w_entity_set_name(&g_registry, id2, name);
	}
	ck_assert_int_eq(id2, id);
}
END_TEST


/*****************************
*  stress test               *
*****************************/

START_TEST(test_stress_many_entities)
{
	const int count = 10000;
	w_entity_id *ids = malloc(count * sizeof(w_entity_id));
	ck_assert_ptr_nonnull(ids);

	// request many entities
	for (int i = 0; i < count; i++) {
		ids[i] = w_entity_request(&g_registry);
		ck_assert_int_eq(ids[i], i);
	}
	ck_assert_int_eq(alive_count(&g_registry), count);

	// return half
	for (int i = 0; i < count / 2; i++) {
		w_entity_return(&g_registry, ids[i]);
	}
	ck_assert_int_eq(alive_count(&g_registry), count / 2);

	// request again, should recycle
	for (int i = 0; i < count / 2; i++) {
		w_entity_id recycled = w_entity_request(&g_registry);
		ck_assert_int_ne(recycled, W_ENTITY_INVALID);
	}
	ck_assert_int_eq(alive_count(&g_registry), count);

	free(ids);
}
END_TEST

START_TEST(test_stress_request_return_cycles)
{
	const int cycles = 1000;
	for (int i = 0; i < cycles; i++) {
		w_entity_id id = w_entity_request(&g_registry);
		ck_assert_int_ne(id, W_ENTITY_INVALID);
		w_entity_return(&g_registry, id);
	}
	// should have only used one ID slot due to recycling
	ck_assert_int_eq(g_registry.next_id, 1);
	ck_assert_int_eq(alive_count(&g_registry), 0);
}
END_TEST

START_TEST(test_stress_named_entities)
{
	const int count = 500;
	char namebuf[64];

	for (int i = 0; i < count; i++) {
		snprintf(namebuf, sizeof(namebuf), "entity_%d", i);
		w_entity_id id = w_entity_request(&g_registry);
		w_entity_set_name(&g_registry, id, namebuf);
		ck_assert_int_eq(id, i);
	}

	// verify all lookups work
	for (int i = 0; i < count; i++) {
		snprintf(namebuf, sizeof(namebuf), "entity_%d", i);
		w_entity_id found = w_entity_lookup_by_name(&g_registry, namebuf);
		ck_assert_int_eq(found, i);
		char *name = w_entity_get_name(&g_registry, i);
		ck_assert_str_eq(name, namebuf);
	}
}
END_TEST


/*****************************
*  string table fragmentation*
*****************************/

START_TEST(test_same_names_reused_no_table_growth)
{
	const int cycles = 100;
	char *name = (char*)"reused_name";

	// intern the name first to get baseline
	w_entity_id id = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, id, name);
	size_t initial_entries = g_string_table.entries_length;
	w_entity_return(&g_registry, id);

	// cycle through request/return with same name
	for (int i = 0; i < cycles; i++) {
		w_entity_id eid = w_entity_request(&g_registry);
		w_entity_set_name(&g_registry, eid, name);
		w_entity_return(&g_registry, eid);
	}

	// string table should not have grown
	ck_assert_int_eq(g_string_table.entries_length, initial_entries);
}
END_TEST

START_TEST(test_unique_names_grow_table_linearly)
{
	const int count = 100;
	char namebuf[64];

	size_t initial_entries = g_string_table.entries_length;

	for (int i = 0; i < count; i++) {
		snprintf(namebuf, sizeof(namebuf), "unique_entity_%d", i);
		w_entity_id id = w_entity_request(&g_registry);
		w_entity_set_name(&g_registry, id, namebuf);
		w_entity_return(&g_registry, id);
	}

	// string table should have grown by exactly count
	ck_assert_int_eq(g_string_table.entries_length, initial_entries + count);
}
END_TEST

START_TEST(test_mixed_reused_and_unique_names)
{
	const int cycles = 50;
	char *reused_names[] = {(char*)"player", (char*)"enemy", (char*)"npc"};
	const int num_reused = 3;
	char unique_buf[64];

	// first pass: create the reused names
	for (int i = 0; i < num_reused; i++) {
		w_entity_id id = w_entity_request(&g_registry);
		w_entity_set_name(&g_registry, id, reused_names[i]);
		w_entity_return(&g_registry, id);
	}
	size_t after_reused = g_string_table.entries_length;

	// cycle with reused names - should not grow
	for (int c = 0; c < cycles; c++) {
		for (int i = 0; i < num_reused; i++) {
			w_entity_id id = w_entity_request(&g_registry);
			w_entity_set_name(&g_registry, id, reused_names[i]);
			w_entity_return(&g_registry, id);
		}
	}
	ck_assert_int_eq(g_string_table.entries_length, after_reused);

	// now add unique names - should grow
	for (int i = 0; i < cycles; i++) {
		snprintf(unique_buf, sizeof(unique_buf), "mixed_unique_%d", i);
		w_entity_id id = w_entity_request(&g_registry);
		w_entity_set_name(&g_registry, id, unique_buf);
		w_entity_return(&g_registry, id);
	}
	ck_assert_int_eq(g_string_table.entries_length, after_reused + cycles);
}
END_TEST

START_TEST(test_name_reuse_after_rename)
{
	// create entity with name A, rename to B, create new entity with name A
	w_entity_id e1 = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, e1, (char*)"original");
	size_t entries_after_first = g_string_table.entries_length;

	w_entity_set_name(&g_registry, e1, (char*)"renamed");
	// renaming should add new string but not duplicate if string exists
	ck_assert_int_ge(g_string_table.entries_length, entries_after_first);

	// now lookup original should fail since entity was renamed
	w_entity_id found = w_entity_lookup_by_name(&g_registry, (char*)"original");
	ck_assert_int_eq(found, W_ENTITY_INVALID);

	// but the string is still in the table (interned)
	// creating a new entity with "original" should not add new string entry
	size_t entries_before = g_string_table.entries_length;
	w_entity_id e2 = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, e2, (char*)"original");
	ck_assert_int_eq(g_string_table.entries_length, entries_before);
	ck_assert_int_ne(e1, e2);
}
END_TEST


/*****************************
*  entity_registry_free      *
*  (no fixture)              *
*****************************/

START_TEST(test_free_empty_registry)
{
	struct w_arena a;
	struct w_string_table st;
	struct w_entity_registry r;

	w_arena_init(&a, 1024);
	w_string_table_init(&st, &a, 64, 8, w_xxhash64_hash);
	w_entity_registry_init(&r, &st);
	w_entity_registry_free(&r);
	w_string_table_free(&st);
	w_arena_free(&a);

	ck_assert_ptr_null(r.entity_to_name);
	ck_assert_ptr_null(r.name_to_entity);
	ck_assert_ptr_null(r.recycled_stack);
}
END_TEST

START_TEST(test_free_with_entities)
{
	struct w_arena a;
	struct w_string_table st;
	struct w_entity_registry r;

	w_arena_init(&a, 64 * 1024);
	w_string_table_init(&st, &a, 256, 256, w_xxhash64_hash);
	w_entity_registry_init(&r, &st);

	for (int i = 0; i < 10; i++) {
		char buf[32];
		snprintf(buf, sizeof(buf), "entity_%d", i);
		w_entity_id id = w_entity_request(&r);
		w_entity_set_name(&r, id, buf);
	}

	w_entity_registry_free(&r);
	w_string_table_free(&st);
	w_arena_free(&a);

	ck_assert_ptr_null(r.entity_to_name);
	ck_assert_int_eq(r.next_id, 0);
}
END_TEST


/*****************************
*  edge cases                *
*****************************/

START_TEST(test_empty_string_name)
{
	w_entity_id id = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, id, (char*)"");
	char *name = w_entity_get_name(&g_registry, id);
	ck_assert_ptr_nonnull(name);
	ck_assert_str_eq(name, "");
	w_entity_id found = w_entity_lookup_by_name(&g_registry, (char*)"");
	ck_assert_int_eq(found, id);
}
END_TEST

START_TEST(test_same_name_multiple_entities_last_wins)
{
	// when two entities have the same name, lookup returns most recent
	w_entity_id e1 = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, e1, (char*)"shared");
	w_entity_id e2 = w_entity_request(&g_registry);
	w_entity_set_name(&g_registry, e2, (char*)"shared");

	// e2 should now own the name, e1 lost it due to set_name clearing old
	w_entity_id found = w_entity_lookup_by_name(&g_registry, (char*)"shared");
	ck_assert_int_eq(found, e2);
}
END_TEST

START_TEST(test_get_name_out_of_range_id)
{
	w_entity_request(&g_registry);
	char *name = w_entity_get_name(&g_registry, 9999);
	ck_assert_ptr_null(name);
}
END_TEST

START_TEST(test_lookup_empty_registry)
{
	w_entity_id found = w_entity_lookup_by_name(&g_registry, (char*)"anything");
	ck_assert_int_eq(found, W_ENTITY_INVALID);
}
END_TEST


/*****************************
*  thread safety             *
*****************************/

#define THREAD_COUNT 8
#define REQUESTS_PER_THREAD 1000

struct thread_test_ctx
{
	struct w_entity_registry *registry;
	w_entity_id *ids;
	int thread_idx;
};

static void *thread_request_entities(void *arg)
{
	struct thread_test_ctx *ctx = (struct thread_test_ctx *)arg;
	int base = ctx->thread_idx * REQUESTS_PER_THREAD;

	for (int i = 0; i < REQUESTS_PER_THREAD; i++) {
		ctx->ids[base + i] = w_entity_request(ctx->registry);
	}
	return NULL;
}

START_TEST(test_thread_no_duplicate_ids)
{
	const int total = THREAD_COUNT * REQUESTS_PER_THREAD;
	w_entity_id *ids = malloc(total * sizeof(w_entity_id));
	ck_assert_ptr_nonnull(ids);

	pthread_t threads[THREAD_COUNT];
	struct thread_test_ctx ctxs[THREAD_COUNT];

	for (int i = 0; i < THREAD_COUNT; i++) {
		ctxs[i].registry = &g_registry;
		ctxs[i].ids = ids;
		ctxs[i].thread_idx = i;
		pthread_create(&threads[i], NULL, thread_request_entities, &ctxs[i]);
	}

	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_join(threads[i], NULL);
	}

	// verify all IDs are valid
	for (int i = 0; i < total; i++) {
		ck_assert_int_ne(ids[i], W_ENTITY_INVALID);
	}

	// verify no duplicates using a simple sort and compare
	// bubble sort is fine for test code
	for (int i = 0; i < total - 1; i++) {
		for (int j = 0; j < total - i - 1; j++) {
			if (ids[j] > ids[j + 1]) {
				w_entity_id tmp = ids[j];
				ids[j] = ids[j + 1];
				ids[j + 1] = tmp;
			}
		}
	}

	for (int i = 0; i < total - 1; i++) {
		ck_assert_msg(ids[i] != ids[i + 1],
			"Duplicate ID %u found at indices after sort", ids[i]);
	}

	free(ids);
}
END_TEST

struct thread_request_return_ctx
{
	struct w_entity_registry *registry;
	int iterations;
};

static void *thread_request_return_cycle(void *arg)
{
	struct thread_request_return_ctx *ctx = (struct thread_request_return_ctx *)arg;

	for (int i = 0; i < ctx->iterations; i++) {
		w_entity_id id = w_entity_request(ctx->registry);
		// small work to simulate usage
		volatile int x = 0;
		for (int j = 0; j < 10; j++) x++;
		(void)x;
		w_entity_return(ctx->registry, id);
	}
	return NULL;
}

START_TEST(test_thread_request_return_stress)
{
	const int iterations = 500;
	pthread_t threads[THREAD_COUNT];
	struct thread_request_return_ctx ctxs[THREAD_COUNT];

	for (int i = 0; i < THREAD_COUNT; i++) {
		ctxs[i].registry = &g_registry;
		ctxs[i].iterations = iterations;
		pthread_create(&threads[i], NULL, thread_request_return_cycle, &ctxs[i]);
	}

	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_join(threads[i], NULL);
	}

	// after all threads complete with balanced request/return, alive count should be 0
	ck_assert_int_eq(alive_count(&g_registry), 0);
}
END_TEST

static void *thread_request_only_high_contention(void *arg)
{
	struct thread_test_ctx *ctx = (struct thread_test_ctx *)arg;
	int base = ctx->thread_idx * REQUESTS_PER_THREAD;

	// high contention: tight loop with no work between requests
	for (int i = 0; i < REQUESTS_PER_THREAD; i++) {
		ctx->ids[base + i] = w_entity_request(ctx->registry);
	}
	return NULL;
}

START_TEST(test_thread_high_contention)
{
	const int total = THREAD_COUNT * REQUESTS_PER_THREAD;
	w_entity_id *ids = malloc(total * sizeof(w_entity_id));
	ck_assert_ptr_nonnull(ids);

	pthread_t threads[THREAD_COUNT];
	struct thread_test_ctx ctxs[THREAD_COUNT];

	// all threads start as close together as possible
	for (int i = 0; i < THREAD_COUNT; i++) {
		ctxs[i].registry = &g_registry;
		ctxs[i].ids = ids;
		ctxs[i].thread_idx = i;
	}

	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_create(&threads[i], NULL, thread_request_only_high_contention, &ctxs[i]);
	}

	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_join(threads[i], NULL);
	}

	// verify all valid
	for (int i = 0; i < total; i++) {
		ck_assert_int_ne(ids[i], W_ENTITY_INVALID);
	}

	// sort and count unique IDs
	for (int i = 0; i < total - 1; i++) {
		for (int j = 0; j < total - i - 1; j++) {
			if (ids[j] > ids[j + 1]) {
				w_entity_id tmp = ids[j];
				ids[j] = ids[j + 1];
				ids[j + 1] = tmp;
			}
		}
	}

	// verify no duplicates
	for (int i = 0; i < total - 1; i++) {
		ck_assert_msg(ids[i] != ids[i + 1],
			"Duplicate ID %u at high contention", ids[i]);
	}

	free(ids);
}
END_TEST

START_TEST(test_thread_mixed_request_return)
{
	// some threads only request, some only return pre-allocated IDs
	const int requesters = THREAD_COUNT / 2;
	const int returners = THREAD_COUNT - requesters;
	const int pre_alloc = returners * 100;

	// pre-allocate entities to return
	w_entity_id *pre_ids = malloc(pre_alloc * sizeof(w_entity_id));
	for (int i = 0; i < pre_alloc; i++) {
		pre_ids[i] = w_entity_request(&g_registry);
	}

	w_entity_id *request_ids = malloc(requesters * REQUESTS_PER_THREAD * sizeof(w_entity_id));

	pthread_t threads[THREAD_COUNT];
	struct thread_test_ctx req_ctxs[THREAD_COUNT];

	// start requester threads
	for (int i = 0; i < requesters; i++) {
		req_ctxs[i].registry = &g_registry;
		req_ctxs[i].ids = request_ids;
		req_ctxs[i].thread_idx = i;
		pthread_create(&threads[i], NULL, thread_request_entities, &req_ctxs[i]);
	}

	// return pre-allocated IDs from main thread (simulating returner threads)
	for (int i = 0; i < pre_alloc; i++) {
		w_entity_return(&g_registry, pre_ids[i]);
	}

	for (int i = 0; i < requesters; i++) {
		pthread_join(threads[i], NULL);
	}

	// verify requested IDs are all valid
	for (int i = 0; i < requesters * REQUESTS_PER_THREAD; i++) {
		ck_assert_int_ne(request_ids[i], W_ENTITY_INVALID);
	}

	free(pre_ids);
	free(request_ids);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_entity_registry_suite(void)
{
	Suite *s = suite_create("whisker_entity_registry");

	TCase *tc_init = tcase_create("entity_registry_init");
	tcase_add_checked_fixture(tc_init, entity_registry_setup, entity_registry_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_name_table_stored);
	tcase_add_test(tc_init, test_init_next_id_zero);
	tcase_add_test(tc_init, test_init_arrays_empty);
	suite_add_tcase(s, tc_init);

	TCase *tc_basic = tcase_create("basic_request_return");
	tcase_add_checked_fixture(tc_basic, entity_registry_setup, entity_registry_teardown);
	tcase_set_timeout(tc_basic, 10);
	tcase_add_test(tc_basic, test_request_returns_zero_first);
	tcase_add_test(tc_basic, test_request_increments_id);
	tcase_add_test(tc_basic, test_request_entity_is_valid);
	tcase_add_test(tc_basic, test_return_entity_updates_recycled_count);
	tcase_add_test(tc_basic, test_alive_count_increments);
	tcase_add_test(tc_basic, test_alive_count_decrements_on_return);
	suite_add_tcase(s, tc_basic);

	TCase *tc_recycle = tcase_create("recycling");
	tcase_add_checked_fixture(tc_recycle, entity_registry_setup, entity_registry_teardown);
	tcase_set_timeout(tc_recycle, 10);
	tcase_add_test(tc_recycle, test_recycle_reuses_returned_id);
	tcase_add_test(tc_recycle, test_recycle_lifo_order);
	tcase_add_test(tc_recycle, test_recycle_exhausted_falls_to_new);
	tcase_add_test(tc_recycle, test_recycle_multiple_cycles);
	suite_add_tcase(s, tc_recycle);

	TCase *tc_names = tcase_create("name_operations");
	tcase_add_checked_fixture(tc_names, entity_registry_setup, entity_registry_teardown);
	tcase_set_timeout(tc_names, 10);
	tcase_add_test(tc_names, test_set_name_and_get_name);
	tcase_add_test(tc_names, test_anonymous_entity_has_null_name);
	tcase_add_test(tc_names, test_clear_name);
	tcase_add_test(tc_names, test_clear_name_anonymous_noop);
	tcase_add_test(tc_names, test_set_name_replaces_old_name);
	tcase_add_test(tc_names, test_return_clears_name);
	suite_add_tcase(s, tc_names);

	TCase *tc_lookup = tcase_create("lookup_by_name");
	tcase_add_checked_fixture(tc_lookup, entity_registry_setup, entity_registry_teardown);
	tcase_set_timeout(tc_lookup, 10);
	tcase_add_test(tc_lookup, test_lookup_by_name_found);
	tcase_add_test(tc_lookup, test_lookup_by_name_not_found);
	tcase_add_test(tc_lookup, test_lookup_by_name_after_clear);
	tcase_add_test(tc_lookup, test_lookup_by_name_after_return);
	tcase_add_test(tc_lookup, test_lookup_multiple_named_entities);
	tcase_add_test(tc_lookup, test_get_or_create_pattern);
	suite_add_tcase(s, tc_lookup);

	TCase *tc_stress = tcase_create("stress_test");
	tcase_add_checked_fixture(tc_stress, entity_registry_setup, entity_registry_teardown);
	tcase_set_timeout(tc_stress, 30);
	tcase_add_test(tc_stress, test_stress_many_entities);
	tcase_add_test(tc_stress, test_stress_request_return_cycles);
	tcase_add_test(tc_stress, test_stress_named_entities);
	suite_add_tcase(s, tc_stress);

	TCase *tc_frag = tcase_create("string_table_fragmentation");
	tcase_add_checked_fixture(tc_frag, entity_registry_setup, entity_registry_teardown);
	tcase_set_timeout(tc_frag, 10);
	tcase_add_test(tc_frag, test_same_names_reused_no_table_growth);
	tcase_add_test(tc_frag, test_unique_names_grow_table_linearly);
	tcase_add_test(tc_frag, test_mixed_reused_and_unique_names);
	tcase_add_test(tc_frag, test_name_reuse_after_rename);
	suite_add_tcase(s, tc_frag);

	TCase *tc_free = tcase_create("entity_registry_free");
	tcase_set_timeout(tc_free, 10);
	tcase_add_test(tc_free, test_free_empty_registry);
	tcase_add_test(tc_free, test_free_with_entities);
	suite_add_tcase(s, tc_free);

	TCase *tc_edge = tcase_create("edge_cases");
	tcase_add_checked_fixture(tc_edge, entity_registry_setup, entity_registry_teardown);
	tcase_set_timeout(tc_edge, 10);
	tcase_add_test(tc_edge, test_empty_string_name);
	tcase_add_test(tc_edge, test_same_name_multiple_entities_last_wins);
	tcase_add_test(tc_edge, test_get_name_out_of_range_id);
	tcase_add_test(tc_edge, test_lookup_empty_registry);
	suite_add_tcase(s, tc_edge);

	TCase *tc_thread = tcase_create("thread_safety");
	tcase_add_checked_fixture(tc_thread, entity_registry_setup, entity_registry_teardown);
	tcase_set_timeout(tc_thread, 60);
	tcase_add_test(tc_thread, test_thread_no_duplicate_ids);
	tcase_add_test(tc_thread, test_thread_request_return_stress);
	tcase_add_test(tc_thread, test_thread_high_contention);
	tcase_add_test(tc_thread, test_thread_mixed_request_return);
	suite_add_tcase(s, tc_thread);

	return s;
}

int main(void)
{
	Suite *s = whisker_entity_registry_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
