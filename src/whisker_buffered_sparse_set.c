/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_buffered_sparse_set
 * @created     : Wednesday Mar 12, 2025 11:09:01 CST
 */

#include "whisker_std.h"

#include "whisker_buffered_sparse_set.h"

/**************************
*  management functions  *
**************************/

// allocate a buffered sparse set instance
w_buffered_sparse_set *w_buf_sparse_set_create_f()
{
	// allocate the bss struct
	w_buffered_sparse_set *bss_new = w_mem_xcalloc_t(1, *bss_new);

	return bss_new;
}

// allocate and init a buffered sparse set instance
w_buffered_sparse_set *w_buf_sparse_set_create_and_init_f(size_t buffer_count, size_t element_size)
{
	w_buffered_sparse_set *bss_new = w_buf_sparse_set_create_f();
	w_buf_sparse_set_init_f(bss_new, buffer_count, element_size);

	return bss_new;
}

// init a buffered sparse set instance
void w_buf_sparse_set_init_f(w_buffered_sparse_set *bss, size_t buffer_count, size_t element_size)
{
	// ensure min buffer count
	assert(buffer_count >= 2);

	// create array of sparse sets
	w_array_init_t(bss->sparse_sets, buffer_count);

	// create sparse sets for each buffer
	for (int i = 0; i < buffer_count; ++i)
	{
		w_sparse_set *ss = w_sparse_set_create_and_init_f(element_size);
		bss->sparse_sets[bss->sparse_sets_length++] = ss;
	}

	// set sparse set front and back pointers
	bss->front_buffer = bss->sparse_sets[0];
	bss->back_buffer = bss->sparse_sets[1];

	bss->element_size = element_size;
	bss->buffer_count = buffer_count;
}

// deallocate sparse sets in buffered sparse set instance
void w_buf_sparse_set_free(w_buffered_sparse_set *bss)
{
	for (int bi = 0; bi < bss->buffer_count; ++bi)
	{
		w_sparse_set_free_all(bss->sparse_sets[bi]);
	}

	free(bss->sparse_sets);
}

// deallocate a buffered sparse set instance and free sparse sets
void w_buf_sparse_set_free_all(w_buffered_sparse_set *bss)
{
	w_buf_sparse_set_free(bss);
	free(bss);
}

/*************************
*  operation functions  *
*************************/
// set a value at the given index on the current back buffer
void w_buf_sparse_set_set(w_buffered_sparse_set *bss, uint64_t index, void *value)
{
	w_sparse_set_set(bss->back_buffer, index, value);
}

// get a value at the given index on the current front buffer
void* w_buf_sparse_set_get(w_buffered_sparse_set *bss, uint64_t index)
{
	return w_sparse_set_get(bss->front_buffer, index);
}

// remove a value at the given index from the current back buffer
void w_buf_sparse_set_remove(w_buffered_sparse_set *bss, uint64_t index)
{
	w_sparse_set_remove(bss->back_buffer, index);
}

// check if the index exists in the current front buffer
bool w_buf_sparse_set_contains(w_buffered_sparse_set *bss, uint64_t index)
{
	return w_sparse_set_contains(bss->front_buffer, index);
}

// sort sparse sets using the provided sort mode
void w_buf_sparse_set_sort(w_buffered_sparse_set *bss, W_BUF_SPARSE_SET_SORT_MODE sort_mode)
{
	switch (sort_mode) {
		case W_BUF_SPARSE_SET_SORT_MODE_ALL:
			for (size_t i = 0; i < bss->buffer_count; ++i)
			{
				w_sparse_set_sort(bss->sparse_sets[i]);
			}		
			break;
		case W_BUF_SPARSE_SET_SORT_MODE_FRONT:
			w_sparse_set_sort(bss->front_buffer);
			break;
		case W_BUF_SPARSE_SET_SORT_MODE_BACK:
			w_sparse_set_sort(bss->back_buffer);
			break;
		case W_BUF_SPARSE_SET_SORT_MODE_FRONT_BACK:
			w_sparse_set_sort(bss->front_buffer);
			w_sparse_set_sort(bss->back_buffer);
			break;
		default:
	}
}


/***********************
*  buffer operations  *
***********************/

// sync any changes from the back buffer to the front
void w_buf_sparse_set_sync(w_buffered_sparse_set *bss)
{

}

// swap the current buffers by 1 position
// note: with buffer size 2, back becomes the front, and front becomes the back
void w_buf_sparse_set_swap(w_buffered_sparse_set *bss)
{
	for (int i = 0; i < bss->buffer_count - 1; ++i)
	{
		bss->sparse_sets[i] = bss->sparse_sets[i+1];
	}
	bss->sparse_sets[bss->buffer_count - 1] = bss->front_buffer;

	bss->front_buffer = bss->sparse_sets[0];
	bss->back_buffer = bss->sparse_sets[1];
}

// sync changes between the back and front, then swap the buffers
void w_buf_sparse_set_sync_and_swap(w_buffered_sparse_set *bss)
{
	w_buf_sparse_set_sync(bss);
	w_buf_sparse_set_swap(bss);
}
