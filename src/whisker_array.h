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

/////////////////////////////
//  macro implementations  //
/////////////////////////////
// the macros allow defining the implementation in a type-safe way
// #define whisker_arr(T) whisker_arr_typedef(T, T)
// #define whisker_arr_typedef(T, ...) \
// 	whisker_arr_dec_struct(T, __VA_ARGS__) \
// 	whisker_arr_dec_create(T, __VA_ARGS__) \
// 	whisker_arr_dec_init(T, __VA_ARGS__) \
// 	whisker_arr_dec_free(T, __VA_ARGS__) \
// 	whisker_arr_dec_resize(T, __VA_ARGS__) \
// 	whisker_arr_dec_increment_size(T, __VA_ARGS__) \
// 	whisker_arr_dec_decrement_size(T, __VA_ARGS__) \
// 	whisker_arr_dec_push(T, __VA_ARGS__) \
// 	whisker_arr_dec_pop(T, __VA_ARGS__) \
// 	whisker_arr_dec_swap(T, __VA_ARGS__) \
// 	whisker_arr_dec_compact(T, __VA_ARGS__) \
// 	whisker_arr_dec_reset(T, __VA_ARGS__) \
//
// #define whisker_arr_impl(T) whisker_arr_impl_typedef(T, T)
// #define whisker_arr_impl_typedef(T, ...) \
// 	whisker_arr_impl_create(T, __VA_ARGS__) \
// 	whisker_arr_impl_init(T, __VA_ARGS__) \
// 	whisker_arr_impl_free(T, __VA_ARGS__) \
// 	whisker_arr_impl_resize(T, __VA_ARGS__) \
// 	whisker_arr_impl_increment_size(T, __VA_ARGS__) \
// 	whisker_arr_impl_decrement_size(T, __VA_ARGS__) \
// 	whisker_arr_impl_push(T, __VA_ARGS__) \
// 	whisker_arr_impl_pop(T, __VA_ARGS__) \
// 	whisker_arr_impl_swap(T, __VA_ARGS__) \
// 	whisker_arr_impl_compact(T, __VA_ARGS__) \
// 	whisker_arr_impl_reset(T, __VA_ARGS__) \
//
// #define whisker_arr_dec_struct(T, n) \
// typedef struct whisker_arr_##n \
// { \
// 	T *arr; \
// 	size_t length; \
// 	size_t alloc_size; \
// 	T swap_buffer; \
// } whisker_arr_##n; \
//
// #define whisker_arr_dec_create(T, n) \
// E_WHISKER_ARR whisker_arr_create_##n(whisker_arr_##n **arr, size_t length); \
//
// #define whisker_arr_impl_create(T, n) \
// E_WHISKER_ARR whisker_arr_create_##n(whisker_arr_##n **arr, size_t length) \
// { \
// 	whisker_arr_##n *a; \
// 	E_WHISKER_MEM err = whisker_mem_try_calloc(1, sizeof(whisker_arr_##n), (void**)&a); \
// 	if (err != E_WHISKER_MEM_OK) { return E_WHISKER_ARR_MEM; } \
// 	a->length = length; \
// 	a->alloc_size = sizeof(T) * a->length; \
// 	if (length > 0) { whisker_arr_init_##n(a); } \
// 	*arr = a; \
// 	return E_WHISKER_ARR_OK; \
// } \
//
// #define whisker_arr_dec_init(T, n) \
// E_WHISKER_ARR whisker_arr_init_##n(whisker_arr_##n *arr); \
//
// #define whisker_arr_impl_init(T, n) \
// E_WHISKER_ARR whisker_arr_init_##n(whisker_arr_##n *arr) \
// { \
// 	if (arr->length > 0 && arr->arr == NULL) {  \
// 		E_WHISKER_MEM err = whisker_mem_try_calloc(1, sizeof(T) * arr->length, (void**)&arr->arr); \
// 		if (err != E_WHISKER_MEM_OK) { return E_WHISKER_ARR_MEM; } \
// 	} \
// 	return E_WHISKER_ARR_OK; \
// } \
//
// #define whisker_arr_dec_free(T, n) \
// void whisker_arr_free_##n(whisker_arr_##n *arr); \
//
// #define whisker_arr_impl_free(T, n) \
// void whisker_arr_free_##n(whisker_arr_##n *arr) \
// { \
// 	if (arr->arr != NULL) { free(arr->arr); } \
// 	free(arr); \
// } \
//
// #define whisker_arr_dec_resize(T, n) \
// E_WHISKER_ARR whisker_arr_resize_##n(whisker_arr_##n *arr, size_t length, bool soft_resize); \
//
// #define whisker_arr_impl_resize(T, n) \
// E_WHISKER_ARR whisker_arr_resize_##n(whisker_arr_##n *arr, size_t length, bool soft_resize) \
// { \
// 	if (length == 0 && arr->alloc_size > 0 && !soft_resize) { \
// 		free(arr->arr); \
// 		arr->arr = NULL; \
// 		arr->alloc_size = 0; \
// 	} else if (arr->length == length) { \
// 		return E_WHISKER_ARR_OK; \
// 	} else if (length < 0 && arr->length == 0) { \
// 		return E_WHISKER_ARR_OUT_OF_BOUNDS; \
// 	} else if ((arr->length > length && soft_resize) || arr->alloc_size >= (length * sizeof(T))) { \
// 		arr->length = length; \
// 		return E_WHISKER_ARR_OK; \
// 	} else { \
// 		E_WHISKER_MEM err = whisker_mem_try_realloc(arr->arr, length * sizeof(T), (void**)&arr->arr); \
// 		if (err != E_WHISKER_MEM_OK) { \
// 			return E_WHISKER_ARR_MEM; \
// 		} \
// 		memset(((unsigned char*)arr->arr) + arr->alloc_size, 0, (length * sizeof(T)) - arr->alloc_size); \
// 		arr->alloc_size = length * sizeof(T); \
// 		arr->length = length; \
// 	} \
// 	return E_WHISKER_ARR_OK; \
// } \
//
// #define whisker_arr_dec_increment_size(T, n) \
// E_WHISKER_ARR whisker_arr_increment_size_##n(whisker_arr_##n *arr); \
//
// #define whisker_arr_impl_increment_size(T, n) \
// E_WHISKER_ARR whisker_arr_increment_size_##n(whisker_arr_##n *arr) \
// { \
// 	return whisker_arr_resize_##n(arr, arr->length + 1, true); \
// } \
//
// #define whisker_arr_dec_decrement_size(T, n) \
// E_WHISKER_ARR whisker_arr_decrement_size_##n(whisker_arr_##n *arr); \
//
// #define whisker_arr_impl_decrement_size(T, n) \
// E_WHISKER_ARR whisker_arr_decrement_size_##n(whisker_arr_##n *arr) \
// { \
// 	if (arr->length == 0) { return E_WHISKER_ARR_OUT_OF_BOUNDS; } \
// 	return whisker_arr_resize_##n(arr, arr->length - 1, true); \
// } \
//
// #define whisker_arr_dec_push(T, n) \
// E_WHISKER_ARR whisker_arr_push_##n(whisker_arr_##n *arr, T value); \
//
// #define whisker_arr_impl_push(T, n) \
// E_WHISKER_ARR whisker_arr_push_##n(whisker_arr_##n *arr, T value) \
// { \
// 	E_WHISKER_ARR err = whisker_arr_increment_size_##n(arr); \
// 	if (err != E_WHISKER_ARR_OK) { return err; } \
// 	arr->arr[arr->length - 1] = value; \
// 	return E_WHISKER_ARR_OK; \
// } \
//
// #define whisker_arr_dec_pop(T, n) \
// E_WHISKER_ARR whisker_arr_pop_##n(whisker_arr_##n *arr, T *out); \
//
// #define whisker_arr_impl_pop(T, n) \
// E_WHISKER_ARR whisker_arr_pop_##n(whisker_arr_##n *arr, T *out) \
// { \
// 	E_WHISKER_ARR err = whisker_arr_decrement_size_##n(arr); \
// 	if (err != E_WHISKER_ARR_OK) { return err; } \
// 	T popped = arr->arr[arr->length]; \
// 	*out = popped; \
// 	return E_WHISKER_ARR_OK; \
// } \
//
// #define whisker_arr_dec_swap(T, n) \
// E_WHISKER_ARR whisker_arr_swap_##n(whisker_arr_##n *arr, size_t index_a, size_t index_b); \
//
// #define whisker_arr_impl_swap(T, n) \
// E_WHISKER_ARR whisker_arr_swap_##n(whisker_arr_##n *arr, size_t index_a, size_t index_b) \
// { \
// 	if (index_a < 0 || index_a > arr->length - 1 || index_b < 0 || index_b > arr->length - 1) { return E_WHISKER_ARR_OUT_OF_BOUNDS; } \
// 	arr->swap_buffer = arr->arr[index_a]; \
// 	arr->arr[index_a] = arr->arr[index_b]; \
// 	arr->arr[index_b] = arr->swap_buffer; \
// 	memset(&arr->swap_buffer, 0x00, sizeof(T)); \
// 	return E_WHISKER_ARR_OK; \
// } \
//
// #define whisker_arr_dec_reset(T, n) \
// void whisker_arr_reset_##n(whisker_arr_##n *arr, bool compact); \
//
// #define whisker_arr_impl_reset(T, n) \
// void whisker_arr_reset_##n(whisker_arr_##n *arr, bool compact) \
// { \
// 	arr->length = 0; \
// 	if (compact) { whisker_arr_resize_##n(arr, 0, false); } \
// } \
//
// #define whisker_arr_dec_compact(T, n) \
// E_WHISKER_ARR whisker_arr_compact_##n(whisker_arr_##n *arr); \
//
// #define whisker_arr_impl_compact(T, n) \
// E_WHISKER_ARR whisker_arr_compact_##n(whisker_arr_##n *arr) \
// { \
// 	return whisker_arr_resize_##n(arr, arr->length, false); \
// } \

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

// arrays of built in types
// whisker_arr_typedef(bool, bool_);
//
// whisker_arr(char);
// whisker_arr_typedef(unsigned char, unsigned_char);
// whisker_arr(short);
// whisker_arr_typedef(unsigned short, unsigned_short);
// whisker_arr(int);
// whisker_arr_typedef(unsigned int, unsigned_int);
// whisker_arr(long);
// whisker_arr_typedef(unsigned long, unsigned_long);
// whisker_arr_typedef(long long, long_long);
// whisker_arr_typedef(unsigned long long, unsigned_long_long);
//
// whisker_arr(float);
// whisker_arr(double);
// whisker_arr_typedef(long double, long_double);
//
// whisker_arr_typedef(void *, void_ptr)
// whisker_arr(uint8_t);
// whisker_arr(uint16_t);
// whisker_arr(uint32_t);
// whisker_arr(uint64_t);

#endif /* end of include guard WHISKER_ARRAY_H */
