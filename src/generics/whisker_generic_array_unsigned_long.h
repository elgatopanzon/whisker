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

#ifndef WHISKER_GENERIC_ARRAY_unsigned_long_H
#define WHISKER_GENERIC_ARRAY_unsigned_long_H

typedef struct whisker_arr_unsigned_long
{
	unsigned long *arr;
	size_t length;
	size_t alloc_size;
	unsigned long swap_buffer;
} whisker_arr_unsigned_long;

E_WHISKER_ARR whisker_arr_create_unsigned_long(whisker_arr_unsigned_long **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_unsigned_long(whisker_arr_unsigned_long *arr);
void whisker_arr_free_unsigned_long(whisker_arr_unsigned_long *arr);
E_WHISKER_ARR whisker_arr_resize_unsigned_long(whisker_arr_unsigned_long *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_unsigned_long(whisker_arr_unsigned_long *arr);
E_WHISKER_ARR whisker_arr_decrement_size_unsigned_long(whisker_arr_unsigned_long *arr);
E_WHISKER_ARR whisker_arr_push_unsigned_long(whisker_arr_unsigned_long *arr, unsigned long value);
E_WHISKER_ARR whisker_arr_pop_unsigned_long(whisker_arr_unsigned_long *arr, unsigned long *out);
E_WHISKER_ARR whisker_arr_swap_unsigned_long(whisker_arr_unsigned_long *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_unsigned_long(whisker_arr_unsigned_long *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_unsigned_long(whisker_arr_unsigned_long *arr);

// utility functions
size_t whisker_arr_contains_value_unsigned_long(whisker_arr_unsigned_long *arr, unsigned long value);

#endif /* WHISKER_GENERIC_ARRAY_H */

