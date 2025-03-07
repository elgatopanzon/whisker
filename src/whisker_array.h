/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_array
 * @created     : Wednesday Feb 05, 2025 12:38:49 CST
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "whisker_memory.h"

#ifndef WHISKER_ARRAY_H
#define WHISKER_ARRAY_H

// errors
typedef enum E_WHISKER_ARR  
{
	E_WHISKER_ARR_OK = 0,
	E_WHISKER_ARR_UNKNOWN = 1,
	E_WHISKER_ARR_MEM = 2,
	E_WHISKER_ARR_OUT_OF_BOUNDS = 3,
} E_WHISKER_ARR;
extern const char* E_WHISKER_ARR_STR[];

// header struct holds info about the array
typedef struct whisker_array_header
{
	size_t size;
	size_t element_size;
	size_t length;
	void *swap_buffer;
} whisker_array_header;

// macros
#define whisker_arr_create(t, l, p) whisker_arr_create_f(sizeof(t), l, (void**) p)
#define whisker_arr_resize(p, e) whisker_arr_resize_f((void**) p, e, false)
#define whisker_arr_increment_size(p) whisker_arr_increment_size_f((void**) p)
#define whisker_arr_push(p, v) whisker_arr_push_f((void**) p, (void*) v)
#define whisker_arr_pop(p, v) whisker_arr_pop_f((void**) p, (void*) v)
#define whisker_arr_pop_front(p, v) whisker_arr_pop_front_f((void**) p, (void*) v)
#define whisker_arr_pop_c(p, t, v) t v; whisker_arr_pop_f((void**) p, (void*) &v)
#define whisker_arr_pop_front_c(p, t, v) t v; whisker_arr_pop_front_f((void**) p, (void*) &v)
#define whisker_arr_compact(p) whisker_arr_compact_f((void**) p)
#define whisker_arr_insert(p, i, v) whisker_arr_insert_f((void**) p, i, (void*) v)
#define whisker_arr_grow_get(p, i) whisker_arr_grow_get_f((void**) p, i)
#define whisker_arr_length(a) whisker_arr_length_f((char*)a)
#define whisker_arr_header(a) whisker_arr_header_f((char*)a)
#define whisker_arr_reset(a) whisker_arr_reset_f((char*)a)

// short macros
#define warr_create whisker_arr_create
#define warr_resize whisker_arr_resize
#define warr_increment_size whisker_arr_increment_size
#define warr_push whisker_arr_push
#define warr_pop whisker_arr_pop
#define warr_pop_c whisker_arr_pop_c
#define warr_pop_front whisker_arr_pop_front
#define warr_pop_front_c whisker_arr_pop_front_c
#define warr_compact whisker_arr_compact
#define warr_insert whisker_arr_insert
#define warr_reset whisker_arr_reset
#define warr_header whisker_arr_header
#define warr_length whisker_arr_length
#define warr_free whisker_arr_free

// array management functions
E_WHISKER_ARR whisker_arr_create_f(size_t type_size, size_t length, void** arr);
E_WHISKER_ARR whisker_arr_resize_f(void** arr, size_t elements, bool allow_shrink);
E_WHISKER_ARR whisker_arr_increment_size_f(void** arr);
E_WHISKER_ARR whisker_arr_push_f(void** arr, void* value);
E_WHISKER_ARR whisker_arr_pop_f(void** arr, void* value);
E_WHISKER_ARR whisker_arr_pop_front_f(void** arr, void* value);
E_WHISKER_ARR whisker_arr_swap(void** arr, size_t index_from, size_t index_to);
E_WHISKER_ARR whisker_arr_compact_f(void** arr);
E_WHISKER_ARR whisker_arr_insert_f(void** arr, size_t index, void* value);
E_WHISKER_ARR whisker_arr_reset_f(char* arr);

// utility functions to work with the array
whisker_array_header* whisker_arr_header_f(char* arr);
size_t whisker_arr_length_f(char* arr);
void* whisker_arr_grow_get_f(void** arr, size_t index);

void whisker_arr_free(void* arr);

#endif /* end of include guard WHISKER_ARRAY_H */
