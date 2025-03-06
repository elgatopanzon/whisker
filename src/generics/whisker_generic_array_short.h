/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_short_H
#define WHISKER_GENERIC_ARRAY_short_H

typedef struct whisker_arr_short
{
	short *arr;
	size_t length;
	size_t alloc_size;
	short swap_buffer;
} whisker_arr_short;

E_WHISKER_ARR whisker_arr_create_short(whisker_arr_short **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_short(whisker_arr_short *arr);
void whisker_arr_free_short(whisker_arr_short *arr);
E_WHISKER_ARR whisker_arr_resize_short(whisker_arr_short *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_short(whisker_arr_short *arr);
E_WHISKER_ARR whisker_arr_decrement_size_short(whisker_arr_short *arr);
E_WHISKER_ARR whisker_arr_push_short(whisker_arr_short *arr, short value);
E_WHISKER_ARR whisker_arr_pop_short(whisker_arr_short *arr, short *out);
E_WHISKER_ARR whisker_arr_swap_short(whisker_arr_short *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_short(whisker_arr_short *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_short(whisker_arr_short *arr);

// utility functions
size_t whisker_arr_contains_value_short(whisker_arr_short *arr, short value);

#endif /* WHISKER_GENERIC_ARRAY_H */

