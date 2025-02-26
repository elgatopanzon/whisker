/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_block_array
 * @created     : Monday Feb 17, 2025 14:25:30 CST
 */

#include "whisker_std.h"
#include "whisker_memory.h"
#include "whisker_array.h"

#include "whisker_block_array.h"

/**************************
*  management functions  *
**************************/

// create a whisker_block_array instance
E_WHISKER_BLOCK_ARR whisker_block_arr_create_f(size_t type_size, size_t block_size, whisker_block_array **block_arr)
{
	whisker_block_array *arr;
	E_WHISKER_MEM err = wmem_try_calloc(1, sizeof(whisker_block_array), (void**)&arr);
	if (err != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_BLOCK_ARR_MEM;
	}

	// create array of block arrays
	void** arrs;
	E_WHISKER_ARR arr_err = warr_create(void*, 0, &arrs);
	if (arr_err != E_WHISKER_ARR_OK)
	{
		free(arr);

		return E_WHISKER_BLOCK_ARR_MEM;
	}

	arr->block_size = block_size;
	arr->type_size = type_size;
	arr->blocks = arrs;

	*block_arr = arr;

	return E_WHISKER_BLOCK_ARR_OK;
}

// free a whisker_block_array instance
void whisker_block_arr_free(whisker_block_array *block_arr)
{
	whisker_block_arr_free_blocks(block_arr);

	free(block_arr);
}

void whisker_block_arr_free_blocks(whisker_block_array *block_arr)
{
	if (block_arr->blocks != NULL)
	{
		for (int i = 0; i < warr_length(block_arr->blocks); ++i)
		{
			if (block_arr->blocks[i] != NULL)
			{
				warr_free(block_arr->blocks[i]);
			}
		}

		warr_free(block_arr->blocks);
		block_arr->blocks = NULL;
	}
}

// create and allocate an array block
E_WHISKER_BLOCK_ARR whisker_block_arr_create_block(whisker_block_array *block_array, size_t block_id)
{
	// resize block pointer array if block id exceeds current length
	if (warr_length(block_array->blocks) < block_id + 1)
	{
		E_WHISKER_ARR resize_err = warr_resize(&block_array->blocks, block_id + 1);
		if (resize_err != E_WHISKER_ARR_OK)
		{
			return E_WHISKER_BLOCK_ARR_MEM;
		}
	}

	// allocate the block if it's currently NULL
	if (block_array->blocks[block_id] == NULL)
	{
		E_WHISKER_ARR arr_err = whisker_arr_create_f(block_array->type_size, block_array->block_size, &block_array->blocks[block_id]);
		if (arr_err != E_WHISKER_ARR_OK)
		{
			return E_WHISKER_BLOCK_ARR_MEM;
		}

		// increase block count
		block_array->block_count++;
	}

	return E_WHISKER_BLOCK_ARR_OK;
}


/*********************
*  array functions  *
*********************/
// get pointer to value with given index translated to block + offset
inline void* whisker_block_arr_get(whisker_block_array *block_arr, size_t index)
{
	size_t block_id = whisker_block_arr_get_block_id(block_arr->block_size, index);
	size_t block_offset = whisker_block_arr_get_block_offset(block_arr->block_size, index);

	E_WHISKER_BLOCK_ARR err = whisker_block_arr_create_block(block_arr, block_id);
	if (err != E_WHISKER_BLOCK_ARR_OK)
	{
		return NULL;
	}

	// return pointer value
	return block_arr->blocks[block_id] + (block_offset * block_arr->type_size);
}

void* whisker_block_arr_get_and_fill(whisker_block_array *block_arr, size_t index, char fill_with)
{
	size_t block_id = whisker_block_arr_get_block_id(block_arr->block_size, index);
	bool block_exists = (warr_length(block_arr->blocks) >= block_id + 1 && block_arr->blocks[block_id] != NULL);

	void* value = whisker_block_arr_get(block_arr, index);

	// if the block didn't exist before, fill it with fill_with
	if (!block_exists)
	{
		memset(block_arr->blocks[block_id], fill_with, block_arr->block_size * block_arr->type_size);
	}

	return value;
}

// set value at given index translated to block + offset
inline E_WHISKER_BLOCK_ARR whisker_block_arr_set(whisker_block_array *block_arr, size_t index, void *value)
{
	size_t block_id = whisker_block_arr_get_block_id(block_arr->block_size, index);
	size_t block_offset = whisker_block_arr_get_block_offset(block_arr->block_size, index);

	E_WHISKER_BLOCK_ARR err = whisker_block_arr_create_block(block_arr, block_id);
	if (err != E_WHISKER_BLOCK_ARR_OK)
	{
		return err;
	}

	// copy value into block index
	memcpy(block_arr->blocks[block_id] + (block_offset * block_arr->type_size), value, block_arr->type_size);

	return E_WHISKER_BLOCK_ARR_OK;
}

E_WHISKER_BLOCK_ARR whisker_block_arr_set_with(whisker_block_array *block_arr, size_t index, char set_with)
{
	size_t block_id = whisker_block_arr_get_block_id(block_arr->block_size, index);
	size_t block_offset = whisker_block_arr_get_block_offset(block_arr->block_size, index);

	E_WHISKER_BLOCK_ARR err = whisker_block_arr_create_block(block_arr, block_id);
	if (err != E_WHISKER_BLOCK_ARR_OK)
	{
		return err;
	}

	// copy value into block index
	memset(block_arr->blocks[block_id] + (block_offset * block_arr->type_size), set_with, block_arr->type_size);

	return E_WHISKER_BLOCK_ARR_OK;
}

/***********************
*  utility functions  *
***********************/
inline size_t whisker_block_arr_get_block_id(size_t block_size, size_t index) {
    return index >> __builtin_ctzll(block_size);
}

inline size_t whisker_block_arr_get_block_offset(size_t block_size, size_t index) {
    return index & (block_size - 1);
}
