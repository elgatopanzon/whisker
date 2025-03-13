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

// allocate and init a buffered sparse set instance
E_WHISKER_BSS whisker_bss_create_f(whisker_buffered_sparse_set **bss, size_t buffer_count, size_t element_size)
{
	// ensure min buffer count
	assert(buffer_count >= 2);

	whisker_buffered_sparse_set *bss_new;

	// allocate the bss struct
	E_WHISKER_MEM err = whisker_mem_try_calloc(1, sizeof(*bss_new), (void**)&bss_new);
	if (err != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_BSS_MEM;
	}

	// create empty array of sparse sets
	E_WHISKER_ARR ss_err = whisker_arr_create_whisker_sparse_set_ptr(&bss_new->sparse_sets, 0);
	if (ss_err != E_WHISKER_ARR_OK)
	{
		free(bss_new);
		return E_WHISKER_BSS_MEM;
	}

	// create sparse sets for each buffer
	for (int i = 0; i < buffer_count; ++i)
	{
		whisker_sparse_set *ss;
		E_WHISKER_SS b_err = whisker_ss_create_f(&ss, element_size);
		if (b_err != E_WHISKER_SS_OK)
		{
			// free any created sparse sets
			for (int bi = 0; bi < buffer_count - (i + 1); ++bi)
			{
				whisker_ss_free(bss_new->sparse_sets->arr[bi]);
			}

			whisker_arr_free_whisker_sparse_set_ptr(bss_new->sparse_sets);
			free(bss_new);

		}

		whisker_arr_push_whisker_sparse_set_ptr(bss_new->sparse_sets, ss);
	}

	// set sparse set front and back pointers
	bss_new->front_buffer = bss_new->sparse_sets->arr[0];
	bss_new->back_buffer = bss_new->sparse_sets->arr[1];

	bss_new->element_size = element_size;
	bss_new->buffer_count = buffer_count;

	*bss = bss_new;

	return E_WHISKER_BSS_OK;
}

// deallocate a buffered sparse set instance
void whisker_bss_free(whisker_buffered_sparse_set *bss)
{
	for (int bi = 0; bi < bss->buffer_count; ++bi)
	{
		whisker_ss_free(bss->sparse_sets->arr[bi]);
	}

	whisker_arr_free_whisker_sparse_set_ptr(bss->sparse_sets);
	free(bss);
}

/*************************
*  operation functions  *
*************************/
// set a value at the given index on the current back buffer
E_WHISKER_SS whisker_bss_set(whisker_buffered_sparse_set *bss, uint64_t index, void *value)
{
	return whisker_ss_set(bss->back_buffer, index, value);
}

// get a value at the given index on the current front buffer
void* whisker_bss_get(whisker_buffered_sparse_set *bss, uint64_t index)
{
	return whisker_ss_get(bss->front_buffer, index);
}

// remove a value at the given index from the current back buffer
E_WHISKER_SS whisker_bss_remove(whisker_buffered_sparse_set *bss, uint64_t index)
{
	return whisker_ss_remove(bss->back_buffer, index);
}

// check if the index exists in the current front buffer
bool whisker_bss_contains(whisker_buffered_sparse_set *bss, uint64_t index)
{
	return whisker_ss_contains(bss->front_buffer, index);
}

// sort sparse sets using the provided sort mode
void whisker_bss_sort(whisker_buffered_sparse_set *bss, WHISKER_BSS_SORT_MODE sort_mode)
{
	switch (sort_mode) {
		case WHISKER_BSS_SORT_MODE_ALL:
			for (size_t i = 0; i < bss->buffer_count; ++i)
			{
				whisker_ss_sort(bss->sparse_sets->arr[i]);
			}		
			break;
		case WHISKER_BSS_SORT_MODE_FRONT:
			whisker_ss_sort(bss->front_buffer);
			break;
		case WHISKER_BSS_SORT_MODE_BACK:
			whisker_ss_sort(bss->back_buffer);
			break;
		case WHISKER_BSS_SORT_MODE_FRONT_BACK:
			whisker_ss_sort(bss->front_buffer);
			whisker_ss_sort(bss->back_buffer);
			break;
		default:
	}
}


/***********************
*  buffer operations  *
***********************/

// sync any changes from the back buffer to the front
E_WHISKER_BSS whisker_bss_sync(whisker_buffered_sparse_set *bss)
{

}

// swap the current buffers by 1 position
// note: with buffer size 2, back becomes the front, and front becomes the back
E_WHISKER_BSS whisker_bss_swap(whisker_buffered_sparse_set *bss)
{
	for (int i = 0; i < bss->buffer_count - 1; ++i)
	{
		bss->sparse_sets->arr[i] = bss->sparse_sets->arr[i+1];
	}
	bss->sparse_sets->arr[bss->buffer_count - 1] = bss->front_buffer;

	bss->front_buffer = bss->sparse_sets->arr[0];
	bss->back_buffer = bss->sparse_sets->arr[1];

	return E_WHISKER_BSS_OK;
}

// sync changes between the back and front, then swap the buffers
E_WHISKER_BSS whisker_bss_sync_and_swap(whisker_buffered_sparse_set *bss)
{
	E_WHISKER_BSS err = whisker_bss_sync(bss);
	if (err != E_WHISKER_BSS_OK)
	{
		return err;
	}

	err = whisker_bss_swap(bss);
	if (err != E_WHISKER_BSS_OK)
	{
		return err;
	}

	return E_WHISKER_BSS_OK;
}
