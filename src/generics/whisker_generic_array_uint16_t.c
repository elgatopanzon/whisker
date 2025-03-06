/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:38:21 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"

#include "whisker_generic_array.h"
#include "whisker_generic_array_uint16_t.h"

E_WHISKER_ARR whisker_arr_create_uint16_t(whisker_arr_uint16_t **arr, size_t length)
{
	whisker_arr_uint16_t *a;
	E_WHISKER_MEM err = whisker_mem_try_calloc(1, sizeof(whisker_arr_uint16_t), (void**)&a);
	if (err != E_WHISKER_MEM_OK) { return E_WHISKER_ARR_MEM; }
	a->length = length;
	a->alloc_size = sizeof(uint16_t) * a->length;
	if (length > 0) { whisker_arr_init_uint16_t(a); }
	*arr = a;
	return E_WHISKER_ARR_OK;
}

E_WHISKER_ARR whisker_arr_init_uint16_t(whisker_arr_uint16_t *arr)
{
	if (arr->length > 0 && arr->arr == NULL) {  \
		E_WHISKER_MEM err = whisker_mem_try_calloc(1, sizeof(uint16_t) * arr->length, (void**)&arr->arr);
		if (err != E_WHISKER_MEM_OK) { return E_WHISKER_ARR_MEM; }
	}
	return E_WHISKER_ARR_OK;
}

void whisker_arr_free_uint16_t(whisker_arr_uint16_t *arr)
{
	if (arr->arr != NULL) { free(arr->arr); }
	free(arr);
}

E_WHISKER_ARR whisker_arr_resize_uint16_t(whisker_arr_uint16_t *arr, size_t length, bool soft_resize)
{
	if (length == 0 && arr->alloc_size > 0 && !soft_resize) {
		free(arr->arr);
		arr->arr = NULL;
		arr->alloc_size = 0;
	} else if (arr->length == length) {
		return E_WHISKER_ARR_OK;
	} else if (length < 0 && arr->length == 0) {
		return E_WHISKER_ARR_OUT_OF_BOUNDS;
	} else if ((arr->length > length && soft_resize) || arr->alloc_size >= (length * sizeof(uint16_t))) {
		arr->length = length;
		return E_WHISKER_ARR_OK;
	} else {
		E_WHISKER_MEM err = whisker_mem_try_realloc(arr->arr, length * sizeof(uint16_t), (void**)&arr->arr);
		if (err != E_WHISKER_MEM_OK) {
			return E_WHISKER_ARR_MEM;
		}
		memset(((unsigned char*)arr->arr) + arr->alloc_size, 0, (length * sizeof(uint16_t)) - arr->alloc_size);
		arr->alloc_size = length * sizeof(uint16_t);
		arr->length = length;
	}
	return E_WHISKER_ARR_OK;
}

E_WHISKER_ARR whisker_arr_increment_size_uint16_t(whisker_arr_uint16_t *arr)
{
	return whisker_arr_resize_uint16_t(arr, arr->length + 1, true);
}

E_WHISKER_ARR whisker_arr_decrement_size_uint16_t(whisker_arr_uint16_t *arr)
{
	if (arr->length == 0) { return E_WHISKER_ARR_OUT_OF_BOUNDS; }
	return whisker_arr_resize_uint16_t(arr, arr->length - 1, true);
}

E_WHISKER_ARR whisker_arr_push_uint16_t(whisker_arr_uint16_t *arr, uint16_t value)
{
	E_WHISKER_ARR err = whisker_arr_increment_size_uint16_t(arr);
	if (err != E_WHISKER_ARR_OK) { return err; }
	arr->arr[arr->length - 1] = value;
	return E_WHISKER_ARR_OK;
}

E_WHISKER_ARR whisker_arr_pop_uint16_t(whisker_arr_uint16_t *arr, uint16_t *out)
{
	E_WHISKER_ARR err = whisker_arr_decrement_size_uint16_t(arr);
	if (err != E_WHISKER_ARR_OK) { return err; }
	uint16_t popped = arr->arr[arr->length];
	*out = popped;
	return E_WHISKER_ARR_OK;
}

E_WHISKER_ARR whisker_arr_swap_uint16_t(whisker_arr_uint16_t *arr, size_t index_a, size_t index_b)
{
	if (index_a < 0 || index_a > arr->length - 1 || index_b < 0 || index_b > arr->length - 1) { return E_WHISKER_ARR_OUT_OF_BOUNDS; }
	arr->swap_buffer = arr->arr[index_a];
	arr->arr[index_a] = arr->arr[index_b];
	arr->arr[index_b] = arr->swap_buffer;
	memset(&arr->swap_buffer, 0x00, sizeof(uint16_t));
	return E_WHISKER_ARR_OK;
}

void whisker_arr_reset_uint16_t(whisker_arr_uint16_t *arr, bool compact)
{
	arr->length = 0;
	if (compact) { whisker_arr_resize_uint16_t(arr, 0, false); }
}

E_WHISKER_ARR whisker_arr_compact_uint16_t(whisker_arr_uint16_t *arr)
{
	return whisker_arr_resize_uint16_t(arr, arr->length, false);
}

/***********************
*  utility functions  *
***********************/

size_t whisker_arr_contains_value_uint16_t(whisker_arr_uint16_t *arr, uint16_t value) {
    for (size_t i = 0; i < arr->length; ++i)
    {
        if (memcmp(&arr->arr[i], &value, sizeof(value)) == 0) {
            return i;
        }
    }
    return -1;
}
