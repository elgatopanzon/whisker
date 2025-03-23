/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include <pthread.h>

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_ecs_v1.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_long_double_H
#define WHISKER_GENERIC_ARRAY_long_double_H

typedef struct whisker_arr_long_double
{
	long double *arr;
	size_t length;
	size_t alloc_size;
	long double swap_buffer;
} whisker_arr_long_double;

E_WHISKER_ARR whisker_arr_create_long_double(whisker_arr_long_double **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_long_double(whisker_arr_long_double *arr);
void whisker_arr_free_long_double(whisker_arr_long_double *arr);
E_WHISKER_ARR whisker_arr_resize_long_double(whisker_arr_long_double *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_long_double(whisker_arr_long_double *arr);
E_WHISKER_ARR whisker_arr_decrement_size_long_double(whisker_arr_long_double *arr);
E_WHISKER_ARR whisker_arr_push_long_double(whisker_arr_long_double *arr, long double value);
E_WHISKER_ARR whisker_arr_pop_long_double(whisker_arr_long_double *arr, long double *out);
E_WHISKER_ARR whisker_arr_swap_long_double(whisker_arr_long_double *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_long_double(whisker_arr_long_double *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_long_double(whisker_arr_long_double *arr);

// utility functions
size_t whisker_arr_contains_value_long_double(whisker_arr_long_double *arr, long double value);

#endif /* WHISKER_GENERIC_ARRAY_H */

