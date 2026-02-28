/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_memory
 * @created     : Tuesday Feb 04, 2025 19:08:25 CST
 * @description : memory allocation wrappers with retry-on-failure and panic callbacks
 */

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "whisker_memory.h"
#include "whisker_debug.h"

// static callback functions for alloc failed warning and fatal
w_mem_alloc_warning_func alloc_warning_callback_ = NULL;
void *alloc_warning_callback_arg_ = NULL;
w_mem_alloc_panic_func alloc_panic_callback_ = NULL;
void *alloc_panic_callback_arg_ = NULL;

/******************************
*  general memory functions  *
******************************/
// malloc the requested size, 0 becomes 1, can return NULL
void *w_mem_malloc(size_t size)
{
	if (size == 0)
	{
		size = 1;
	}
	return malloc(size);
}

// calloc the requested size, 0 becomes 1, can return NULL
void *w_mem_calloc(size_t count, size_t size)
{
	if (size == 0)
	{
		size = 1;
	}
	if (count == 0)
	{
		count = 1;
	}
	return calloc(count, size);
}

// realloc with requested size, 0 becomes 1, can return NULL
void *w_mem_realloc(void* ptr, size_t size_new)
{
	if (size_new == 0)
	{
		size_new = 1;
	}
	return realloc(ptr, size_new);
}

// internal malloc wrapper
void *w_mem_xmalloc_(size_t size, size_t source_line, char *source_file, w_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg, w_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg)
{
	void *p = w_mem_malloc(size);
	if (p == NULL)
	{
		w_mem_handle_alloc_warning_(size, NULL, source_line, source_file, alloc_warning_func, alloc_warning_func_arg);
		p = w_mem_malloc(size);
	}

	if (p == NULL)
	{
		w_mem_handle_alloc_failed_(size, NULL, source_line, source_file, alloc_failed_func, alloc_failed_func_arg);

		exit(2);
	}

	return p;
}

void *w_mem_xcalloc_(size_t count, size_t size, size_t source_line, char *source_file, w_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg, w_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg)
{
	void *p = w_mem_calloc(count, size);
	if (p == NULL)
	{
		w_mem_handle_alloc_warning_(size, NULL, source_line, source_file, alloc_warning_func, alloc_warning_func_arg);
		p = w_mem_calloc(count, size);
	}

	if (p == NULL)
	{
		w_mem_handle_alloc_failed_(size, NULL, source_line, source_file, alloc_failed_func, alloc_failed_func_arg);

		exit(2);
	}

	return p;
}

void *w_mem_xrealloc_(void* ptr, size_t size_new, size_t source_line, char *source_file, w_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg, w_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg)
{
	void *p = w_mem_realloc(ptr, size_new);
	if (p == NULL)
	{
		w_mem_handle_alloc_warning_(size_new, ptr, source_line, source_file, alloc_warning_func, alloc_warning_func_arg);
		p = w_mem_realloc(ptr, size_new);
	}

	if (p == NULL)
	{
		w_mem_handle_alloc_failed_(size_new, ptr, source_line, source_file, alloc_failed_func, alloc_failed_func_arg);

		exit(2);
	}

	return p;
}


void w_mem_handle_alloc_warning_(size_t size, void *realloc_ptr, size_t source_line, char *source_file, w_mem_alloc_warning_func try_free_func, void *try_free_func_arg)
{
	fprintf(stderr, "WARNING: memory alloc (size %zu realloc %p) failed %s:%zu pthread %zu: attempting memory free callback at %p\n", size, realloc_ptr, source_file, source_line, pthread_self(), try_free_func);
	if (try_free_func != NULL) { try_free_func(try_free_func_arg); }
}

void w_mem_handle_alloc_failed_(size_t size, void *realloc_ptr, size_t source_line, char *source_file, w_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg)
{
	fprintf(stderr, "FATAL: memory alloc (size: %zu, realloc: %p) failed %s:%zu pthread %zu: running panic callback at %p\n", size, realloc_ptr, source_file, source_line, pthread_self(), alloc_failed_func);
	if (alloc_failed_func != NULL) { alloc_failed_func(alloc_failed_func_arg); }
}

// register a global callback function and argument to call when an alloc
// warning occurs and the allocation is about to be re-tried
void w_mem_register_alloc_warning_callback(w_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg)
{
	alloc_warning_callback_ = alloc_warning_func;
	alloc_warning_callback_arg_ = alloc_warning_func_arg;
}
// register a global callback function and argument to call when an alloc
// failure occurs and the program is about to abort
void w_mem_register_alloc_panic_callback(w_mem_alloc_panic_func alloc_panic_func, void *alloc_panic_func_arg)
{
	alloc_panic_callback_ = alloc_panic_func;
	alloc_panic_callback_arg_ = alloc_panic_func_arg;
}
