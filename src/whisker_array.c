/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_array
 * @created     : Wednesday Feb 05, 2025 12:39:07 CST
 */

#include <string.h>

#include "whisker_array.h"
#include "whisker_array_internal.h"
#include "whisker_memory.h"

const char* E_WHISKER_ARR_STR[] = {
	[E_WHISKER_ARR_OK]="OK",
	[E_WHISKER_ARR_UNKNOWN]="Unknown error",
	[E_WHISKER_ARR_MEM]="Memory error during operation",
};

// create a block and use it to create the array
E_WHISKER_ARR whisker_arr_create_f(size_t type_size, size_t length, void** arr)
{
	// alloc whisker_mem block for the array + header struct
	whisker_memory_block_t* block;
	E_WHISKER_MEM err = whisker_mem_block_try_malloc(type_size * length, sizeof(whisker_array_header_t), &block);
	if (err != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_ARR_MEM;
	}

	// set header values
	whisker_array_header_t* header = block->header;
	header->element_size = type_size;
	header->length = length;
	header->size = type_size * length;

	// set array pointer to data pointer
	*arr = block->data;
	free(block);

	return E_WHISKER_ARR_OK;
}

// resize an array with the given pointer, putting the new pointer
E_WHISKER_ARR whisker_arr_resize_f(void** arr, size_t elements)
{
	// build block from array pointer
	whisker_array_header_t* header = whisker_arr_header(*arr);
	whisker_memory_block_t block = {
		.header = header,
		.header_size = sizeof(whisker_array_header_t),
		.data_size = header->size,
	};

	// try to realloc the underlying block to the new size
	E_WHISKER_MEM err = whisker_mem_block_try_realloc_data(&block, elements * header->element_size);
	if (err != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_ARR_MEM;
	}

	// set new pointer and update length
	*arr = block.data;
	header = whisker_arr_header(*arr);
	header->length = elements;
	header->size = elements * header->element_size;

	return E_WHISKER_ARR_OK;
}

// increment array size by +1
// shortcut used by push/insert functions
E_WHISKER_ARR whisker_arr_increment_size_f(void** arr)
{
	whisker_array_header_t* header = whisker_arr_header(*arr);
	E_WHISKER_ARR err = whisker_arr_try_resize_if_required_(arr, header->length + 1);
	if (err != E_WHISKER_ARR_OK)
	{
		return err;
	}

	return E_WHISKER_ARR_OK;
}

// extend the array by +1 and insert value
E_WHISKER_ARR whisker_arr_push_f(void** arr, void* value)
{
	E_WHISKER_ARR err = whisker_arr_increment_size_f(arr);
	if (err != E_WHISKER_ARR_OK)
	{
		return err;
	}

	whisker_array_header_t* header = whisker_arr_header(*arr);

	// copy value into end
	memcpy(((char*)*arr) + ((header->length - 1) * header->element_size), value, header->element_size);

	return E_WHISKER_ARR_OK;
}

// resize array to it's actual length
E_WHISKER_ARR whisker_arr_compact_f(void** arr)
{
	whisker_array_header_t* header = whisker_arr_header(*arr);
	E_WHISKER_ARR err = whisker_arr_resize_f(arr, header->length);
	if (err != E_WHISKER_ARR_OK)
	{
		return err;
	}

	return E_WHISKER_ARR_OK;
}

// retreive the last element, then shrink the array length keeping same capacity
E_WHISKER_ARR whisker_arr_pop_f(void** arr, void* value)
{
	whisker_array_header_t* header = whisker_arr_header(*arr);

	memcpy(value, ((char*)*arr) + (header->length - 1) * header->element_size, header->element_size);

	header->length--;

	return E_WHISKER_ARR_OK;
}

// insert value at index and shift values forward
E_WHISKER_ARR whisker_arr_insert_f(void** arr, size_t index, void* value)
{
	// resize array +1 to fit the new element
	E_WHISKER_ARR err = whisker_arr_increment_size_f(arr);
	if (err != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_ARR_MEM;
	}

	whisker_array_header_t* header = whisker_arr_header(*arr);

	// move all elements from index forward by 1 element
	// note: this will overwrite the end value
	memmove((char*)*arr + (index + 1) * header->element_size, (char*)*arr + index * header->element_size, ((header->length - 1) - index) * header->element_size);

	// copy value again into the index slot
	memcpy((char*)*arr + index * header->element_size, value, header->element_size);

	return E_WHISKER_ARR_OK;
}

// free the array by obtaining the header
void whisker_arr_free(void* arr)
{
	free(whisker_arr_header(arr));
}

// obtain the header from the array pointer
whisker_array_header_t* whisker_arr_header(void* arr)
{
	return whisker_mem_block_header_from_data_pointer(arr, sizeof(whisker_array_header_t));
}

// get array length from underlying header
// shortcut to use in loops
size_t whisker_arr_length(void* arr)
{
	return whisker_arr_header(arr)->length;
}
