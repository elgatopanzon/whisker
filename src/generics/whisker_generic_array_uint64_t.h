/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_uint64_t_H
#define WHISKER_GENERIC_ARRAY_uint64_t_H

typedef struct whisker_arr_uint64_t
{
	uint64_t *arr;
	size_t length;
	size_t alloc_size;
	uint64_t swap_buffer;
} whisker_arr_uint64_t;

E_WHISKER_ARR whisker_arr_create_uint64_t(whisker_arr_uint64_t **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_uint64_t(whisker_arr_uint64_t *arr);
void whisker_arr_free_uint64_t(whisker_arr_uint64_t *arr);
E_WHISKER_ARR whisker_arr_resize_uint64_t(whisker_arr_uint64_t *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_uint64_t(whisker_arr_uint64_t *arr);
E_WHISKER_ARR whisker_arr_decrement_size_uint64_t(whisker_arr_uint64_t *arr);
E_WHISKER_ARR whisker_arr_push_uint64_t(whisker_arr_uint64_t *arr, uint64_t value);
E_WHISKER_ARR whisker_arr_pop_uint64_t(whisker_arr_uint64_t *arr, uint64_t *out);
E_WHISKER_ARR whisker_arr_swap_uint64_t(whisker_arr_uint64_t *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_uint64_t(whisker_arr_uint64_t *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_uint64_t(whisker_arr_uint64_t *arr);

#endif /* WHISKER_GENERIC_ARRAY_H */

