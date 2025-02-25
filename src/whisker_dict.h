/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_dict
 * @created     : Thursday Feb 06, 2025 21:03:16 CST
 */

#include <stdlib.h>
#include "whisker_array.h"
#include "whisker_trie.h"

#ifndef WHISKER_DICT_H
#define WHISKER_DICT_H

// errors
typedef enum E_WHISKER_DICT  
{
	E_WHISKER_DICT_OK = 0,
	E_WHISKER_DICT_UNKNOWN = 1,
	E_WHISKER_DICT_MEM = 2,
	E_WHISKER_DICT_ARR = 3,
	E_WHISKER_DICT_TRIE = 4,
	E_WHISKER_DICT_MISSING_KEY = 5,
	E_WHISKER_DICT_KEY_EXISTS = 6,
} E_WHISKER_DICT;
extern const char* E_WHISKER_DICT_STR[];

// the dictionary makes use of a trie for fast lookups with keys
// the keys nodes hold an offset to the value in the dict's values array
// this allows for the following:
// - fast key lookup in the trie
// - fast iteration of unordered values
// - fast iteration of unordered (but same order) keys
// - fast removal (1 array shift, 1 pointer removal)
// it has the following flaws:
// - slow value lookup O(n) (could be fixed with second trie)
// - slow addition (dynamic realloc required depending on current size)

// the struct header
typedef struct whisker_dict_header
{
	// the Trie pointer containing all the data
	whisker_trie* trie;

	// cache array of the keys
	void** keys;

	// this makes the struct array compatible
	whisker_array_header arr_header;
} whisker_dict_header;

// macros
#define whisker_dict_create(d, t, s) whisker_dict_create_f((void**)d, sizeof(t), s)
#define whisker_dict_get(d, k, s) whisker_dict_get_f((void**)d, k, s)
#define whisker_dict_add(d, k, s, v) whisker_dict_add_f((void**)d, k, s, (void*)v)
#define whisker_dict_set(d, k, s, v) whisker_dict_set_f((void**)d, k, s, (void*)v)
#define whisker_dict_clear(d) whisker_dict_clear_f((void**)d)
#define whisker_dict_remove(d, k, s) whisker_dict_remove_f((void**)d, k, s)

#define whisker_dict_get_keyt(d, k, t) whisker_dict_get_f((void**)d, k, sizeof(t))
#define whisker_dict_add_keyt(d, k, t, v) whisker_dict_add_f((void**)d, k, sizeof(t), (void*)v)
#define whisker_dict_set_keyt(d, k, t, v) whisker_dict_set_f((void**)d, k, sizeof(t), (void*)v)
#define whisker_dict_remove_keyt(d, k, t) whisker_dict_remove_f((void**)d, k, sizeof(t))
#define whisker_dict_copy_keyt(d, k, t, v) whisker_dict_copy((void**)d, k, sizeof(t), v)
#define whisker_dict_remove_keyt(d, k, t) whisker_dict_remove_f((void**)d, k, sizeof(t))
#define whisker_dict_contains_key_keyt(d, k, t) whisker_dict_contains_key((void**)d, k, sizeof(t))
#define whisker_dict_get_index_keyt(d, k, t, v) whisker_dict_get_index((void**)d, k, sizeof(t), v)

#define whisker_dict_get_strk(d, k) whisker_dict_get_f((void**)d, k, strlen(k))
#define whisker_dict_add_strk(d, k, v) whisker_dict_add_f((void**)d, k, strlen(k), (void*)v)
#define whisker_dict_set_strk(d, k, v) whisker_dict_set_f((void**)d, k, strlen(k), (void*)v)
#define whisker_dict_copy_strk(d, k, v) whisker_dict_copy((void**)d, k, strlen(k), v)
#define whisker_dict_remove_strk(d, k) whisker_dict_remove_f((void**)d, k, strlen(k))
#define whisker_dict_contains_key_strk(d, k) whisker_dict_contains_key((void**)d, k, strlen(k))
#define whisker_dict_get_index_strk(d, k, v) whisker_dict_get_index((void**)d, k, strlen(k), v)

// short macros
#define wdict_create whisker_dict_create
#define wdict_add whisker_dict_add
#define wdict_set whisker_dict_set
#define wdict_get_index whisker_dict_get_index
#define wdict_get whisker_dict_get
#define wdict_copy whisker_dict_copy
#define wdict_remove whisker_dict_remove
#define wdict_clear whisker_dict_clear
#define wdict_free whisker_dict_free
#define wdict_resize whisker_dict_resize
#define wdict_header whisker_dict_header
#define wdict_contains_key whisker_dict_contains_key
#define wdict_contains_value whisker_dict_contains_value
#define wdict_keys whisker_dict_keys
#define wdict_values whisker_dict_values
#define wdict_count whisker_dict_count

// dictionary management functions
E_WHISKER_DICT whisker_dict_create_f(void** dict, size_t element_size, size_t capacity);
E_WHISKER_DICT whisker_dict_add_f(void** dict, void* key, size_t key_size, void* value);
E_WHISKER_DICT whisker_dict_set_f(void** dict, void* key, size_t key_size, void* value);
E_WHISKER_DICT whisker_dict_get_index(void* dict, void* key, size_t key_size, size_t** index);
void* whisker_dict_get_f(void* dict, void* key, size_t key_size);
E_WHISKER_DICT whisker_dict_copy(void* dict, void* key, size_t key_size, void* dest);
E_WHISKER_DICT whisker_dict_remove_f(void** dict, void* key, size_t key_size);
E_WHISKER_DICT whisker_dict_clear_f(void** dict);
E_WHISKER_DICT whisker_dict_free(void* dict);

// internal functions
E_WHISKER_DICT whisker_dict_resize_(void** dict, size_t capacity);

// dictionary utility functions
whisker_dict_header* whisker_dict_get_header(void* dict);
bool whisker_dict_contains_key(void* dict, void* key, size_t key_size);
bool whisker_dict_contains_value(void* dict, void* value);
void** whisker_dict_keys(void* dict);
void* whisker_dict_values(void* dict);
size_t whisker_dict_count(void* dict);

#endif /* end of include guard WHISKER_DICT_H */
