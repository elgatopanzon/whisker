/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_v1
 * @created     : Tuesday Feb 11, 2025 13:33:11 CST
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "whisker_trie.h"

#ifndef ECS_V1_H
#define ECS_V1_H


typedef enum E_WHISKER_ARR  
{
	E_WHISKER_ARR_OK = 0,
	E_WHISKER_ARR_UNKNOWN = 1,
	E_WHISKER_ARR_MEM = 2,
	E_WHISKER_ARR_OUT_OF_BOUNDS = 3,
} E_WHISKER_ARR;
extern const char* E_WHISKER_ARR_STR[];

// header struct holds info about the array
typedef struct whisker_array_header
{
	size_t size;
	size_t element_size;
	size_t length;
	void *swap_buffer;
} whisker_array_header;


// macros
#define whisker_arr_create(t, l, p) whisker_arr_create_f(sizeof(t), l, (void**) p)
#define whisker_arr_resize(p, e) whisker_arr_resize_f((void**) p, e, false)
#define whisker_arr_increment_size(p) whisker_arr_increment_size_f((void**) p)
#define whisker_arr_push(p, v) whisker_arr_push_f((void**) p, (void*) v)
#define whisker_arr_pop(p, v) whisker_arr_pop_f((void**) p, (void*) v)
#define whisker_arr_pop_front(p, v) whisker_arr_pop_front_f((void**) p, (void*) v)
#define whisker_arr_pop_c(p, t, v) t v; whisker_arr_pop_f((void**) p, (void*) &v)
#define whisker_arr_pop_front_c(p, t, v) t v; whisker_arr_pop_front_f((void**) p, (void*) &v)
#define whisker_arr_compact(p) whisker_arr_compact_f((void**) p)
#define whisker_arr_insert(p, i, v) whisker_arr_insert_f((void**) p, i, (void*) v)
#define whisker_arr_grow_get(p, i) whisker_arr_grow_get_f((void**) p, i)
#define whisker_arr_length(a) whisker_arr_length_f((char*)a)
#define whisker_arr_header(a) whisker_arr_header_f((char*)a)
#define whisker_arr_reset(a) whisker_arr_reset_f((char*)a)

// array management functions
E_WHISKER_ARR whisker_arr_create_f(size_t type_size, size_t length, void** arr);
E_WHISKER_ARR whisker_arr_resize_f(void** arr, size_t elements, bool allow_shrink);
E_WHISKER_ARR whisker_arr_increment_size_f(void** arr);
E_WHISKER_ARR whisker_arr_push_f(void** arr, void* value);
E_WHISKER_ARR whisker_arr_pop_f(void** arr, void* value);
E_WHISKER_ARR whisker_arr_pop_front_f(void** arr, void* value);
E_WHISKER_ARR whisker_arr_swap(void** arr, size_t index_from, size_t index_to);
E_WHISKER_ARR whisker_arr_compact_f(void** arr);
E_WHISKER_ARR whisker_arr_insert_f(void** arr, size_t index, void* value);
E_WHISKER_ARR whisker_arr_reset_f(char* arr);

// utility functions to work with the array
whisker_array_header* whisker_arr_header_f(char* arr);
size_t whisker_arr_length_f(char* arr);
void* whisker_arr_grow_get_f(void** arr, size_t index);

void whisker_arr_free(void* arr);


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
	w_trie_node* trie;

	// cache array of the keys
	whisker_array_header keys_header;
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

void whisker_dict_order_by_key(void** dict);
void whisker_dict_order_by_key_(void **keys, void *values, size_t length, size_t key_size, size_t type_size, void* key_temp, void* value_temp);

// internal functions
E_WHISKER_DICT whisker_dict_resize_(void** dict, size_t capacity);

// dictionary utility functions
whisker_dict_header* whisker_dict_get_header(void* dict);
bool whisker_dict_contains_key(void* dict, void* key, size_t key_size);
bool whisker_dict_contains_value(void* dict, void* value);
void** whisker_dict_keys(void* dict);
void* whisker_dict_values(void* dict);
size_t whisker_dict_count(void* dict);


typedef enum E_WHISKER_STR  
{
	E_WHISKER_STR_OK = 0,
	E_WHISKER_STR_UNKNOWN = 1,
	E_WHISKER_STR_MEM = 2,
	E_WHISKER_STR_ARR = 3,
} E_WHISKER_STR;

// operation functions
E_WHISKER_STR whisker_str(char* str, char** w_str);
E_WHISKER_STR whisker_str_join(char* delimiter, char** w_str, ...);
E_WHISKER_STR whisker_str_copy(char* w_str_a, char** w_str_b);
void whisker_str_free(char* w_str);

// utility functions
whisker_array_header* whisker_str_header(char* w_str);
size_t whisker_str_length(char* w_str);
int whisker_str_contains(char* w_haystack, char* needle);


/*********
*  ECS  *
*********/
#ifndef ENTITY_MAX
#define ENTITY_MAX 128
#endif /* ifndef ENTITY_MAX */
#ifndef COMPONENT_MAX
#define COMPONENT_MAX 32
#endif /* ifndef ENTITY_MAX */
#ifndef SYSTEM_MAX
#define SYSTEM_MAX 32
#endif /* ifndef ENTITY_MAX */
extern size_t entity[ENTITY_MAX];
extern size_t entity_recycled[ENTITY_MAX];
extern size_t component_entity[COMPONENT_MAX][ENTITY_MAX];
// extern void* component_array[COMPONENT_MAX];
extern void (*system_array[SYSTEM_MAX])(float);

// ecs functions
void init_ecs();
void deinit_ecs();

// entity functions
void add_entity(size_t e);
void recycle_entity(size_t e);
void remove_entity(size_t e);
size_t get_recycled_entity();
size_t create_entity();

// component functions
bool add_component_entity(size_t component_id, size_t entity_id);
bool remove_component_entity(size_t component_id, size_t entity_id);
bool has_component_entity(size_t component_id, size_t entity_id);

// component array functions
void init_component_array(size_t component_id, size_t component_size);
void set_component(size_t component_id, size_t component_size, size_t entity, void* component_value);
void remove_component(size_t component_id, size_t component_size, size_t entity);
void* get_component(size_t component_id, size_t component_size, size_t entity);
size_t set_component_entities(size_t component_id, size_t entity_list[]);

#define GET_COMPONENT(component_id, type, entity) \
    ((type*)get_component(component_id, sizeof(type), entity))
#define REMOVE_COMPONENT(component_id, type, entity) \
    remove_component(component_id, sizeof(type), entity)
#define SET_COMPONENT(component_id, type, entity, value) \
	set_component(component_id, sizeof(type), entity, value)

// system functions
void register_system(void (*system_ptr)(float));
void deregister_system(void (*system_ptr)(float));
void update_systems(float delta_time);

#endif /* end of include guard ECS_V1_H */
