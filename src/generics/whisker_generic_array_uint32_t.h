/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_uint32_t_H
#define WHISKER_GENERIC_ARRAY_uint32_t_H

typedef struct whisker_arr_uint32_t
{
	uint32_t *arr;
	size_t length;
	size_t alloc_size;
	uint32_t swap_buffer;
} whisker_arr_uint32_t;

E_WHISKER_ARR whisker_arr_create_uint32_t(whisker_arr_uint32_t **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_uint32_t(whisker_arr_uint32_t *arr);
void whisker_arr_free_uint32_t(whisker_arr_uint32_t *arr);
E_WHISKER_ARR whisker_arr_resize_uint32_t(whisker_arr_uint32_t *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_uint32_t(whisker_arr_uint32_t *arr);
E_WHISKER_ARR whisker_arr_decrement_size_uint32_t(whisker_arr_uint32_t *arr);
E_WHISKER_ARR whisker_arr_push_uint32_t(whisker_arr_uint32_t *arr, uint32_t value);
E_WHISKER_ARR whisker_arr_pop_uint32_t(whisker_arr_uint32_t *arr, uint32_t *out);
E_WHISKER_ARR whisker_arr_swap_uint32_t(whisker_arr_uint32_t *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_uint32_t(whisker_arr_uint32_t *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_uint32_t(whisker_arr_uint32_t *arr);

#endif /* WHISKER_GENERIC_ARRAY_H */

