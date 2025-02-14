/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_dict
 * @created     : Thursday Feb 06, 2025 21:03:26 CST
 */

#include <string.h>
#include "whisker_string.h"
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
	whisker_memory_block_t* block;
	E_WHISKER_MEM block_err = whisker_mem_block_try_malloc(element_size * capacity, sizeof(whisker_dict_header_t), &block);
	if (block_err != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_DICT_MEM;
	}

	// create an array for the keys cache
	// this will be stored in the dict header as a pointer
	char** keys_arr;
	E_WHISKER_ARR keys_arr_err = whisker_arr_create(char*, 0, &keys_arr);
	if (keys_arr_err != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_DICT_MEM;
	}

	// create Trie to hold keys and values pointers into the dict array
	Trie* trie;
	whisker_trie_create_node(&trie);

	// set dict header values
	whisker_dict_header_t* header = block->header;
	header->trie = trie;
	header->keys = keys_arr;
	
	// set array values
	header->arr_header.length = capacity;
	header->arr_header.element_size = element_size;
	header->arr_header.size = capacity * element_size;
	
	*dict = block->data;
	free(block);

	return E_WHISKER_DICT_OK;
}

// set the value for a key overwriting if required
E_WHISKER_DICT whisker_dict_set_f(void** dict, char* key, void* value)
{
	// it's faster to simply remove the previous key if it exists then add again
	if (whisker_dict_contains_key(*dict, key))
	{
		E_WHISKER_DICT remove_err = whisker_dict_remove_f(dict, key);
		if (remove_err != E_WHISKER_DICT_OK)
		{
			return remove_err;
		}
	}

	return whisker_dict_add(dict, key, value);
}

// add a value to the dict with the provided key
E_WHISKER_DICT whisker_dict_add_f(void** dict, char* key, void* value)
{
	// use contains_key() to avoid adding the same key twice
	if (whisker_dict_contains_key(*dict, key))
	{
		return E_WHISKER_DICT_KEY_EXISTS;
	}
	
	// attempt to resize the underlying block and values
	// could trigger realloc
	size_t length = whisker_arr_length(*dict);
	whisker_dict_resize_(dict, length + 1);

	// push value to the end
	whisker_dict_header_t* header = whisker_dict_header(*dict);
	void* array_dest = ((char*)*dict) + ((header->arr_header.length - 1) * header->arr_header.element_size);
	memcpy(array_dest, value, header->arr_header.element_size);

	// create wstring from key and add it to keys array
	char* key_wstring;
	whisker_str(key, &key_wstring);
	whisker_arr_push(&header->keys, &key_wstring);

	// add the key pointing to the dict array index
	size_t* array_index;
	E_WHISKER_MEM m_err = whisker_mem_try_malloc(sizeof(size_t), (void**)&array_index);
	if (m_err != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_DICT_MEM;
	}

	*array_index = header->arr_header.length - 1;
	if (whisker_trie_set_value(&header->trie, key, array_index) != E_WHISKER_TRIE_OK)
	{
		return E_WHISKER_DICT_TRIE;
	}

	return E_WHISKER_DICT_OK;
}

// get underlying array values index for the given key
E_WHISKER_DICT whisker_dict_get_index(void* dict, char* key, size_t** index)
{
	whisker_dict_header_t* header = whisker_dict_header(dict);

	// try to get the node with the pointer to the value
	// if we can't get the value, then it doesn't exist in the dict
	size_t* index_pointer;
	E_WHISKER_TRIE err = whisker_trie_search_value(header->trie, key, &index_pointer);
	if (err != E_WHISKER_TRIE_OK)
	{
		return E_WHISKER_DICT_MISSING_KEY;
	}

	// set value pointer based on trie stored array index for the value
	*index = index_pointer;

	return E_WHISKER_DICT_OK;
}

// get a value from the dict with the provided key
void* whisker_dict_get_f(void* dict, char* key)
{
	size_t* index;
	E_WHISKER_DICT err = whisker_dict_get_index(dict, key, &index);
	if (err != E_WHISKER_DICT_OK)
	{
		return NULL;
	}

	whisker_dict_header_t* header = whisker_dict_header(dict);
	return (char*)dict + (*index * header->arr_header.element_size);
}

// copy dict value into destination
E_WHISKER_DICT whisker_dict_copy(void* dict, char* key, void* dest)
{
	size_t* index;
	E_WHISKER_DICT err = whisker_dict_get_index(dict, key, &index);
	if (err != E_WHISKER_DICT_OK)
	{
		return err;
	}

	whisker_dict_header_t* header = whisker_dict_header(dict);

	// set value pointer based on trie stored array index for the value
	void* value_pointer = (char*)dict + (*index * header->arr_header.element_size);

	// copy value from trie
	memcpy(dest, value_pointer, header->arr_header.element_size);

	return E_WHISKER_DICT_OK;
}

// remove a value and its key
E_WHISKER_DICT whisker_dict_remove_f(void** dict, char* key)
{
	whisker_dict_header_t* header = whisker_dict_header(*dict);
	size_t dict_length = header->arr_header.length;

	if (dict_length <= 1)
	{
		whisker_dict_clear(dict);
		return E_WHISKER_DICT_OK;
	}

	// remove trie value for key 
	Trie* removed_node;
	E_WHISKER_TRIE trie_err = whisker_trie_search_node(header->trie, key, &removed_node);
	if (trie_err != E_WHISKER_TRIE_OK)
	{
		return E_WHISKER_DICT_TRIE;
	}

	size_t removed_index = *(size_t*)removed_node->value;
	char* end_key = header->keys[dict_length - 1];

	// inteligently re-pack values/keys array by moving the end element
	// note: only required if the size is > 1 and not the end element
	if (removed_index != dict_length - 1)
	{
		// copy the value using the copy function
		whisker_dict_copy(*dict, end_key, ((char*)*dict) + (removed_index * header->arr_header.element_size));

		// free the end node's index value
		Trie* end_node;
		trie_err = whisker_trie_search_node(header->trie, end_key, &end_node);
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

	// clear the node value to invalid the key lookup
	removed_node->value = NULL;
	
	// decrease array sizes
	header->arr_header.length--;
	whisker_str_free(header->keys[dict_length - 1]);
	whisker_arr_header(header->keys)->length--;
	

	return E_WHISKER_DICT_OK;
}

// remove all keys and values
// effectively a free + create operation
E_WHISKER_DICT whisker_dict_clear_f(void** dict)
{
	// backup previous element size
	whisker_dict_header_t* header = whisker_dict_header(*dict);
	size_t element_size = header->arr_header.element_size;

	// free and re-create dict with same element size
	whisker_dict_free(*dict);
	return whisker_dict_create_f(dict, element_size, 0);
}

// free all resources used by the dict
E_WHISKER_DICT whisker_dict_free(void* dict)
{
	whisker_dict_header_t* header = whisker_dict_header(dict);
	whisker_trie_free_node(header->trie, true);

	// free each key string
	for (int i = 0; i < whisker_arr_length(header->keys); ++i)
	{
		whisker_str_free(header->keys[i]);
	}
	whisker_arr_free(header->keys);

	free(header);

	return E_WHISKER_DICT_OK;
}


/************************
*  internal functions  *
************************/

// resize the underlying dict array if capacity is larger than memory
E_WHISKER_DICT whisker_dict_resize_(void** dict, size_t capacity)
{
	whisker_dict_header_t header_old = *whisker_dict_header(*dict);
	if (capacity * header_old.arr_header.element_size > header_old.arr_header.size)
	{
		// build block from dict pointer
		whisker_dict_header_t* header = whisker_dict_header(*dict);
		whisker_memory_block_t block = {
			.header = header,
			.header_size = sizeof(whisker_dict_header_t),
			.data_size = header_old.arr_header.size,
		};

		// try to realloc the underlying block to the new size
		E_WHISKER_MEM err = whisker_mem_block_try_realloc_data(&block, capacity * header->arr_header.element_size);
		if (err != E_WHISKER_MEM_OK)
		{
			return E_WHISKER_DICT_MEM;
		}

		// set new pointers and update length
		*dict = block.data;
		whisker_dict_header_t* header_new = whisker_dict_header(*dict);
		header_new->trie = header_old.trie;
		header_new->keys = header_old.keys;
		header_new->arr_header.length = capacity;
		header_new->arr_header.size = capacity * header_old.arr_header.element_size;
	}
	else
	{
		whisker_dict_header_t* header = whisker_dict_header(*dict);
		header->arr_header.length = capacity;
	}

	return E_WHISKER_DICT_OK;
}

/***********************
*  utility functions  *
***********************/

// get dict header from opaque pointer
whisker_dict_header_t* whisker_dict_header(void* dict)
{
	return whisker_mem_block_header_from_data_pointer(dict, sizeof(whisker_dict_header_t));
}

// check if the dict contains the provided key
// note: O(1) lookup on the trie based on key length
bool whisker_dict_contains_key(void* dict, char* key)
{
	void* value;
	return (whisker_trie_search_value(whisker_dict_header(dict)->trie, key, &value) == E_WHISKER_TRIE_OK);
}

// check if the dict contains the provided value
// note: O(n) lookup on the values cache array!
bool whisker_dict_contains_value(void* dict, void* value)
{
	whisker_dict_header_t* header = whisker_dict_header(dict);
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
char** whisker_dict_keys(void* dict)
{
	return whisker_dict_header(dict)->keys;
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
	return whisker_dict_header(dict)->arr_header.length;
}
