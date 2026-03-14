/**
 * @author      : ElGatoPanzon
 * @file        : test_relationships
 * @created     : Friday Mar 13, 2026 17:44:31 CST
 * @description : tests for whisker_relationships module
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"
#include "whisker_relationships.h"

#include <stdio.h>
#include <stdlib.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_ecs_world g_world;
static struct w_string_table g_string_table;
static struct w_arena g_arena;

static void relationships_setup(void)
{
	w_arena_init(&g_arena, 4096);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);
	wm_relationships_init(&g_world);
}

static void relationships_teardown(void)
{
	wm_relationships_free(&g_world);
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}


/*****************************
*  init/free smoke test      *
*****************************/

START_TEST(test_init_free_no_crash)
{
	/* wm_relationships_init and free ran in fixture without crash */
	ck_assert(true);
}
END_TEST

START_TEST(test_registry_singleton_exists)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);
	ck_assert_ptr_nonnull(reg);
}
END_TEST


/*****************************
*  pair encoding tests       *
*****************************/

START_TEST(test_pack_pair_order_independent)
{
	uint64_t p1 = w_relationship_pack_pair(3, 7);
	uint64_t p2 = w_relationship_pack_pair(7, 3);
	ck_assert_uint_eq(p1, p2);
}
END_TEST

START_TEST(test_pack_pair_same_entity)
{
	uint64_t p = w_relationship_pack_pair(5, 5);
	w_entity_id lo, hi;
	w_relationship_unpack_pair(p, &lo, &hi);
	ck_assert_uint_eq(lo, 5);
	ck_assert_uint_eq(hi, 5);
}
END_TEST

START_TEST(test_unpack_pair_roundtrip)
{
	uint64_t packed = w_relationship_pack_pair(10, 42);
	w_entity_id lo, hi;
	w_relationship_unpack_pair(packed, &lo, &hi);
	ck_assert_uint_eq(lo, 10);
	ck_assert_uint_eq(hi, 42);
}
END_TEST

START_TEST(test_pack_pair_different_pairs_differ)
{
	uint64_t p1 = w_relationship_pack_pair(1, 2);
	uint64_t p2 = w_relationship_pack_pair(1, 3);
	ck_assert_uint_ne(p1, p2);
}
END_TEST

START_TEST(test_pack_pair_zero_entity)
{
	uint64_t packed = w_relationship_pack_pair(0, 100);
	w_entity_id lo, hi;
	w_relationship_unpack_pair(packed, &lo, &hi);
	ck_assert_uint_eq(lo, 0);
	ck_assert_uint_eq(hi, 100);
}
END_TEST

START_TEST(test_pack_pair_max_entity)
{
	// use a large entity ID (not W_ENTITY_INVALID which is UINT32_MAX sentinel)
	w_entity_id large = UINT32_MAX - 1;
	uint64_t packed = w_relationship_pack_pair(1, large);
	w_entity_id lo, hi;
	w_relationship_unpack_pair(packed, &lo, &hi);
	ck_assert_uint_eq(lo, 1);
	ck_assert_uint_eq(hi, large);
}
END_TEST


/*****************************
*  pair_map operations       *
*****************************/

START_TEST(test_pair_map_add_and_get)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);

	w_entity_id a = w_ecs_request_entity(&g_world);
	w_entity_id b = w_ecs_request_entity(&g_world);
	w_entity_id comp = 99;

	w_relationship_registry_add(reg, a, b, comp);

	struct w_relationship_entry_list *list =
		w_relationship_registry_get_pair(reg, a, b);
	ck_assert_ptr_nonnull(list);
	ck_assert_uint_eq(list->entries_length, 1);
	ck_assert_uint_eq(list->entries[0].owner, a);
	ck_assert_uint_eq(list->entries[0].target, b);
	ck_assert_uint_eq(list->entries[0].component_id, comp);
}
END_TEST

START_TEST(test_pair_map_get_reversed_order)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);

	w_entity_id a = w_ecs_request_entity(&g_world);
	w_entity_id b = w_ecs_request_entity(&g_world);
	w_relationship_registry_add(reg, a, b, 1);

	// get with reversed arg order should find the same list
	struct w_relationship_entry_list *list =
		w_relationship_registry_get_pair(reg, b, a);
	ck_assert_ptr_nonnull(list);
	ck_assert_uint_eq(list->entries_length, 1);
}
END_TEST

START_TEST(test_pair_map_multiple_components_same_pair)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);

	w_entity_id a = w_ecs_request_entity(&g_world);
	w_entity_id b = w_ecs_request_entity(&g_world);

	w_relationship_registry_add(reg, a, b, 10);
	w_relationship_registry_add(reg, a, b, 20);
	w_relationship_registry_add(reg, b, a, 30);

	struct w_relationship_entry_list *list =
		w_relationship_registry_get_pair(reg, a, b);
	ck_assert_ptr_nonnull(list);
	ck_assert_uint_eq(list->entries_length, 3);
}
END_TEST

START_TEST(test_pair_map_duplicate_add_ignored)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);

	w_entity_id a = w_ecs_request_entity(&g_world);
	w_entity_id b = w_ecs_request_entity(&g_world);

	w_relationship_registry_add(reg, a, b, 1);
	w_relationship_registry_add(reg, a, b, 1);
	w_relationship_registry_add(reg, a, b, 1);

	struct w_relationship_entry_list *list =
		w_relationship_registry_get_pair(reg, a, b);
	ck_assert_uint_eq(list->entries_length, 1);
}
END_TEST

START_TEST(test_pair_map_get_nonexistent_returns_null)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);
	struct w_relationship_entry_list *list =
		w_relationship_registry_get_pair(reg, 999, 888);
	ck_assert_ptr_null(list);
}
END_TEST

START_TEST(test_pair_map_remove_specific)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);

	w_entity_id a = w_ecs_request_entity(&g_world);
	w_entity_id b = w_ecs_request_entity(&g_world);

	w_relationship_registry_add(reg, a, b, 10);
	w_relationship_registry_add(reg, a, b, 20);

	bool removed = w_relationship_registry_remove(reg, a, b, 10);
	ck_assert(removed);

	struct w_relationship_entry_list *list =
		w_relationship_registry_get_pair(reg, a, b);
	ck_assert_uint_eq(list->entries_length, 1);
	ck_assert_uint_eq(list->entries[0].component_id, 20);
}
END_TEST

START_TEST(test_pair_map_remove_last_cleans_adjacency)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);

	w_entity_id a = w_ecs_request_entity(&g_world);
	w_entity_id b = w_ecs_request_entity(&g_world);

	w_relationship_registry_add(reg, a, b, 1);
	w_relationship_registry_remove(reg, a, b, 1);

	// adjacency should be cleaned up
	struct w_relationship_adjacency_list *adj =
		w_relationship_registry_get_adjacent(reg, a);
	ck_assert(adj == NULL || adj->entities_length == 0);
}
END_TEST

START_TEST(test_pair_map_remove_nonexistent_returns_false)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);
	bool removed = w_relationship_registry_remove(reg, 999, 888, 77);
	ck_assert(!removed);
}
END_TEST


/*****************************
*  adjacency operations      *
*****************************/

START_TEST(test_adjacency_both_directions)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);

	w_entity_id a = w_ecs_request_entity(&g_world);
	w_entity_id b = w_ecs_request_entity(&g_world);

	w_relationship_registry_add(reg, a, b, 1);

	// a should see b as adjacent
	struct w_relationship_adjacency_list *adj_a =
		w_relationship_registry_get_adjacent(reg, a);
	ck_assert_ptr_nonnull(adj_a);
	ck_assert_uint_eq(adj_a->entities_length, 1);
	ck_assert_uint_eq(adj_a->entities[0], b);

	// b should see a as adjacent
	struct w_relationship_adjacency_list *adj_b =
		w_relationship_registry_get_adjacent(reg, b);
	ck_assert_ptr_nonnull(adj_b);
	ck_assert_uint_eq(adj_b->entities_length, 1);
	ck_assert_uint_eq(adj_b->entities[0], a);
}
END_TEST

START_TEST(test_adjacency_no_duplicate_entities)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);

	w_entity_id a = w_ecs_request_entity(&g_world);
	w_entity_id b = w_ecs_request_entity(&g_world);

	// multiple relationship components between same pair
	w_relationship_registry_add(reg, a, b, 1);
	w_relationship_registry_add(reg, a, b, 2);
	w_relationship_registry_add(reg, b, a, 3);

	struct w_relationship_adjacency_list *adj =
		w_relationship_registry_get_adjacent(reg, a);
	ck_assert_ptr_nonnull(adj);
	// adjacency deduplicates: only one entry for b
	ck_assert_uint_eq(adj->entities_length, 1);
}
END_TEST

START_TEST(test_adjacency_multiple_neighbors)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);

	w_entity_id a = w_ecs_request_entity(&g_world);
	w_entity_id b = w_ecs_request_entity(&g_world);
	w_entity_id c = w_ecs_request_entity(&g_world);

	w_relationship_registry_add(reg, a, b, 1);
	w_relationship_registry_add(reg, a, c, 2);

	struct w_relationship_adjacency_list *adj =
		w_relationship_registry_get_adjacent(reg, a);
	ck_assert_ptr_nonnull(adj);
	ck_assert_uint_eq(adj->entities_length, 2);
}
END_TEST

START_TEST(test_adjacency_nonexistent_returns_null)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);
	struct w_relationship_adjacency_list *adj =
		w_relationship_registry_get_adjacent(reg, 12345);
	ck_assert_ptr_null(adj);
}
END_TEST

START_TEST(test_remove_entity_clears_all)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);

	w_entity_id a = w_ecs_request_entity(&g_world);
	w_entity_id b = w_ecs_request_entity(&g_world);
	w_entity_id c = w_ecs_request_entity(&g_world);

	w_relationship_registry_add(reg, a, b, 1);
	w_relationship_registry_add(reg, a, c, 2);
	w_relationship_registry_add(reg, b, c, 3);

	w_relationship_registry_remove_entity(reg, a);

	// a's adjacency gone
	ck_assert_ptr_null(w_relationship_registry_get_adjacent(reg, a));

	// a-b pair gone
	ck_assert_ptr_null(w_relationship_registry_get_pair(reg, a, b));

	// a-c pair gone
	ck_assert_ptr_null(w_relationship_registry_get_pair(reg, a, c));

	// b-c pair untouched
	struct w_relationship_entry_list *bc =
		w_relationship_registry_get_pair(reg, b, c);
	ck_assert_ptr_nonnull(bc);
	ck_assert_uint_eq(bc->entries_length, 1);

	// b's adjacency should only have c (not a)
	struct w_relationship_adjacency_list *adj_b =
		w_relationship_registry_get_adjacent(reg, b);
	ck_assert_ptr_nonnull(adj_b);
	ck_assert_uint_eq(adj_b->entities_length, 1);
	ck_assert_uint_eq(adj_b->entities[0], c);
}
END_TEST

START_TEST(test_remove_entity_no_relationships_is_noop)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);
	// should not crash
	w_relationship_registry_remove_entity(reg, 99999);
}
END_TEST


/*****************************
*  world integration tests   *
*****************************/

START_TEST(test_world_api_add_and_query)
{
	w_entity_id a = w_ecs_request_entity(&g_world);
	w_entity_id b = w_ecs_request_entity(&g_world);

	wm_relationships_add(&g_world, a, b, 42);

	struct w_relationship_entry_list *list =
		wm_relationships_get_pair(&g_world, a, b);
	ck_assert_ptr_nonnull(list);
	ck_assert_uint_eq(list->entries_length, 1);

	struct w_relationship_adjacency_list *adj =
		wm_relationships_get_adjacent(&g_world, a);
	ck_assert_ptr_nonnull(adj);
	ck_assert_uint_eq(adj->entities_length, 1);
}
END_TEST

START_TEST(test_world_api_remove)
{
	w_entity_id a = w_ecs_request_entity(&g_world);
	w_entity_id b = w_ecs_request_entity(&g_world);

	wm_relationships_add(&g_world, a, b, 42);
	bool removed = wm_relationships_remove(&g_world, a, b, 42);
	ck_assert(removed);

	struct w_relationship_entry_list *list =
		wm_relationships_get_pair(&g_world, a, b);
	ck_assert(list == NULL || list->entries_length == 0);
}
END_TEST

START_TEST(test_cascade_entity_destroy_unbuffered)
{
	// when buffering is off, entity destroy fires synchronously
	// which queues a buffered command, then we manually flush
	w_entity_id parent = w_ecs_request_entity(&g_world);
	w_entity_id child = w_ecs_request_entity(&g_world);

	wm_relationships_add(&g_world, parent, child, 1);

	// confirm relationship exists
	struct w_relationship_entry_list *list =
		wm_relationships_get_pair(&g_world, parent, child);
	ck_assert_ptr_nonnull(list);
	ck_assert_uint_eq(list->entries_length, 1);

	// destroy parent (buffering off, hook fires synchronously, queues cleanup cmd)
	g_world.buffering_enabled = false;
	w_ecs_return_entity(&g_world, parent);

	// the entity_destroy hook queued a cleanup command, flush it
	w_command_buffer_flush(&g_world.command_buffer);

	// relationships should be cleaned up
	list = wm_relationships_get_pair(&g_world, parent, child);
	ck_assert_ptr_null(list);

	// child's adjacency should no longer reference parent
	struct w_relationship_adjacency_list *adj =
		wm_relationships_get_adjacent(&g_world, child);
	ck_assert(adj == NULL || adj->entities_length == 0);
}
END_TEST

START_TEST(test_cascade_entity_destroy_buffered)
{
	// when buffering is on, return_entity is buffered
	// flushing processes return_entity cmd -> hook fires -> queues cleanup cmd
	// flush loop picks up cleanup cmd in same cycle
	w_entity_id parent = w_ecs_request_entity(&g_world);
	w_entity_id child = w_ecs_request_entity(&g_world);

	wm_relationships_add(&g_world, parent, child, 1);

	// enable buffering (simulates being inside update loop)
	g_world.buffering_enabled = true;

	// this queues the return_entity command
	w_ecs_return_entity(&g_world, parent);

	// relationships still exist (nothing flushed yet)
	struct w_relationship_entry_list *list =
		wm_relationships_get_pair(&g_world, parent, child);
	ck_assert_ptr_nonnull(list);

	// flush: processes return_entity -> hook fires -> queues cleanup -> cleanup runs
	w_command_buffer_flush(&g_world.command_buffer);
	g_world.buffering_enabled = false;

	// relationships should be cleaned up
	list = wm_relationships_get_pair(&g_world, parent, child);
	ck_assert_ptr_null(list);
}
END_TEST

START_TEST(test_cascade_destroy_both_entities)
{
	// destroy both entities in a relationship, cleanup should be idempotent
	w_entity_id a = w_ecs_request_entity(&g_world);
	w_entity_id b = w_ecs_request_entity(&g_world);

	wm_relationships_add(&g_world, a, b, 1);

	g_world.buffering_enabled = true;
	w_ecs_return_entity(&g_world, a);
	w_ecs_return_entity(&g_world, b);

	// flush should handle both cleanups without crash
	w_command_buffer_flush(&g_world.command_buffer);
	g_world.buffering_enabled = false;

	ck_assert_ptr_null(wm_relationships_get_pair(&g_world, a, b));
	ck_assert_ptr_null(wm_relationships_get_adjacent(&g_world, a));
	ck_assert_ptr_null(wm_relationships_get_adjacent(&g_world, b));
}
END_TEST

START_TEST(test_cascade_with_update_cycle)
{
	// register a phase and timestep so w_ecs_update can run
	struct w_scheduler_time_step ts = { .enabled = true };
	size_t ts_id = w_ecs_register_system_time_step(&g_world, &ts);
	(void)ts_id;

	struct w_scheduler_phase phase = { .enabled = true };
	w_ecs_register_system_phase(&g_world, &phase);

	w_entity_id parent = w_ecs_request_entity(&g_world);
	w_entity_id child = w_ecs_request_entity(&g_world);

	wm_relationships_add(&g_world, parent, child, 1);

	// run one update cycle to enable buffering, then destroy parent
	w_ecs_update(&g_world);

	// buffering is now on (world keeps it on after update)
	g_world.buffering_enabled = true;
	w_ecs_return_entity(&g_world, parent);

	// run another update cycle to flush
	w_ecs_update(&g_world);

	// relationships should be cleaned up after the flush at phase end
	struct w_relationship_entry_list *list =
		wm_relationships_get_pair(&g_world, parent, child);
	ck_assert_ptr_null(list);
}
END_TEST

/*****************************
*  hook integration tests    *
*****************************/

START_TEST(test_hook_set_component_tracks_relationship)
{
	w_entity_id parent = w_ecs_request_entity(&g_world);
	w_entity_id child = w_ecs_request_entity(&g_world);
	w_entity_id comp_type = w_ecs_get_component_by_name(&g_world, "child_of");

	// setting a w_entity_id component should auto-track the relationship
	w_entity_id target = child;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_w_entity_id, comp_type, parent, &target, sizeof(target));

	struct w_relationship_entry_list *list =
		wm_relationships_get_pair(&g_world, parent, child);
	ck_assert_ptr_nonnull(list);
	ck_assert_uint_eq(list->entries_length, 1);
	ck_assert_uint_eq(list->entries[0].owner, parent);
	ck_assert_uint_eq(list->entries[0].target, child);
	ck_assert_uint_eq(list->entries[0].component_id, comp_type);

	struct w_relationship_adjacency_list *adj =
		wm_relationships_get_adjacent(&g_world, parent);
	ck_assert_ptr_nonnull(adj);
	ck_assert_uint_eq(adj->entities_length, 1);
}
END_TEST

START_TEST(test_hook_remove_component_cleans_relationship)
{
	w_entity_id parent = w_ecs_request_entity(&g_world);
	w_entity_id child = w_ecs_request_entity(&g_world);
	w_entity_id comp_type = w_ecs_get_component_by_name(&g_world, "child_of");

	// set component (auto-tracks relationship)
	w_entity_id target = child;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_w_entity_id, comp_type, parent, &target, sizeof(target));

	// remove component should auto-clean the relationship
	w_ecs_remove_component_(&g_world, comp_type, parent);

	struct w_relationship_entry_list *list =
		wm_relationships_get_pair(&g_world, parent, child);
	ck_assert(list == NULL || list->entries_length == 0);

	struct w_relationship_adjacency_list *adj =
		wm_relationships_get_adjacent(&g_world, parent);
	ck_assert(adj == NULL || adj->entities_length == 0);
}
END_TEST

START_TEST(test_hook_destroy_entity_cascades)
{
	w_entity_id parent = w_ecs_request_entity(&g_world);
	w_entity_id child = w_ecs_request_entity(&g_world);
	w_entity_id comp_type = w_ecs_get_component_by_name(&g_world, "child_of");

	// set component (auto-tracks relationship)
	w_entity_id target = child;
	w_ecs_set_component_(&g_world, W_COMPONENT_TYPE_w_entity_id, comp_type, parent, &target, sizeof(target));

	// destroy parent entity (unbuffered, fires hook synchronously)
	g_world.buffering_enabled = false;
	w_ecs_return_entity(&g_world, parent);
	w_command_buffer_flush(&g_world.command_buffer);

	// relationships should be cascaded away
	struct w_relationship_entry_list *list =
		wm_relationships_get_pair(&g_world, parent, child);
	ck_assert_ptr_null(list);

	struct w_relationship_adjacency_list *adj =
		wm_relationships_get_adjacent(&g_world, parent);
	ck_assert(adj == NULL || adj->entities_length == 0);
}
END_TEST


START_TEST(test_many_relationships_stress)
{
	struct w_relationship_registry *reg = wm_relationships_get_registry(&g_world);

	// create a star topology: center connected to 100 nodes
	w_entity_id center = w_ecs_request_entity(&g_world);
	w_entity_id nodes[100];

	for (int i = 0; i < 100; ++i)
	{
		nodes[i] = w_ecs_request_entity(&g_world);
		w_relationship_registry_add(reg, center, nodes[i], (w_entity_id)i);
	}

	// center should have 100 adjacent entities
	struct w_relationship_adjacency_list *adj =
		w_relationship_registry_get_adjacent(reg, center);
	ck_assert_ptr_nonnull(adj);
	ck_assert_uint_eq(adj->entities_length, 100);

	// remove center, all relationships should be cleaned up
	w_relationship_registry_remove_entity(reg, center);

	ck_assert_ptr_null(w_relationship_registry_get_adjacent(reg, center));

	// each node should have no adjacency to center
	for (int i = 0; i < 100; ++i)
	{
		struct w_relationship_adjacency_list *node_adj =
			w_relationship_registry_get_adjacent(reg, nodes[i]);
		ck_assert(node_adj == NULL || node_adj->entities_length == 0);
	}
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *relationships_suite(void)
{
	Suite *s = suite_create("relationships");

	TCase *tc_smoke = tcase_create("smoke");
	tcase_add_checked_fixture(tc_smoke, relationships_setup, relationships_teardown);
	tcase_set_timeout(tc_smoke, 10);
	tcase_add_test(tc_smoke, test_init_free_no_crash);
	tcase_add_test(tc_smoke, test_registry_singleton_exists);
	suite_add_tcase(s, tc_smoke);

	TCase *tc_pair = tcase_create("pair_encoding");
	tcase_add_checked_fixture(tc_pair, relationships_setup, relationships_teardown);
	tcase_set_timeout(tc_pair, 10);
	tcase_add_test(tc_pair, test_pack_pair_order_independent);
	tcase_add_test(tc_pair, test_pack_pair_same_entity);
	tcase_add_test(tc_pair, test_unpack_pair_roundtrip);
	tcase_add_test(tc_pair, test_pack_pair_different_pairs_differ);
	tcase_add_test(tc_pair, test_pack_pair_zero_entity);
	tcase_add_test(tc_pair, test_pack_pair_max_entity);
	suite_add_tcase(s, tc_pair);

	TCase *tc_pairmap = tcase_create("pair_map");
	tcase_add_checked_fixture(tc_pairmap, relationships_setup, relationships_teardown);
	tcase_set_timeout(tc_pairmap, 10);
	tcase_add_test(tc_pairmap, test_pair_map_add_and_get);
	tcase_add_test(tc_pairmap, test_pair_map_get_reversed_order);
	tcase_add_test(tc_pairmap, test_pair_map_multiple_components_same_pair);
	tcase_add_test(tc_pairmap, test_pair_map_duplicate_add_ignored);
	tcase_add_test(tc_pairmap, test_pair_map_get_nonexistent_returns_null);
	tcase_add_test(tc_pairmap, test_pair_map_remove_specific);
	tcase_add_test(tc_pairmap, test_pair_map_remove_last_cleans_adjacency);
	tcase_add_test(tc_pairmap, test_pair_map_remove_nonexistent_returns_false);
	suite_add_tcase(s, tc_pairmap);

	TCase *tc_adj = tcase_create("adjacency");
	tcase_add_checked_fixture(tc_adj, relationships_setup, relationships_teardown);
	tcase_set_timeout(tc_adj, 10);
	tcase_add_test(tc_adj, test_adjacency_both_directions);
	tcase_add_test(tc_adj, test_adjacency_no_duplicate_entities);
	tcase_add_test(tc_adj, test_adjacency_multiple_neighbors);
	tcase_add_test(tc_adj, test_adjacency_nonexistent_returns_null);
	tcase_add_test(tc_adj, test_remove_entity_clears_all);
	tcase_add_test(tc_adj, test_remove_entity_no_relationships_is_noop);
	suite_add_tcase(s, tc_adj);

	TCase *tc_world = tcase_create("world_integration");
	tcase_add_checked_fixture(tc_world, relationships_setup, relationships_teardown);
	tcase_set_timeout(tc_world, 10);
	tcase_add_test(tc_world, test_world_api_add_and_query);
	tcase_add_test(tc_world, test_world_api_remove);
	tcase_add_test(tc_world, test_cascade_entity_destroy_unbuffered);
	tcase_add_test(tc_world, test_cascade_entity_destroy_buffered);
	tcase_add_test(tc_world, test_cascade_destroy_both_entities);
	tcase_add_test(tc_world, test_cascade_with_update_cycle);
	tcase_add_test(tc_world, test_many_relationships_stress);
	suite_add_tcase(s, tc_world);

	TCase *tc_hooks = tcase_create("hooks");
	tcase_add_checked_fixture(tc_hooks, relationships_setup, relationships_teardown);
	tcase_set_timeout(tc_hooks, 10);
	tcase_add_test(tc_hooks, test_hook_set_component_tracks_relationship);
	tcase_add_test(tc_hooks, test_hook_remove_component_cleans_relationship);
	tcase_add_test(tc_hooks, test_hook_destroy_entity_cascades);
	suite_add_tcase(s, tc_hooks);

	return s;
}

int main(void)
{
	Suite *s = relationships_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
