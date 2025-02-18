/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_block_array
 * @created     : Monday Feb 17, 2025 14:25:25 CST
 */

#include "whisker_std.h"

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

typedef struct whisker_block_array
{
	size_t block_size;
	size_t block_count;
	size_t type_size;
	void **blocks;
} whisker_block_array;

// management functions
E_WHISKER_BLOCK_ARR whisker_block_arr_create_f(size_t type_size, size_t block_size, whisker_block_array **block_arr);
void whisker_block_arr_free(whisker_block_array *block_arr);
void whisker_block_arr_free_blocks(whisker_block_array *block_arr);
E_WHISKER_BLOCK_ARR whisker_block_arr_create_block(whisker_block_array *block_arr, size_t block_id);

// array functions
void* whisker_block_arr_get(whisker_block_array *block_arr, size_t index);
E_WHISKER_BLOCK_ARR whisker_block_arr_set(whisker_block_array *block_arr, size_t index, void *value);

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
#define wbarr_get_t whisker_block_arr_get_t
#define wbarr_set whisker_block_arr_set

#endif /* WHISKER_BLOCK_ARRAY_H */

