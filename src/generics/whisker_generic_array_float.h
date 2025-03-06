/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_float_H
#define WHISKER_GENERIC_ARRAY_float_H

typedef struct whisker_arr_float
{
	float *arr;
	size_t length;
	size_t alloc_size;
	float swap_buffer;
} whisker_arr_float;

E_WHISKER_ARR whisker_arr_create_float(whisker_arr_float **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_float(whisker_arr_float *arr);
void whisker_arr_free_float(whisker_arr_float *arr);
E_WHISKER_ARR whisker_arr_resize_float(whisker_arr_float *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_float(whisker_arr_float *arr);
E_WHISKER_ARR whisker_arr_decrement_size_float(whisker_arr_float *arr);
E_WHISKER_ARR whisker_arr_push_float(whisker_arr_float *arr, float value);
E_WHISKER_ARR whisker_arr_pop_float(whisker_arr_float *arr, float *out);
E_WHISKER_ARR whisker_arr_swap_float(whisker_arr_float *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_float(whisker_arr_float *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_float(whisker_arr_float *arr);

// utility functions
size_t whisker_arr_contains_value_float(whisker_arr_float *arr, float value);

#endif /* WHISKER_GENERIC_ARRAY_H */

