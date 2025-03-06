/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_long_H
#define WHISKER_GENERIC_ARRAY_long_H

typedef struct whisker_arr_long
{
	long *arr;
	size_t length;
	size_t alloc_size;
	long swap_buffer;
} whisker_arr_long;

E_WHISKER_ARR whisker_arr_create_long(whisker_arr_long **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_long(whisker_arr_long *arr);
void whisker_arr_free_long(whisker_arr_long *arr);
E_WHISKER_ARR whisker_arr_resize_long(whisker_arr_long *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_long(whisker_arr_long *arr);
E_WHISKER_ARR whisker_arr_decrement_size_long(whisker_arr_long *arr);
E_WHISKER_ARR whisker_arr_push_long(whisker_arr_long *arr, long value);
E_WHISKER_ARR whisker_arr_pop_long(whisker_arr_long *arr, long *out);
E_WHISKER_ARR whisker_arr_swap_long(whisker_arr_long *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_long(whisker_arr_long *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_long(whisker_arr_long *arr);

// utility functions
size_t whisker_arr_contains_value_long(whisker_arr_long *arr, long value);

#endif /* WHISKER_GENERIC_ARRAY_H */

