/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_unsigned_short_H
#define WHISKER_GENERIC_ARRAY_unsigned_short_H

typedef struct whisker_arr_unsigned_short
{
	unsigned short *arr;
	size_t length;
	size_t alloc_size;
	unsigned short swap_buffer;
} whisker_arr_unsigned_short;

E_WHISKER_ARR whisker_arr_create_unsigned_short(whisker_arr_unsigned_short **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_unsigned_short(whisker_arr_unsigned_short *arr);
void whisker_arr_free_unsigned_short(whisker_arr_unsigned_short *arr);
E_WHISKER_ARR whisker_arr_resize_unsigned_short(whisker_arr_unsigned_short *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_unsigned_short(whisker_arr_unsigned_short *arr);
E_WHISKER_ARR whisker_arr_decrement_size_unsigned_short(whisker_arr_unsigned_short *arr);
E_WHISKER_ARR whisker_arr_push_unsigned_short(whisker_arr_unsigned_short *arr, unsigned short value);
E_WHISKER_ARR whisker_arr_pop_unsigned_short(whisker_arr_unsigned_short *arr, unsigned short *out);
E_WHISKER_ARR whisker_arr_swap_unsigned_short(whisker_arr_unsigned_short *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_unsigned_short(whisker_arr_unsigned_short *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_unsigned_short(whisker_arr_unsigned_short *arr);

#endif /* WHISKER_GENERIC_ARRAY_H */

