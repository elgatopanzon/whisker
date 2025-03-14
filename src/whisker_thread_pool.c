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
E_WHISKER_TP whisker_tp_create_f(whisker_thread_pool **tp, size_t count)
{
	whisker_thread_pool *tp_new;
	E_WHISKER_MEM mem_err = whisker_mem_try_calloc(1, sizeof(*tp_new), (void**)&tp_new);
	if (mem_err != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_TP_MEM;
	}

	// create the work array
	E_WHISKER_ARR arr_err = whisker_arr_create_void_(&tp_new->work_queue, 0);
	if (arr_err != E_WHISKER_ARR_OK)
	{
		free(tp_new);
		return E_WHISKER_TP_ARR;
	}

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
		return E_WHISKER_TP_MUTEX;
	}

	// create thread context array
	arr_err = whisker_arr_create_void_(&tp_new->thread_contexts, count);
	if (arr_err != E_WHISKER_ARR_OK)
	{
		pthread_mutex_destroy(&tp_new->thread_mutex_worker);
		pthread_cond_destroy(&tp_new->thread_new_work_signal);
		pthread_cond_destroy(&tp_new->thread_working_signal);
		free(tp_new);
		return E_WHISKER_TP_ARR;
	}

	// create and detach threads
	for (int i = 0; i < count; ++i)
	{
		pthread_t thread;
		whisker_thread_pool_context *context = calloc(1, sizeof(*context));
		if (context == NULL)
		{
			whisker_tp_free(tp_new);
			return E_WHISKER_TP_MEM;
		}
		context->thread_pool = tp_new;
		context->thread_id = i;

		if (pthread_create(&thread, NULL, whisker_tp_worker_func_, context) != 0)
		{
			free(context);
			whisker_tp_free(tp_new);
			return E_WHISKER_TP_THREAD;
		}
		pthread_detach(thread);

		if (whisker_arr_push_void_(tp_new->thread_contexts, context) != E_WHISKER_ARR_OK)
		{
			free(context);
			whisker_tp_free(tp_new);
			return E_WHISKER_TP_ARR;
		}
	}

	*tp = tp_new;
	return E_WHISKER_TP_OK;
}

// deallocate a thread pool instance
void whisker_tp_free(whisker_thread_pool *tp)
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
	for (int i = 0; i < tp->work_queue->length; ++i)
	{
		free(tp->work_queue->arr[i]);
	}

	// free thread contexts
	for (int i = 0; i < tp->thread_contexts->length; ++i)
	{
		free(tp->thread_contexts->arr[i]);
	}

	whisker_arr_free_void_(tp->work_queue);
	whisker_arr_free_void_(tp->thread_contexts);
	free(tp);
}

// queue a function to run in the thread pool
E_WHISKER_TP whisker_tp_queue_work(whisker_thread_pool *tp, whisker_thread_pool_func func, void *arg)
{
	if (tp == NULL || func == NULL)
	{
		return E_WHISKER_TP_WORK;
	}

	whisker_thread_pool_work *work = whisker_tp_create_work(func, arg);
	if (work == NULL)
	{
		return E_WHISKER_TP_WORK;
	}

	pthread_mutex_lock(&tp->thread_mutex_worker);
	if (whisker_arr_push_void_(tp->work_queue, work) != E_WHISKER_ARR_OK)
	{
		pthread_mutex_unlock(&tp->thread_mutex_worker);
		whisker_tp_free_work(work);
		return E_WHISKER_TP_ARR;
	}

	pthread_cond_broadcast(&tp->thread_new_work_signal);
	pthread_mutex_unlock(&tp->thread_mutex_worker);

	return E_WHISKER_TP_OK;
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
		if (tp->work_queue->length > 0 || (!tp->stop && tp->thread_count_working != 0) || (tp->stop && tp->thread_count != 0))
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
	whisker_thread_pool_work *work;
	E_WHISKER_MEM mem_err = whisker_mem_try_calloc(1, sizeof(*work), (void**)&work);
	if (mem_err != E_WHISKER_MEM_OK)
	{
		return NULL;
	}

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
	whisker_thread_pool_work *work;
	if (whisker_arr_pop_void_(tp->work_queue, (void**)&work) == E_WHISKER_ARR_OK)
	{
		return work;
	}
	return NULL;
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
		while (tp->work_queue->length == 0 && !tp->stop) 
		{
			pthread_cond_wait(&tp->thread_new_work_signal, &tp->thread_mutex_worker);
		}

		// stop the loop if stop is set
		if (tp->stop && tp->work_queue->length == 0)
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
		if (!tp->stop && tp->thread_count_working == 0 && tp->work_queue->length == 0)
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
