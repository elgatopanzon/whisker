/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_buffered_sparse_set
 * @created     : Wednesday Mar 12, 2025 11:08:56 CST
 */

#include "whisker_std.h"
#include "whisker_sparse_set.h"
#include "whisker_array.h"

/* whisker_buffered_sparse_set
 * this data structure holds multiple sparse set buffers offering up functions
 * to perform double/triple buffering on the underlying sets
 *
 * the set and get functions act just like the normal sparse set, the only
 * difference is that they are operating on the current front and back buffer
 * sets
 */

#ifndef WHISKER_BUFFERED_SPARSE_SET_H
#define WHISKER_BUFFERED_SPARSE_SET_H

typedef enum WHISKER_BSS_SORT_MODE { 
	WHISKER_BSS_SORT_MODE_ALL,
	WHISKER_BSS_SORT_MODE_FRONT,
	WHISKER_BSS_SORT_MODE_BACK,
	WHISKER_BSS_SORT_MODE_FRONT_BACK,
} WHISKER_BSS_SORT_MODE;

typedef struct whisker_buffered_sparse_set
{
	whisker_arr_declare(whisker_sparse_set *, sparse_sets);
	whisker_sparse_set *front_buffer;
	whisker_sparse_set *back_buffer;
	size_t buffer_count;
	size_t element_size;
} whisker_buffered_sparse_set;

#define whisker_bss_create_t(b, t) whisker_bss_create_f(b, sizeof(t))
#define whisker_bss_create_s(b, s) whisker_bss_create_f(b, s)
#define whisker_bss_create_and_init_t(b, t) whisker_bss_create_and_init_f(b, sizeof(t))
#define whisker_bss_create_and_init_s(b, s) whisker_bss_create_and_init_f(b, s)

// short macros
#define wbss_create_t whisker_bss_create_t
#define wbss_create_s whisker_bss_create_s
#define wbss_free whisker_bss_free
#define wbss_set whisker_bss_set
#define wbss_get whisker_bss_get
#define wbss_remove whisker_bss_remove
#define wbss_contains whisker_bss_contains

// management functions
whisker_buffered_sparse_set *whisker_bss_create_f();
void whisker_bss_init_f(whisker_buffered_sparse_set *bss, size_t buffer_count, size_t element_size);
whisker_buffered_sparse_set *whisker_bss_create_and_init_f(size_t buffer_count, size_t element_size);
void whisker_bss_free(whisker_buffered_sparse_set *bss);
void whisker_bss_free_all(whisker_buffered_sparse_set *bss);

// operation functions
void whisker_bss_set(whisker_buffered_sparse_set *bss, uint64_t index, void *value);
void* whisker_bss_get(whisker_buffered_sparse_set *bss, uint64_t index);
void whisker_bss_remove(whisker_buffered_sparse_set *bss, uint64_t index);
bool whisker_bss_contains(whisker_buffered_sparse_set *bss, uint64_t index);

void whisker_bss_sort(whisker_buffered_sparse_set *bss, WHISKER_BSS_SORT_MODE sort_mode);

// buffer operations
void whisker_bss_sync(whisker_buffered_sparse_set *bss);
void whisker_bss_swap(whisker_buffered_sparse_set *bss);
void whisker_bss_sync_and_swap(whisker_buffered_sparse_set *bss);

#endif /* WHISKER_BUFFERED_SPARSE_SET_H */

