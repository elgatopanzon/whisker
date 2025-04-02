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

#define WHISKER_THREAD_POOL_WORK_QUEUE_BLOCK_SIZE 16384 / sizeof(w_thread_pool_work)

struct w_thread_pool_context;
typedef void (*w_thread_pool_func)(void *arg, struct w_thread_pool_context *context);

typedef struct w_thread_pool_work
{
	w_thread_pool_func func;
	void *arg;
	uint64_t thread_id;
} w_thread_pool_work;

typedef struct w_thread_pool
{
	// optional friendly name for the thread pool
	char *name;

	// thread count in pool
	size_t thread_max;
	size_t thread_count;
	size_t thread_count_working;
	w_array_declare(struct w_thread_pool_context, thread_contexts)

	// work queue holds list of work items to execute on the thread
	w_array_declare(struct w_thread_pool_work *, work_queue)

	// work pool holds unused work items recycled after being processed
	w_array_declare(struct w_thread_pool_work *, work_pool)

	// mutex and thread conditions to signal working/not working
	pthread_mutex_t thread_mutex_worker;
	pthread_cond_t thread_new_work_signal;
	pthread_cond_t thread_working_signal;

	// stop all threads when true
	bool stop;
} w_thread_pool;

typedef struct w_thread_pool_context
{
	w_thread_pool *thread_pool;
	uint64_t thread_id;
} w_thread_pool_context;

int w_thread_pool_system_core_count();

// management functions
w_thread_pool *w_thread_pool_create();
w_thread_pool *w_thread_pool_create_and_init(size_t count, char *name);
void w_thread_pool_init(w_thread_pool *tp, size_t count, char *name);
void w_thread_pool_free(w_thread_pool *tp);
void w_thread_pool_free_all(w_thread_pool *tp);

// operation functions
void w_thread_pool_queue_work(w_thread_pool *tp, w_thread_pool_func func, void *arg);
void w_thread_pool_queue_work_item(w_thread_pool *tp, w_thread_pool_work *work);
void w_thread_pool_queue_work_all(w_thread_pool *tp, w_thread_pool_func func, void *arg);
void w_thread_pool_wait_work(w_thread_pool *tp);

// thread work functions
w_thread_pool_work *w_thread_pool_create_work(w_thread_pool_func func, void* arg);
w_thread_pool_work *w_thread_pool_get_new_work_item(w_thread_pool *tp, w_thread_pool_func func, void* arg);
void w_thread_pool_free_work(w_thread_pool_work *work);
w_thread_pool_work *w_thread_pool_get_work(w_thread_pool *tp);
void *w_thread_pool_worker_func_(void *arg);

#endif /* WHISKER_THREAD_POOL_H */

