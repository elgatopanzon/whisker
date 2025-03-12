/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_sparse_set.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_whisker_sparse_set_H
#define WHISKER_GENERIC_ARRAY_whisker_sparse_set_H

typedef struct whisker_arr_whisker_sparse_set
{
	whisker_sparse_set *arr;
	size_t length;
	size_t alloc_size;
	whisker_sparse_set swap_buffer;
} whisker_arr_whisker_sparse_set;

E_WHISKER_ARR whisker_arr_create_whisker_sparse_set(whisker_arr_whisker_sparse_set **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_whisker_sparse_set(whisker_arr_whisker_sparse_set *arr);
void whisker_arr_free_whisker_sparse_set(whisker_arr_whisker_sparse_set *arr);
E_WHISKER_ARR whisker_arr_resize_whisker_sparse_set(whisker_arr_whisker_sparse_set *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_whisker_sparse_set(whisker_arr_whisker_sparse_set *arr);
E_WHISKER_ARR whisker_arr_decrement_size_whisker_sparse_set(whisker_arr_whisker_sparse_set *arr);
E_WHISKER_ARR whisker_arr_push_whisker_sparse_set(whisker_arr_whisker_sparse_set *arr, whisker_sparse_set value);
E_WHISKER_ARR whisker_arr_pop_whisker_sparse_set(whisker_arr_whisker_sparse_set *arr, whisker_sparse_set *out);
E_WHISKER_ARR whisker_arr_swap_whisker_sparse_set(whisker_arr_whisker_sparse_set *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_whisker_sparse_set(whisker_arr_whisker_sparse_set *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_whisker_sparse_set(whisker_arr_whisker_sparse_set *arr);

// utility functions
size_t whisker_arr_contains_value_whisker_sparse_set(whisker_arr_whisker_sparse_set *arr, whisker_sparse_set value);

typedef struct whisker_arr_whisker_sparse_set_ptr
{
	whisker_sparse_set **arr;
	size_t length;
	size_t alloc_size;
	whisker_sparse_set *swap_buffer;
} whisker_arr_whisker_sparse_set_ptr;

E_WHISKER_ARR whisker_arr_create_whisker_sparse_set_ptr(whisker_arr_whisker_sparse_set_ptr **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_whisker_sparse_set_ptr(whisker_arr_whisker_sparse_set_ptr *arr);
void whisker_arr_free_whisker_sparse_set_ptr(whisker_arr_whisker_sparse_set_ptr *arr);
E_WHISKER_ARR whisker_arr_resize_whisker_sparse_set_ptr(whisker_arr_whisker_sparse_set_ptr *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_whisker_sparse_set_ptr(whisker_arr_whisker_sparse_set_ptr *arr);
E_WHISKER_ARR whisker_arr_decrement_size_whisker_sparse_set_ptr(whisker_arr_whisker_sparse_set_ptr *arr);
E_WHISKER_ARR whisker_arr_push_whisker_sparse_set_ptr(whisker_arr_whisker_sparse_set_ptr *arr, whisker_sparse_set *value);
E_WHISKER_ARR whisker_arr_pop_whisker_sparse_set_ptr(whisker_arr_whisker_sparse_set_ptr *arr, whisker_sparse_set **out);
E_WHISKER_ARR whisker_arr_swap_whisker_sparse_set_ptr(whisker_arr_whisker_sparse_set_ptr *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_whisker_sparse_set_ptr(whisker_arr_whisker_sparse_set_ptr *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_whisker_sparse_set_ptr(whisker_arr_whisker_sparse_set_ptr *arr);

// utility functions
size_t whisker_arr_contains_value_whisker_sparse_set_ptr(whisker_arr_whisker_sparse_set_ptr *arr, whisker_sparse_set *value);

#endif /* WHISKER_GENERIC_ARRAY_H */

