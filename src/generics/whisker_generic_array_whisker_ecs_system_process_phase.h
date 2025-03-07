/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_array
 * @created     : Sunday Mar 02, 2025 11:09:46 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"

#ifndef WHISKER_GENERIC_ARRAY_whisker_ecs_system_process_phase_H
#define WHISKER_GENERIC_ARRAY_whisker_ecs_system_process_phase_H

typedef struct whisker_arr_whisker_ecs_system_process_phase
{
	whisker_ecs_system_process_phase *arr;
	size_t length;
	size_t alloc_size;
	whisker_ecs_system_process_phase swap_buffer;
} whisker_arr_whisker_ecs_system_process_phase;

E_WHISKER_ARR whisker_arr_create_whisker_ecs_system_process_phase(whisker_arr_whisker_ecs_system_process_phase **arr, size_t length);
E_WHISKER_ARR whisker_arr_init_whisker_ecs_system_process_phase(whisker_arr_whisker_ecs_system_process_phase *arr);
void whisker_arr_free_whisker_ecs_system_process_phase(whisker_arr_whisker_ecs_system_process_phase *arr);
E_WHISKER_ARR whisker_arr_resize_whisker_ecs_system_process_phase(whisker_arr_whisker_ecs_system_process_phase *arr, size_t length, bool soft_resize);
E_WHISKER_ARR whisker_arr_increment_size_whisker_ecs_system_process_phase(whisker_arr_whisker_ecs_system_process_phase *arr);
E_WHISKER_ARR whisker_arr_decrement_size_whisker_ecs_system_process_phase(whisker_arr_whisker_ecs_system_process_phase *arr);
E_WHISKER_ARR whisker_arr_push_whisker_ecs_system_process_phase(whisker_arr_whisker_ecs_system_process_phase *arr, whisker_ecs_system_process_phase value);
E_WHISKER_ARR whisker_arr_pop_whisker_ecs_system_process_phase(whisker_arr_whisker_ecs_system_process_phase *arr, whisker_ecs_system_process_phase *out);
E_WHISKER_ARR whisker_arr_swap_whisker_ecs_system_process_phase(whisker_arr_whisker_ecs_system_process_phase *arr, size_t index_a, size_t index_b);
void whisker_arr_reset_whisker_ecs_system_process_phase(whisker_arr_whisker_ecs_system_process_phase *arr, bool compact);
E_WHISKER_ARR whisker_arr_compact_whisker_ecs_system_process_phase(whisker_arr_whisker_ecs_system_process_phase *arr);

// utility functions
size_t whisker_arr_contains_value_whisker_ecs_system_process_phase(whisker_arr_whisker_ecs_system_process_phase *arr, whisker_ecs_system_process_phase value);

#endif /* WHISKER_GENERIC_ARRAY_H */

