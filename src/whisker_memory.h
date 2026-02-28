/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_memory
 * @created     : Tuesday Feb 04, 2025 19:08:06 CST
 * @description : safe memory allocation and x-style allocation macros
 */

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "whisker_debug.h"

#ifndef WHISKER_MEMORY_H
#define WHISKER_MEMORY_H

// function pointers to register callbacks for failed allocations
typedef void (*w_mem_alloc_warning_func)(void *arg);
typedef void (*w_mem_alloc_panic_func)(void *arg);

extern w_mem_alloc_warning_func alloc_warning_callback_;
extern void *alloc_warning_callback_arg_;
extern w_mem_alloc_panic_func alloc_panic_callback_;
extern void *alloc_panic_callback_arg_;

// general memory functions
void *w_mem_malloc(size_t size);
void *w_mem_calloc(size_t count, size_t size);
void *w_mem_realloc(void* ptr, size_t size_new);

// internal xmalloc functions
void *w_mem_xmalloc_(size_t size, size_t source_line, char *source_file, w_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg, w_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg);
void *w_mem_xcalloc_(size_t count, size_t size, size_t source_line, char *source_file, w_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg, w_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg);
void *w_mem_xrealloc_(void* ptr, size_t size_new, size_t source_line, char *source_file, w_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg, w_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg);
void w_mem_register_alloc_warning_callback(w_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg);
void w_mem_handle_alloc_warning_(size_t size, void *realloc_ptr, size_t source_line, char *source_file, w_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg);
void w_mem_register_alloc_panic_callback(w_mem_alloc_panic_func alloc_failed_func, void *alloc_panic_func_arg);
void w_mem_handle_alloc_failed_(size_t size, void *realloc_ptr, size_t source_line, char *source_file, w_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg);


// x-style memory allocation macros to allocate or die
#define w_mem_xmalloc(size) w_mem_xmalloc_(size, __LINE__, __FILE__, alloc_warning_callback_, alloc_warning_callback_arg_, alloc_panic_callback_, alloc_panic_callback_arg_)
#define w_mem_xcalloc(count, size) w_mem_xcalloc_(count, size, __LINE__, __FILE__, alloc_warning_callback_, alloc_warning_callback_arg_, alloc_panic_callback_, alloc_panic_callback_arg_)
#define w_mem_xrealloc(ptr, size) w_mem_xrealloc_(ptr, size, __LINE__, __FILE__, alloc_warning_callback_, alloc_warning_callback_arg_, alloc_panic_callback_, alloc_panic_callback_arg_)
#define w_mem_xrecalloc(ptr, old_size, new_size) \
	w_mem_xrealloc(ptr, new_size); \
	if (new_size - old_size > 0) { memset(((unsigned char*)ptr) + old_size, 0, new_size - old_size); } \

#define w_mem_xmalloc_t(t) w_mem_xmalloc(sizeof(t))
#define w_mem_xcalloc_t(count, t) w_mem_xcalloc(count, sizeof(t))
#define w_mem_xrealloc_t(ptr, t) w_mem_xrealloc(ptr, sizeof(t))

#define free_null(p) if (p) { free(p); p = NULL; }

#endif // end of include guard WHISKER_MEMORY_H
