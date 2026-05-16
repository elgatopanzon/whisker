/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : test_whisker_id_pool
 * @created     : Saturday May 16, 2026 15:32:09 CST
 * @description : tests for whisker_id_pool.h ID management
 */

#include "whisker_std.h"
#include "whisker_id_pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <check.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_id_pool g_pool;

static void id_pool_setup(void)
{
	w_id_pool_init(&g_pool, 0);
}

static void id_pool_teardown(void)
{
	w_id_pool_free(&g_pool);
}


/*****************************
*  id_pool_init              *
*****************************/

START_TEST(test_init_next_id_zero)
{
	ck_assert_int_eq(g_pool.next_id, 0);
}
END_TEST

START_TEST(test_init_recycled_empty)
{
	ck_assert_int_eq(g_pool.recycled_length, 0);
}
END_TEST

START_TEST(test_init_custom_block_size)
{
	struct w_id_pool pool;
	w_id_pool_init(&pool, 128);
	ck_assert_int_eq(pool.realloc_block_size, 128);
	w_id_pool_free(&pool);
}
END_TEST

START_TEST(test_init_default_block_size)
{
	ck_assert_int_eq(g_pool.realloc_block_size, W_ID_POOL_REALLOC_BLOCK_SIZE);
}
END_TEST


/*****************************
*  basic request/return      *
*****************************/

START_TEST(test_request_returns_zero_first)
{
	w_id id = w_id_pool_request(&g_pool);
	ck_assert_int_eq(id, 0);
}
END_TEST

START_TEST(test_request_increments_id)
{
	w_id id1 = w_id_pool_request(&g_pool);
	w_id id2 = w_id_pool_request(&g_pool);
	w_id id3 = w_id_pool_request(&g_pool);
	ck_assert_int_eq(id1, 0);
	ck_assert_int_eq(id2, 1);
	ck_assert_int_eq(id3, 2);
}
END_TEST

START_TEST(test_request_id_is_valid)
{
	w_id id = w_id_pool_request(&g_pool);
	ck_assert_int_ne(id, W_ID_POOL_INVALID);
}
END_TEST

START_TEST(test_return_updates_recycled_count)
{
	w_id id = w_id_pool_request(&g_pool);
	ck_assert_int_eq(g_pool.recycled_length, 0);
	w_id_pool_return(&g_pool, id);
	ck_assert_int_eq(g_pool.recycled_length, 1);
}
END_TEST

START_TEST(test_alive_count_increments)
{
	ck_assert_int_eq(w_id_pool_alive_count(&g_pool), 0);
	w_id_pool_request(&g_pool);
	ck_assert_int_eq(w_id_pool_alive_count(&g_pool), 1);
	w_id_pool_request(&g_pool);
	ck_assert_int_eq(w_id_pool_alive_count(&g_pool), 2);
}
END_TEST

START_TEST(test_alive_count_decrements_on_return)
{
	w_id id1 = w_id_pool_request(&g_pool);
	w_id id2 = w_id_pool_request(&g_pool);
	ck_assert_int_eq(w_id_pool_alive_count(&g_pool), 2);
	w_id_pool_return(&g_pool, id1);
	ck_assert_int_eq(w_id_pool_alive_count(&g_pool), 1);
	w_id_pool_return(&g_pool, id2);
	ck_assert_int_eq(w_id_pool_alive_count(&g_pool), 0);
}
END_TEST


/*****************************
*  recycling                 *
*****************************/

START_TEST(test_recycle_reuses_returned_id)
{
	w_id id1 = w_id_pool_request(&g_pool);
	w_id_pool_return(&g_pool, id1);
	w_id id2 = w_id_pool_request(&g_pool);
	ck_assert_int_eq(id1, id2);
}
END_TEST

START_TEST(test_recycle_lifo_order)
{
	w_id a = w_id_pool_request(&g_pool);
	w_id b = w_id_pool_request(&g_pool);
	w_id c = w_id_pool_request(&g_pool);
	w_id_pool_return(&g_pool, a);
	w_id_pool_return(&g_pool, b);
	w_id_pool_return(&g_pool, c);
	// LIFO: c returned last, requested first
	ck_assert_int_eq(w_id_pool_request(&g_pool), c);
	ck_assert_int_eq(w_id_pool_request(&g_pool), b);
	ck_assert_int_eq(w_id_pool_request(&g_pool), a);
}
END_TEST

START_TEST(test_recycle_exhausted_falls_to_new)
{
	w_id id1 = w_id_pool_request(&g_pool);
	w_id_pool_return(&g_pool, id1);
	w_id id2 = w_id_pool_request(&g_pool);
	ck_assert_int_eq(id2, id1);
	// recycled exhausted, should get next_id
	w_id id3 = w_id_pool_request(&g_pool);
	ck_assert_int_eq(id3, 1);
}
END_TEST

START_TEST(test_recycle_multiple_cycles)
{
	for (int cycle = 0; cycle < 5; cycle++) {
		w_id id = w_id_pool_request(&g_pool);
		ck_assert_int_eq(id, 0);
		w_id_pool_return(&g_pool, id);
	}
	// next_id should still be 1 (never incremented past the recycling)
	ck_assert_int_eq(g_pool.next_id, 1);
}
END_TEST


/*****************************
*  stress test               *
*****************************/

START_TEST(test_stress_many_ids)
{
	const int count = 10000;
	w_id *ids = malloc(count * sizeof(w_id));
	ck_assert_ptr_nonnull(ids);

	// request many IDs
	for (int i = 0; i < count; i++) {
		ids[i] = w_id_pool_request(&g_pool);
		ck_assert_int_eq(ids[i], i);
	}
	ck_assert_int_eq(w_id_pool_alive_count(&g_pool), count);

	// return half
	for (int i = 0; i < count / 2; i++) {
		w_id_pool_return(&g_pool, ids[i]);
	}
	ck_assert_int_eq(w_id_pool_alive_count(&g_pool), count / 2);

	// request again, should recycle
	for (int i = 0; i < count / 2; i++) {
		w_id recycled = w_id_pool_request(&g_pool);
		ck_assert_int_ne(recycled, W_ID_POOL_INVALID);
	}
	ck_assert_int_eq(w_id_pool_alive_count(&g_pool), count);

	free(ids);
}
END_TEST

START_TEST(test_stress_request_return_cycles)
{
	const int cycles = 1000;
	for (int i = 0; i < cycles; i++) {
		w_id id = w_id_pool_request(&g_pool);
		ck_assert_int_ne(id, W_ID_POOL_INVALID);
		w_id_pool_return(&g_pool, id);
	}
	// should have only used one ID slot due to recycling
	ck_assert_int_eq(g_pool.next_id, 1);
	ck_assert_int_eq(w_id_pool_alive_count(&g_pool), 0);
}
END_TEST


/*****************************
*  id_pool_free              *
*****************************/

START_TEST(test_free_empty_pool)
{
	struct w_id_pool pool;
	w_id_pool_init(&pool, 0);
	w_id_pool_free(&pool);

	ck_assert_ptr_null(pool.recycled);
	ck_assert_int_eq(pool.next_id, 0);
	ck_assert_int_eq(pool.recycled_length, 0);
}
END_TEST

START_TEST(test_free_with_ids)
{
	struct w_id_pool pool;
	w_id_pool_init(&pool, 0);

	for (int i = 0; i < 10; i++) {
		w_id_pool_request(&pool);
	}

	w_id_pool_free(&pool);

	ck_assert_ptr_null(pool.recycled);
	ck_assert_int_eq(pool.next_id, 0);
}
END_TEST


/*****************************
*  thread safety             *
*****************************/

#define THREAD_COUNT 8
#define REQUESTS_PER_THREAD 1000

struct thread_test_ctx
{
	struct w_id_pool *pool;
	w_id *ids;
	int thread_idx;
};

static void *thread_request_ids(void *arg)
{
	struct thread_test_ctx *ctx = (struct thread_test_ctx *)arg;
	int base = ctx->thread_idx * REQUESTS_PER_THREAD;

	for (int i = 0; i < REQUESTS_PER_THREAD; i++) {
		ctx->ids[base + i] = w_id_pool_request(ctx->pool);
	}
	return NULL;
}

START_TEST(test_thread_no_duplicate_ids)
{
	const int total = THREAD_COUNT * REQUESTS_PER_THREAD;
	w_id *ids = malloc(total * sizeof(w_id));
	ck_assert_ptr_nonnull(ids);

	pthread_t threads[THREAD_COUNT];
	struct thread_test_ctx ctxs[THREAD_COUNT];

	for (int i = 0; i < THREAD_COUNT; i++) {
		ctxs[i].pool = &g_pool;
		ctxs[i].ids = ids;
		ctxs[i].thread_idx = i;
		pthread_create(&threads[i], NULL, thread_request_ids, &ctxs[i]);
	}

	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_join(threads[i], NULL);
	}

	// verify all IDs are valid
	for (int i = 0; i < total; i++) {
		ck_assert_int_ne(ids[i], W_ID_POOL_INVALID);
	}

	// verify no duplicates using simple sort
	for (int i = 0; i < total - 1; i++) {
		for (int j = 0; j < total - i - 1; j++) {
			if (ids[j] > ids[j + 1]) {
				w_id tmp = ids[j];
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
	struct w_id_pool *pool;
	int iterations;
};

static void *thread_request_return_cycle(void *arg)
{
	struct thread_request_return_ctx *ctx = (struct thread_request_return_ctx *)arg;

	for (int i = 0; i < ctx->iterations; i++) {
		w_id id = w_id_pool_request(ctx->pool);
		// small work to simulate usage
		volatile int x = 0;
		for (int j = 0; j < 10; j++) x++;
		(void)x;
		w_id_pool_return(ctx->pool, id);
	}
	return NULL;
}

START_TEST(test_thread_request_return_stress)
{
	const int iterations = 500;
	pthread_t threads[THREAD_COUNT];
	struct thread_request_return_ctx ctxs[THREAD_COUNT];

	for (int i = 0; i < THREAD_COUNT; i++) {
		ctxs[i].pool = &g_pool;
		ctxs[i].iterations = iterations;
		pthread_create(&threads[i], NULL, thread_request_return_cycle, &ctxs[i]);
	}

	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_join(threads[i], NULL);
	}

	// after all threads complete with balanced request/return, alive count should be 0
	ck_assert_int_eq(w_id_pool_alive_count(&g_pool), 0);
}
END_TEST

START_TEST(test_thread_high_contention)
{
	const int total = THREAD_COUNT * REQUESTS_PER_THREAD;
	w_id *ids = malloc(total * sizeof(w_id));
	ck_assert_ptr_nonnull(ids);

	pthread_t threads[THREAD_COUNT];
	struct thread_test_ctx ctxs[THREAD_COUNT];

	// all threads start as close together as possible
	for (int i = 0; i < THREAD_COUNT; i++) {
		ctxs[i].pool = &g_pool;
		ctxs[i].ids = ids;
		ctxs[i].thread_idx = i;
	}

	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_create(&threads[i], NULL, thread_request_ids, &ctxs[i]);
	}

	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_join(threads[i], NULL);
	}

	// verify all valid
	for (int i = 0; i < total; i++) {
		ck_assert_int_ne(ids[i], W_ID_POOL_INVALID);
	}

	// sort and verify no duplicates
	for (int i = 0; i < total - 1; i++) {
		for (int j = 0; j < total - i - 1; j++) {
			if (ids[j] > ids[j + 1]) {
				w_id tmp = ids[j];
				ids[j] = ids[j + 1];
				ids[j + 1] = tmp;
			}
		}
	}

	for (int i = 0; i < total - 1; i++) {
		ck_assert_msg(ids[i] != ids[i + 1],
			"Duplicate ID %u at high contention", ids[i]);
	}

	free(ids);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *whisker_id_pool_suite(void)
{
	Suite *s = suite_create("whisker_id_pool");

	TCase *tc_init = tcase_create("id_pool_init");
	tcase_add_checked_fixture(tc_init, id_pool_setup, id_pool_teardown);
	tcase_set_timeout(tc_init, 10);
	tcase_add_test(tc_init, test_init_next_id_zero);
	tcase_add_test(tc_init, test_init_recycled_empty);
	tcase_add_test(tc_init, test_init_custom_block_size);
	tcase_add_test(tc_init, test_init_default_block_size);
	suite_add_tcase(s, tc_init);

	TCase *tc_basic = tcase_create("basic_request_return");
	tcase_add_checked_fixture(tc_basic, id_pool_setup, id_pool_teardown);
	tcase_set_timeout(tc_basic, 10);
	tcase_add_test(tc_basic, test_request_returns_zero_first);
	tcase_add_test(tc_basic, test_request_increments_id);
	tcase_add_test(tc_basic, test_request_id_is_valid);
	tcase_add_test(tc_basic, test_return_updates_recycled_count);
	tcase_add_test(tc_basic, test_alive_count_increments);
	tcase_add_test(tc_basic, test_alive_count_decrements_on_return);
	suite_add_tcase(s, tc_basic);

	TCase *tc_recycle = tcase_create("recycling");
	tcase_add_checked_fixture(tc_recycle, id_pool_setup, id_pool_teardown);
	tcase_set_timeout(tc_recycle, 10);
	tcase_add_test(tc_recycle, test_recycle_reuses_returned_id);
	tcase_add_test(tc_recycle, test_recycle_lifo_order);
	tcase_add_test(tc_recycle, test_recycle_exhausted_falls_to_new);
	tcase_add_test(tc_recycle, test_recycle_multiple_cycles);
	suite_add_tcase(s, tc_recycle);

	TCase *tc_stress = tcase_create("stress_test");
	tcase_add_checked_fixture(tc_stress, id_pool_setup, id_pool_teardown);
	tcase_set_timeout(tc_stress, 30);
	tcase_add_test(tc_stress, test_stress_many_ids);
	tcase_add_test(tc_stress, test_stress_request_return_cycles);
	suite_add_tcase(s, tc_stress);

	TCase *tc_free = tcase_create("id_pool_free");
	tcase_set_timeout(tc_free, 10);
	tcase_add_test(tc_free, test_free_empty_pool);
	tcase_add_test(tc_free, test_free_with_ids);
	suite_add_tcase(s, tc_free);

	TCase *tc_thread = tcase_create("thread_safety");
	tcase_add_checked_fixture(tc_thread, id_pool_setup, id_pool_teardown);
	tcase_set_timeout(tc_thread, 60);
	tcase_add_test(tc_thread, test_thread_no_duplicate_ids);
	tcase_add_test(tc_thread, test_thread_request_return_stress);
	tcase_add_test(tc_thread, test_thread_high_contention);
	suite_add_tcase(s, tc_thread);

	return s;
}

int main(void)
{
	Suite *s = whisker_id_pool_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
