/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_memory
 * @created     : Tuesday Feb 04, 2025 19:08:06 CST
 */

#include <stdlib.h>
#include <stdbool.h>

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

// general memory functions
E_WHISKER_MEM whisker_mem_try_malloc(size_t size, void** ptr);
E_WHISKER_MEM whisker_mem_try_calloc(size_t count, size_t size, void** ptr);
E_WHISKER_MEM whisker_mem_try_realloc(void* ptr, size_t size, void** ptr_new);

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
