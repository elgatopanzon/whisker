/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_unsigned_char_H
#define WHISKER_GENERIC_ARRAY_unsigned_char_H

typedef struct whisker_arr_unsigned_char
{
	unsigned char *arr;
	size_t length;
	size_t alloc_size;
	unsigned char swap_buffer;
} whisker_arr_unsigned_char;

E_WHISKER_ARR whisker_arr_create_unsigned_char(whisker_arr_unsigned_char **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_unsigned_char(whisker_arr_unsigned_char *arr);
void whisker_arr_free_unsigned_char(whisker_arr_unsigned_char *arr);
E_WHISKER_ARR whisker_arr_resize_unsigned_char(whisker_arr_unsigned_char *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_unsigned_char(whisker_arr_unsigned_char *arr);
E_WHISKER_ARR whisker_arr_decrement_size_unsigned_char(whisker_arr_unsigned_char *arr);
E_WHISKER_ARR whisker_arr_push_unsigned_char(whisker_arr_unsigned_char *arr, unsigned char value);
E_WHISKER_ARR whisker_arr_pop_unsigned_char(whisker_arr_unsigned_char *arr, unsigned char *out);
E_WHISKER_ARR whisker_arr_swap_unsigned_char(whisker_arr_unsigned_char *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_unsigned_char(whisker_arr_unsigned_char *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_unsigned_char(whisker_arr_unsigned_char *arr);

// utility functions
size_t whisker_arr_contains_value_unsigned_char(whisker_arr_unsigned_char *arr, unsigned char value);

#endif /* WHISKER_GENERIC_ARRAY_H */

