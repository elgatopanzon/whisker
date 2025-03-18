/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_dict
 * @created     : Thursday Feb 06, 2025 21:03:26 CST
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "whisker_array.h"
#include "whisker_trie.h"
#include "whisker_memory.h"
#include "whisker_dict.h"

const char* E_WHISKER_DICT_STR[] = {
	[E_WHISKER_DICT_OK]="OK",
	[E_WHISKER_DICT_UNKNOWN]="Unknown error",
	[E_WHISKER_DICT_MEM]="Memory error during operation",
	[E_WHISKER_DICT_ARR]="Array error during operation",
	[E_WHISKER_DICT_TRIE]="Trie error during operation",
	[E_WHISKER_DICT_MISSING_KEY]="Key does not exist",
	[E_WHISKER_DICT_KEY_EXISTS]="Key already exists",
};

/**************************
*  management functions  *
**************************/

// create a dict
E_WHISKER_DICT whisker_dict_create_f(void** dict, size_t element_size, size_t capacity)
{
	// create memory block for the dict header
	whisker_memory_block* block = whisker_mem_block_create_and_init(element_size * capacity, sizeof(whisker_dict_header));

	// create an array for the keys cache
	// this will be stored in the dict header as a pointer
	void** keys_arr;
	E_WHISKER_ARR keys_arr_err = whisker_arr_create(void*, 0, &keys_arr);
	if (keys_arr_err != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_DICT_MEM;
	}

	// create Trie to hold keys and values pointers into the dict array
	whisker_trie* trie;
	E_WHISKER_TRIE trie_err = whisker_trie_create_node(&trie);
	if (trie_err != E_WHISKER_TRIE_OK)
	{
		return E_WHISKER_DICT_MEM;
	}

	// set dict header values
	whisker_dict_header* header = block->header;
	header->trie = trie;
	header->keys = keys_arr;
	
	// set array values
	header->arr_header.length = capacity;
	header->arr_header.element_size = element_size;
	header->arr_header.size = capacity * element_size;
	header->arr_header.swap_buffer = whisker_mem_xmalloc(element_size);
	
	*dict = block->data;
	free(block);

	return E_WHISKER_DICT_OK;
}

// set the value for a key overwriting if required
E_WHISKER_DICT whisker_dict_set_f(void** dict, void *key, size_t key_size, void* value)
{
	// it's faster to simply remove the previous key if it exists then add again
	if (whisker_dict_contains_key(*dict, key, key_size))
	{
		E_WHISKER_DICT remove_err = whisker_dict_remove_f(dict, key, key_size);
		if (remove_err != E_WHISKER_DICT_OK)
		{
			return remove_err;
		}
	}

	return whisker_dict_add_f(dict, key, key_size, value);
}

// add a value to the dict with the provided key
E_WHISKER_DICT whisker_dict_add_f(void** dict, void *key, size_t key_size, void* value)
{
	// use contains_key() to avoid adding the same key twice
	if (whisker_dict_contains_key(*dict, key, key_size))
	{
		return E_WHISKER_DICT_KEY_EXISTS;
	}
	
	// attempt to resize the underlying block and values
	// could trigger realloc
	size_t length = whisker_arr_length(*dict);
	E_WHISKER_DICT resize_err = whisker_dict_resize_(dict, length + 1);
	if (resize_err != E_WHISKER_DICT_OK)
	{
		return resize_err;
	}

	// push value to the end
	whisker_dict_header* header = whisker_dict_get_header(*dict);
	void* array_dest = ((char*)*dict) + ((header->arr_header.length - 1) * header->arr_header.element_size);
	memcpy(array_dest, value, header->arr_header.element_size);

	// create wstring from key and add it to keys array
	void* key_mem = whisker_mem_xmalloc(key_size);
	memcpy(key_mem, key, key_size);
	whisker_arr_push(&header->keys, &key_mem);

	// add the key pointing to the dict array index
	size_t *array_index = whisker_mem_xmalloc_t(*array_index);

	*array_index = header->arr_header.length - 1;
	if (whisker_trie_set_value(&header->trie, key, key_size, array_index) != E_WHISKER_TRIE_OK)
	{
		return E_WHISKER_DICT_TRIE;
	}

	return E_WHISKER_DICT_OK;
}

// get underlying array values index for the given key
E_WHISKER_DICT whisker_dict_get_index(void* dict, void *key, size_t key_size, size_t** index)
{
	whisker_dict_header* header = whisker_dict_get_header(dict);

	// try to get the node with the pointer to the value
	// if we can't get the value, then it doesn't exist in the dict
	size_t* index_pointer;
	E_WHISKER_TRIE err = whisker_trie_search_value_f(header->trie, key, key_size, (void**)&index_pointer);
	if (err != E_WHISKER_TRIE_OK)
	{
		return E_WHISKER_DICT_MISSING_KEY;
	}

	// set value pointer based on trie stored array index for the value
	*index = index_pointer;

	return E_WHISKER_DICT_OK;
}

// get a value from the dict with the provided key
void* whisker_dict_get_f(void* dict, void *key, size_t key_size)
{
	size_t* index;
	E_WHISKER_DICT err = whisker_dict_get_index(dict, key, key_size, &index);
	if (err != E_WHISKER_DICT_OK)
	{
		return NULL;
	}

	whisker_dict_header* header = whisker_dict_get_header(dict);
	return (char*)dict + (*index * header->arr_header.element_size);
}

// copy dict value into destination
E_WHISKER_DICT whisker_dict_copy(void* dict, void *key, size_t key_size, void* dest)
{
	size_t* index;
	E_WHISKER_DICT err = whisker_dict_get_index(dict, key, key_size, &index);
	if (err != E_WHISKER_DICT_OK)
	{
		return err;
	}

	whisker_dict_header* header = whisker_dict_get_header(dict);

	// set value pointer based on trie stored array index for the value
	void* value_pointer = (char*)dict + (*index * header->arr_header.element_size);

	// copy value from trie
	memcpy(dest, value_pointer, header->arr_header.element_size);

	return E_WHISKER_DICT_OK;
}

// remove a value and its key
E_WHISKER_DICT whisker_dict_remove_f(void** dict, void *key, size_t key_size)
{
	whisker_dict_header* header = whisker_dict_get_header(*dict);
	size_t dict_length = header->arr_header.length;

	if (dict_length <= 1)
	{
		whisker_dict_clear(dict);
		return E_WHISKER_DICT_OK;
	}

	// remove trie value for key 
	whisker_trie* removed_node;
	E_WHISKER_TRIE trie_err = whisker_trie_search_node_(header->trie, key, key_size, 0, false, &removed_node);
	if (trie_err != E_WHISKER_TRIE_OK)
	{
		return E_WHISKER_DICT_TRIE;
	}

	size_t removed_index = *(size_t*)removed_node->value;
	void* end_key = header->keys[dict_length - 1];
	void* removed_key = header->keys[removed_index];

	// inteligently re-pack values/keys array by moving the end element
	// note: only required if the size is > 1 and not the end element
	if (removed_index != dict_length - 1)
	{
		// copy the value using the copy function
		whisker_dict_copy(*dict, end_key, key_size, ((char*)*dict) + (removed_index * header->arr_header.element_size));

		// free the end node's index value
		whisker_trie* end_node;
		trie_err = whisker_trie_search_node_(header->trie, end_key, key_size, 0, false, &end_node);
		if (trie_err != E_WHISKER_TRIE_OK)
		{
			return E_WHISKER_DICT_TRIE;
		}

		// set the end node to point to the index value of the removed key
		free(end_node->value);
		end_node->value = removed_node->value;
	}
	else
	{
		free(removed_node->value);
	}

	free(end_key);

	// clear the node value to invalid the key lookup
	removed_node->value = NULL;
	
	// decrease array sizes
	header->arr_header.length--;
	whisker_arr_header(header->keys)->length--;
	

	return E_WHISKER_DICT_OK;
}

// remove all keys and values
// effectively a free + create operation
E_WHISKER_DICT whisker_dict_clear_f(void** dict)
{
	// backup previous element size
	whisker_dict_header* header = whisker_dict_get_header(*dict);
	size_t element_size = header->arr_header.element_size;

	// free and re-create dict with same element size
	whisker_dict_free(*dict);
	return whisker_dict_create_f(dict, element_size, 0);
}

// free all resources used by the dict
E_WHISKER_DICT whisker_dict_free(void* dict)
{
	whisker_dict_header* header = whisker_dict_get_header(dict);
	whisker_trie_free_node(header->trie, true);

	// free each key string
	for (int i = 0; i < header->arr_header.length; ++i)
	{
		if (header->keys[i] != NULL)
		{
			free(header->keys[i]);
		}
	}
	whisker_arr_free(header->keys);

	free(header->arr_header.swap_buffer);
	free(header);

	return E_WHISKER_DICT_OK;
}

void whisker_dict_order_by_key(void** dict)
{
	whisker_dict_header *header = whisker_dict_get_header(*dict);
	whisker_dict_order_by_key_(header->keys, *dict, warr_length(*dict), warr_header(header->keys)->element_size, header->arr_header.element_size, warr_header(header->keys)->swap_buffer, header->arr_header.swap_buffer);
}

void whisker_dict_order_by_key_(void **keys, void *values, size_t length, size_t key_size, size_t value_size, void* key_temp, void* value_temp)
{
	whisker_dict_header *header = whisker_dict_get_header(values);

    for (size_t i = 0; i < length - 1; i++) {
        for (size_t j = i + 1; j < length; j++) {
            uint64_t key_i = *(uint64_t*)((char*)keys[i]);
            uint64_t key_j = *(uint64_t*)((char*)keys[j]);
            if (key_i > key_j) {
                memcpy(key_temp, keys[i], key_size);
                memcpy(keys[i], keys[j], key_size);
                memcpy(keys[j], key_temp, key_size);

                memcpy(value_temp, (char*)values + i * value_size, value_size);
                memcpy((char*)values + i * value_size, (char*)values + j * value_size, value_size);
                memcpy((char*)values + j * value_size, value_temp, value_size);

				// swap index nodes
				whisker_trie* key_i_node;
				E_WHISKER_TRIE trie_err = whisker_trie_search_node_(header->trie, keys[i], key_size, 0, false, &key_i_node);
				if (trie_err != E_WHISKER_TRIE_OK)
				{
					return;
				}

				whisker_trie* key_j_node;
				E_WHISKER_TRIE trie_err2 = whisker_trie_search_node_(header->trie, keys[j], key_size, 0, false, &key_j_node);
				if (trie_err2 != E_WHISKER_TRIE_OK)
				{
					return;
				}

				void* trie_key_backup = key_i_node->value;
				key_i_node->value = key_j_node->value;
				key_j_node->value = trie_key_backup;
            }
        }
    }
}

/************************
*  internal functions  *
************************/

// resize the underlying dict array if capacity is larger than memory
E_WHISKER_DICT whisker_dict_resize_(void** dict, size_t capacity)
{
	whisker_dict_header header_old = *whisker_dict_get_header(*dict);
	if (capacity * header_old.arr_header.element_size > header_old.arr_header.size)
	{
		// build block from dict pointer
		whisker_dict_header* header = whisker_dict_get_header(*dict);
		whisker_memory_block block = {
			.header = header,
			.header_size = sizeof(whisker_dict_header),
			.data_size = header_old.arr_header.size,
		};

		// try to realloc the underlying block to the new size
		whisker_mem_block_realloc(&block, capacity * header->arr_header.element_size);

		// set new pointers and update length
		*dict = block.data;
		whisker_dict_header* header_new = whisker_dict_get_header(*dict);
		header_new->trie = header_old.trie;
		header_new->keys = header_old.keys;
		header_new->arr_header.length = capacity;
		header_new->arr_header.size = capacity * header_old.arr_header.element_size;
	}
	else
	{
		whisker_dict_header* header = whisker_dict_get_header(*dict);
		header->arr_header.length = capacity;
	}

	return E_WHISKER_DICT_OK;
}

/***********************
*  utility functions  *
***********************/

// get dict header from opaque pointer
whisker_dict_header* whisker_dict_get_header(void* dict)
{
	return (whisker_dict_header*)((char*)dict - sizeof(whisker_dict_header));
}

// check if the dict contains the provided key
// note: O(1) lookup on the trie based on key length
bool whisker_dict_contains_key(void* dict, void *key, size_t key_size)
{
	void* value;
	return (whisker_trie_search_value_f(whisker_dict_get_header(dict)->trie, key, key_size, &value) == E_WHISKER_TRIE_OK);
}

// check if the dict contains the provided value
// note: O(n) lookup on the values cache array!
bool whisker_dict_contains_value(void* dict, void* value)
{
	whisker_dict_header* header = whisker_dict_get_header(dict);
	size_t element_size = header->arr_header.element_size;
	size_t length = header->arr_header.length;
	for (size_t i = 0; i < length; ++i)
	{
    	bool match = memcmp((char*)dict + (i * element_size), value, element_size) == 0;

    	if (match)
    	{
    		return match;
    	}
	}

	return false;
}

// return the dict keys array
void** whisker_dict_keys(void* dict)
{
	return whisker_dict_get_header(dict)->keys;
}

// return the dict values array 
// (a bit useless, because it's the same pointer, but just in case that changes)
void* whisker_dict_values(void* dict)
{
	return dict;
}

// return the dict values count
size_t whisker_dict_count(void* dict)
{
	return whisker_dict_get_header(dict)->arr_header.length;
}
