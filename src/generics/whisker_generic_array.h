/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include <pthread.h>

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_w_T_H
#define WHISKER_GENERIC_ARRAY_w_T_H

typedef struct whisker_arr_w_T
{
	wT *arr;
	size_t length;
	size_t alloc_size;
	wT swap_buffer;
} whisker_arr_w_T;

E_WHISKER_ARR whisker_arr_create_w_T(whisker_arr_w_T **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_w_T(whisker_arr_w_T *arr);
void whisker_arr_free_w_T(whisker_arr_w_T *arr);
E_WHISKER_ARR whisker_arr_resize_w_T(whisker_arr_w_T *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_w_T(whisker_arr_w_T *arr);
E_WHISKER_ARR whisker_arr_decrement_size_w_T(whisker_arr_w_T *arr);
E_WHISKER_ARR whisker_arr_push_w_T(whisker_arr_w_T *arr, wT value);
E_WHISKER_ARR whisker_arr_pop_w_T(whisker_arr_w_T *arr, wT *out);
E_WHISKER_ARR whisker_arr_swap_w_T(whisker_arr_w_T *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_w_T(whisker_arr_w_T *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_w_T(whisker_arr_w_T *arr);

// utility functions
size_t whisker_arr_contains_value_w_T(whisker_arr_w_T *arr, wT value);

#endif /* WHISKER_GENERIC_ARRAY_H */

