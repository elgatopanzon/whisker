/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_char_H
#define WHISKER_GENERIC_ARRAY_char_H

typedef struct whisker_arr_char
{
	char *arr;
	size_t length;
	size_t alloc_size;
	char swap_buffer;
} whisker_arr_char;

E_WHISKER_ARR whisker_arr_create_char(whisker_arr_char **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_char(whisker_arr_char *arr);
void whisker_arr_free_char(whisker_arr_char *arr);
E_WHISKER_ARR whisker_arr_resize_char(whisker_arr_char *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_char(whisker_arr_char *arr);
E_WHISKER_ARR whisker_arr_decrement_size_char(whisker_arr_char *arr);
E_WHISKER_ARR whisker_arr_push_char(whisker_arr_char *arr, char value);
E_WHISKER_ARR whisker_arr_pop_char(whisker_arr_char *arr, char *out);
E_WHISKER_ARR whisker_arr_swap_char(whisker_arr_char *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_char(whisker_arr_char *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_char(whisker_arr_char *arr);

// utility functions
size_t whisker_arr_contains_value_char(whisker_arr_char *arr, char value);

#endif /* WHISKER_GENERIC_ARRAY_H */

