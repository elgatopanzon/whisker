/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_memory
 * @created     : Tuesday Feb 04, 2025 19:08:06 CST
 */

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "whisker_debug.h"

#ifndef WHISKER_MEMORY_H
#define WHISKER_MEMORY_H

// short macros
#define wmemb_free whisker_mem_block_free
#define wmemb_header whisker_mem_block_header_from_data_pointer
#define wmemb_header_size whisker_mem_block_calc_header_size

// function pointers to register callbacks for failed allocations
typedef void (*whisker_mem_alloc_warning_func)(void *arg);
typedef void (*whisker_mem_alloc_panic_func)(void *arg);

extern whisker_mem_alloc_warning_func alloc_warning_callback_;
extern void *alloc_warning_callback_arg_;
extern whisker_mem_alloc_panic_func alloc_panic_callback_;
extern void *alloc_panic_callback_arg_;

// general memory functions
void *whisker_mem_malloc(size_t size);
void *whisker_mem_calloc(size_t count, size_t size);
void *whisker_mem_realloc(void* ptr, size_t size_new);

// internal xmalloc functions
void *whisker_mem_xmalloc_(size_t size, size_t source_line, char *source_file, whisker_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg, whisker_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg);
void *whisker_mem_xcalloc_(size_t count, size_t size, size_t source_line, char *source_file, whisker_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg, whisker_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg);
void *whisker_mem_xrealloc_(void* ptr, size_t size_new, size_t source_line, char *source_file, whisker_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg, whisker_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg);
void whisker_mem_register_alloc_warning_callback(whisker_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg);
void whisker_mem_handle_alloc_warning_(size_t size, void *realloc_ptr, size_t source_line, char *source_file, whisker_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg);
void whisker_mem_register_alloc_failed_callback(whisker_mem_alloc_panic_func alloc_failed_func, void *alloc_panic_func_arg);
void whisker_mem_handle_alloc_failed_(size_t size, void *realloc_ptr, size_t source_line, char *source_file, whisker_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg);


// x-style memory allocation macros to allocate or die
#define whisker_mem_xmalloc(size) whisker_mem_xmalloc_(size, __LINE__, __FILE__, alloc_warning_callback_, alloc_warning_callback_arg_, alloc_panic_callback_, alloc_panic_callback_arg_)
#define whisker_mem_xcalloc(count, size) whisker_mem_xcalloc_(count, size, __LINE__, __FILE__, alloc_warning_callback_arg_, alloc_warning_callback_, alloc_panic_callback_, alloc_panic_callback_arg_)
#define whisker_mem_xrealloc(ptr, size) whisker_mem_xrealloc_(ptr, size, __LINE__, __FILE__, alloc_warning_callback_arg_, alloc_warning_callback_, alloc_panic_callback_, alloc_panic_callback_arg_)
#define whisker_mem_xmalloc_t(t) whisker_mem_xmalloc(sizeof(t))
#define whisker_mem_xcalloc_t(count, t) whisker_mem_xcalloc(count, sizeof(t))
#define whisker_mem_xrealloc_t(ptr, t) whisker_mem_xrealloc(ptr, sizeof(t))

// memory blocks
// a block is a managed header and data pointer
typedef struct whisker_memory_block
{
	void* header;
	size_t header_size;
	void* data;
	size_t data_size;
} whisker_memory_block;

whisker_memory_block *whisker_mem_block_create(size_t data_size, size_t header_size);
void whisker_mem_block_init(whisker_memory_block *block);
whisker_memory_block *whisker_mem_block_create_and_init(size_t data_size, size_t header_size);
void whisker_mem_block_realloc(whisker_memory_block* block, size_t size);
void whisker_mem_block_free(whisker_memory_block* block);
void whisker_mem_block_free_all(whisker_memory_block* block);
void* whisker_mem_block_header_from_data_pointer(char* data, size_t header_size);
size_t whisker_mem_block_calc_header_size(size_t header_type_size, size_t data_type_size);

#endif /* end of include guard WHISKER_MEMORY_H */
