/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_thread_pool
 * @created     : Thursday Mar 13, 2025 18:00:22 CST
 */

#include "whisker_std.h"

#include "whisker_thread_pool.h"

#include <stdio.h>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__) || defined(__MACH__) || defined(__linux__) || defined(__unix__)
#include <unistd.h>
#endif

// get the system reported core count
int whisker_tp_system_core_count() {
#if defined(_WIN32)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors / 2;
#elif defined(__APPLE__) || defined(__MACH__) || defined(__linux__) || defined(__unix__)
    return sysconf(_SC_NPROCESSORS_ONLN) / 2;
#else
    return -1; // Unsupported platform
#endif
}

// create and init a thread pool instance with the given thread count
whisker_thread_pool *whisker_tp_create()
{
	whisker_thread_pool *tp_new = whisker_mem_xcalloc_t(1, *tp_new);

	return tp_new;
}

// create and init a thread pool instance with the given thread count
whisker_thread_pool *whisker_tp_create_and_init(size_t count)
{
	whisker_thread_pool *tp_new = whisker_tp_create();
	whisker_tp_init(tp_new, count);

	return tp_new;
}

// init an instance of a thread pool
void whisker_tp_init(whisker_thread_pool *tp, size_t count)
{
	whisker_thread_pool *tp_new = whisker_mem_xcalloc_t(1, *tp_new);

	// create the work array
	whisker_arr_init_t(tp_new->work_queue, WHISKER_THREAD_POOL_WORK_QUEUE_ALLOC_COUNT);

	// set minimum to 1 thread
	if (count == 0)
	{
		count = whisker_tp_system_core_count();
	}
	tp_new->thread_count = count;

	// init pthread mutex and conds
	if (pthread_mutex_init(&tp_new->thread_mutex_worker, NULL) != 0 ||
	    pthread_cond_init(&tp_new->thread_new_work_signal, NULL) != 0 ||
	    pthread_cond_init(&tp_new->thread_working_signal, NULL) != 0)
	{
		free(tp_new);
		return;
	}

	// create thread context array
	whisker_arr_init_t(tp_new->thread_contexts, count);

	// create and detach threads
	for (int i = 0; i < count; ++i)
	{
		pthread_t thread;
		whisker_thread_pool_context context = {
			.thread_pool = tp_new,
			.thread_id = i,
		};
		tp_new->thread_contexts[i] = context;

		if (pthread_create(&thread, NULL, whisker_tp_worker_func_, &tp_new->thread_contexts[i]) != 0)
		{
			whisker_tp_free_all(tp_new);
			return;
		}
		pthread_detach(thread);
	}
}

// deallocate a thread pool instance
void whisker_tp_free_all(whisker_thread_pool *tp)
{
	if (tp == NULL)
	{
		return;
	}

	pthread_mutex_lock(&tp->thread_mutex_worker);
	tp->stop = true;
	pthread_cond_broadcast(&tp->thread_new_work_signal);
	pthread_mutex_unlock(&tp->thread_mutex_worker);

	whisker_tp_wait_work(tp);

	pthread_mutex_destroy(&tp->thread_mutex_worker);
	pthread_cond_destroy(&tp->thread_new_work_signal);
	pthread_cond_destroy(&tp->thread_working_signal);

	// free all unfinished work
	for (int i = 0; i < tp->work_queue_length; ++i)
	{
		free(tp->work_queue[i]);
	}

	free(tp->work_queue);
	free(tp->thread_contexts);
	free(tp);
}

// queue a function to run in the thread pool
void whisker_tp_queue_work(whisker_thread_pool *tp, whisker_thread_pool_func func, void *arg)
{
	if (tp == NULL || func == NULL)
	{
		return;
	}

	whisker_thread_pool_work *work = whisker_tp_create_work(func, arg);
	if (work == NULL)
	{
		return;
	}

	pthread_mutex_lock(&tp->thread_mutex_worker);

	size_t alloc_block_size = ((tp->work_queue_length + 1) * sizeof(*tp->work_queue) / WHISKER_THREAD_POOL_WORK_QUEUE_ALLOC_COUNT) + sizeof(*tp->work_queue);
	printf("block alloc size %zu current size %zu\n", alloc_block_size, tp->work_queue_size);
	if (tp->work_queue_size < alloc_block_size)
	{
		printf("realloc required\n");
		whisker_arr_ensure_alloc(tp->work_queue, alloc_block_size / sizeof(*tp->work_queue));
	}
	printf("%zu\n", tp->work_queue_length);
	tp->work_queue[tp->work_queue_length++] = work;

	pthread_cond_broadcast(&tp->thread_new_work_signal);
	pthread_mutex_unlock(&tp->thread_mutex_worker);
}

// wait for the thread pool to finish executing work
void whisker_tp_wait_work(whisker_thread_pool *tp)
{
	if (tp == NULL)
	{
		return;
	}

	pthread_mutex_lock(&tp->thread_mutex_worker);

	while (true) 
	{
		if (tp->work_queue_length > 0 || (!tp->stop && tp->thread_count_working != 0) || (tp->stop && tp->thread_count != 0))
		{
			pthread_cond_wait(&tp->thread_working_signal, &tp->thread_mutex_worker);
		}
		else
		{
			break;
		}
	}

	pthread_mutex_unlock(&tp->thread_mutex_worker);
}

// create and init a work object
whisker_thread_pool_work *whisker_tp_create_work(whisker_thread_pool_func func, void* arg)
{
	whisker_thread_pool_work *work = whisker_mem_xcalloc_t(1, *work);

	work->func = func;
	work->arg = arg;

	return work;
}

// free a work object
void whisker_tp_free_work(whisker_thread_pool_work *work)
{
	if (work == NULL)
	{
		return;
	}
	free(work);
}

// get a work item if there's any
whisker_thread_pool_work *whisker_tp_get_work(whisker_thread_pool *tp)
{
	return (tp->work_queue_length > 0) ? tp->work_queue[--tp->work_queue_length] : NULL;
}

// the function used by the thread pool workers
void *whisker_tp_worker_func_(void *arg)
{
	whisker_thread_pool_context *context = arg;
	whisker_thread_pool *tp = context->thread_pool;
	whisker_thread_pool_work *work;

	debug_printf("thread pool: thread %zu ready\n", context->thread_id);

	// infinitely loop looking for and executing work
	while (true) 
	{
		pthread_mutex_lock(&tp->thread_mutex_worker);

		// wait for the new work signal or stop signal before waking up
		while (tp->work_queue_length == 0 && !tp->stop) 
		{
			pthread_cond_wait(&tp->thread_new_work_signal, &tp->thread_mutex_worker);
		}

		// stop the loop if stop is set
		if (tp->stop && tp->work_queue_length == 0)
		{
			break;
		}

		// get a work item and unlock the mutex
		work = whisker_tp_get_work(tp);
		tp->thread_count_working++;
		pthread_mutex_unlock(&tp->thread_mutex_worker);

		// process work if it's not NULL
		if (work != NULL)
		{
			work->func(work->arg);
			whisker_tp_free_work(work);
		}

		// decrease working count
		pthread_mutex_lock(&tp->thread_mutex_worker);
		tp->thread_count_working--;

		// wait for working signal if there's no work
		if (!tp->stop && tp->thread_count_working == 0 && tp->work_queue_length == 0)
		{
			pthread_cond_signal(&tp->thread_working_signal);
		}
		pthread_mutex_unlock(&tp->thread_mutex_worker);
	}

	// remove thread from thread pool
	tp->thread_count--;
	pthread_cond_signal(&tp->thread_working_signal);
	pthread_mutex_unlock(&tp->thread_mutex_worker);

	debug_printf("thread pool: thread %zu stopping\n", context->thread_id);

	return NULL;
}
