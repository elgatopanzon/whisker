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

#ifndef WHISKER_GENERIC_ARRAY_pthread_t_H
#define WHISKER_GENERIC_ARRAY_pthread_t_H

typedef struct whisker_arr_pthread_t
{
	pthread_t *arr;
	size_t length;
	size_t alloc_size;
	pthread_t swap_buffer;
} whisker_arr_pthread_t;

E_WHISKER_ARR whisker_arr_create_pthread_t(whisker_arr_pthread_t **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_pthread_t(whisker_arr_pthread_t *arr);
void whisker_arr_free_pthread_t(whisker_arr_pthread_t *arr);
E_WHISKER_ARR whisker_arr_resize_pthread_t(whisker_arr_pthread_t *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_pthread_t(whisker_arr_pthread_t *arr);
E_WHISKER_ARR whisker_arr_decrement_size_pthread_t(whisker_arr_pthread_t *arr);
E_WHISKER_ARR whisker_arr_push_pthread_t(whisker_arr_pthread_t *arr, pthread_t value);
E_WHISKER_ARR whisker_arr_pop_pthread_t(whisker_arr_pthread_t *arr, pthread_t *out);
E_WHISKER_ARR whisker_arr_swap_pthread_t(whisker_arr_pthread_t *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_pthread_t(whisker_arr_pthread_t *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_pthread_t(whisker_arr_pthread_t *arr);

// utility functions
size_t whisker_arr_contains_value_pthread_t(whisker_arr_pthread_t *arr, pthread_t value);

#endif /* WHISKER_GENERIC_ARRAY_H */

