/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:38:21 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"

#include "whisker_generic_array.h"
// wT #include "whisker_generic_array_w_T.h"

E_WHISKER_ARR whisker_arr_create_w_T(whisker_arr_w_T **arr, size_t length)
{
	whisker_arr_w_T *a;
	E_WHISKER_MEM err = whisker_mem_try_calloc(1, sizeof(whisker_arr_w_T), (void**)&a);
	if (err != E_WHISKER_MEM_OK) { return E_WHISKER_ARR_MEM; }
	a->length = length;
	a->alloc_size = sizeof(wT) * a->length;
	if (length > 0) { whisker_arr_init_w_T(a); }
	*arr = a;
	return E_WHISKER_ARR_OK;
}

E_WHISKER_ARR whisker_arr_init_w_T(whisker_arr_w_T *arr)
{
	if (arr->length > 0 && arr->arr == NULL) {  \
		E_WHISKER_MEM err = whisker_mem_try_calloc(1, sizeof(wT) * arr->length, (void**)&arr->arr);
		if (err != E_WHISKER_MEM_OK) { return E_WHISKER_ARR_MEM; }
	}
	return E_WHISKER_ARR_OK;
}

void whisker_arr_free_w_T(whisker_arr_w_T *arr)
{
	if (arr->arr != NULL) { free(arr->arr); }
	free(arr);
}

E_WHISKER_ARR whisker_arr_resize_w_T(whisker_arr_w_T *arr, size_t length, bool soft_resize)
{
	if (length == 0 && arr->alloc_size > 0 && !soft_resize) {
		free(arr->arr);
		arr->arr = NULL;
		arr->alloc_size = 0;
	} else if (arr->length == length) {
		return E_WHISKER_ARR_OK;
	} else if (length < 0 && arr->length == 0) {
		return E_WHISKER_ARR_OUT_OF_BOUNDS;
	} else if ((arr->length > length && soft_resize) || arr->alloc_size >= (length * sizeof(wT))) {
		arr->length = length;
		return E_WHISKER_ARR_OK;
	} else {
		E_WHISKER_MEM err = whisker_mem_try_realloc(arr->arr, length * sizeof(wT), (void**)&arr->arr);
		if (err != E_WHISKER_MEM_OK) {
			return E_WHISKER_ARR_MEM;
		}
		memset(((unsigned char*)arr->arr) + arr->alloc_size, 0, (length * sizeof(wT)) - arr->alloc_size);
		arr->alloc_size = length * sizeof(wT);
		arr->length = length;
	}
	return E_WHISKER_ARR_OK;
}

E_WHISKER_ARR whisker_arr_increment_size_w_T(whisker_arr_w_T *arr)
{
	return whisker_arr_resize_w_T(arr, arr->length + 1, true);
}

E_WHISKER_ARR whisker_arr_decrement_size_w_T(whisker_arr_w_T *arr)
{
	if (arr->length == 0) { return E_WHISKER_ARR_OUT_OF_BOUNDS; }
	return whisker_arr_resize_w_T(arr, arr->length - 1, true);
}

E_WHISKER_ARR whisker_arr_push_w_T(whisker_arr_w_T *arr, wT value)
{
	E_WHISKER_ARR err = whisker_arr_increment_size_w_T(arr);
	if (err != E_WHISKER_ARR_OK) { return err; }
	arr->arr[arr->length - 1] = value;
	return E_WHISKER_ARR_OK;
}

E_WHISKER_ARR whisker_arr_pop_w_T(whisker_arr_w_T *arr, wT *out)
{
	E_WHISKER_ARR err = whisker_arr_decrement_size_w_T(arr);
	if (err != E_WHISKER_ARR_OK) { return err; }
	wT popped = arr->arr[arr->length];
	*out = popped;
	return E_WHISKER_ARR_OK;
}

E_WHISKER_ARR whisker_arr_swap_w_T(whisker_arr_w_T *arr, size_t index_a, size_t index_b)
{
	if (index_a < 0 || index_a > arr->length - 1 || index_b < 0 || index_b > arr->length - 1) { return E_WHISKER_ARR_OUT_OF_BOUNDS; }
	arr->swap_buffer = arr->arr[index_a];
	arr->arr[index_a] = arr->arr[index_b];
	arr->arr[index_b] = arr->swap_buffer;
	memset(&arr->swap_buffer, 0x00, sizeof(wT));
	return E_WHISKER_ARR_OK;
}

void whisker_arr_reset_w_T(whisker_arr_w_T *arr, bool compact)
{
	arr->length = 0;
	if (compact) { whisker_arr_resize_w_T(arr, 0, false); }
}

E_WHISKER_ARR whisker_arr_compact_w_T(whisker_arr_w_T *arr)
{
	return whisker_arr_resize_w_T(arr, arr->length, false);
}
