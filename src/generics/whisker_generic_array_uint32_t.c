/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:38:21 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"

#include "whisker_generic_array.h"
#include "whisker_generic_array_uint32_t.h"

// create an instance of a managed array with type uint32_t
E_WHISKER_ARR whisker_arr_create_uint32_t(whisker_arr_uint32_t **arr, size_t length)
{
	whisker_arr_uint32_t *a;
	E_WHISKER_MEM err = whisker_mem_try_calloc(1, sizeof(whisker_arr_uint32_t), (void**)&a);
	if (err != E_WHISKER_MEM_OK) { return E_WHISKER_ARR_MEM; }
	a->length = length;
	a->alloc_size = sizeof(uint32_t) * a->length;
	if (length > 0) { whisker_arr_init_uint32_t(a); }
	*arr = a;
	return E_WHISKER_ARR_OK;
}

// init an instance of a managed array with type uint32_t
E_WHISKER_ARR whisker_arr_init_uint32_t(whisker_arr_uint32_t *arr)
{
	if (arr->length > 0 && arr->arr == NULL) {  \
		E_WHISKER_MEM err = whisker_mem_try_calloc(1, sizeof(uint32_t) * arr->length, (void**)&arr->arr);
		if (err != E_WHISKER_MEM_OK) { return E_WHISKER_ARR_MEM; }
	}
	return E_WHISKER_ARR_OK;
}

// deallocate an instance of a managed array with type uint32_t
void whisker_arr_free_uint32_t(whisker_arr_uint32_t *arr)
{
	if (arr->arr != NULL) { free(arr->arr); }
	free(arr);
}

// resize a managed array of type uint32_t
E_WHISKER_ARR whisker_arr_resize_uint32_t(whisker_arr_uint32_t *arr, size_t length, bool soft_resize)
{
	if (length == 0 && arr->alloc_size > 0 && !soft_resize) {
		free(arr->arr);
		arr->arr = NULL;
		arr->alloc_size = 0;
	} else if (arr->length == length) {
		return E_WHISKER_ARR_OK;
	} else if (length < 0 && arr->length == 0) {
		return E_WHISKER_ARR_OUT_OF_BOUNDS;
	} else if ((arr->length > length && soft_resize) || arr->alloc_size >= (length * sizeof(uint32_t))) {
		arr->length = length;
		return E_WHISKER_ARR_OK;
	} else {
		E_WHISKER_MEM err = whisker_mem_try_realloc(arr->arr, length * sizeof(uint32_t), (void**)&arr->arr);
		if (err != E_WHISKER_MEM_OK) {
			return E_WHISKER_ARR_MEM;
		}
		memset(((unsigned char*)arr->arr) + arr->alloc_size, 0, (length * sizeof(uint32_t)) - arr->alloc_size);
		arr->alloc_size = length * sizeof(uint32_t);
		arr->length = length;
	}
	return E_WHISKER_ARR_OK;
}

// increase the size by +1
E_WHISKER_ARR whisker_arr_increment_size_uint32_t(whisker_arr_uint32_t *arr)
{
	return whisker_arr_resize_uint32_t(arr, arr->length + 1, true);
}

// decrease the size by +1
E_WHISKER_ARR whisker_arr_decrement_size_uint32_t(whisker_arr_uint32_t *arr)
{
	if (arr->length == 0) { return E_WHISKER_ARR_OUT_OF_BOUNDS; }
	return whisker_arr_resize_uint32_t(arr, arr->length - 1, true);
}

// push a uint32_t value into the array, resizing and re-allocating if required
E_WHISKER_ARR whisker_arr_push_uint32_t(whisker_arr_uint32_t *arr, uint32_t value)
{
	E_WHISKER_ARR err = whisker_arr_increment_size_uint32_t(arr);
	if (err != E_WHISKER_ARR_OK) { return err; }
	arr->arr[arr->length - 1] = value;
	return E_WHISKER_ARR_OK;
}

// pop the last uint32_t value from the array, decreasing the managed length
// note: does not decrease the allocation size
E_WHISKER_ARR whisker_arr_pop_uint32_t(whisker_arr_uint32_t *arr, uint32_t *out)
{
	E_WHISKER_ARR err = whisker_arr_decrement_size_uint32_t(arr);
	if (err != E_WHISKER_ARR_OK) { return err; }
	uint32_t popped = arr->arr[arr->length];
	*out = popped;
	return E_WHISKER_ARR_OK;
}

// swap 2 uint32_t values with the given indexes using the swap buffer
E_WHISKER_ARR whisker_arr_swap_uint32_t(whisker_arr_uint32_t *arr, size_t index_a, size_t index_b)
{
	if (index_a < 0 || index_a > arr->length - 1 || index_b < 0 || index_b > arr->length - 1) { return E_WHISKER_ARR_OUT_OF_BOUNDS; }
	arr->swap_buffer = arr->arr[index_a];
	arr->arr[index_a] = arr->arr[index_b];
	arr->arr[index_b] = arr->swap_buffer;
	memset(&arr->swap_buffer, 0x00, sizeof(uint32_t));
	return E_WHISKER_ARR_OK;
}

// reset the managed length to 0
// note: optionally compact down to 0 (free the allocation)
void whisker_arr_reset_uint32_t(whisker_arr_uint32_t *arr, bool compact)
{
	arr->length = 0;
	if (compact) { whisker_arr_resize_uint32_t(arr, 0, false); }
}

// compact and resize the allocation to fit the managed size
E_WHISKER_ARR whisker_arr_compact_uint32_t(whisker_arr_uint32_t *arr)
{
	return whisker_arr_resize_uint32_t(arr, arr->length, false);
}

/***********************
*  utility functions  *
***********************/

// get the index of the uint32_t value, -1 if it doesn't exist
// note: does a memcmp to find the values
size_t whisker_arr_contains_value_uint32_t(whisker_arr_uint32_t *arr, uint32_t value) {
    for (size_t i = 0; i < arr->length; ++i)
    {
        if (memcmp(&arr->arr[i], &value, sizeof(value)) == 0) {
            return i;
        }
    }
    return -1;
}
