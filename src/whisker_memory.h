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

// errors
typedef enum E_WHISKER_MEM  
{
	E_WHISKER_MEM_OK = 0,
	E_WHISKER_MEM_UNKNOWN = 1,
	E_WHISKER_MEM_MALLOC_FAILED = 2,
	E_WHISKER_MEM_CALLOC_FAILED = 3,
	E_WHISKER_MEM_REALLOC_FAILED = 4,
} E_WHISKER_MEM;
extern const char* E_WHISKER_MEM_STR[];

// short macros
#define wmem_try_malloc whisker_mem_try_malloc
#define wmem_try_calloc whisker_mem_try_calloc
#define wmem_try_realloc whisker_mem_try_realloc
#define wmem_try_malloc_t(t, p) whisker_mem_try_malloc(sizeof(t), (void**) p)
#define wmem_try_calloc_t(c, t, p) whisker_mem_try_calloc(c, sizeof(t), (void**) p)
#define wmem_try_realloc_t(p, t, pn) whisker_mem_try_realloc(p, sizeof(t), (void**) pn);
#define wmemb_try_malloc whisker_mem_block_try_malloc
#define wmemb_try_realloc_data whisker_mem_block_try_realloc_data
#define wmemb_free whisker_mem_block_free
#define wmemb_header whisker_mem_block_header_from_data_pointer
#define wmemb_header_size whisker_mem_block_calc_header_size

// function pointers to register callbacks for failed allocations
typedef void (*whisker_mem_alloc_warning_func)(void *arg);
typedef void (*whisker_mem_alloc_panic_func)(void *arg);

static whisker_mem_alloc_warning_func alloc_warning_callback_;
static void *alloc_warning_callback_arg_;
static whisker_mem_alloc_panic_func alloc_panic_callback_;
static void *alloc_panic_callback_arg_;

// general memory functions
void *whisker_mem_malloc(size_t size);
void *whisker_mem_calloc(size_t count, size_t size);
void *whisker_mem_realloc(void* ptr, size_t size_new);
E_WHISKER_MEM whisker_mem_try_malloc(size_t size, void** ptr);
E_WHISKER_MEM whisker_mem_try_calloc(size_t count, size_t size, void** ptr);
E_WHISKER_MEM whisker_mem_try_realloc(void* ptr, size_t size, void** ptr_new);

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

// memory blocks
// a block is a managed header and data pointer
typedef struct whisker_memory_block
{
	void* header;
	size_t header_size;
	void* data;
	size_t data_size;
} whisker_memory_block;

E_WHISKER_MEM whisker_mem_block_try_malloc(size_t data_size, size_t header_size, whisker_memory_block** block);
E_WHISKER_MEM whisker_mem_block_try_realloc_data(whisker_memory_block* block, size_t size);
void whisker_mem_block_free(whisker_memory_block* block);
void* whisker_mem_block_header_from_data_pointer(char* data, size_t header_size);
size_t whisker_mem_block_calc_header_size(size_t header_type_size, size_t data_type_size);

#endif /* end of include guard WHISKER_MEMORY_H */
