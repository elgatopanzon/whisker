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

typedef enum W_BUF_SPARSE_SET_SORT_MODE { 
	W_BUF_SPARSE_SET_SORT_MODE_ALL,
	W_BUF_SPARSE_SET_SORT_MODE_FRONT,
	W_BUF_SPARSE_SET_SORT_MODE_BACK,
	W_BUF_SPARSE_SET_SORT_MODE_FRONT_BACK,
} W_BUF_SPARSE_SET_SORT_MODE;

typedef struct w_buffered_sparse_set
{
	w_array_declare(w_sparse_set *, sparse_sets);
	w_sparse_set *front_buffer;
	w_sparse_set *back_buffer;
	size_t buffer_count;
	size_t element_size;
} w_buffered_sparse_set;

#define w_buf_sparse_set_create_t(b, t) w_buf_sparse_set_create_f(b, sizeof(t))
#define w_buf_sparse_set_create_s(b, s) w_buf_sparse_set_create_f(b, s)
#define w_buf_sparse_set_create_and_init_t(b, t) w_buf_sparse_set_create_and_init_f(b, sizeof(t))
#define w_buf_sparse_set_create_and_init_s(b, s) w_buf_sparse_set_create_and_init_f(b, s)

// management functions
w_buffered_sparse_set *w_buf_sparse_set_create_f();
void w_buf_sparse_set_init_f(w_buffered_sparse_set *bss, size_t buffer_count, size_t element_size);
w_buffered_sparse_set *w_buf_sparse_set_create_and_init_f(size_t buffer_count, size_t element_size);
void w_buf_sparse_set_free(w_buffered_sparse_set *bss);
void w_buf_sparse_set_free_all(w_buffered_sparse_set *bss);

// operation functions
void w_buf_sparse_set_set(w_buffered_sparse_set *bss, uint64_t index, void *value);
void* w_buf_sparse_set_get(w_buffered_sparse_set *bss, uint64_t index);
void w_buf_sparse_set_remove(w_buffered_sparse_set *bss, uint64_t index);
bool w_buf_sparse_set_contains(w_buffered_sparse_set *bss, uint64_t index);

void w_buf_sparse_set_sort(w_buffered_sparse_set *bss, W_BUF_SPARSE_SET_SORT_MODE sort_mode);

// buffer operations
void w_buf_sparse_set_sync(w_buffered_sparse_set *bss);
void w_buf_sparse_set_swap(w_buffered_sparse_set *bss);
void w_buf_sparse_set_sync_and_swap(w_buffered_sparse_set *bss);

#endif /* WHISKER_BUFFERED_SPARSE_SET_H */

