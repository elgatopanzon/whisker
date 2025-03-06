/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_int_H
#define WHISKER_GENERIC_ARRAY_int_H

typedef struct whisker_arr_int
{
	int *arr;
	size_t length;
	size_t alloc_size;
	int swap_buffer;
} whisker_arr_int;

E_WHISKER_ARR whisker_arr_create_int(whisker_arr_int **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_int(whisker_arr_int *arr);
void whisker_arr_free_int(whisker_arr_int *arr);
E_WHISKER_ARR whisker_arr_resize_int(whisker_arr_int *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_int(whisker_arr_int *arr);
E_WHISKER_ARR whisker_arr_decrement_size_int(whisker_arr_int *arr);
E_WHISKER_ARR whisker_arr_push_int(whisker_arr_int *arr, int value);
E_WHISKER_ARR whisker_arr_pop_int(whisker_arr_int *arr, int *out);
E_WHISKER_ARR whisker_arr_swap_int(whisker_arr_int *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_int(whisker_arr_int *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_int(whisker_arr_int *arr);

// utility functions
size_t whisker_arr_contains_value_int(whisker_arr_int *arr, int value);

#endif /* WHISKER_GENERIC_ARRAY_H */

