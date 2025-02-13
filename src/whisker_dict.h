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
typedef struct whisker_dict_header_s
{
	// the Trie pointer containing all the data
	Trie* trie;

	// cache array of the keys
	char** keys;

	// this makes the struct array compatible
	whisker_array_header_t arr_header;
} whisker_dict_header_t;

// macros
#define whisker_dict_create(d, t, s) whisker_dict_create_f((void**)d, sizeof(t), s)
#define whisker_dict_get(d, k, v) whisker_dict_get_f((void**)d, k, (void**)v)
#define whisker_dict_add(d, k, v) whisker_dict_add_f((void**)d, k, (void*)v)
#define whisker_dict_set(d, k, v) whisker_dict_set_f((void**)d, k, (void*)v)
#define whisker_dict_clear(d) whisker_dict_clear_f((void**)d)
#define whisker_dict_remove(d, k) whisker_dict_remove_f((void**)d, k)

// dictionary management functions
E_WHISKER_DICT whisker_dict_create_f(void** dict, size_t element_size, size_t capacity);
E_WHISKER_DICT whisker_dict_add_f(void** dict, char* key, void* value);
E_WHISKER_DICT whisker_dict_set_f(void** dict, char* key, void* value);
E_WHISKER_DICT whisker_dict_get_index(void* dict, char* key, size_t** index);
E_WHISKER_DICT whisker_dict_get_f(void* dict, char* key, void** value);
E_WHISKER_DICT whisker_dict_copy(void* dict, char* key, void* dest);
E_WHISKER_DICT whisker_dict_remove_f(void** dict, char* key);
E_WHISKER_DICT whisker_dict_clear_f(void** dict);
E_WHISKER_DICT whisker_dict_free(void* dict);

// internal functions
E_WHISKER_DICT whisker_dict_resize_(void** dict, size_t capacity);

// dictionary utility functions
whisker_dict_header_t* whisker_dict_header(void* dict);
bool whisker_dict_contains_key(void* dict, char* key);
bool whisker_dict_contains_value(void* dict, void* value);
char** whisker_dict_keys(void* dict);
void* whisker_dict_values(void* dict);
size_t whisker_dict_count(void* dict);

#endif /* end of include guard WHISKER_DICT_H */
