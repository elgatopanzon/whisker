/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_query_registry
 * @created     : Thursday Mar 06, 2026 16:30:00 CST
 * @description : tests for whisker_query_registry query parsing
 */

#include "whisker_std.h"

#include "whisker_query_registry.h"
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
static struct w_component_registry g_components;
static struct w_query_registry g_registry;

static void query_registry_setup(void)
{
	w_arena_init(&g_arena, 64 * 1024);
	w_string_table_init(&g_string_table, &g_arena,
		WHISKER_STRING_TABLE_REALLOC_SIZE,
		WHISKER_STRING_TABLE_BUCKETS_SIZE,
		w_xxhash64_hash);
	w_entity_registry_init(&g_entities, &g_string_table);
	w_component_registry_init(&g_components, &g_arena, &g_entities);
	w_query_registry_init(&g_registry, &g_string_table, &g_components, &g_arena);
}

static void query_registry_teardown(void)
{
	w_query_registry_free(&g_registry);
	w_component_registry_free(&g_components);
	w_entity_registry_free(&g_entities);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}

// helper: register a named component
static w_entity_id register_component(const char *name)
{
	return w_component_get_id(&g_components, (char *)name);
}


/*****************************
*  registry_init             *
*****************************/

START_TEST(test_init_string_table_stored)
{
	ck_assert_ptr_eq(g_registry.string_table, &g_string_table);
}
END_TEST

START_TEST(test_init_component_registry_stored)
{
	ck_assert_ptr_eq(g_registry.component_registry, &g_components);
}
END_TEST

START_TEST(test_init_queries_length_zero)
{
	ck_assert_int_eq(g_registry.queries_length, 0);
}
END_TEST


/*****************************
*  stage 1: query splitting  *
*****************************/

START_TEST(test_stage1_single_term)
{
	// single term query should parse into 1 term
	struct w_query *q = w_query_registry_get_query(&g_registry, "read component_a");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 1);
}
END_TEST

START_TEST(test_stage1_two_terms)
{
	// two terms separated by comma
	struct w_query *q = w_query_registry_get_query(&g_registry, "read component_a, read component_b");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 2);
}
END_TEST

START_TEST(test_stage1_three_terms)
{
	// three terms
	struct w_query *q = w_query_registry_get_query(&g_registry, "read a, write b, optional c");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 3);
}
END_TEST

START_TEST(test_stage1_no_whitespace_after_comma)
{
	// "write component_a,write component_b" is valid per header comment
	struct w_query *q = w_query_registry_get_query(&g_registry, "read a,read b");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 2);
}
END_TEST

START_TEST(test_stage1_multiple_spaces_after_comma)
{
	// extra whitespace should be handled
	struct w_query *q = w_query_registry_get_query(&g_registry, "read a,   read b");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 2);
}
END_TEST

START_TEST(test_stage1_raw_term_interned)
{
	struct w_query *q = w_query_registry_get_query(&g_registry, "read test_comp");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_ge(q->terms_length, 1);
	// first term's raw_term should be valid
	ck_assert_int_ne(q->terms[0].raw_term, W_STRING_TABLE_INVALID_ID);
	// verify the raw term string is correct
	char *raw = w_string_table_lookup(&g_string_table, q->terms[0].raw_term);
	ck_assert_ptr_nonnull(raw);
	ck_assert_str_eq(raw, "read test_comp");
}
END_TEST

START_TEST(test_stage1_multi_term_raw_strings)
{
	struct w_query *q = w_query_registry_get_query(&g_registry, "read a, write b");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 2);

	char *raw0 = w_string_table_lookup(&g_string_table, q->terms[0].raw_term);
	char *raw1 = w_string_table_lookup(&g_string_table, q->terms[1].raw_term);
	ck_assert_str_eq(raw0, "read a");
	ck_assert_str_eq(raw1, "write b");
}
END_TEST

START_TEST(test_stage1_state_transitions_to_query_parsed)
{
	struct w_query *q = w_query_registry_get_query(&g_registry, "read component");
	ck_assert_ptr_nonnull(q);
	// after stage 1, state should be at least QUERY_PARSED
	ck_assert_int_ge(q->query_parse_state, W_QUERY_PARSE_STATE_QUERY_PARSED);
}
END_TEST


/*****************************
*  stage 2: term parsing     *
*****************************/

START_TEST(test_stage2_read_access)
{
	struct w_query *q = w_query_registry_get_query(&g_registry, "read mycomp");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_ge(q->terms_length, 1);
	ck_assert_int_eq(q->terms[0].access_type, W_QUERY_ACCESS_READ);
}
END_TEST

START_TEST(test_stage2_write_access)
{
	struct w_query *q = w_query_registry_get_query(&g_registry, "write mycomp");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_ge(q->terms_length, 1);
	ck_assert_int_eq(q->terms[0].access_type, W_QUERY_ACCESS_WRITE);
}
END_TEST

START_TEST(test_stage2_optional_access)
{
	struct w_query *q = w_query_registry_get_query(&g_registry, "optional mycomp");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_ge(q->terms_length, 1);
	ck_assert_int_eq(q->terms[0].access_type, W_QUERY_ACCESS_OPTIONAL);
}
END_TEST

START_TEST(test_stage2_unknown_access)
{
	// invalid access type should become NONE
	struct w_query *q = w_query_registry_get_query(&g_registry, "invalid mycomp");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_ge(q->terms_length, 1);
	ck_assert_int_eq(q->terms[0].access_type, W_QUERY_ACCESS_NONE);
}
END_TEST

START_TEST(test_stage2_component_name_interned)
{
	struct w_query *q = w_query_registry_get_query(&g_registry, "read position");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_ge(q->terms_length, 1);
	ck_assert_int_ne(q->terms[0].component_name, W_STRING_TABLE_INVALID_ID);

	char *name = w_string_table_lookup(&g_string_table, q->terms[0].component_name);
	ck_assert_ptr_nonnull(name);
	ck_assert_str_eq(name, "position");
}
END_TEST

START_TEST(test_stage2_multiple_terms_parsed)
{
	struct w_query *q = w_query_registry_get_query(&g_registry, "read a, write b, optional c");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 3);

	ck_assert_int_eq(q->terms[0].access_type, W_QUERY_ACCESS_READ);
	ck_assert_int_eq(q->terms[1].access_type, W_QUERY_ACCESS_WRITE);
	ck_assert_int_eq(q->terms[2].access_type, W_QUERY_ACCESS_OPTIONAL);

	char *name0 = w_string_table_lookup(&g_string_table, q->terms[0].component_name);
	char *name1 = w_string_table_lookup(&g_string_table, q->terms[1].component_name);
	char *name2 = w_string_table_lookup(&g_string_table, q->terms[2].component_name);
	ck_assert_str_eq(name0, "a");
	ck_assert_str_eq(name1, "b");
	ck_assert_str_eq(name2, "c");
}
END_TEST

START_TEST(test_stage2_state_transitions_to_terms_parsed)
{
	struct w_query *q = w_query_registry_get_query(&g_registry, "read comp");
	ck_assert_ptr_nonnull(q);
	// should reach at least TERMS_PARSED
	ck_assert_int_ge(q->query_parse_state, W_QUERY_PARSE_STATE_TERMS_PARSED);
}
END_TEST


/*****************************
*  stage 3: component IDs    *
*****************************/

START_TEST(test_stage3_component_id_resolved)
{
	// register component first
	w_entity_id comp_id = register_component("position");

	struct w_query *q = w_query_registry_get_query(&g_registry, "read position");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_ge(q->terms_length, 1);
	ck_assert_int_eq(q->terms[0].component_id, comp_id);
}
END_TEST

START_TEST(test_stage3_multiple_components_resolved)
{
	w_entity_id pos_id = register_component("pos");
	w_entity_id vel_id = register_component("vel");

	struct w_query *q = w_query_registry_get_query(&g_registry, "read pos, write vel");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 2);
	ck_assert_int_eq(q->terms[0].component_id, pos_id);
	ck_assert_int_eq(q->terms[1].component_id, vel_id);
}
END_TEST

START_TEST(test_stage3_unregistered_component_invalid)
{
	// component not registered, should remain invalid
	struct w_query *q = w_query_registry_get_query(&g_registry, "read unknown_comp");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_ge(q->terms_length, 1);
	ck_assert_int_eq(q->terms[0].component_id, W_ENTITY_INVALID);
}
END_TEST

START_TEST(test_stage3_partial_resolution)
{
	// one component registered, one not
	w_entity_id known_id = register_component("known");

	struct w_query *q = w_query_registry_get_query(&g_registry, "read known, write unknown");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 2);
	ck_assert_int_eq(q->terms[0].component_id, known_id);
	ck_assert_int_eq(q->terms[1].component_id, W_ENTITY_INVALID);
}
END_TEST

START_TEST(test_stage3_state_components_parsed_when_all_resolved)
{
	register_component("resolved_comp");

	struct w_query *q = w_query_registry_get_query(&g_registry, "read resolved_comp");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->query_parse_state, W_QUERY_PARSE_STATE_COMPONENTS_PARSED);
}
END_TEST

START_TEST(test_stage3_state_not_components_parsed_when_unresolved)
{
	// component not registered
	struct w_query *q = w_query_registry_get_query(&g_registry, "read unresolved_comp");
	ck_assert_ptr_nonnull(q);
	// should not reach COMPONENTS_PARSED state
	ck_assert_int_ne(q->query_parse_state, W_QUERY_PARSE_STATE_COMPONENTS_PARSED);
}
END_TEST

START_TEST(test_stage3_late_registration_resolves)
{
	// first call, component not registered
	struct w_query *q = w_query_registry_get_query(&g_registry, "read late_comp");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms[0].component_id, W_ENTITY_INVALID);

	// register the component
	w_entity_id comp_id = register_component("late_comp");

	// second call should now resolve
	q = w_query_registry_get_query(&g_registry, "read late_comp");
	ck_assert_int_eq(q->terms[0].component_id, comp_id);
}
END_TEST


/*****************************
*  query caching             *
*****************************/

START_TEST(test_cache_same_query_returns_same_ptr)
{
	register_component("cached");

	struct w_query *q1 = w_query_registry_get_query(&g_registry, "read cached");
	struct w_query *q2 = w_query_registry_get_query(&g_registry, "read cached");
	ck_assert_ptr_eq(q1, q2);
}
END_TEST

START_TEST(test_cache_different_queries_different_ptrs)
{
	struct w_query *q1 = w_query_registry_get_query(&g_registry, "read comp_a");
	struct w_query *q2 = w_query_registry_get_query(&g_registry, "read comp_b");
	ck_assert_ptr_ne(q1, q2);
}
END_TEST

START_TEST(test_cache_queries_length_increments)
{
	ck_assert_int_eq(g_registry.queries_length, 0);

	w_query_registry_get_query(&g_registry, "read first");
	ck_assert_int_eq(g_registry.queries_length, 1);

	w_query_registry_get_query(&g_registry, "read second");
	ck_assert_int_eq(g_registry.queries_length, 2);

	// duplicate shouldn't increment
	w_query_registry_get_query(&g_registry, "read first");
	ck_assert_int_eq(g_registry.queries_length, 2);
}
END_TEST

START_TEST(test_cache_raw_query_interned)
{
	struct w_query *q = w_query_registry_get_query(&g_registry, "read test");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_ne(q->raw_query, W_STRING_TABLE_INVALID_ID);

	char *raw = w_string_table_lookup(&g_string_table, q->raw_query);
	ck_assert_str_eq(raw, "read test");
}
END_TEST


/*****************************
*  header query examples     *
*****************************/

START_TEST(test_example_read_write_optional)
{
	// from header: "read component_a, read component_b, write component_c, optional component_d"
	register_component("component_a");
	register_component("component_b");
	register_component("component_c");
	register_component("component_d");

	struct w_query *q = w_query_registry_get_query(&g_registry,
		"read component_a, read component_b, write component_c, optional component_d");

	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 4);
	ck_assert_int_eq(q->query_parse_state, W_QUERY_PARSE_STATE_COMPONENTS_PARSED);

	ck_assert_int_eq(q->terms[0].access_type, W_QUERY_ACCESS_READ);
	ck_assert_int_eq(q->terms[1].access_type, W_QUERY_ACCESS_READ);
	ck_assert_int_eq(q->terms[2].access_type, W_QUERY_ACCESS_WRITE);
	ck_assert_int_eq(q->terms[3].access_type, W_QUERY_ACCESS_OPTIONAL);
}
END_TEST

START_TEST(test_example_two_reads)
{
	// from header: "read component_a, read component_b"
	register_component("component_a");
	register_component("component_b");

	struct w_query *q = w_query_registry_get_query(&g_registry,
		"read component_a, read component_b");

	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 2);
	ck_assert_int_eq(q->terms[0].access_type, W_QUERY_ACCESS_READ);
	ck_assert_int_eq(q->terms[1].access_type, W_QUERY_ACCESS_READ);
}
END_TEST

START_TEST(test_example_two_writes)
{
	// from header: "write component_a, write component_b"
	register_component("component_a");
	register_component("component_b");

	struct w_query *q = w_query_registry_get_query(&g_registry,
		"write component_a, write component_b");

	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 2);
	ck_assert_int_eq(q->terms[0].access_type, W_QUERY_ACCESS_WRITE);
	ck_assert_int_eq(q->terms[1].access_type, W_QUERY_ACCESS_WRITE);
}
END_TEST

START_TEST(test_example_no_whitespace)
{
	// from header: "write component_a,write component_b" note: missing white space
	register_component("component_a");
	register_component("component_b");

	struct w_query *q = w_query_registry_get_query(&g_registry,
		"write component_a,write component_b");

	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 2);
	ck_assert_int_eq(q->terms[0].access_type, W_QUERY_ACCESS_WRITE);
	ck_assert_int_eq(q->terms[1].access_type, W_QUERY_ACCESS_WRITE);
}
END_TEST


/*****************************
*  edge cases                *
*****************************/

START_TEST(test_edge_very_long_component_name)
{
	char name[256];
	memset(name, 'x', 255);
	name[255] = '\0';

	register_component(name);

	char query[300];
	snprintf(query, sizeof(query), "read %s", name);

	struct w_query *q = w_query_registry_get_query(&g_registry, query);
	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 1);
	ck_assert_int_eq(q->query_parse_state, W_QUERY_PARSE_STATE_COMPONENTS_PARSED);
}
END_TEST

START_TEST(test_edge_many_terms)
{
	// 10 terms
	char names[10][16];
	for (int i = 0; i < 10; i++) {
		snprintf(names[i], sizeof(names[i]), "comp%d", i);
		register_component(names[i]);
	}

	struct w_query *q = w_query_registry_get_query(&g_registry,
		"read comp0, read comp1, read comp2, read comp3, read comp4, "
		"read comp5, read comp6, read comp7, read comp8, read comp9");

	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->terms_length, 10);
}
END_TEST

START_TEST(test_edge_many_queries)
{
	// create many unique queries
	for (int i = 0; i < 100; i++) {
		char query[64];
		snprintf(query, sizeof(query), "read component_%d", i);
		struct w_query *q = w_query_registry_get_query(&g_registry, query);
		ck_assert_ptr_nonnull(q);
	}
	ck_assert_int_eq(g_registry.queries_length, 100);
}
END_TEST


/*****************************
*  cache rebuild             *
*****************************/

// helper: set component on entity to populate bitset
static void set_component_on_entity(w_entity_id comp_id, w_entity_id entity_id)
{
	int dummy = 0;
	w_component_set(&g_components, int, comp_id, entity_id, &dummy);
}

START_TEST(test_cache_all_dense)
{
	// 20 contiguous IDs >= MIN (16) threshold -> 1 dense slice
	w_entity_id comp_a = register_component("cache_dense_a");
	w_entity_id comp_b = register_component("cache_dense_b");
	w_entity_id comp_c = register_component("cache_dense_c");

	// set components on entities 0-19 (20 contiguous)
	for (w_entity_id e = 0; e < 20; e++)
	{
		set_component_on_entity(comp_a, e);
		set_component_on_entity(comp_b, e);
		set_component_on_entity(comp_c, e);
	}

	struct w_query *q = w_query_registry_get_query(&g_registry,
		"read cache_dense_a, read cache_dense_b, read cache_dense_c");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_eq(q->query_parse_state, W_QUERY_PARSE_STATE_COMPONENTS_PARSED);

	bool rebuilt = w_query_rebuild_cache(&g_registry, q);
	ck_assert(rebuilt);

	// should have 1 dense slice covering all 20
	ck_assert_int_eq(q->archetype_slices_dense_length, 1);
	ck_assert_int_eq(q->archetype_slices_sparse_length, 0);
	ck_assert_int_eq(q->archetype_slices_dense[0].start_id, 0);
	ck_assert_int_eq(q->archetype_slices_dense[0].slice_length, 20);
}
END_TEST

START_TEST(test_cache_all_sparse)
{
	// 4 non-contiguous IDs, each run < MIN threshold -> 4 sparse slices
	w_entity_id comp_a = register_component("cache_sparse_a");
	w_entity_id comp_b = register_component("cache_sparse_b");
	w_entity_id comp_c = register_component("cache_sparse_c");

	// set components on entities 0, 100, 200, 300 (non-contiguous)
	w_entity_id entities[] = {0, 100, 200, 300};
	for (int i = 0; i < 4; i++)
	{
		set_component_on_entity(comp_a, entities[i]);
		set_component_on_entity(comp_b, entities[i]);
		set_component_on_entity(comp_c, entities[i]);
	}

	struct w_query *q = w_query_registry_get_query(&g_registry,
		"read cache_sparse_a, read cache_sparse_b, read cache_sparse_c");
	ck_assert_ptr_nonnull(q);

	bool rebuilt = w_query_rebuild_cache(&g_registry, q);
	ck_assert(rebuilt);

	// should have 0 dense slices, 4 sparse slices (each length 1)
	ck_assert_int_eq(q->archetype_slices_dense_length, 0);
	ck_assert_int_eq(q->archetype_slices_sparse_length, 4);

	for (int i = 0; i < 4; i++)
	{
		ck_assert_int_eq(q->archetype_slices_sparse[i].start_id, entities[i]);
		ck_assert_int_eq(q->archetype_slices_sparse[i].slice_length, 1);
	}
}
END_TEST

START_TEST(test_cache_mixed_dense_sparse)
{
	// dense run of 20 + sparse run of 3 -> 1 dense + 1 sparse
	w_entity_id comp_a = register_component("cache_mixed_a");
	w_entity_id comp_b = register_component("cache_mixed_b");
	w_entity_id comp_c = register_component("cache_mixed_c");

	// dense: entities 0-19
	for (w_entity_id e = 0; e < 20; e++)
	{
		set_component_on_entity(comp_a, e);
		set_component_on_entity(comp_b, e);
		set_component_on_entity(comp_c, e);
	}

	// sparse: entities 1000, 1001, 1002 (3 contiguous but < MIN=16)
	for (w_entity_id e = 1000; e < 1003; e++)
	{
		set_component_on_entity(comp_a, e);
		set_component_on_entity(comp_b, e);
		set_component_on_entity(comp_c, e);
	}

	struct w_query *q = w_query_registry_get_query(&g_registry,
		"read cache_mixed_a, read cache_mixed_b, read cache_mixed_c");
	ck_assert_ptr_nonnull(q);

	bool rebuilt = w_query_rebuild_cache(&g_registry, q);
	ck_assert(rebuilt);

	// 1 dense slice (0-19), 1 sparse slice (1000-1002)
	ck_assert_int_eq(q->archetype_slices_dense_length, 1);
	ck_assert_int_eq(q->archetype_slices_sparse_length, 1);

	ck_assert_int_eq(q->archetype_slices_dense[0].start_id, 0);
	ck_assert_int_eq(q->archetype_slices_dense[0].slice_length, 20);

	ck_assert_int_eq(q->archetype_slices_sparse[0].start_id, 1000);
	ck_assert_int_eq(q->archetype_slices_sparse[0].slice_length, 3);
}
END_TEST

START_TEST(test_cache_dense_max_splits)
{
	// contiguous run > MAX (1024) should split into multiple dense slices
	// use 2064 entities: 2 full slices (1024 each) + 1 partial (16, min for dense)
	w_entity_id comp_a = register_component("cache_max_a");
	w_entity_id comp_b = register_component("cache_max_b");
	w_entity_id comp_c = register_component("cache_max_c");

	for (w_entity_id e = 0; e < 2064; e++)
	{
		set_component_on_entity(comp_a, e);
		set_component_on_entity(comp_b, e);
		set_component_on_entity(comp_c, e);
	}

	struct w_query *q = w_query_registry_get_query(&g_registry,
		"read cache_max_a, read cache_max_b, read cache_max_c");
	ck_assert_ptr_nonnull(q);

	bool rebuilt = w_query_rebuild_cache(&g_registry, q);
	ck_assert(rebuilt);

	// should have 3 dense slices: [0-1023], [1024-2047], [2048-2063]
	ck_assert_int_eq(q->archetype_slices_dense_length, 3);
	ck_assert_int_eq(q->archetype_slices_sparse_length, 0);

	ck_assert_int_eq(q->archetype_slices_dense[0].start_id, 0);
	ck_assert_int_eq(q->archetype_slices_dense[0].slice_length, 1024);

	ck_assert_int_eq(q->archetype_slices_dense[1].start_id, 1024);
	ck_assert_int_eq(q->archetype_slices_dense[1].slice_length, 1024);

	ck_assert_int_eq(q->archetype_slices_dense[2].start_id, 2048);
	ck_assert_int_eq(q->archetype_slices_dense[2].slice_length, 16);
}
END_TEST

START_TEST(test_cache_rebuild_returns_false_on_unparsed)
{
	// query with unresolved component should fail rebuild
	struct w_query *q = w_query_registry_get_query(&g_registry, "read nonexistent_comp");
	ck_assert_ptr_nonnull(q);
	ck_assert_int_ne(q->query_parse_state, W_QUERY_PARSE_STATE_COMPONENTS_PARSED);

	bool rebuilt = w_query_rebuild_cache(&g_registry, q);
	ck_assert(!rebuilt);
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
	struct w_query_registry qr;

	w_arena_init(&a, 64 * 1024);
	w_string_table_init(&st, &a, WHISKER_STRING_TABLE_REALLOC_SIZE, WHISKER_STRING_TABLE_BUCKETS_SIZE, w_xxhash64_hash);
	w_entity_registry_init(&er, &st);
	w_component_registry_init(&cr, &a, &er);
	w_query_registry_init(&qr, &st, &cr, &a);

	w_query_registry_free(&qr);

	ck_assert_ptr_null(qr.queries);
	ck_assert_int_eq(qr.queries_length, 0);

	w_component_registry_free(&cr);
	w_entity_registry_free(&er);
	w_string_table_free(&st);
	w_arena_free(&a);
}
END_TEST

START_TEST(test_free_with_queries)
{
	struct w_arena a;
	struct w_string_table st;
	struct w_entity_registry er;
	struct w_component_registry cr;
	struct w_query_registry qr;

	w_arena_init(&a, 64 * 1024);
	w_string_table_init(&st, &a, WHISKER_STRING_TABLE_REALLOC_SIZE, WHISKER_STRING_TABLE_BUCKETS_SIZE, w_xxhash64_hash);
	w_entity_registry_init(&er, &st);
	w_component_registry_init(&cr, &a, &er);
	w_query_registry_init(&qr, &st, &cr, &a);

	// create some queries
	for (int i = 0; i < 5; i++) {
		char query[64];
		snprintf(query, sizeof(query), "read comp_%d", i);
		w_query_registry_get_query(&qr, query);
	}

	w_query_registry_free(&qr);

	ck_assert_ptr_null(qr.queries);
	ck_assert_int_eq(qr.queries_length, 0);

	w_component_registry_free(&cr);
	w_entity_registry_free(&er);
	w_string_table_free(&st);
	w_arena_free(&a);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_query_registry_suite(void)
{
	Suite *s = suite_create("whisker_query_registry");

	TCase *tc_init = tcase_create("registry_init");
	tcase_add_checked_fixture(tc_init, query_registry_setup, query_registry_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_string_table_stored);
	tcase_add_test(tc_init, test_init_component_registry_stored);
	tcase_add_test(tc_init, test_init_queries_length_zero);
	suite_add_tcase(s, tc_init);

	TCase *tc_stage1 = tcase_create("stage1_query_splitting");
	tcase_add_checked_fixture(tc_stage1, query_registry_setup, query_registry_teardown);
	tcase_set_timeout(tc_stage1, 10);
	tcase_add_test(tc_stage1, test_stage1_single_term);
	tcase_add_test(tc_stage1, test_stage1_two_terms);
	tcase_add_test(tc_stage1, test_stage1_three_terms);
	tcase_add_test(tc_stage1, test_stage1_no_whitespace_after_comma);
	tcase_add_test(tc_stage1, test_stage1_multiple_spaces_after_comma);
	tcase_add_test(tc_stage1, test_stage1_raw_term_interned);
	tcase_add_test(tc_stage1, test_stage1_multi_term_raw_strings);
	tcase_add_test(tc_stage1, test_stage1_state_transitions_to_query_parsed);
	suite_add_tcase(s, tc_stage1);

	TCase *tc_stage2 = tcase_create("stage2_term_parsing");
	tcase_add_checked_fixture(tc_stage2, query_registry_setup, query_registry_teardown);
	tcase_set_timeout(tc_stage2, 10);
	tcase_add_test(tc_stage2, test_stage2_read_access);
	tcase_add_test(tc_stage2, test_stage2_write_access);
	tcase_add_test(tc_stage2, test_stage2_optional_access);
	tcase_add_test(tc_stage2, test_stage2_unknown_access);
	tcase_add_test(tc_stage2, test_stage2_component_name_interned);
	tcase_add_test(tc_stage2, test_stage2_multiple_terms_parsed);
	tcase_add_test(tc_stage2, test_stage2_state_transitions_to_terms_parsed);
	suite_add_tcase(s, tc_stage2);

	TCase *tc_stage3 = tcase_create("stage3_component_ids");
	tcase_add_checked_fixture(tc_stage3, query_registry_setup, query_registry_teardown);
	tcase_set_timeout(tc_stage3, 10);
	tcase_add_test(tc_stage3, test_stage3_component_id_resolved);
	tcase_add_test(tc_stage3, test_stage3_multiple_components_resolved);
	tcase_add_test(tc_stage3, test_stage3_unregistered_component_invalid);
	tcase_add_test(tc_stage3, test_stage3_partial_resolution);
	tcase_add_test(tc_stage3, test_stage3_state_components_parsed_when_all_resolved);
	tcase_add_test(tc_stage3, test_stage3_state_not_components_parsed_when_unresolved);
	tcase_add_test(tc_stage3, test_stage3_late_registration_resolves);
	suite_add_tcase(s, tc_stage3);

	TCase *tc_cache = tcase_create("query_caching");
	tcase_add_checked_fixture(tc_cache, query_registry_setup, query_registry_teardown);
	tcase_set_timeout(tc_cache, 10);
	tcase_add_test(tc_cache, test_cache_same_query_returns_same_ptr);
	tcase_add_test(tc_cache, test_cache_different_queries_different_ptrs);
	tcase_add_test(tc_cache, test_cache_queries_length_increments);
	tcase_add_test(tc_cache, test_cache_raw_query_interned);
	suite_add_tcase(s, tc_cache);

	TCase *tc_examples = tcase_create("header_examples");
	tcase_add_checked_fixture(tc_examples, query_registry_setup, query_registry_teardown);
	tcase_set_timeout(tc_examples, 10);
	tcase_add_test(tc_examples, test_example_read_write_optional);
	tcase_add_test(tc_examples, test_example_two_reads);
	tcase_add_test(tc_examples, test_example_two_writes);
	tcase_add_test(tc_examples, test_example_no_whitespace);
	suite_add_tcase(s, tc_examples);

	TCase *tc_edge = tcase_create("edge_cases");
	tcase_add_checked_fixture(tc_edge, query_registry_setup, query_registry_teardown);
	tcase_set_timeout(tc_edge, 10);
	tcase_add_test(tc_edge, test_edge_very_long_component_name);
	tcase_add_test(tc_edge, test_edge_many_terms);
	tcase_add_test(tc_edge, test_edge_many_queries);
	suite_add_tcase(s, tc_edge);

	TCase *tc_cache_rebuild = tcase_create("cache_rebuild");
	tcase_add_checked_fixture(tc_cache_rebuild, query_registry_setup, query_registry_teardown);
	tcase_set_timeout(tc_cache_rebuild, 30);
	tcase_add_test(tc_cache_rebuild, test_cache_all_dense);
	tcase_add_test(tc_cache_rebuild, test_cache_all_sparse);
	tcase_add_test(tc_cache_rebuild, test_cache_mixed_dense_sparse);
	tcase_add_test(tc_cache_rebuild, test_cache_dense_max_splits);
	tcase_add_test(tc_cache_rebuild, test_cache_rebuild_returns_false_on_unparsed);
	suite_add_tcase(s, tc_cache_rebuild);

	TCase *tc_free = tcase_create("registry_free");
	tcase_set_timeout(tc_free, 10);
	tcase_add_test(tc_free, test_free_empty_registry);
	tcase_add_test(tc_free, test_free_with_queries);
	suite_add_tcase(s, tc_free);

	return s;
}

int main(void)
{
	Suite *s = whisker_query_registry_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
