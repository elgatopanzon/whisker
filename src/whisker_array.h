/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_array
 * @created     : Wednesday Feb 05, 2025 12:38:49 CST
 * @description : dynamic array declaration and bounds-checked reallocation macros
 */

#include "whisker_std.h"
#include "whisker_debug.h"
#include "whisker_memory.h"

#ifndef WHISKER_ARRAY_H
#define WHISKER_ARRAY_H

// helper macros
#define w_array_declare(t, name) \
	t *name; _Atomic size_t name##_size; _Atomic size_t name##_length;

#define w_array_declare_struct(t, name) \
	struct name { t *arr; _Atomic size_t arr_size; _Atomic size_t arr_length; };

#define w_array_init_t(name, count) \
	name = w_mem_xcalloc_t((count), *name); \
	name##_size = (count) * sizeof(*name); \

#define w_array_realloc(name, length) \
	name = w_mem_xrecalloc(name, name##_size, (length) * sizeof(*name)); \
	name##_size = (length) * sizeof(*name); \
	if ((length) < name##_length) { name##_length = (length); } \

#define w_array_ensure_alloc(arr, length) \
	if (arr##_size < ((length) * sizeof(*arr))) { w_array_realloc(arr, (length)); } \

#define w_array_ensure_alloc_block_size(arr, length, block_size) \
	do { \
		size_t adjusted_length = ((size_t) (((length) / block_size) + 1)) * block_size; \
		w_array_ensure_alloc(arr, adjusted_length); \
	} while (0)

#endif // end of include guard WHISKER_ARRAY_H
