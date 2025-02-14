/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_trie
 * @created     : Thursday Feb 06, 2025 15:14:51 CST
 */

#include <stdbool.h>

#ifndef WHISKER_TRIE_H
#define WHISKER_TRIE_H

// errors
typedef enum E_WHISKER_TRIE  
{
	E_WHISKER_TRIE_OK = 0,
	E_WHISKER_TRIE_UNKNOWN = 1,
	E_WHISKER_TRIE_MEM = 2,
	E_WHISKER_TRIE_SEARCH_MISSING_ALL = 3,
	E_WHISKER_TRIE_SEARCH_MISSING_NODE = 4,
	E_WHISKER_TRIE_SEARCH_MISSING_VALUE = 5,
} E_WHISKER_TRIE;
extern const char* E_WHISKER_TRIE_STR[];

#define WHISKER_TRIE_NODE_CAPACITY 256

// struct to hold data for a trie node
typedef struct whisker_trie_node_s
{
	// pointer to child nodes array
	struct whisker_trie_node_s* nodes[WHISKER_TRIE_NODE_CAPACITY];

	// pointer to target data
	void* value;
} Trie;

// macros
#define whisker_trie_search_value(r, k, v) whisker_trie_search_value_f(r, k, (void**)v)

// short macros
#define wtrie_create_node whisker_trie_create_node
#define wtrie_create_child_node whisker_trie_create_child_node
#define wtrie_set_child_node whisker_trie_set_child_node
#define wtrie_search_node whisker_trie_search_node
#define wtrie_search_value whisker_trie_search_value
#define wtrie_set_value whisker_trie_set_value
#define wtrie_free_node whisker_trie_free_node

// trie management functions
E_WHISKER_TRIE whisker_trie_create_node(Trie** node);
E_WHISKER_TRIE whisker_trie_create_child_node(Trie* node, char byte, Trie** child);
void whisker_trie_set_child_node(Trie* node, char byte, Trie* child);
E_WHISKER_TRIE whisker_trie_search_node(Trie* root, char* key, Trie** match);
E_WHISKER_TRIE whisker_trie_search_node_(Trie* root, char* key, int key_position, bool create_missing_nodes, Trie** match);
E_WHISKER_TRIE whisker_trie_search_value_f(Trie* root, char* key, void** match);
E_WHISKER_TRIE whisker_trie_set_value(Trie** root, char* key, void* value);
E_WHISKER_TRIE whisker_trie_free_node(Trie* node, bool free_values);

#endif /* end of include guard WHISKER_TRIE_H */
