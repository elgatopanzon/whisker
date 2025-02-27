/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_sparse_set
 * @created     : Tuesday Feb 25, 2025 18:32:11 CST
 */

#include "whisker_std.h"
#include "whisker_memory.h"
/* #include "whisker_block_array.h" */
#include "whisker_array.h"
#include "whisker_trie.h"

#include "whisker_sparse_set.h"

/************************
*  management functions  *
************************/
// allocate a new sparse set
E_WHISKER_SS whisker_ss_create_f(whisker_sparse_set **ss, size_t element_size)
{
	whisker_sparse_set *ss_new;

	// allocate the ss struct
	E_WHISKER_MEM err = whisker_mem_try_malloc(sizeof(*ss_new), (void**)&ss_new);
	if (err != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_SS_MEM;
	}

	// allocate sparse block array
	E_WHISKER_ARR arr_err = whisker_arr_create_f(sizeof(uint64_t), 0, (void**)&ss_new->sparse);
	if (arr_err != E_WHISKER_ARR_OK)
	{
		free(ss_new);

		return E_WHISKER_SS_ARR;
	}

	E_WHISKER_ARR arr_err2 = whisker_arr_create_f(element_size, 0, (void**)&ss_new->dense);
	if (arr_err2 != E_WHISKER_ARR_OK)
	{
		free(ss_new);
		warr_free(ss_new->sparse);

		return E_WHISKER_SS_ARR;
	}

	E_WHISKER_ARR arr_err3 = whisker_arr_create_f(sizeof(uint64_t), 0, (void**)&ss_new->sparse_index);
	if (arr_err3 != E_WHISKER_ARR_OK)
	{
		free(ss_new);
		warr_free(ss_new->sparse);
		warr_free(ss_new->dense);

		return E_WHISKER_SS_ARR;
	}

	E_WHISKER_TRIE trie_err = whisker_trie_create_node(&ss_new->sparse_trie);
	if (trie_err != E_WHISKER_TRIE_OK)
	{
		free(ss_new);
		warr_free(ss_new->sparse);
		warr_free(ss_new->dense);
		warr_free(ss_new->sparse_index);
	}

	ss_new->length = 0;
	ss_new->sparse_length = 0;
	ss_new->element_size = element_size;
	ss_new->swap_buffer = warr_header(ss_new->dense)->swap_buffer;

	*ss = ss_new;

	return E_WHISKER_SS_OK;
}

// free a sparse set
void whisker_ss_free(whisker_sparse_set *ss)
{
	warr_free(ss->sparse);
	warr_free(ss->dense);
	warr_free(ss->sparse_index);
	wtrie_free_node(ss->sparse_trie, true);
	free(ss);
}

/***************************
*  operational functions  *
***************************/
// set an element in the sparse set with the given index
E_WHISKER_SS whisker_ss_set(whisker_sparse_set *ss, uint64_t index, void *value)
{
    uint64_t dense_index = whisker_ss_get_dense_index(ss, index);

    if (dense_index == UINT64_MAX) {
    	E_WHISKER_SS set_err = whisker_ss_set_dense_index(ss, index, ss->length);

    	if (set_err != E_WHISKER_SS_OK)
    	{
        	return E_WHISKER_SS_ARR;
    	}

    	E_WHISKER_ARR push_err = warr_push(&ss->dense, value);
    	if (push_err != E_WHISKER_ARR_OK)
    	{
        	return E_WHISKER_SS_ARR;
    	}

    	push_err = warr_push(&ss->sparse_index, &index);
    	if (push_err != E_WHISKER_ARR_OK)
    	{
        	return E_WHISKER_SS_ARR;
    	}

		ss->length = warr_length(ss->dense);

		if (WHISKER_SPARSE_SET_AUTOSORT)
		{
    		whisker_ss_sort(ss);
		}
    }
    else
    {
		memcpy(ss->dense + dense_index * ss->element_size, value, ss->element_size);
    }

    return E_WHISKER_SS_OK;
}

void* whisker_ss_get(whisker_sparse_set *ss, uint64_t index, bool create) {
    uint64_t dense_index = whisker_ss_get_dense_index(ss, index);

    if (dense_index == UINT64_MAX) {
        if (!create) return NULL;
        whisker_ss_set(ss, index, ss->swap_buffer);
    	dense_index = whisker_ss_get_dense_index(ss, index);
    }

    return ss->dense + dense_index * ss->element_size;
}

E_WHISKER_SS whisker_ss_remove(whisker_sparse_set *ss, uint64_t index) {
    uint64_t dense_index = whisker_ss_get_dense_index(ss, index);
    if (dense_index == UINT64_MAX) return E_WHISKER_SS_OK;


	// last into the dense and sparse_index array
    size_t last_index = ss->length - 1;
    uint64_t sparse_index_last = ss->sparse_index[last_index];
    uint64_t dense_index_last = whisker_ss_get_dense_index(ss, sparse_index_last);

    /* debug_printf("sparse index remove: %zu\n", index); */
    /* debug_printf("dense index remove: %zu\n", dense_index); */
    /* debug_printf("sparse index last: %zu\n", sparse_index_last); */
    /* debug_printf("dense index last: %zu\n", dense_index_last); */

	// swap end dense values and indexes
    size_t element_size = ss->element_size;
	ss->sparse_index[dense_index] = sparse_index_last;
    memcpy(ss->dense + dense_index * element_size, ss->dense + dense_index_last * element_size, element_size);
    whisker_ss_set_dense_index(ss, sparse_index_last, dense_index);

	// clear out old index
    whisker_ss_set_dense_index(ss, index, UINT64_MAX);
    warr_header(ss->dense)->length--;
    warr_header(ss->sparse_index)->length--;
    ss->length--;

	if (WHISKER_SPARSE_SET_AUTOSORT)
	{
    	whisker_ss_sort(ss);
	}

    return E_WHISKER_SS_OK;
}

bool whisker_ss_contains(whisker_sparse_set *ss, uint64_t index)
{
    return (whisker_ss_get_dense_index(ss, index) != UINT64_MAX);
}

E_WHISKER_SS whisker_ss_set_dense_index(whisker_sparse_set *ss, uint64_t index, uint64_t dense_index)
{
    if (index > UINT_MAX) {
        whisker_trie *node;
        whisker_trie_search_node_(ss->sparse_trie, &index, sizeof(index), 0, false, &node);
        if (node) {
            free(node->value);
        }
        uint64_t *dense_index_value = malloc(sizeof(uint64_t));
        if (dense_index_value) {
            *dense_index_value = dense_index;
            whisker_trie_set_value(&ss->sparse_trie, &index, sizeof(index), dense_index_value);
        }
    } else {
    	whisker_ss_init_dense_index(ss, index);
    	ss->sparse[index] = dense_index;
    }
    return E_WHISKER_SS_OK;
}

E_WHISKER_SS whisker_ss_init_dense_index(whisker_sparse_set *ss, uint64_t index)
{
	if (ss->sparse_length < index + 1)
	{
		E_WHISKER_ARR err = warr_resize(&ss->sparse, index + 1);
		if (err != E_WHISKER_ARR_OK)
		{
			return E_WHISKER_SS_MEM;
		}

		memset(ss->sparse + ss->sparse_length, 0xFF, (index + 1 - ss->sparse_length) * sizeof(ss->sparse[0]));

		ss->sparse_length = index + 1;
	}

	return E_WHISKER_SS_OK;
}

uint64_t whisker_ss_get_dense_index(whisker_sparse_set *ss, uint64_t index)
{
    if (index > UINT_MAX) {
        uint64_t *dense_index = NULL;
        whisker_trie_search_value_f(ss->sparse_trie, &index, sizeof(index), (void**)&dense_index);
        return dense_index ? *dense_index : UINT64_MAX;
    }

    whisker_ss_init_dense_index(ss, index);

    return ss->sparse[index];
}

void whisker_ss_sort(whisker_sparse_set *ss)
{
    uint64_t *dense_index = ss->sparse_index;
    void *dense = ss->dense;
    size_t n = ss->length;
    size_t element_size = ss->element_size;
    void* temp_dense = ss->swap_buffer;

	for (size_t i = 0; i < n - 1; i++) {
    	for (size_t j = 0; j < n - i - 1; j++) {
        	if (dense_index[j] > dense_index[j + 1]) {
            	uint64_t temp_index = dense_index[j];
            	uint64_t temp_index2 = dense_index[j + 1];

            	dense_index[j] = dense_index[j + 1];
            	dense_index[j + 1] = temp_index;

            	whisker_ss_set_dense_index(ss, temp_index, j + 1);
            	whisker_ss_set_dense_index(ss, temp_index2, j);

            	memcpy(temp_dense, (char *)dense + j * element_size, element_size);
            	memcpy((char *)dense + j * element_size, (char *)dense + (j + 1) * element_size, element_size);
            	memcpy((char *)dense + (j + 1) * element_size, temp_dense, element_size);
        	}
    	}
	}
}
