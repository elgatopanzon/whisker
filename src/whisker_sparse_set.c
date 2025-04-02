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
w_sparse_set *w_sparse_set_create()
{
	// allocate the ss struct
	w_sparse_set *ss_new = w_mem_xcalloc_t(1, *ss_new);

	return ss_new;
}

// allocate and init new sparse set
void w_sparse_set_init_f(w_sparse_set *ss, size_t element_size)
{
	// allocate sparse array
	w_array_init_t(ss->sparse, WHISKER_SPARSE_SET_SPARSE_BLOCK_SIZE);
	w_array_init_t(ss->sparse_index, WHISKER_SPARSE_SET_SPARSE_INDEX_BLOCK_SIZE);
	
	// allocate dense array
	ss->dense = w_mem_xcalloc(1, WHISKER_SPARSE_SET_DENSE_REALLOC_BLOCK_SIZE_MULTIPLIER);
	ss->dense_size = WHISKER_SPARSE_SET_DENSE_REALLOC_BLOCK_SIZE_MULTIPLIER;

	// allocate mutations array
	w_array_init_t(ss->mutations, WHISKER_SPARSE_SET_SPARSE_BLOCK_SIZE);

	ss->element_size = element_size;
	ss->swap_buffer = w_mem_xcalloc(1, element_size);

	// link the length to the sparse_index->length
	ss->length = &ss->sparse_index_length;
}

// allocate and init new sparse set
w_sparse_set *w_sparse_set_create_and_init_f(size_t element_size)
{
	// allocate the ss struct
	w_sparse_set *ss_new = w_sparse_set_create();
	w_sparse_set_init_f(ss_new, element_size);
	return ss_new;
}

// free a sparse set
void w_sparse_set_free_all(w_sparse_set *ss)
{
	w_sparse_set_free(ss);
	free(ss);
}

// free sparse set allocations
void w_sparse_set_free(w_sparse_set *ss)
{
	free(ss->sparse);
	free(ss->sparse_index);
	free(ss->dense);
	free(ss->mutations);
	free(ss->swap_buffer);
}

/***************************
*  operational functions  *
***************************/
// set an element in the sparse set with the given index
void w_sparse_set_set(w_sparse_set *ss, uint64_t index, void *value)
{
	// if the index exists already we can update it
	if (w_sparse_set_contains(ss, index))
	{
    	uint64_t dense_index = w_sparse_set_get_dense_index_(ss, index);
		memcpy(ss->dense + dense_index * ss->element_size, value, ss->element_size);

		return;
	}

	// if it doesn't exist, lets create it
    w_sparse_set_set_dense_index_(ss, index, ss->sparse_index_length);

	// increase size if dense array if new length exceeds current size
	if (ss->dense_size <= (ss->dense_length + 1) * ss->element_size)
	{
    	size_t new_size = ss->dense_size + WHISKER_SPARSE_SET_DENSE_REALLOC_BLOCK_SIZE_MULTIPLIER;
    	ss->dense = w_mem_xrecalloc(ss->dense, ss->dense_size, new_size);
    	ss->dense_size = new_size;
	}

	// set dense value
	memcpy(ss->dense + ss->dense_length * ss->element_size, value, ss->element_size);
	ss->dense_length++;

	// set sparse index
	w_array_ensure_alloc_block_size(
		ss->sparse_index, 
		(ss->sparse_index_length + 1),
		WHISKER_SPARSE_SET_SPARSE_INDEX_BLOCK_SIZE
	);
	ss->sparse_index[ss->sparse_index_length++] = index;

	// create a mutation
	w_sparse_set_mutation(ss, index, ss->sparse_index_length - 1, WHISKER_SPARSE_SET_MUTATION_TYPE_ADD);

	if (WHISKER_SPARSE_SET_AUTOSORT)
	{
    	w_sparse_set_sort(ss);
	}
}

// get an element from the sparse set by index, or NULL if it doesn't exist
void* w_sparse_set_get(w_sparse_set *ss, uint64_t index) {
    uint64_t dense_index = w_sparse_set_get_dense_index_(ss, index);

    if (dense_index == UINT64_MAX) {
    	return NULL;
    }

    return ss->dense + dense_index * ss->element_size;
}

// remove an element from the sparse set by index
void w_sparse_set_remove(w_sparse_set *ss, uint64_t index) {
    uint64_t dense_index = w_sparse_set_get_dense_index_(ss, index);
    if (dense_index == UINT64_MAX) return;


	// last into the dense and sparse_index array
    size_t last_index = ss->sparse_index_length - 1;
    uint64_t sparse_index_last = ss->sparse_index[last_index];
    uint64_t dense_index_last = w_sparse_set_get_dense_index_(ss, sparse_index_last);

	// swap end dense values and indexes
    size_t element_size = ss->element_size;
	ss->sparse_index[dense_index] = sparse_index_last;
    memcpy(ss->dense + dense_index * element_size, ss->dense + dense_index_last * element_size, element_size);
    w_sparse_set_set_dense_index_(ss, sparse_index_last, dense_index);

	// clear out old index
    w_sparse_set_set_dense_index_(ss, index, UINT64_MAX);
    ss->dense_length--;
    ss->sparse_index_length--;

	// create mutation
	w_sparse_set_mutation(ss, sparse_index_last, dense_index, WHISKER_SPARSE_SET_MUTATION_TYPE_ADD);
	w_sparse_set_mutation(ss, index, dense_index, WHISKER_SPARSE_SET_MUTATION_TYPE_REMOVE);

	if (WHISKER_SPARSE_SET_AUTOSORT)
	{
    	w_sparse_set_sort(ss);
	}
}

// check if the sparse set contains the index
bool w_sparse_set_contains(w_sparse_set *ss, uint64_t index)
{
    return ss->sparse_length >= index + 1 && ss->sparse[index] != UINT64_MAX && ss->sparse_index[ss->sparse[index]] == index;
}

// record a mutation to be processed later during a sort
void w_sparse_set_mutation(w_sparse_set *ss, uint64_t index_mutated, uint64_t sparse_index_mutated, enum W_SPARSE_SET_MUTATION_TYPE mutation_type)
{
	w_array_ensure_alloc_block_size(
		ss->mutations, 
		(ss->mutations_length + 1),
		WHISKER_SPARSE_SET_BLOCK_SIZE
	);
	size_t mutation_idx = ss->mutations_length++;
	ss->mutations[mutation_idx].mutated_index = index_mutated;
	ss->mutations[mutation_idx].mutated_sparse_index = sparse_index_mutated;
	ss->mutations[mutation_idx].mutation_type = mutation_type;
}


// set the dense index for the given index
void w_sparse_set_set_dense_index_(w_sparse_set *ss, uint64_t index, uint64_t dense_index)
{
    w_sparse_set_init_dense_index_(ss, index);
    ss->sparse[index] = dense_index;
    return;
}

// init the dense index for the given index
void w_sparse_set_init_dense_index_(w_sparse_set *ss, uint64_t index)
{
	if (ss->sparse_length < index + 1)
	{
		uint64_t sparse_length = ss->sparse_length;

		w_array_ensure_alloc_block_size(
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

	return;
}

// get the dense index for the given index
uint64_t w_sparse_set_get_dense_index_(w_sparse_set *ss, uint64_t index)
{
    w_sparse_set_init_dense_index_(ss, index);
    return ss->sparse[index];
}

// sort the sparse set by sparse index ascending
void w_sparse_set_sort(w_sparse_set *ss)
{
    uint64_t *dense_index = ss->sparse_index;
    void *dense = ss->dense;
    size_t n = ss->sparse_index_length;
    size_t element_size = ss->element_size;
    void* temp_dense = ss->swap_buffer;

    size_t start = 0;
    size_t end = n;

    if (n == 0)
    {
        return;
    }

    if (ss->mutations_length == 0)
    {
		return;
    }
    ss->mutations_length = 0;

    for (size_t i = start; i < end - 1; i++) {
        bool swapped = false;
        for (size_t j = 0; j < end - i - 1; j++) {
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
