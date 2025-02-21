/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_memory
 * @created     : Tuesday Feb 04, 2025 19:08:25 CST
 */

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "whisker_memory.h"

const char* E_WHISKER_MEM_STR[] = {
	[E_WHISKER_MEM_OK]="OK",
	[E_WHISKER_MEM_UNKNOWN]="Unknown error",
	[E_WHISKER_MEM_MALLOC_FAILED]="Malloc operation failed",
	[E_WHISKER_MEM_CALLOC_FAILED]="Calloc operation failed",
	[E_WHISKER_MEM_REALLOC_FAILED]="Realloc operation failed",
};

/******************************
*  general memory functions  *
******************************/
E_WHISKER_MEM whisker_mem_try_malloc(size_t size, void** ptr)
{
    *ptr = malloc(size);
    return (*ptr != NULL) ? E_WHISKER_MEM_OK : E_WHISKER_MEM_MALLOC_FAILED;
}

E_WHISKER_MEM whisker_mem_try_calloc(size_t count, size_t size, void** ptr)
{
    *ptr = calloc(count, size);
    return (*ptr != NULL) ? E_WHISKER_MEM_OK : E_WHISKER_MEM_CALLOC_FAILED;
}

E_WHISKER_MEM whisker_mem_try_realloc(void* ptr, size_t size, void** ptr_new)
{
    void* ptr_reallocd = realloc(ptr, size);
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

// allocate the underlying data for a memory block
E_WHISKER_MEM whisker_mem_block_try_malloc(size_t data_size, size_t header_size, whisker_memory_block** block)
{
	// create the full block data
	void* block_data;
	E_WHISKER_MEM err = whisker_mem_try_calloc(1, header_size + data_size, &block_data);
	if (err != E_WHISKER_MEM_OK)
	{
		return err;
	}

	// malloc the block
	E_WHISKER_MEM errBlock = whisker_mem_try_malloc(sizeof(whisker_memory_block), (void**)block);
	if (errBlock != E_WHISKER_MEM_OK)
	{
		free(block_data);
		return errBlock;
	}

	// assign the block values
	(*block)->header = block_data;
	(*block)->header_size = header_size;
	(*block)->data = (char*)block_data + header_size;
	(*block)->data_size = data_size;

	return E_WHISKER_MEM_OK;
}

// realloc the data pointer for a memory block
E_WHISKER_MEM whisker_mem_block_try_realloc_data(whisker_memory_block* block, size_t size)
{
	size_t header_size = block->header_size;
	size_t original_data_size = block->data_size;

	// backup header
	void* header_backup;
	E_WHISKER_MEM err = whisker_mem_try_malloc(header_size, &header_backup);
	if (err != E_WHISKER_MEM_OK)
	{
		return err;
	}

	memcpy(header_backup, block->header, header_size);

	void* reallocd;
	E_WHISKER_MEM errRealloc = whisker_mem_try_realloc(block->header, size + header_size, &reallocd);
	if (errRealloc != E_WHISKER_MEM_OK)
	{
		free(header_backup);
		return errRealloc;
	}

	// restore header and free
	memcpy(reallocd, header_backup, header_size);
	free(header_backup);

	// update block
	block->header = reallocd;
	block->data = (unsigned char*)reallocd + header_size;

	// 0 out the new memory
	if (size > original_data_size)
	{
		memset(((unsigned char*)block->data) + original_data_size, 0, (size - original_data_size));
	}

	block->data_size = size;

	return E_WHISKER_MEM_OK;
}

// free the underlying pointer and block data for a memory block
void whisker_mem_block_free(whisker_memory_block* block)
{
	free(block->header);
	free(block);
}

// get the header pointer from the data pointer by shifting it by the known
// header size
// NOTE: pretty unsafe function when passed an incorrect data pointer and/or
// header size as it could effectively point anywhere
inline void* whisker_mem_block_header_from_data_pointer(void* data, size_t header_size)
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
