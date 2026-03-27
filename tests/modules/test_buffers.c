/**
 * @author      : ElGatoPanzon
 * @file        : test_buffers
 * @created     : Thursday Mar 26, 2026 20:32:23 CST
 * @description : Tests for whisker_buffers rental system
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"
#include "whisker_buffers.h"
#include "whisker_serialisation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>

/* custom struct for testing arbitrary-type buffers */
typedef struct {
	float x;
	float y;
	float z;
	uint32_t id;
} test_vertex;


/*****************************
*  fixture                   *
*****************************/

static struct w_ecs_world g_world;
static struct w_string_table g_string_table;
static struct w_arena g_arena;

static void buffers_setup(void)
{
	w_arena_init(&g_arena, 4096);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);
	w_buffers_init(&g_world);
}

static void buffers_teardown(void)
{
	w_buffers_free(&g_world);
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}


/* fixture with serialisation module enabled */
static void buffers_serial_setup(void)
{
	w_arena_init(&g_arena, 4096);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);
	wm_serialisation_init(&g_world);
	w_buffers_init(&g_world);
}

static void buffers_serial_teardown(void)
{
	w_buffers_free(&g_world);
	wm_serialisation_free(&g_world);
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}


/*****************************
*  init/free smoke tests     *
*****************************/

START_TEST(test_init_free_no_crash)
{
	ck_assert(true);
}
END_TEST


/*****************************
*  buffer create tests       *
*****************************/

START_TEST(test_create_returns_valid_handle)
{
	w_pack32x2 h = w_buffer_create_typed(&g_world, "buf_u32", uint32_t, 10);
	ck_assert_uint_ne(W_BUFFER_HANDLE_OFFSET(h), UINT32_MAX);
	ck_assert_uint_eq(W_BUFFER_HANDLE_LENGTH(h), 10);
}
END_TEST

START_TEST(test_create_has_meta)
{
	w_buffer_create_typed(&g_world, "buf_meta", float, 5);
	struct w_buffer_meta *meta = w_buffer_get_meta(&g_world, "buf_meta");

	ck_assert_ptr_nonnull(meta);
	ck_assert_uint_eq(meta->type_id, W_COMPONENT_TYPE_float);
	ck_assert_uint_eq(meta->type_size, sizeof(float));
}
END_TEST

START_TEST(test_create_zero_count_fails)
{
	w_pack32x2 h = w_buffer_create_typed(&g_world, "buf_z", uint32_t, 0);
	ck_assert_uint_eq(W_BUFFER_HANDLE_OFFSET(h), UINT32_MAX);
}
END_TEST

START_TEST(test_create_zero_size_fails)
{
	w_pack32x2 h = w_buffer_create(&g_world, "buf_zero", 0, 0, 10);
	ck_assert_uint_eq(W_BUFFER_HANDLE_OFFSET(h), UINT32_MAX);
}
END_TEST

START_TEST(test_create_null_name_fails)
{
	w_pack32x2 h = w_buffer_create(&g_world, NULL, W_COMPONENT_TYPE_uint32_t,
		sizeof(uint32_t), 10);
	ck_assert_uint_eq(W_BUFFER_HANDLE_OFFSET(h), UINT32_MAX);
}
END_TEST

START_TEST(test_create_typed_macro)
{
	w_buffer_create_typed(&g_world, "buf_dbl", double, 3);
	struct w_buffer_meta *meta = w_buffer_get_meta(&g_world, "buf_dbl");

	ck_assert_ptr_nonnull(meta);
	ck_assert_uint_eq(meta->type_id, W_COMPONENT_TYPE_double);
	ck_assert_uint_eq(meta->type_size, sizeof(double));
}
END_TEST

START_TEST(test_create_custom_struct)
{
	w_buffer_create(&g_world, "buf_vtx", 0, sizeof(test_vertex), 8);
	struct w_buffer_meta *meta = w_buffer_get_meta(&g_world, "buf_vtx");

	ck_assert_ptr_nonnull(meta);
	ck_assert_uint_eq(meta->type_size, sizeof(test_vertex));
}
END_TEST

START_TEST(test_create_first_offset_is_zero)
{
	w_pack32x2 h = w_buffer_create_typed(&g_world, "buf_off0", uint32_t, 100);
	ck_assert_uint_eq(W_BUFFER_HANDLE_OFFSET(h), 0);
}
END_TEST


/*****************************
*  rental range tests        *
*****************************/

START_TEST(test_rental_second_range_follows_first)
{
	w_pack32x2 h1 = w_buffer_create_typed(&g_world, "rent_seq", uint32_t, 100);
	w_pack32x2 h2 = w_buffer_create_typed(&g_world, "rent_seq", uint32_t, 50);

	/* second range starts right after first */
	ck_assert_uint_eq(W_BUFFER_HANDLE_OFFSET(h1), 0);
	ck_assert_uint_eq(W_BUFFER_HANDLE_LENGTH(h1), 100);
	ck_assert_uint_eq(W_BUFFER_HANDLE_OFFSET(h2), 100);
	ck_assert_uint_eq(W_BUFFER_HANDLE_LENGTH(h2), 50);
}
END_TEST

START_TEST(test_rental_third_range_follows_second)
{
	w_pack32x2 h1 = w_buffer_create_typed(&g_world, "rent_3", uint32_t, 10);
	w_pack32x2 h2 = w_buffer_create_typed(&g_world, "rent_3", uint32_t, 20);
	w_pack32x2 h3 = w_buffer_create_typed(&g_world, "rent_3", uint32_t, 30);

	ck_assert_uint_eq(W_BUFFER_HANDLE_OFFSET(h1), 0);
	ck_assert_uint_eq(W_BUFFER_HANDLE_OFFSET(h2), 10);
	ck_assert_uint_eq(W_BUFFER_HANDLE_OFFSET(h3), 30);
	ck_assert_uint_eq(W_BUFFER_HANDLE_LENGTH(h3), 30);
}
END_TEST

START_TEST(test_rental_return_and_reuse)
{
	w_pack32x2 h1 = w_buffer_create_typed(&g_world, "rent_reuse", uint32_t, 50);
	w_pack32x2 h2 = w_buffer_create_typed(&g_world, "rent_reuse", uint32_t, 50);

	/* return the first range */
	w_buffer_return(&g_world, "rent_reuse", h1);

	/* new rental should reuse the freed range at offset 0 */
	w_pack32x2 h3 = w_buffer_create_typed(&g_world, "rent_reuse", uint32_t, 50);
	ck_assert_uint_eq(W_BUFFER_HANDLE_OFFSET(h3), 0);

	/* h2 should still be at 50 */
	(void)h2;
}
END_TEST

START_TEST(test_rental_return_partial_reuse)
{
	w_pack32x2 h1 = w_buffer_create_typed(&g_world, "rent_partial", uint32_t, 100);
	w_buffer_create_typed(&g_world, "rent_partial", uint32_t, 50);

	/* return the first range (100 slots at offset 0) */
	w_buffer_return(&g_world, "rent_partial", h1);

	/* rent a smaller range -- should fit in the freed gap */
	w_pack32x2 h3 = w_buffer_create_typed(&g_world, "rent_partial", uint32_t, 30);
	ck_assert_uint_eq(W_BUFFER_HANDLE_OFFSET(h3), 0);
	ck_assert_uint_eq(W_BUFFER_HANDLE_LENGTH(h3), 30);
}
END_TEST


/*****************************
*  data access tests         *
*****************************/

START_TEST(test_get_ptr_returns_valid)
{
	w_pack32x2 h = w_buffer_create_typed(&g_world, "ptr_buf", uint32_t, 10);
	void *ptr = w_buffer_get_ptr(&g_world, "ptr_buf", h);
	ck_assert_ptr_nonnull(ptr);
}
END_TEST

START_TEST(test_u32_write_read_via_ptr)
{
	w_pack32x2 h = w_buffer_create_typed(&g_world, "u32_rw", uint32_t, 10);
	uint32_t *data = (uint32_t *)w_buffer_get_ptr(&g_world, "u32_rw", h);

	for (uint32_t i = 0; i < 10; i++)
		data[i] = i * 100;

	for (uint32_t i = 0; i < 10; i++)
		ck_assert_uint_eq(data[i], i * 100);
}
END_TEST

START_TEST(test_float_write_read_via_ptr)
{
	w_pack32x2 h = w_buffer_create_typed(&g_world, "flt_rw", float, 5);
	float *data = (float *)w_buffer_get_ptr(&g_world, "flt_rw", h);

	data[0] = 3.14f;
	data[1] = 2.718f;
	data[4] = 1.414f;

	ck_assert_float_eq(data[0], 3.14f);
	ck_assert_float_eq(data[1], 2.718f);
	ck_assert_float_eq(data[4], 1.414f);
}
END_TEST

START_TEST(test_double_write_read_via_ptr)
{
	w_pack32x2 h = w_buffer_create_typed(&g_world, "dbl_rw", double, 5);
	double *data = (double *)w_buffer_get_ptr(&g_world, "dbl_rw", h);

	data[0] = 2.718281828;
	data[3] = 9.99;

	ck_assert_double_eq(data[0], 2.718281828);
	ck_assert_double_eq(data[3], 9.99);
}
END_TEST

START_TEST(test_struct_write_read_via_ptr)
{
	w_pack32x2 h = w_buffer_create(&g_world, "vtx_rw", 0, sizeof(test_vertex), 4);
	test_vertex *data = (test_vertex *)w_buffer_get_ptr(&g_world, "vtx_rw", h);

	data[0] = (test_vertex){ .x = 1.0f, .y = 2.0f, .z = 3.0f, .id = 99 };
	data[3] = (test_vertex){ .x = 4.0f, .y = 5.0f, .z = 6.0f, .id = 42 };

	ck_assert_float_eq(data[0].x, 1.0f);
	ck_assert_float_eq(data[0].y, 2.0f);
	ck_assert_float_eq(data[0].z, 3.0f);
	ck_assert_uint_eq(data[0].id, 99);
	ck_assert_uint_eq(data[3].id, 42);
}
END_TEST

START_TEST(test_independent_ranges_no_overlap)
{
	w_pack32x2 h1 = w_buffer_create_typed(&g_world, "no_overlap", uint32_t, 10);
	w_pack32x2 h2 = w_buffer_create_typed(&g_world, "no_overlap", uint32_t, 10);

	uint32_t *d1 = (uint32_t *)w_buffer_get_ptr(&g_world, "no_overlap", h1);
	uint32_t *d2 = (uint32_t *)w_buffer_get_ptr(&g_world, "no_overlap", h2);

	/* write to both ranges */
	for (uint32_t i = 0; i < 10; i++)
	{
		d1[i] = 1000 + i;
		d2[i] = 2000 + i;
	}

	/* verify no cross-contamination */
	for (uint32_t i = 0; i < 10; i++)
	{
		ck_assert_uint_eq(d1[i], 1000 + i);
		ck_assert_uint_eq(d2[i], 2000 + i);
	}
}
END_TEST


/*****************************
*  get_length tests          *
*****************************/

START_TEST(test_get_length_matches_create)
{
	w_pack32x2 h = w_buffer_create_typed(&g_world, "len_buf", uint32_t, 42);
	ck_assert_uint_eq(w_buffer_get_length(h), 42);
}
END_TEST

START_TEST(test_get_length_invalid_handle)
{
	ck_assert_uint_eq(w_buffer_get_length(W_BUFFER_HANDLE_INVALID), 0);
}
END_TEST


/*****************************
*  for_each iteration tests  *
*****************************/

START_TEST(test_for_each_iterates_range)
{
	w_pack32x2 h = w_buffer_create_typed(&g_world, "iter_buf", uint32_t, 10);
	uint32_t *data = (uint32_t *)w_buffer_get_ptr(&g_world, "iter_buf", h);

	for (uint32_t i = 0; i < 10; i++)
		data[i] = i * 10;

	uint32_t count = 0;
	uint32_t sum = 0;
	w_buffer_for_each(h, {
		sum += *((uint32_t *)w_buffer_get_ptr(&g_world, "iter_buf", h) +
			(buf_index - W_BUFFER_HANDLE_OFFSET(h)));
		count++;
	});

	ck_assert_uint_eq(count, 10);
	ck_assert_uint_eq(sum, 0 + 10 + 20 + 30 + 40 + 50 + 60 + 70 + 80 + 90);
}
END_TEST

START_TEST(test_for_each_second_range)
{
	w_buffer_create_typed(&g_world, "iter2", uint32_t, 5);
	w_pack32x2 h2 = w_buffer_create_typed(&g_world, "iter2", uint32_t, 3);

	/* h2 should iterate indices 5, 6, 7 */
	uint32_t count = 0;
	uint32_t first_index = UINT32_MAX;
	w_buffer_for_each(h2, {
		if (first_index == UINT32_MAX) first_index = buf_index;
		count++;
	});

	ck_assert_uint_eq(count, 3);
	ck_assert_uint_eq(first_index, 5);
}
END_TEST

START_TEST(test_for_each_zero_length)
{
	w_pack32x2 h = W_BUFFER_HANDLE_INVALID;
	h.right = 0; /* length = 0 */

	uint32_t count = 0;
	w_buffer_for_each(h, {
		count++;
	});

	ck_assert_uint_eq(count, 0);
}
END_TEST


/*****************************
*  multiple buffer name tests*
*****************************/

START_TEST(test_multiple_buffer_names_independent)
{
	w_pack32x2 h_u32 = w_buffer_create_typed(&g_world, "multi_u32", uint32_t, 5);
	w_pack32x2 h_flt = w_buffer_create_typed(&g_world, "multi_flt", float, 5);

	uint32_t *du = (uint32_t *)w_buffer_get_ptr(&g_world, "multi_u32", h_u32);
	float *df = (float *)w_buffer_get_ptr(&g_world, "multi_flt", h_flt);

	du[0] = 42;
	df[0] = 3.14f;

	ck_assert_uint_eq(du[0], 42);
	ck_assert_float_eq(df[0], 3.14f);

	/* return one doesn't affect the other */
	w_buffer_return(&g_world, "multi_u32", h_u32);
	ck_assert_float_eq(df[0], 3.14f);
}
END_TEST


/*****************************
*  return tests              *
*****************************/

START_TEST(test_return_clears_bits)
{
	w_pack32x2 h = w_buffer_create_typed(&g_world, "ret_bits", uint32_t, 10);

	/* verify bits are set */
	struct w_component_entry *entry = w_buffer_get_entry(&g_world, "ret_bits");
	ck_assert_ptr_nonnull(entry);
	for (uint32_t i = 0; i < 10; i++)
		ck_assert(w_sparse_bitset_get(&entry->data_bitset, i));

	w_buffer_return(&g_world, "ret_bits", h);

	/* bits should be cleared */
	for (uint32_t i = 0; i < 10; i++)
		ck_assert(!w_sparse_bitset_get(&entry->data_bitset, i));
}
END_TEST


/*****************************
*  destroy tests             *
*****************************/

START_TEST(test_destroy_clears_all)
{
	w_buffer_create_typed(&g_world, "destroy_buf", uint32_t, 10);
	ck_assert_ptr_nonnull(w_buffer_get_entry(&g_world, "destroy_buf"));
	ck_assert_ptr_nonnull(w_buffer_get_meta(&g_world, "destroy_buf"));

	w_buffer_destroy(&g_world, "destroy_buf");

	ck_assert_ptr_null(w_buffer_get_entry(&g_world, "destroy_buf"));
	ck_assert_ptr_null(w_buffer_get_meta(&g_world, "destroy_buf"));
}
END_TEST


/*****************************
*  entry access              *
*****************************/

START_TEST(test_get_entry_valid_after_create)
{
	w_buffer_create_typed(&g_world, "entry_buf", uint32_t, 5);
	struct w_component_entry *entry = w_buffer_get_entry(&g_world, "entry_buf");
	ck_assert_ptr_nonnull(entry);
	ck_assert_uint_eq(entry->type_size, sizeof(uint32_t));
}
END_TEST

START_TEST(test_get_entry_null_before_create)
{
	/* entry doesn't exist until buffer_create is called */
	struct w_component_entry *entry = w_buffer_get_entry(&g_world, "nonexistent");
	ck_assert_ptr_null(entry);
}
END_TEST


/*****************************
*  setid serialisation tests *
*****************************/

START_TEST(test_buffer_has_setid_tag)
{
	w_buffer_create_typed(&g_world, "tag_buf", uint32_t, 5);
	w_entity_id buf_id = w_ecs_get_component_by_name(&g_world, "tag_buf");
	ck_assert(w_ecs_has_tag_str(&g_world, WM_SERIALISATION_SERIALISE_AS_ID_TAG_NAME, buf_id));
}
END_TEST

START_TEST(test_e2e_setid_roundtrip_uint32)
{
	w_pack32x2 h = w_buffer_create_typed(&g_world, "rt_u32", uint32_t, 5);
	uint32_t *data = (uint32_t *)w_buffer_get_ptr(&g_world, "rt_u32", h);

	data[0] = 100;
	data[2] = 555;
	data[4] = 99999;

	/* serialise */
	struct wm_serialisation_ctx sctx = {0};
	bool dump_ok = w_serialisation_dump_to_buffer(&g_world, &sctx);
	ck_assert(dump_ok);

	/* verify setid commands for non-zero values */
	ck_assert_ptr_nonnull(strstr(sctx.buffer, "setid 0 \"rt_u32\" uint32_t 100"));
	ck_assert_ptr_nonnull(strstr(sctx.buffer, "setid 2 \"rt_u32\" uint32_t 555"));
	ck_assert_ptr_nonnull(strstr(sctx.buffer, "setid 4 \"rt_u32\" uint32_t 99999"));

	/* restore into a fresh world */
	struct w_ecs_world world2 = {0};
	struct w_string_table st2 = {0};
	struct w_arena arena2 = {0};
	w_arena_init(&arena2, 4096);
	w_string_table_init(&st2, &arena2, 16, 64, NULL);
	w_ecs_world_init(&world2, &st2, &arena2);
	wm_serialisation_init(&world2);
	w_buffers_init(&world2);

	struct wm_deserialisation_ctx dctx = {0};
	bool restore_ok = w_serialisation_restore_from_buffer(&world2,
		sctx.buffer, sctx.buffer_length, &dctx);
	ck_assert(restore_ok);

	/* verify data at exact same indices via raw ECS access */
	w_entity_id buf2 = w_ecs_get_component_by_name(&world2, "rt_u32");
	ck_assert(w_ecs_is_valid_entity(buf2));

	uint32_t *g0 = (uint32_t *)w_ecs_get_component_(&world2, buf2, 0);
	uint32_t *g2 = (uint32_t *)w_ecs_get_component_(&world2, buf2, 2);
	uint32_t *g4 = (uint32_t *)w_ecs_get_component_(&world2, buf2, 4);

	ck_assert_ptr_nonnull(g0);
	ck_assert_ptr_nonnull(g2);
	ck_assert_ptr_nonnull(g4);
	ck_assert_uint_eq(*g0, 100);
	ck_assert_uint_eq(*g2, 555);
	ck_assert_uint_eq(*g4, 99999);

	free(dctx.unparsed);
	free(sctx.buffer);
	free(sctx.entities);
	free(sctx.components);
	w_buffers_free(&world2);
	wm_serialisation_free(&world2);
	w_ecs_world_free(&world2);
	w_string_table_free(&st2);
	w_arena_free(&arena2);
}
END_TEST

START_TEST(test_e2e_setid_roundtrip_float)
{
	w_pack32x2 h = w_buffer_create_typed(&g_world, "rt_flt", float, 5);
	float *data = (float *)w_buffer_get_ptr(&g_world, "rt_flt", h);

	data[0] = 3.14f;
	data[2] = 2.718f;
	data[4] = 1.414f;

	/* serialise */
	struct wm_serialisation_ctx sctx = {0};
	bool dump_ok = w_serialisation_dump_to_buffer(&g_world, &sctx);
	ck_assert(dump_ok);

	ck_assert_ptr_nonnull(strstr(sctx.buffer, "setid"));

	/* restore into a fresh world */
	struct w_ecs_world world2 = {0};
	struct w_string_table st2 = {0};
	struct w_arena arena2 = {0};
	w_arena_init(&arena2, 4096);
	w_string_table_init(&st2, &arena2, 16, 64, NULL);
	w_ecs_world_init(&world2, &st2, &arena2);
	wm_serialisation_init(&world2);
	w_buffers_init(&world2);

	struct wm_deserialisation_ctx dctx = {0};
	bool restore_ok = w_serialisation_restore_from_buffer(&world2,
		sctx.buffer, sctx.buffer_length, &dctx);
	ck_assert(restore_ok);

	w_entity_id buf2 = w_ecs_get_component_by_name(&world2, "rt_flt");

	float *g0 = (float *)w_ecs_get_component_(&world2, buf2, 0);
	float *g2 = (float *)w_ecs_get_component_(&world2, buf2, 2);
	float *g4 = (float *)w_ecs_get_component_(&world2, buf2, 4);

	ck_assert_ptr_nonnull(g0);
	ck_assert_ptr_nonnull(g2);
	ck_assert_ptr_nonnull(g4);
	ck_assert_float_eq_tol(*g0, 3.14f, 0.001f);
	ck_assert_float_eq_tol(*g2, 2.718f, 0.001f);
	ck_assert_float_eq_tol(*g4, 1.414f, 0.001f);

	free(dctx.unparsed);
	free(sctx.buffer);
	free(sctx.entities);
	free(sctx.components);
	w_buffers_free(&world2);
	wm_serialisation_free(&world2);
	w_ecs_world_free(&world2);
	w_string_table_free(&st2);
	w_arena_free(&arena2);
}
END_TEST

START_TEST(test_e2e_setid_roundtrip_multiple_buffers)
{
	w_pack32x2 h_u = w_buffer_create_typed(&g_world, "rt_multi_u", uint32_t, 10);
	w_pack32x2 h_d = w_buffer_create_typed(&g_world, "rt_multi_d", double, 10);

	uint32_t *du = (uint32_t *)w_buffer_get_ptr(&g_world, "rt_multi_u", h_u);
	double *dd = (double *)w_buffer_get_ptr(&g_world, "rt_multi_d", h_d);

	du[7] = 777;
	dd[3] = 42.42;

	/* serialise */
	struct wm_serialisation_ctx sctx = {0};
	bool dump_ok = w_serialisation_dump_to_buffer(&g_world, &sctx);
	ck_assert(dump_ok);

	/* restore into fresh world */
	struct w_ecs_world world2 = {0};
	struct w_string_table st2 = {0};
	struct w_arena arena2 = {0};
	w_arena_init(&arena2, 4096);
	w_string_table_init(&st2, &arena2, 16, 64, NULL);
	w_ecs_world_init(&world2, &st2, &arena2);
	wm_serialisation_init(&world2);
	w_buffers_init(&world2);

	struct wm_deserialisation_ctx dctx = {0};
	bool restore_ok = w_serialisation_restore_from_buffer(&world2,
		sctx.buffer, sctx.buffer_length, &dctx);
	ck_assert(restore_ok);

	/* verify both buffers independently */
	w_entity_id buf_u2 = w_ecs_get_component_by_name(&world2, "rt_multi_u");
	w_entity_id buf_d2 = w_ecs_get_component_by_name(&world2, "rt_multi_d");

	uint32_t *gu = (uint32_t *)w_ecs_get_component_(&world2, buf_u2, 7);
	double *gd = (double *)w_ecs_get_component_(&world2, buf_d2, 3);

	ck_assert_ptr_nonnull(gu);
	ck_assert_ptr_nonnull(gd);
	ck_assert_uint_eq(*gu, 777);
	ck_assert_double_eq_tol(*gd, 42.42, 0.001);

	free(dctx.unparsed);
	free(sctx.buffer);
	free(sctx.entities);
	free(sctx.components);
	w_buffers_free(&world2);
	wm_serialisation_free(&world2);
	w_ecs_world_free(&world2);
	w_string_table_free(&st2);
	w_arena_free(&arena2);
}
END_TEST


/*****************************
*  test suite                *
*****************************/

Suite *buffers_suite(void)
{
	Suite *s = suite_create("buffers");

	/* smoke tests */
	TCase *tc_smoke = tcase_create("smoke");
	tcase_set_timeout(tc_smoke, 10);
	tcase_add_checked_fixture(tc_smoke, buffers_setup, buffers_teardown);
	tcase_add_test(tc_smoke, test_init_free_no_crash);
	suite_add_tcase(s, tc_smoke);

	/* create */
	TCase *tc_create = tcase_create("create");
	tcase_set_timeout(tc_create, 10);
	tcase_add_checked_fixture(tc_create, buffers_setup, buffers_teardown);
	tcase_add_test(tc_create, test_create_returns_valid_handle);
	tcase_add_test(tc_create, test_create_has_meta);
	tcase_add_test(tc_create, test_create_zero_count_fails);
	tcase_add_test(tc_create, test_create_zero_size_fails);
	tcase_add_test(tc_create, test_create_null_name_fails);
	tcase_add_test(tc_create, test_create_typed_macro);
	tcase_add_test(tc_create, test_create_custom_struct);
	tcase_add_test(tc_create, test_create_first_offset_is_zero);
	suite_add_tcase(s, tc_create);

	/* rental ranges */
	TCase *tc_rental = tcase_create("rental");
	tcase_set_timeout(tc_rental, 10);
	tcase_add_checked_fixture(tc_rental, buffers_setup, buffers_teardown);
	tcase_add_test(tc_rental, test_rental_second_range_follows_first);
	tcase_add_test(tc_rental, test_rental_third_range_follows_second);
	tcase_add_test(tc_rental, test_rental_return_and_reuse);
	tcase_add_test(tc_rental, test_rental_return_partial_reuse);
	suite_add_tcase(s, tc_rental);

	/* data access */
	TCase *tc_data = tcase_create("data_access");
	tcase_set_timeout(tc_data, 10);
	tcase_add_checked_fixture(tc_data, buffers_setup, buffers_teardown);
	tcase_add_test(tc_data, test_get_ptr_returns_valid);
	tcase_add_test(tc_data, test_u32_write_read_via_ptr);
	tcase_add_test(tc_data, test_float_write_read_via_ptr);
	tcase_add_test(tc_data, test_double_write_read_via_ptr);
	tcase_add_test(tc_data, test_struct_write_read_via_ptr);
	tcase_add_test(tc_data, test_independent_ranges_no_overlap);
	suite_add_tcase(s, tc_data);

	/* get_length */
	TCase *tc_length = tcase_create("get_length");
	tcase_set_timeout(tc_length, 10);
	tcase_add_checked_fixture(tc_length, buffers_setup, buffers_teardown);
	tcase_add_test(tc_length, test_get_length_matches_create);
	tcase_add_test(tc_length, test_get_length_invalid_handle);
	suite_add_tcase(s, tc_length);

	/* iteration */
	TCase *tc_iter = tcase_create("iteration");
	tcase_set_timeout(tc_iter, 10);
	tcase_add_checked_fixture(tc_iter, buffers_setup, buffers_teardown);
	tcase_add_test(tc_iter, test_for_each_iterates_range);
	tcase_add_test(tc_iter, test_for_each_second_range);
	tcase_add_test(tc_iter, test_for_each_zero_length);
	suite_add_tcase(s, tc_iter);

	/* multiple buffer names */
	TCase *tc_multi = tcase_create("multiple_buffers");
	tcase_set_timeout(tc_multi, 10);
	tcase_add_checked_fixture(tc_multi, buffers_setup, buffers_teardown);
	tcase_add_test(tc_multi, test_multiple_buffer_names_independent);
	suite_add_tcase(s, tc_multi);

	/* return */
	TCase *tc_return = tcase_create("return");
	tcase_set_timeout(tc_return, 10);
	tcase_add_checked_fixture(tc_return, buffers_setup, buffers_teardown);
	tcase_add_test(tc_return, test_return_clears_bits);
	suite_add_tcase(s, tc_return);

	/* destroy */
	TCase *tc_destroy = tcase_create("destroy");
	tcase_set_timeout(tc_destroy, 10);
	tcase_add_checked_fixture(tc_destroy, buffers_setup, buffers_teardown);
	tcase_add_test(tc_destroy, test_destroy_clears_all);
	suite_add_tcase(s, tc_destroy);

	/* entry access */
	TCase *tc_entry = tcase_create("entry_access");
	tcase_set_timeout(tc_entry, 10);
	tcase_add_checked_fixture(tc_entry, buffers_setup, buffers_teardown);
	tcase_add_test(tc_entry, test_get_entry_valid_after_create);
	tcase_add_test(tc_entry, test_get_entry_null_before_create);
	suite_add_tcase(s, tc_entry);

	/* setid serialisation */
	TCase *tc_setid = tcase_create("setid_serialisation");
	tcase_set_timeout(tc_setid, 10);
	tcase_add_checked_fixture(tc_setid, buffers_serial_setup, buffers_serial_teardown);
	tcase_add_test(tc_setid, test_buffer_has_setid_tag);
	tcase_add_test(tc_setid, test_e2e_setid_roundtrip_uint32);
	tcase_add_test(tc_setid, test_e2e_setid_roundtrip_float);
	tcase_add_test(tc_setid, test_e2e_setid_roundtrip_multiple_buffers);
	suite_add_tcase(s, tc_setid);

	return s;
}

int main(void)
{
	Suite *s = buffers_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	int failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
