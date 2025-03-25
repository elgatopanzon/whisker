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
whisker_thread_pool *whisker_tp_create_and_init(size_t count, char *name)
{
	whisker_thread_pool *tp_new = whisker_tp_create();
	whisker_tp_init(tp_new, count, name);

	return tp_new;
}

// init an instance of a thread pool
void whisker_tp_init(whisker_thread_pool *tp, size_t count, char *name)
{
	// create the work array
	whisker_arr_init_t(tp->work_queue, WHISKER_THREAD_POOL_WORK_QUEUE_BLOCK_SIZE);
	whisker_arr_init_t(tp->work_pool, WHISKER_THREAD_POOL_WORK_QUEUE_BLOCK_SIZE);

	// set default thread pool name if NULL
	if (!name)
	{
		name = "default_thread_pool";
	}
	tp->name = whisker_mem_xcalloc(1, strlen(name) + 1);
	strncpy(tp->name, name, strlen(name));

	// set minimum to 1 thread
	if (count == 0)
	{
		count = whisker_tp_system_core_count();
	}
	tp->thread_count = count;

	// init pthread mutex and conds
	if (pthread_mutex_init(&tp->thread_mutex_worker, NULL) != 0 ||
	    pthread_cond_init(&tp->thread_new_work_signal, NULL) != 0 ||
	    pthread_cond_init(&tp->thread_working_signal, NULL) != 0)
	{
		free(tp);
		return;
	}

	// create thread context array
	whisker_arr_init_t(tp->thread_contexts, count);

	// create and detach threads
	for (int i = 0; i < count; ++i)
	{
		pthread_t thread;
		whisker_thread_pool_context context = {
			.thread_pool = tp,
			.thread_id = i,
		};
		tp->thread_contexts[i] = context;

		if (pthread_create(&thread, NULL, whisker_tp_worker_func_, &tp->thread_contexts[i]) != 0)
		{
			whisker_tp_free_all(tp);
			return;
		}
		pthread_detach(thread);
	}
}

// stop a thread pool instance and deallocate
void whisker_tp_free(whisker_thread_pool *tp)
{
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
	// free all unused work
	for (int i = 0; i < tp->work_pool_length; ++i)
	{
		free(tp->work_pool[i]);
	}

	free(tp->name);
	free(tp->work_queue);
	free(tp->work_pool);
	free(tp->thread_contexts);
}

// deallocate a thread pool instance and stop all threads
void whisker_tp_free_all(whisker_thread_pool *tp)
{
	whisker_tp_free(tp);
	free(tp);
}

// queue a function to run in the thread pool
void whisker_tp_queue_work(whisker_thread_pool *tp, whisker_thread_pool_func func, void *arg)
{
	if (tp == NULL || func == NULL)
	{
		return;
	}

	whisker_thread_pool_work *work;

	pthread_mutex_lock(&tp->thread_mutex_worker);

	// pull off a work item from the pool
	if (tp->work_pool_length == 0)
	{
		work = whisker_tp_create_work(func, arg);
	}
	else 
	{
		work = tp->work_pool[--tp->work_pool_length];
		work->func = func;
		work->arg = arg;
	}

	whisker_arr_ensure_alloc_block_size(
		tp->work_queue, 
		tp->work_queue_length + 1, 
		WHISKER_THREAD_POOL_WORK_QUEUE_BLOCK_SIZE
	);
	tp->work_queue[tp->work_queue_length++] = work;

	pthread_cond_broadcast(&tp->thread_new_work_signal);
	pthread_mutex_unlock(&tp->thread_mutex_worker);
}

// return a used work item to the work pool
void whisker_tp_return_work(whisker_thread_pool *tp, whisker_thread_pool_work *work)
{
	pthread_mutex_lock(&tp->thread_mutex_worker);

	whisker_arr_ensure_alloc_block_size(
		tp->work_pool, 
		tp->work_pool_length + 1, 
		WHISKER_THREAD_POOL_WORK_QUEUE_BLOCK_SIZE
	);
	tp->work_pool[tp->work_pool_length++] = work;

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

	debug_log(DEBUG, thread_pool, "%s: thread %zu ready", tp->name, context->thread_id);

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
			whisker_tp_return_work(tp, work);
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
	debug_log(DEBUG, thread_pool, "%s: thread %zu stopping", tp->name, context->thread_id);

	tp->thread_count--;
	pthread_cond_signal(&tp->thread_working_signal);
	pthread_mutex_unlock(&tp->thread_mutex_worker);

	return NULL;
}
