/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_memory
 * @created     : Tuesday Feb 04, 2025 19:08:25 CST
 */

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "whisker_memory.h"
#include "whisker_debug.h"

const char* E_WHISKER_MEM_STR[] = {
	[E_WHISKER_MEM_OK]="OK",
	[E_WHISKER_MEM_UNKNOWN]="Unknown error",
	[E_WHISKER_MEM_MALLOC_FAILED]="Malloc operation failed",
	[E_WHISKER_MEM_CALLOC_FAILED]="Calloc operation failed",
	[E_WHISKER_MEM_REALLOC_FAILED]="Realloc operation failed",
};

// static callback functions for alloc failed warning and fatal
whisker_mem_alloc_warning_func alloc_warning_callback_ = NULL;
void *alloc_warning_callback_arg_ = NULL;
whisker_mem_alloc_panic_func alloc_panic_callback_ = NULL;
void *alloc_panic_callback_arg_ = NULL;

/******************************
*  general memory functions  *
******************************/
// malloc the requested size, 0 becomes 1, can return NULL
void *whisker_mem_malloc(size_t size)
{
	if (size == 0)
	{
		size = 1;
	}
	return malloc(size);
}

// calloc the requested size, 0 becomes 1, can return NULL
void *whisker_mem_calloc(size_t count, size_t size)
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
void *whisker_mem_realloc(void* ptr, size_t size_new)
{
	if (size_new == 0)
	{
		size_new = 1;
	}
	return realloc(ptr, size_new);
}

// internal malloc wrapper
void *whisker_mem_xmalloc_(size_t size, size_t source_line, char *source_file, whisker_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg, whisker_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg)
{
	void *p = whisker_mem_malloc(size);
	if (p == NULL)
	{
		whisker_mem_handle_alloc_warning_(size, NULL, source_line, source_file, alloc_warning_func, alloc_warning_func_arg);
		p = whisker_mem_malloc(size);
	}

	if (p == NULL)
	{
		whisker_mem_handle_alloc_failed_(size, NULL, source_line, source_file, alloc_failed_func, alloc_failed_func_arg);

		exit(2);
	}

	return p;
}

void *whisker_mem_xcalloc_(size_t count, size_t size, size_t source_line, char *source_file, whisker_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg, whisker_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg)
{
	void *p = whisker_mem_calloc(count, size);
	if (p == NULL)
	{
		whisker_mem_handle_alloc_warning_(size, NULL, source_line, source_file, alloc_warning_func, alloc_warning_func_arg);
		p = whisker_mem_calloc(count, size);
	}

	if (p == NULL)
	{
		whisker_mem_handle_alloc_failed_(size, NULL, source_line, source_file, alloc_failed_func, alloc_failed_func_arg);

		exit(2);
	}

	return p;
}

void *whisker_mem_xrealloc_(void* ptr, size_t size_new, size_t source_line, char *source_file, whisker_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg, whisker_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg)
{
	void *p = whisker_mem_realloc(ptr, size_new);
	if (p == NULL)
	{
		whisker_mem_handle_alloc_warning_(size_new, ptr, source_line, source_file, alloc_warning_func, alloc_warning_func_arg);
		p = whisker_mem_realloc(ptr, size_new);
	}

	if (p == NULL)
	{
		whisker_mem_handle_alloc_failed_(size_new, ptr, source_line, source_file, alloc_failed_func, alloc_failed_func_arg);

		exit(2);
	}

	return p;
}


void whisker_mem_handle_alloc_warning_(size_t size, void *realloc_ptr, size_t source_line, char *source_file, whisker_mem_alloc_warning_func try_free_func, void *try_free_func_arg)
{
	fprintf(stderr, "WARNING: memory alloc (size %zu realloc %p) failed %s:%zu pthread %zu: attempting memory free callback at %p\n", size, realloc_ptr, source_file, source_line, pthread_self(), try_free_func);
	if (try_free_func != NULL) { try_free_func(try_free_func_arg); }
}

void whisker_mem_handle_alloc_failed_(size_t size, void *realloc_ptr, size_t source_line, char *source_file, whisker_mem_alloc_panic_func alloc_failed_func, void *alloc_failed_func_arg)
{
	fprintf(stderr, "FATAL: memory alloc (size: %zu, realloc: %p) failed %s:%zu pthread %zu: running panic callback at %p\n", size, realloc_ptr, source_file, source_line, pthread_self(), alloc_failed_func);
	if (alloc_failed_func != NULL) { alloc_failed_func(alloc_failed_func_arg); }
}

// register a global callback function and argument to call when an alloc
// warning occurs and the allocation is about to be re-tried
void whisker_mem_register_alloc_warning_callback(whisker_mem_alloc_warning_func alloc_warning_func, void *alloc_warning_func_arg)
{
	alloc_warning_callback_ = alloc_warning_func;
	alloc_warning_callback_arg_ = alloc_warning_func_arg;
}
// register a global callback function and argument to call when an alloc
// failure occurs and the program is about to abort
void whisker_mem_register_alloc_panic_callback(whisker_mem_alloc_warning_func alloc_panic_func, void *alloc_panic_func_arg)
{
	alloc_panic_callback_ = alloc_panic_func;
	alloc_panic_callback_arg_ = alloc_panic_func_arg;
}


E_WHISKER_MEM whisker_mem_try_malloc(size_t size, void** ptr)
{
    *ptr = whisker_mem_xmalloc(size);
    return (*ptr != NULL) ? E_WHISKER_MEM_OK : E_WHISKER_MEM_MALLOC_FAILED;
}

E_WHISKER_MEM whisker_mem_try_calloc(size_t count, size_t size, void** ptr)
{
    *ptr = whisker_mem_xcalloc(count, size);
    return (*ptr != NULL) ? E_WHISKER_MEM_OK : E_WHISKER_MEM_CALLOC_FAILED;
}

E_WHISKER_MEM whisker_mem_try_realloc(void* ptr, size_t size, void** ptr_new)
{
    void* ptr_reallocd = whisker_mem_xrealloc(ptr, size);
    if (ptr_reallocd == NULL)
    {
    	return E_WHISKER_MEM_REALLOC_FAILED;
    }

    *ptr_new = ptr_reallocd;

    return E_WHISKER_MEM_OK;
}

/****************************
*  memory block functions  *
****************************/
// a memory block contains a header and a data pointer with a stored size

// allocate memory block struct
whisker_memory_block *whisker_mem_block_create(size_t data_size, size_t header_size)
{
	// malloc the block
	whisker_memory_block *block = whisker_mem_xcalloc_t(1, *block);

	// assign the block values
	block->header_size = header_size;
	block->data_size = data_size;

	return block;
}

// allocate the underlying memory for the block header and data
void whisker_mem_block_init(whisker_memory_block *block)
{
	whisker_assert_ptr_ne(NULL, block);
	whisker_assert_ulong_gt(block->header_size, 0);

	// create the full block data
	void* block_data = whisker_mem_xcalloc(1, block->header_size + block->data_size);

	// assign the block values
	block->header = block_data;
	block->data = (char*)block_data + block->header_size;
}

// allocate memory block and initialise it
whisker_memory_block *whisker_mem_block_create_and_init(size_t data_size, size_t header_size)
{
	whisker_memory_block *block = whisker_mem_block_create(data_size, header_size);
	whisker_mem_block_init(block);
	return block;
}

// realloc the data pointer for a memory block
void whisker_mem_block_realloc(whisker_memory_block* block, size_t size)
{
	// validate input
	whisker_assert_ptr_ne(NULL, block);
	whisker_assert_ptr_ne(NULL, block->header);

	size_t header_size = block->header_size;
	size_t original_data_size = block->data_size;

	void* header_backup = whisker_mem_xmalloc(header_size);
	memcpy(header_backup, block->header, header_size);

	void* reallocd = whisker_mem_xrealloc(block->header, size + header_size);
	memcpy(reallocd, header_backup, header_size);
	free(header_backup);

	block->header = reallocd;
	block->data = (unsigned char*)reallocd + header_size;

	if (size > original_data_size) {
		memset(((unsigned char*)block->data) + original_data_size, 0, (size - original_data_size));
	}

	block->data_size = size;
}

// free the underlying pointer and block data for a memory block
void whisker_mem_block_free(whisker_memory_block* block)
{
	// freeing the header frees everything else
	if (block->header) free(block->header);
}

// free the underlying pointer and block data for a memory block, including the
// block itself
void whisker_mem_block_free_all(whisker_memory_block* block)
{
	whisker_mem_block_free(block);
	free(block);
}

// get the header pointer from the data pointer by shifting it by the known
// header size
// NOTE: pretty unsafe function when passed an incorrect data pointer and/or
// header size as it could effectively point anywhere
inline void* whisker_mem_block_header_from_data_pointer(char* data, size_t header_size)
{
	return data - header_size;
}


// calculate optimal header size based on a provided type size
size_t whisker_mem_block_calc_header_size(size_t header_type_size, size_t data_type_size)
{
	// no padding required
	if (header_type_size <= data_type_size)
	{
		return data_type_size;
	}
	else if (header_type_size > data_type_size)
	{
    	return (header_type_size / data_type_size) * data_type_size;
	}
    return 0;
}
