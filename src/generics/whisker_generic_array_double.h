/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_double_H
#define WHISKER_GENERIC_ARRAY_double_H

typedef struct whisker_arr_double
{
	double *arr;
	size_t length;
	size_t alloc_size;
	double swap_buffer;
} whisker_arr_double;

E_WHISKER_ARR whisker_arr_create_double(whisker_arr_double **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_double(whisker_arr_double *arr);
void whisker_arr_free_double(whisker_arr_double *arr);
E_WHISKER_ARR whisker_arr_resize_double(whisker_arr_double *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_double(whisker_arr_double *arr);
E_WHISKER_ARR whisker_arr_decrement_size_double(whisker_arr_double *arr);
E_WHISKER_ARR whisker_arr_push_double(whisker_arr_double *arr, double value);
E_WHISKER_ARR whisker_arr_pop_double(whisker_arr_double *arr, double *out);
E_WHISKER_ARR whisker_arr_swap_double(whisker_arr_double *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_double(whisker_arr_double *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_double(whisker_arr_double *arr);

// utility functions
size_t whisker_arr_contains_value_double(whisker_arr_double *arr, double value);

#endif /* WHISKER_GENERIC_ARRAY_H */

