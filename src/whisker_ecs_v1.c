/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_v1
 * @created     : Tuesday Feb 11, 2025 13:34:00 CST
 */

#include "whisker_array.h"
#include "whisker_debug.h"
#include "whisker_ecs_v1.h"

/********************
*  whisker_arr v1  *
********************/

E_WHISKER_ARR whisker_arr_try_resize_if_required_(void** arr, size_t elements)
{
	whisker_array_header* header = whisker_arr_header(*arr);
	if (elements * header->element_size > header->size)
	{
		E_WHISKER_ARR err = whisker_arr_resize_f(arr, elements, false);
		if (err != E_WHISKER_ARR_OK)
		{
			return err;
		}
	}
	else
	{
		header->length++;
	}

	return E_WHISKER_ARR_OK;
}

// create a block and use it to create the array
E_WHISKER_ARR whisker_arr_create_f(size_t type_size, size_t length, void** arr)
{
	// alloc whisker_mem block for the array + header struct
	w_mem_block* block = w_mem_block_create_and_init(type_size * length, sizeof(whisker_array_header));

	// set header values
	whisker_array_header* header = block->header;
	header->element_size = type_size;
	header->length = length;
	header->size = type_size * length;
	header->swap_buffer = w_mem_xcalloc(1, type_size);

	// set array pointer to data pointer
	*arr = block->data;
	free(block);

	return E_WHISKER_ARR_OK;
}

// resize an array with the given pointer, putting the new pointer
E_WHISKER_ARR whisker_arr_resize_f(void** arr, size_t elements, bool allow_shrink)
{
	// build block from array pointer
	whisker_array_header* header = whisker_arr_header(*arr);

	// if the new size is the same, nothing needs to be done
	if (header->length == elements)
	{
		return E_WHISKER_ARR_OK;
	}
	if (header->length > elements && !allow_shrink)
	{
		header->length = elements;
		return E_WHISKER_ARR_OK;
	}

	w_mem_block block = {
		.header = header,
		.header_size = sizeof(whisker_array_header),
		.data_size = header->size,
	};

	// try to realloc the underlying block to the new size
	w_mem_block_realloc(&block, elements * header->element_size);

	// set new pointer and update length
	*arr = block.data;
	header = whisker_arr_header(*arr);
	header->length = elements;
	header->size = elements * header->element_size;

	return E_WHISKER_ARR_OK;
}

// increment array size by +1
// shortcut used by push/insert functions
E_WHISKER_ARR whisker_arr_increment_size_f(void** arr)
{
	whisker_array_header* header = whisker_arr_header(*arr);
	E_WHISKER_ARR err = whisker_arr_try_resize_if_required_(arr, header->length + 1);
	if (err != E_WHISKER_ARR_OK)
	{
		return err;
	}

	return E_WHISKER_ARR_OK;
}

// extend the array by +1 and insert value
E_WHISKER_ARR whisker_arr_push_f(void** arr, void* value)
{
	E_WHISKER_ARR err = whisker_arr_increment_size_f(arr);
	if (err != E_WHISKER_ARR_OK)
	{
		return err;
	}

	whisker_array_header* header = whisker_arr_header(*arr);

	// copy value into end
	memcpy(((char*)*arr) + ((header->length - 1) * header->element_size), value, header->element_size);

	return E_WHISKER_ARR_OK;
}

// resize array to it's actual length
E_WHISKER_ARR whisker_arr_compact_f(void** arr)
{
	whisker_array_header* header = whisker_arr_header(*arr);
	E_WHISKER_ARR err = whisker_arr_resize_f(arr, header->length, true);
	if (err != E_WHISKER_ARR_OK)
	{
		return err;
	}

	return E_WHISKER_ARR_OK;
}

// retreive the last element, then shrink the array length keeping same capacity
E_WHISKER_ARR whisker_arr_pop_f(void** arr, void* value)
{
	whisker_array_header* header = whisker_arr_header(*arr);

	if (header->length > 0)
	{
		memcpy(value, ((char*)*arr) + (header->length - 1) * header->element_size, header->element_size);

		header->length--;

		return E_WHISKER_ARR_OK;
	}

	return E_WHISKER_ARR_OUT_OF_BOUNDS;
}

// retreive the first element, then shrink the array length keeping same capacity
E_WHISKER_ARR whisker_arr_pop_front_f(void** arr, void* value)
{
	whisker_array_header* header = whisker_arr_header(*arr);

	// copy first value
	memcpy(value, ((char*)*arr), header->element_size);

	// swap first and last index values
	whisker_arr_swap(arr, 0, header->length - 1);

	// resize array by -1
	header->length--;

	return E_WHISKER_ARR_OK;
}

// swap elements in 2 index positions
E_WHISKER_ARR whisker_arr_swap(void** arr, size_t index_a, size_t index_b)
{
	whisker_array_header* header = whisker_arr_header(*arr);
	if (index_a + 1 > header->length || index_b + 1 > header->length)
	{
		return E_WHISKER_ARR_OUT_OF_BOUNDS;
	}

	// copy a into temp
	void* temp = header->swap_buffer;
	memcpy(temp, ((char*)*arr) + (index_a * header->element_size), header->element_size);

	// copy over a
	memcpy(((char*)*arr) + (index_a * header->element_size), ((char*)*arr) + (index_b * header->element_size), header->element_size);

	// copy temp into b
	memcpy(((char*)*arr) + (index_b * header->element_size), temp, header->element_size);

	return E_WHISKER_ARR_OK;
}

// insert value at index and shift values forward
E_WHISKER_ARR whisker_arr_insert_f(void** arr, size_t index, void* value)
{
	// resize array +1 to fit the new element
	E_WHISKER_ARR err = whisker_arr_increment_size_f(arr);
	if (err != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_ARR_MEM;
	}

	whisker_array_header* header = whisker_arr_header(*arr);

	// move all elements from index forward by 1 element
	// note: this will overwrite the end value
	memmove((char*)*arr + (index + 1) * header->element_size, (char*)*arr + index * header->element_size, ((header->length - 1) - index) * header->element_size);

	// copy value again into the index slot
	memcpy((char*)*arr + index * header->element_size, value, header->element_size);

	return E_WHISKER_ARR_OK;
}

// reset length to 0 keeping allocated size and values
E_WHISKER_ARR whisker_arr_reset_f(char* arr)
{
	whisker_arr_header(arr)->length = 0;

	return E_WHISKER_ARR_OK;
}


// free the array by obtaining the header
void whisker_arr_free(void* arr)
{
	whisker_array_header *header = whisker_arr_header(arr);
	free(header->swap_buffer);
	free(header);
}

// obtain the header from the array pointer
inline whisker_array_header* whisker_arr_header_f(char* arr)
{
	return (whisker_array_header*)((char*)arr - sizeof(whisker_array_header));
}

// get array length from underlying header
// shortcut to use in loops
inline size_t whisker_arr_length_f(char* arr)
{
	return whisker_arr_header(arr)->length;
}

// get value at the given index, growing and returning pointer
void* whisker_arr_grow_get_f(void** arr, size_t index)
{
	whisker_array_header* header = whisker_arr_header(*arr);

	if (index + 1 > header->length)
	{
		E_WHISKER_ARR err = whisker_arr_resize(arr, index + 1);
		if (err != E_WHISKER_ARR_OK)
		{
			return NULL;
		}
	}

	return &(*arr)[index];
}

/*********************
*  whisker_dict v1  *
*********************/

// create a dict
E_WHISKER_DICT whisker_dict_create_f(void** dict, size_t element_size, size_t capacity)
{
	// create memory block for the dict header
	w_mem_block* block = w_mem_block_create_and_init(element_size * capacity, sizeof(whisker_dict_header));

	// create an array for the keys cache
	// this will be stored in the dict header as a pointer
	void** keys_arr;
	E_WHISKER_ARR keys_arr_err = whisker_arr_create(void*, 0, &keys_arr);
	if (keys_arr_err != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_DICT_MEM;
	}

	// create Trie to hold keys and values pointers into the dict array
	w_trie_node* trie = w_mem_xcalloc_t(1, w_trie_node);

	// set dict header values
	whisker_dict_header* header = block->header;
	header->trie = trie;
	header->keys = keys_arr;
	
	// set array values
	header->arr_header.length = capacity;
	header->arr_header.element_size = element_size;
	header->arr_header.size = capacity * element_size;
	header->arr_header.swap_buffer = w_mem_xmalloc(element_size);
	
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
	void* key_mem = w_mem_xmalloc(key_size);
	memcpy(key_mem, key, key_size);
	whisker_arr_push(&header->keys, &key_mem);

	// add the key pointing to the dict array index
	size_t *array_index = w_mem_xmalloc_t(*array_index);

	*array_index = header->arr_header.length - 1;
	if (!w_trie_set_value(header->trie, key, key_size, array_index))
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
	size_t* index_pointer = w_trie_search_value_f(header->trie, key, key_size);
	if (index_pointer == NULL)
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
	w_trie_node* removed_node = w_trie_search_node_(header->trie, key, key_size, 0, false);
	if (removed_node == NULL)
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
		w_trie_node* end_node = w_trie_search_node_(header->trie, end_key, key_size, 0, false);
		if (end_node == NULL)
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
	w_trie_free_all(header->trie);

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
	whisker_dict_order_by_key_(header->keys, *dict, whisker_arr_length(*dict), whisker_arr_header(header->keys)->element_size, header->arr_header.element_size, whisker_arr_header(header->keys)->swap_buffer, header->arr_header.swap_buffer);
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
				w_trie_node* key_i_node = w_trie_search_node_(header->trie, keys[i], key_size, 0, false);
				if (key_i_node == NULL)
				{
					return;
				}
				w_trie_node* key_j_node = w_trie_search_node_(header->trie, keys[j], key_size, 0, false);
				if (key_j_node == NULL)
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
		w_mem_block block = {
			.header = header,
			.header_size = sizeof(whisker_dict_header),
			.data_size = header_old.arr_header.size,
		};

		// try to realloc the underlying block to the new size
		w_mem_block_realloc(&block, capacity * header->arr_header.element_size);

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
	return (w_trie_search_value_f(whisker_dict_get_header(dict)->trie, key, key_size) != NULL);
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


/*********************
*  whisker_str v1   *
*********************/

// convert a string to a whisker headered string
E_WHISKER_STR whisker_str(char* str, char** w_str)
{
	size_t str_size = strlen(str) + 1;

	E_WHISKER_ARR err = whisker_arr_create(char, str_size, w_str);
	if (err != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_STR_ARR;
	}

	// copy string into w_str
	memcpy(*w_str, str, str_size);

	return E_WHISKER_STR_OK;
}

// join multiple strings together with a delimiter
E_WHISKER_STR whisker_str_join(char* delimiter, char** w_str, ...) {
	// count string args length and arg count
    va_list args;
    va_start(args, w_str);
    size_t joined_length = 0;
    size_t delimiter_length = strlen(delimiter);
    char* current_str;

    while ((current_str = va_arg(args, char*)) != NULL) {
        joined_length += strlen(current_str) + delimiter_length;
    }
    va_end(args);

    // create a whisker string matching the size
	E_WHISKER_ARR err = whisker_arr_create(char, joined_length, w_str);
	if (err != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_STR_ARR;
	}

    va_start(args, w_str);
    char* dest = *w_str;
    int first = 1;
    while ((current_str = va_arg(args, char*)) != NULL) {
        if (!first) {
            strncpy(dest, delimiter, delimiter_length);
            dest += delimiter_length;
        }
        size_t current_length = strlen(current_str);
        strncpy(dest, current_str, current_length + 1);
        dest += current_length;
        first = 0;
    }
    va_end(args);

    return E_WHISKER_STR_OK;
}

// copy A into B, allocating a new string
E_WHISKER_STR whisker_str_copy(char* w_str_a, char** w_str_b)
{
    // create a whisker string from existing string
    // NOTE: it's just a wrapper over whisker_str, really
    // it would even work with a normal string
	E_WHISKER_STR err = whisker_str(w_str_a, w_str_b);
	if (err != E_WHISKER_STR_OK)
	{
		return err;
	}

    return E_WHISKER_STR_OK;
}

// obtain the header from the string pointer
whisker_array_header* whisker_str_header(char* w_str)
{
	return whisker_arr_header(w_str);
}

// obtain string length from w string
size_t whisker_str_length(char* w_str)
{
	whisker_array_header* header = whisker_str_header(w_str);
	return header->length - 1; // -1 to cut off the \0 byte
}

// check if string (heystack) contains another string (needle)
int whisker_str_contains(char* w_haystack, char* needle)
{
    size_t haystack_length = whisker_str_length(w_haystack);
    size_t needle_length = strlen(needle);

    if (needle_length == 0) return -1;
    if (haystack_length < needle_length) return -1;

    for (size_t i = 0; i <= haystack_length - needle_length; i++) {
        size_t j;
        for (j = 0; j < needle_length; j++) {
            if (w_haystack[i + j] != needle[j]) {
                break;
            }
        }
        if (j == needle_length) {
            return i;
        }
    }
    return -1;
}

// free the string by obtaining the header
void whisker_str_free(char* w_str)
{
	whisker_arr_free(w_str);
}


/************
*  ecs v1  *
************/


size_t entity[ENTITY_MAX] = {};
size_t entity_recycled[ENTITY_MAX] = {};
size_t component_entity[COMPONENT_MAX][ENTITY_MAX] = {0};
/* void* component_array[COMPONENT_MAX] = {}; */
void* components;
void (*system_array[SYSTEM_MAX])(float) = {};

typedef struct components
{
	void* components;
} components_array;

void init_ecs()
{
	debug_printf("ecs:init\n");

	for (size_t i = 0; i < ENTITY_MAX; ++i)
	{
		entity[i] = -1;
		entity_recycled[i] = -1;
	}

	for (size_t i = 0; i < COMPONENT_MAX; ++i)
	{
		for (size_t ii = 0; ii < ENTITY_MAX; ++ii)
		{
    		component_entity[i][ii] = 0;
		}
	}

	whisker_dict_create(&components, components_array, 0);
}

void deinit_ecs()
{
	debug_printf("ecs:deinit\n");

	for (size_t i = 0; i < COMPONENT_MAX; ++i)
	{
		for (size_t ii = 0; ii < ENTITY_MAX; ++ii)
		{
    		/* free(component_entity[i]); */
		}

		if (whisker_dict_contains_key_strk(components, (char*) &i))
		{
			components_array* component_array = whisker_dict_get_strk(components, (char*) &i);
			whisker_arr_free(component_array->components);
		}
	}

	whisker_dict_free(components);
}

/**********************
*  entity functions  *
**********************/
void add_entity(size_t e)
{
	for (size_t i = 0; i < ENTITY_MAX; ++i)
	{
		if (entity[i] == -1)
		{
			/* debug_printf("ecs:add_entity:e = %zu @ %zu\n", e, i); */

			entity[i] = e;
			break;
		}
	}
}

void recycle_entity(size_t e)
{
	for (size_t i = 0; i < ENTITY_MAX; ++i)
	{
		if (entity_recycled[i] == -1)
		{
			/* debug_printf("ecs:recycle_entity:e = %zu @ %zu\n", e, i); */

			entity_recycled[i] = e;
			break;
		}
	}
}

void remove_entity(size_t e)
{
	for (size_t i = 0; i < ENTITY_MAX; ++i)
	{
		if (entity[i] == e)
		{
			/* debug_printf("ecs:remove_entity:e = %d @ %d\n", e, i); */

			for (size_t i = 0; i < COMPONENT_MAX; ++i)
			{
    			component_entity[i][e] = 0;
			}

			entity[i] = -1;
			recycle_entity(e);
			break;
		}
	}
}

size_t get_recycled_entity()
{
	for (size_t i = 0; i < ENTITY_MAX; ++i)
	{
		if (entity_recycled[i] != -1) {
			size_t recycled = entity_recycled[i];
			entity_recycled[i] = -1;

			return recycled;
		}
	}

	return -1;
}

size_t create_entity()
{
	// first try to get a recycled entity
	size_t recycled_entity = get_recycled_entity();

	if (recycled_entity == -1) {
		// use the entity current
		size_t largest_entity = 0;
		for (size_t i = 0; i < ENTITY_MAX; ++i)
		{
			/* debug_printf("%d %d %d\n", i, entity[i], largest_entity); */
			if (entity[i] != -1 && entity[i] > largest_entity) {
				largest_entity = entity[i];
			}
		}
		/* debug_printf("ecs:create_entity:largest_entity+1 = %d\n", largest_entity + 1); */
		add_entity(largest_entity + 1);
		return largest_entity + 1;
	}
	else
	{
		/* debug_printf("ecs:create_entity:recycled_entity = %d\n", recycled_entity); */
		add_entity(recycled_entity);
	}

	return recycled_entity;
}



/*************************
*  component functions  *
*************************/
bool add_component_entity(size_t component_id, size_t entity_id)
{
    if (component_id >= COMPONENT_MAX || entity_id >= ENTITY_MAX) {
        return false;
    }

    /* debug_printf("ecs:add_component_entity:component_id = %zu\n", component_id); */
    /* debug_printf("ecs:add_component_entity:entity_id = %zu\n", entity_id); */

    component_entity[component_id][entity_id] = 1;
    return true;
}

bool remove_component_entity(size_t component_id, size_t entity_id)
{
    if (component_id >= COMPONENT_MAX || entity_id >= ENTITY_MAX) {
        return false;
    }

    /* debug_printf("ecs:remove_component_entity:component_id = %zu\n", component_id); */
    /* debug_printf("ecs:remove_component_entity:entity_id = %zu\n", entity_id); */

    component_entity[component_id][entity_id] = 0;
    return true;
}

bool has_component_entity(size_t component_id, size_t entity_id)
{
    if (component_id >= COMPONENT_MAX || entity_id >= ENTITY_MAX) {
        return false;
    }

    return (component_entity[component_id][entity_id] == 1);
}


/*******************************
*  component array functions  *
*******************************/
void init_component_array(size_t component_id, size_t component_size)
{
	if (!whisker_dict_contains_key_strk(components, (char*) &component_id))
	{
		components_array component_array = {};
		whisker_arr_create_f(component_size, ENTITY_MAX, &component_array.components);
		whisker_dict_set_strk(&components, (char*) &component_id, &component_array);
	}
}

void set_component(size_t component_id, size_t component_size, size_t entity, void* component_value)
{
	init_component_array(component_id, component_size);

	components_array* component_array = whisker_dict_get_strk(components, (char*) &component_id);

    memcpy((char*)component_array->components + entity * component_size, component_value, component_size);
	add_component_entity(component_id, entity);    
}

void remove_component(size_t component_id, size_t component_size, size_t entity)
{
	init_component_array(component_id, component_size);

	components_array* component_array = whisker_dict_get_strk(components, (char*) &component_id);

    memset((char*)component_array->components + entity * component_size, 0, component_size);
	remove_component_entity(component_id, entity);    
}

void* get_component(size_t component_id, size_t component_size, size_t entity)
{
	init_component_array(component_id, component_size);

	components_array* component_array = whisker_dict_get_strk(components, (char*) &component_id);

    return (char*)component_array->components + entity * component_size;
}

size_t set_component_entities(size_t component_id, size_t entity_list[])
{
	size_t entity_l[ENTITY_MAX];
	size_t entity_i = 0;
	for (size_t e = 0; e < ENTITY_MAX; ++e)
	{
		size_t ce = component_entity[component_id][e];
		if (ce == 0) {
			continue;
		}

		entity_l[entity_i] = e;
		entity_i++;
	}

	memcpy(entity_list, entity_l, entity_i * sizeof(size_t));

	return entity_i;
}


/**********************
*  system functions  *
**********************/
void update_systems(float delta_time)
{
	for (size_t i = 0; i < SYSTEM_MAX; ++i)
	{
		if (system_array[i] != NULL) {
			system_array[i](delta_time);
		}
	}
}

void register_system(void (*system_ptr)(float))
{
	deregister_system(system_ptr);

	for (size_t i = 0; i < SYSTEM_MAX; ++i)
	{
		if (system_array[i] == NULL)
		{
			system_array[i] = system_ptr;
			break;
		}
	}	
}
void deregister_system(void (*system_ptr)(float))
{
	for (size_t i = 0; i < SYSTEM_MAX; ++i)
	{
		if (system_array[i] == system_ptr)
		{
			system_array[i] = NULL;
		}
	}	
}
