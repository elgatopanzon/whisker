/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_array
 * @created     : Wednesday Feb 05, 2025 12:39:07 CST
 */

#include <string.h>

#include "whisker_debug.h"
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
	whisker_memory_block* block;
	E_WHISKER_MEM err = whisker_mem_block_try_malloc(type_size * length, sizeof(whisker_array_header), &block);
	if (err != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_ARR_MEM;
	}

	// set header values
	whisker_array_header* header = block->header;
	header->element_size = type_size;
	header->length = length;
	header->size = type_size * length;
	header->swap_buffer = whisker_mem_xcalloc(1, type_size);

	// set array pointer to data pointer
	*arr = block->data;
	free(block);

	return E_WHISKER_ARR_OK;
}

// resize an array with the given pointer, putting the new pointer
E_WHISKER_ARR whisker_arr_resize_f(void** arr, size_t elements, bool allow_shrink)
{
	// build block from array pointer
	whisker_array_header* header = whisker_arr_header(*arr);

	// if the new size is the same, nothing needs to be done
	if (header->length == elements)
	{
		return E_WHISKER_ARR_OK;
	}
	if (header->length > elements && !allow_shrink)
	{
		header->length = elements;
		return E_WHISKER_ARR_OK;
	}

	whisker_memory_block block = {
		.header = header,
		.header_size = sizeof(whisker_array_header),
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
	whisker_array_header* header = whisker_arr_header(*arr);
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

	whisker_array_header* header = whisker_arr_header(*arr);

	// copy value into end
	memcpy(((char*)*arr) + ((header->length - 1) * header->element_size), value, header->element_size);

	return E_WHISKER_ARR_OK;
}

// resize array to it's actual length
E_WHISKER_ARR whisker_arr_compact_f(void** arr)
{
	whisker_array_header* header = whisker_arr_header(*arr);
	E_WHISKER_ARR err = whisker_arr_resize_f(arr, header->length, true);
	if (err != E_WHISKER_ARR_OK)
	{
		return err;
	}

	return E_WHISKER_ARR_OK;
}

// retreive the last element, then shrink the array length keeping same capacity
E_WHISKER_ARR whisker_arr_pop_f(void** arr, void* value)
{
	whisker_array_header* header = whisker_arr_header(*arr);

	if (header->length > 0)
	{
		memcpy(value, ((char*)*arr) + (header->length - 1) * header->element_size, header->element_size);

		header->length--;

		return E_WHISKER_ARR_OK;
	}

	return E_WHISKER_ARR_OUT_OF_BOUNDS;
}

// retreive the first element, then shrink the array length keeping same capacity
E_WHISKER_ARR whisker_arr_pop_front_f(void** arr, void* value)
{
	whisker_array_header* header = whisker_arr_header(*arr);

	// copy first value
	memcpy(value, ((char*)*arr), header->element_size);

	// swap first and last index values
	whisker_arr_swap(arr, 0, header->length - 1);

	// resize array by -1
	header->length--;

	return E_WHISKER_ARR_OK;
}

// swap elements in 2 index positions
E_WHISKER_ARR whisker_arr_swap(void** arr, size_t index_a, size_t index_b)
{
	whisker_array_header* header = whisker_arr_header(*arr);
	if (index_a + 1 > header->length || index_b + 1 > header->length)
	{
		return E_WHISKER_ARR_OUT_OF_BOUNDS;
	}

	// copy a into temp
	void* temp = header->swap_buffer;
	memcpy(temp, ((char*)*arr) + (index_a * header->element_size), header->element_size);

	// copy over a
	memcpy(((char*)*arr) + (index_a * header->element_size), ((char*)*arr) + (index_b * header->element_size), header->element_size);

	// copy temp into b
	memcpy(((char*)*arr) + (index_b * header->element_size), temp, header->element_size);

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

	whisker_array_header* header = whisker_arr_header(*arr);

	// move all elements from index forward by 1 element
	// note: this will overwrite the end value
	memmove((char*)*arr + (index + 1) * header->element_size, (char*)*arr + index * header->element_size, ((header->length - 1) - index) * header->element_size);

	// copy value again into the index slot
	memcpy((char*)*arr + index * header->element_size, value, header->element_size);

	return E_WHISKER_ARR_OK;
}

// reset length to 0 keeping allocated size and values
E_WHISKER_ARR whisker_arr_reset_f(char* arr)
{
	whisker_arr_header(arr)->length = 0;

	return E_WHISKER_ARR_OK;
}


// free the array by obtaining the header
void whisker_arr_free(void* arr)
{
	whisker_array_header *header = whisker_arr_header(arr);
	free(header->swap_buffer);
	free(header);
}

// obtain the header from the array pointer
inline whisker_array_header* whisker_arr_header_f(char* arr)
{
	return (whisker_array_header*)((char*)arr - sizeof(whisker_array_header));
}

// get array length from underlying header
// shortcut to use in loops
inline size_t whisker_arr_length_f(char* arr)
{
	return whisker_arr_header(arr)->length;
}

// get value at the given index, growing and returning pointer
void* whisker_arr_grow_get_f(void** arr, size_t index)
{
	whisker_array_header* header = whisker_arr_header(*arr);

	if (index + 1 > header->length)
	{
		E_WHISKER_ARR err = whisker_arr_resize(arr, index + 1);
		if (err != E_WHISKER_ARR_OK)
		{
			return NULL;
		}
	}

	return &(*arr)[index];
}
