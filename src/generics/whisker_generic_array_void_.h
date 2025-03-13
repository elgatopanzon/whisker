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

#ifndef WHISKER_GENERIC_ARRAY_void__H
#define WHISKER_GENERIC_ARRAY_void__H

typedef struct whisker_arr_void_
{
	void* *arr;
	size_t length;
	size_t alloc_size;
	void* swap_buffer;
} whisker_arr_void_;

E_WHISKER_ARR whisker_arr_create_void_(whisker_arr_void_ **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_void_(whisker_arr_void_ *arr);
void whisker_arr_free_void_(whisker_arr_void_ *arr);
E_WHISKER_ARR whisker_arr_resize_void_(whisker_arr_void_ *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_void_(whisker_arr_void_ *arr);
E_WHISKER_ARR whisker_arr_decrement_size_void_(whisker_arr_void_ *arr);
E_WHISKER_ARR whisker_arr_push_void_(whisker_arr_void_ *arr, void* value);
E_WHISKER_ARR whisker_arr_pop_void_(whisker_arr_void_ *arr, void* *out);
E_WHISKER_ARR whisker_arr_swap_void_(whisker_arr_void_ *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_void_(whisker_arr_void_ *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_void_(whisker_arr_void_ *arr);

// utility functions
size_t whisker_arr_contains_value_void_(whisker_arr_void_ *arr, void* value);

#endif /* WHISKER_GENERIC_ARRAY_H */

