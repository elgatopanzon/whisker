/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_thread_pool
 * @created     : Thursday Mar 13, 2025 18:00:17 CST
 */

#include <pthread.h>
#include "whisker_std.h"
#include "whisker_memory.h"
#include "whisker_array.h"
#include "whisker_debug.h"

#ifndef WHISKER_THREAD_POOL_H
#define WHISKER_THREAD_POOL_H

#define WHISKER_THREAD_POOL_WORK_QUEUE_BLOCK_SIZE 16384 / sizeof(whisker_thread_pool_work)

struct whisker_thread_pool_context;
typedef void (*whisker_thread_pool_func)(void *arg, struct whisker_thread_pool_context *context);

typedef struct whisker_thread_pool_work
{
	whisker_thread_pool_func func;
	void *arg;
	uint64_t thread_id;
} whisker_thread_pool_work;

typedef struct whisker_thread_pool
{
	// optional friendly name for the thread pool
	char *name;

	// thread count in pool
	size_t thread_max;
	size_t thread_count;
	size_t thread_count_working;
	whisker_arr_declare(struct whisker_thread_pool_context, thread_contexts)

	// work queue holds list of work items to execute on the thread
	whisker_arr_declare(struct whisker_thread_pool_work *, work_queue)

	// work pool holds unused work items recycled after being processed
	whisker_arr_declare(struct whisker_thread_pool_work *, work_pool)

	// mutex and thread conditions to signal working/not working
	pthread_mutex_t thread_mutex_worker;
	pthread_cond_t thread_new_work_signal;
	pthread_cond_t thread_working_signal;

	// stop all threads when true
	bool stop;
} whisker_thread_pool;

typedef struct whisker_thread_pool_context
{
	whisker_thread_pool *thread_pool;
	uint64_t thread_id;
} whisker_thread_pool_context;

int whisker_tp_system_core_count();

// management functions
whisker_thread_pool *whisker_tp_create();
whisker_thread_pool *whisker_tp_create_and_init(size_t count, char *name);
void whisker_tp_init(whisker_thread_pool *tp, size_t count, char *name);
void whisker_tp_free(whisker_thread_pool *tp);
void whisker_tp_free_all(whisker_thread_pool *tp);

// operation functions
void whisker_tp_queue_work(whisker_thread_pool *tp, whisker_thread_pool_func func, void *arg);
void whisker_tp_queue_work_item(whisker_thread_pool *tp, whisker_thread_pool_work *work);
void whisker_tp_queue_work_all(whisker_thread_pool *tp, whisker_thread_pool_func func, void *arg);
void whisker_tp_wait_work(whisker_thread_pool *tp);

// thread work functions
whisker_thread_pool_work *whisker_tp_create_work(whisker_thread_pool_func func, void* arg);
whisker_thread_pool_work *whisker_tp_get_new_work_item(whisker_thread_pool *tp, whisker_thread_pool_func func, void* arg);
void whisker_tp_free_work(whisker_thread_pool_work *work);
whisker_thread_pool_work *whisker_tp_get_work(whisker_thread_pool *tp);
void *whisker_tp_worker_func_(void *arg);

#endif /* WHISKER_THREAD_POOL_H */

