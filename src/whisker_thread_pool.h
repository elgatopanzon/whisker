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
#include "generics/whisker_generic_array_void_.h"

#ifndef WHISKER_THREAD_POOL_H
#define WHISKER_THREAD_POOL_H

// errors
typedef enum E_WHISKER_TP  
{
	E_WHISKER_TP_OK = 0,
	E_WHISKER_TP_UNKNOWN = 1,
	E_WHISKER_TP_MEM = 2,
	E_WHISKER_TP_ARR = 3,
	E_WHISKER_TP_THREAD = 4,
	E_WHISKER_TP_WORK = 5,
	E_WHISKER_TP_MUTEX = 6,
} E_WHISKER_TP;
extern const char* E_WHISKER_TP_STR[];


typedef void (*whisker_thread_pool_func)(void *arg);

typedef struct whisker_thread_pool_work
{
	whisker_thread_pool_func func;
	void *arg;
} whisker_thread_pool_work;

typedef struct whisker_thread_pool
{
	// thread count in pool
	size_t thread_count;
	size_t thread_count_working;
	whisker_arr_void_ *thread_contexts;

	// work queue holds list of work items to execute on the thread
	whisker_arr_void_ *work_queue;

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
E_WHISKER_TP whisker_tp_create_f(whisker_thread_pool **tp, size_t count);
void whisker_tp_free(whisker_thread_pool *tp);

// operation functions
E_WHISKER_TP whisker_tp_queue_work(whisker_thread_pool *tp, whisker_thread_pool_func func, void *arg);
void whisker_tp_wait_work(whisker_thread_pool *tp);

// thread work functions
whisker_thread_pool_work *whisker_tp_create_work(whisker_thread_pool_func func, void* arg);
void whisker_tp_free_work(whisker_thread_pool_work *work);
whisker_thread_pool_work *whisker_tp_get_work(whisker_thread_pool *tp);
void *whisker_tp_worker_func_(void *arg);

#endif /* WHISKER_THREAD_POOL_H */

