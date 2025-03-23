/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_sparse_set
 * @created     : Tuesday Feb 25, 2025 18:32:11 CST
 */

#include "whisker_std.h"
#include "whisker_memory.h"
#include "whisker_array.h"
#include "whisker_trie.h"

#include "whisker_sparse_set.h"

/************************
*  management functions  *
************************/
// allocate a new sparse set
E_WHISKER_SS whisker_ss_create_f(whisker_sparse_set **ss, size_t element_size)
{
	// allocate the ss struct
	whisker_sparse_set *ss_new = whisker_mem_xcalloc_t(1, *ss_new);

	// allocate sparse array
	whisker_arr_init_t(ss_new->sparse, WHISKER_SPARSE_SET_SPARSE_BLOCK_SIZE);
	whisker_arr_init_t(ss_new->sparse_index, WHISKER_SPARSE_SET_SPARSE_INDEX_BLOCK_SIZE);

	// allocate dense array
	ss_new->dense = whisker_mem_xcalloc(1, WHISKER_SPARSE_SET_DENSE_REALLOC_BLOCK_SIZE_MULTIPLIER);
	ss_new->dense_size = WHISKER_SPARSE_SET_DENSE_REALLOC_BLOCK_SIZE_MULTIPLIER;

	// allocate root trie node
	ss_new->sparse_trie = whisker_mem_xcalloc_t(1, whisker_trie);

	ss_new->element_size = element_size;
	ss_new->swap_buffer = whisker_mem_xcalloc(1, element_size);

	// link the length to the sparse_index->length
	ss_new->length = &ss_new->sparse_index_length;

	*ss = ss_new;

	return E_WHISKER_SS_OK;
}

// free a sparse set
void whisker_ss_free(whisker_sparse_set *ss)
{
	free(ss->sparse);
	free(ss->sparse_index);
	free(ss->dense);
	whisker_trie_free_all(ss->sparse_trie);
	free(ss->swap_buffer);
	free(ss);
}

/***************************
*  operational functions  *
***************************/
// set an element in the sparse set with the given index
E_WHISKER_SS whisker_ss_set(whisker_sparse_set *ss, uint64_t index, void *value)
{
	// if the index exists already we can update it
	if (whisker_ss_contains(ss, index))
	{
    	uint64_t dense_index = whisker_ss_get_dense_index(ss, index);
		memcpy(ss->dense + dense_index * ss->element_size, value, ss->element_size);

		return E_WHISKER_SS_OK;
	}

	// if it doesn't exist, lets create it
    whisker_ss_set_dense_index(ss, index, ss->sparse_index_length);

	// increase size if dense array if new length exceeds current size
	if (ss->dense_size <= (ss->dense_length + 1) * ss->element_size)
	{
    	size_t new_size = ss->dense_size + WHISKER_SPARSE_SET_DENSE_REALLOC_BLOCK_SIZE_MULTIPLIER;
    	ss->dense = whisker_mem_xrecalloc(ss->dense, ss->dense_size, new_size);
    	ss->dense_size = new_size;
	}

	// set dense value
	memcpy(ss->dense + ss->dense_length * ss->element_size, value, ss->element_size);
	ss->dense_length++;

	// set sparse index
	whisker_arr_ensure_alloc_block_size(
		ss->sparse_index, 
		(ss->sparse_index_length + 1),
		WHISKER_SPARSE_SET_SPARSE_INDEX_BLOCK_SIZE
	);
	ss->sparse_index[ss->sparse_index_length++] = index;

	if (WHISKER_SPARSE_SET_AUTOSORT)
	{
    	whisker_ss_sort(ss);
	}

    return E_WHISKER_SS_OK;
}

// get an element from the sparse set by index, or NULL if it doesn't exist
void* whisker_ss_get(whisker_sparse_set *ss, uint64_t index) {
    uint64_t dense_index = whisker_ss_get_dense_index(ss, index);

    if (dense_index == UINT64_MAX) {
    	return NULL;
    }

    return ss->dense + dense_index * ss->element_size;
}

// remove an element from the sparse set by index
E_WHISKER_SS whisker_ss_remove(whisker_sparse_set *ss, uint64_t index) {
    uint64_t dense_index = whisker_ss_get_dense_index(ss, index);
    if (dense_index == UINT64_MAX) return E_WHISKER_SS_OK;


	// last into the dense and sparse_index array
    size_t last_index = ss->sparse_index_length - 1;
    uint64_t sparse_index_last = ss->sparse_index[last_index];
    uint64_t dense_index_last = whisker_ss_get_dense_index(ss, sparse_index_last);

	// swap end dense values and indexes
    size_t element_size = ss->element_size;
	ss->sparse_index[dense_index] = sparse_index_last;
    memcpy(ss->dense + dense_index * element_size, ss->dense + dense_index_last * element_size, element_size);
    E_WHISKER_SS err = whisker_ss_set_dense_index(ss, sparse_index_last, dense_index);
    if (err != E_WHISKER_SS_OK)
    {
    	return err;
    }

	// clear out old index
    err = whisker_ss_set_dense_index(ss, index, UINT64_MAX);
    if (err != E_WHISKER_SS_OK)
    {
    	return err;
    }
    ss->dense_length--;
    ss->sparse_index_length--;

	if (WHISKER_SPARSE_SET_AUTOSORT)
	{
    	whisker_ss_sort(ss);
	}

    return E_WHISKER_SS_OK;
}

// check if the sparse set contains the index
bool whisker_ss_contains(whisker_sparse_set *ss, uint64_t index)
{
	if (index > UINT_MAX)
	{
		return whisker_ss_get_dense_index(ss, index) != UINT64_MAX;
	}

    return ss->sparse_length >= index + 1 && ss->sparse[index] != UINT64_MAX && ss->sparse_index[ss->sparse[index]] == index;
}

// set the dense index for the given index
E_WHISKER_SS whisker_ss_set_dense_index(whisker_sparse_set *ss, uint64_t index, uint64_t dense_index)
{
    if (index > UINT_MAX) {
        whisker_trie *node = whisker_trie_search_node_(ss->sparse_trie, &index, sizeof(index), 0, false);
        if (node) {
            free(node->value);
        }
        uint64_t *dense_index_value = whisker_mem_xmalloc_t(*dense_index_value);
        if (dense_index_value) {
            *dense_index_value = dense_index;
            if (!whisker_trie_set_value(ss->sparse_trie, &index, sizeof(index), dense_index_value))
            {
            	return E_WHISKER_SS_ARR;
            }
        }
    } else {
    	E_WHISKER_SS init_err = whisker_ss_init_dense_index(ss, index);
    	if (init_err != E_WHISKER_SS_OK)
    	{
    		return init_err;
    	}
    	ss->sparse[index] = dense_index;
    }
    return E_WHISKER_SS_OK;
}

// init the dense index for the given index
E_WHISKER_SS whisker_ss_init_dense_index(whisker_sparse_set *ss, uint64_t index)
{
	if (ss->sparse_length < index + 1)
	{
		uint64_t sparse_length = ss->sparse_length;

		whisker_arr_ensure_alloc_block_size(
			ss->sparse, 
			(index + 1),
			WHISKER_SPARSE_SET_SPARSE_BLOCK_SIZE
		);
		ss->sparse_length = index + 1;

		for (int i = sparse_length; i < ss->sparse_length; ++i)
		{
			ss->sparse[i] = UINT64_MAX;
		}
	}

	return E_WHISKER_SS_OK;
}

// get the dense index for the given index
uint64_t whisker_ss_get_dense_index(whisker_sparse_set *ss, uint64_t index)
{
    if (index > UINT_MAX) {
        uint64_t *dense_index = whisker_trie_search_value_f(ss->sparse_trie, &index, sizeof(index));
        return dense_index ? *dense_index : UINT64_MAX;
    }

    E_WHISKER_SS err = whisker_ss_init_dense_index(ss, index);
    if (err != E_WHISKER_SS_OK)
    {
    	// TODO: panic here
    	// for now just return invalid ID if it fails
    	return UINT64_MAX;
    }

    return ss->sparse[index];
}

// sort the sparse set by sparse index ascending
void whisker_ss_sort(whisker_sparse_set *ss)
{
    uint64_t *dense_index = ss->sparse_index;
    void *dense = ss->dense;
    size_t n = ss->sparse_index_length;
    size_t element_size = ss->element_size;
    void* temp_dense = ss->swap_buffer;

    if (n == 0)
    {
        return;
    }

    for (size_t i = 0; i < n - 1; i++) {
        bool swapped = false;
        for (size_t j = 0; j < n - i - 1; j++) {
            if (dense_index[j] > dense_index[j + 1]) {
                uint64_t temp_index = dense_index[j];
                uint64_t temp_index2 = dense_index[j + 1];

                dense_index[j] = dense_index[j + 1];
                dense_index[j + 1] = temp_index;

    			ss->sparse[temp_index] = j + 1;
    			ss->sparse[temp_index2] = j;

                memcpy(temp_dense, (char *)dense + j * element_size, element_size);
                memcpy((char *)dense + j * element_size, (char *)dense + (j + 1) * element_size, element_size);
                memcpy((char *)dense + (j + 1) * element_size, temp_dense, element_size);

                swapped = true;
            }
        }
        if (!swapped) {
            break;
        }
    }
}
