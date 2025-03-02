/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_block_array
 * @created     : Monday Feb 17, 2025 14:25:25 CST
 */

#include <stdint.h>
#include "whisker_std.h"
#include "whisker_array.h"
#include "generics/whisker_generic_array_char.h"
#include "generics/whisker_generic_array_unsigned_char.h"
#include "generics/whisker_generic_array_short.h"
#include "generics/whisker_generic_array_unsigned_short.h"
#include "generics/whisker_generic_array_int.h"
#include "generics/whisker_generic_array_unsigned_int.h"
#include "generics/whisker_generic_array_long.h"
#include "generics/whisker_generic_array_unsigned_long.h"
#include "generics/whisker_generic_array_long_long.h"
#include "generics/whisker_generic_array_unsigned_long_long.h"
#include "generics/whisker_generic_array_float.h"
#include "generics/whisker_generic_array_double.h"
#include "generics/whisker_generic_array_long_double.h"
#include "generics/whisker_generic_array_void_.h"
#include "generics/whisker_generic_array_uint8_t.h"
#include "generics/whisker_generic_array_uint16_t.h"
#include "generics/whisker_generic_array_uint32_t.h"
#include "generics/whisker_generic_array_uint64_t.h"

#ifndef WHISKER_BLOCK_ARRAY_H
#define WHISKER_BLOCK_ARRAY_H

// errors
typedef enum E_WHISKER_BLOCK_ARR  
{
	E_WHISKER_BLOCK_ARR_OK = 0,
	E_WHISKER_BLOCK_ARR_UNKNOWN = 1,
	E_WHISKER_BLOCK_ARR_MEM = 2,
} E_WHISKER_BLOCK_ARR;
extern const char* E_WHISKER_ARR_BLOCK_STR[];

/////////////////////////////
//  macro implementations  //
/////////////////////////////
#define whisker_block_arr(T) whisker_block_arr_typedef(T, T)
#define whisker_block_arr_typedef(T, n) \
	whisker_block_arr_dec_struct(T, n) \
	whisker_block_arr_dec_create(T, n) \
	whisker_block_arr_dec_free(T, n) \
	whisker_block_arr_dec_set(T, n) \
	whisker_block_arr_dec_init_block(T, n) \
	whisker_block_arr_dec_get(T, n) \
	whisker_block_arr_dec_contains(T, n) \

#define whisker_block_arr_impl(T) whisker_block_arr_impl_typedef(T, T)
#define whisker_block_arr_impl_typedef(T, n) \
	whisker_block_arr_impl_create(T, n) \
	whisker_block_arr_impl_free(T, n) \
	whisker_block_arr_impl_set(T, n) \
	whisker_block_arr_impl_init_block(T, n) \
	whisker_block_arr_impl_get(T, n) \
	whisker_block_arr_impl_contains(T, n) \

#define whisker_block_arr_dec_struct(T, n) \
typedef struct whisker_block_arr_##n \
{ \
	whisker_arr_##n *blocks; \
	size_t block_size; \
} whisker_block_arr_##n; \

#define whisker_block_arr_dec_create(T, n) \
E_WHISKER_BLOCK_ARR whisker_block_arr_create_##n(whisker_block_arr_##n **arr, size_t block_size); \

#define whisker_block_arr_impl_create(T, n) \
E_WHISKER_BLOCK_ARR whisker_block_arr_create_##n(whisker_block_arr_##n **barr, size_t block_size) \
{ \
	whisker_block_arr_##n *ba; \
	E_WHISKER_MEM err = whisker_mem_try_calloc(1, sizeof(whisker_block_arr_##n), (void**)&ba); \
	if (err != E_WHISKER_MEM_OK) { return E_WHISKER_BLOCK_ARR_MEM; } \
	whisker_arr_##n *blocks; \
	whisker_arr_create_##n(&blocks, 0); \
	ba->blocks = blocks; \
	ba->block_size = block_size; \
	*barr = ba; \
	return E_WHISKER_BLOCK_ARR_OK; \
} \

#define whisker_block_arr_dec_free(T, n) \
void whisker_block_arr_free_##n(whisker_block_arr_##n *barr); \

#define whisker_block_arr_impl_free(T, n) \
void whisker_block_arr_free_##n(whisker_block_arr_##n *barr) \
{ \
	for (size_t i = 0; i < barr->blocks->length; ++i) \
	{ \
		if (barr->blocks->arr[i].arr != NULL) { free(barr->blocks->arr[i].arr); } \
	} \
	whisker_arr_free_##n(barr->blocks); \
	free(barr); \
} \

#define whisker_block_arr_dec_set(T, n) \
E_WHISKER_BLOCK_ARR whisker_block_arr_set_##n(whisker_block_arr_##n *barr, size_t index, T value); \

#define whisker_block_arr_impl_set(T, n) \
E_WHISKER_BLOCK_ARR whisker_block_arr_set_##n(whisker_block_arr_##n *barr, size_t index, T value) \
{ \
	size_t block_id = whisker_block_arr_get_block_id(barr->block_size, index); \
	size_t block_offset = whisker_block_arr_get_block_offset(barr->block_size, index); \
	if (barr->blocks->length < index + 1 || barr->blocks->arr[block_id].arr == NULL) { \
		E_WHISKER_BLOCK_ARR err = whisker_block_arr_init_block_##n(barr, block_id); \
		if (err != E_WHISKER_BLOCK_ARR_OK) { return err; } \
	} \
	barr->blocks->arr[block_id].arr[block_offset] = value; \
	return E_WHISKER_BLOCK_ARR_OK; \
} \

#define whisker_block_arr_dec_init_block(T, n) \
E_WHISKER_BLOCK_ARR whisker_block_arr_init_block_##n(whisker_block_arr_##n *barr, size_t block_id); \

#define whisker_block_arr_impl_init_block(T, n) \
E_WHISKER_BLOCK_ARR whisker_block_arr_init_block_##n(whisker_block_arr_##n *barr, size_t block_id) \
{ \
	if (barr->blocks->length < index + 1) { \
		E_WHISKER_ARR err = whisker_arr_resize_##n(barr->blocks, block_id + 1, true); \
		if (err != E_WHISKER_ARR_OK) { return E_WHISKER_BLOCK_ARR_MEM; } \
	} \
	if (barr->blocks->arr[block_id].arr == NULL) { \
		whisker_arr_##n *block; \
		E_WHISKER_ARR err = whisker_arr_create_##n(&block, barr->block_size); \
		if (err != E_WHISKER_ARR_OK) { return E_WHISKER_BLOCK_ARR_MEM; } \
		barr->blocks->arr[block_id] = *block; \
		free(block); \
	} \
	return E_WHISKER_BLOCK_ARR_OK; \
} \

#define whisker_block_arr_dec_get(T, n) \
T whisker_block_arr_get_##n(whisker_block_arr_##n *barr, size_t index); \

#define whisker_block_arr_impl_get(T, n) \
T whisker_block_arr_get_##n(whisker_block_arr_##n *barr, size_t index) \
{ \
	size_t block_id = whisker_block_arr_get_block_id(barr->block_size, index); \
	size_t block_offset = whisker_block_arr_get_block_offset(barr->block_size, index); \
	return barr->blocks->arr[block_id].arr[block_offset]; \
} \

#define whisker_block_arr_dec_contains(T, n) \
T whisker_block_arr_contains_##n(whisker_block_arr_##n *barr, size_t index); \

#define whisker_block_arr_impl_contains(T, n) \
T whisker_block_arr_contains_##n(whisker_block_arr_##n *barr, size_t index) \
{ \
	size_t block_id = whisker_block_arr_get_block_id(barr->block_size, index); \
	size_t block_offset = whisker_block_arr_get_block_offset(barr->block_size, index); \
	return barr->blocks->length > block_id + 1 && barr->blocks->arr[block_id].arr != NULL; \
} \

typedef struct whisker_block_array
{
	size_t block_size;
	size_t block_count;
	size_t type_size;
	void **blocks;
	size_t blocks_length;
} whisker_block_array;

// management functions
E_WHISKER_BLOCK_ARR whisker_block_arr_create_f(size_t type_size, size_t block_size, whisker_block_array **block_arr);
void whisker_block_arr_free(whisker_block_array *block_arr);
void whisker_block_arr_free_blocks(whisker_block_array *block_arr);
E_WHISKER_BLOCK_ARR whisker_block_arr_create_block(whisker_block_array *block_arr, size_t block_id);

// array functions
void* whisker_block_arr_get(whisker_block_array *block_arr, size_t index);
void* whisker_block_arr_get_and_fill(whisker_block_array *block_arr, size_t index, char fill_with);
E_WHISKER_BLOCK_ARR whisker_block_arr_set(whisker_block_array *block_arr, size_t index, void *value);
E_WHISKER_BLOCK_ARR whisker_block_arr_set_with(whisker_block_array *block_arr, size_t index, char set_with);

// utility functions
size_t whisker_block_arr_get_block_id(size_t block_size, size_t index);
size_t whisker_block_arr_get_block_offset(size_t block_size, size_t index);

// macros
#define whisker_block_arr_create(t, s, p) whisker_block_arr_create_f(sizeof(t), s, p)
#define whisker_block_arr_get_t(p, i, t) (t*) whisker_block_arr_get(p, i)
#define whisker_block_arr_set(p, i, v) whisker_block_arr_set(p, i, v)

// short macros
#define wbarr_create whisker_block_arr_create
#define wbarr_create_block whisker_block_arr_create_block
#define wbarr_free whisker_block_arr_free
#define wbarr_free_blocks whisker_block_arr_free_blocks
#define wbarr_get whisker_block_arr_get
#define wbarr_get_and_fill whisker_block_arr_get_and_fill
#define wbarr_get_t whisker_block_arr_get_t
#define wbarr_set whisker_block_arr_set
#define wbarr_set_with whisker_block_arr_set_with

// whisker_block_arr(char);
// whisker_block_arr_typedef(unsigned char, unsigned_char);
// whisker_block_arr(short);
// whisker_block_arr_typedef(unsigned short, unsigned_short);
// whisker_block_arr(int);
// whisker_block_arr_typedef(unsigned int, unsigned_int);
// whisker_block_arr(long);
// whisker_block_arr_typedef(unsigned long, unsigned_long);
// whisker_block_arr_typedef(long long, long_long);
// whisker_block_arr_typedef(unsigned long long, unsigned_long_long);
//
// whisker_block_arr(float);
// whisker_block_arr(double);
// whisker_block_arr_typedef(long double, long_double);
//
// // whisker_block_arr_typedef(void *, void_ptr)
// whisker_block_arr(uint8_t);
// whisker_block_arr(uint16_t);
// whisker_block_arr(uint32_t);
// whisker_block_arr(uint64_t);

#endif /* WHISKER_BLOCK_ARRAY_H */

