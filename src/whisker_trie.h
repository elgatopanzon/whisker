/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_trie
 * @created     : Thursday Feb 06, 2025 15:14:51 CST
 */

#include <stdbool.h>
#include <stdlib.h>
#include "whisker_memory.h"

#ifndef WHISKER_TRIE_H
#define WHISKER_TRIE_H

#define WHISKER_TRIE_NODE_CAPACITY 256

// struct to hold data for a trie node
typedef struct whisker_trie
{
	// pointer to child nodes array
	struct whisker_trie *nodes[WHISKER_TRIE_NODE_CAPACITY];

	// pointer to target data
	void* value;
} whisker_trie;

// macros
#define whisker_trie_create_child_node(node, byte) node->nodes[byte] = whisker_mem_xcalloc_t(1, whisker_trie)
#define whisker_trie_search_value_key(r, k, t) whisker_trie_search_value_f((whisker_trie *)r, k, sizeof(t))
#define whisker_trie_search_value_str(r, k) whisker_trie_search_value_f((whisker_trie *)r, k, strlen(k))
#define whisker_trie_search_node_str(r, k) whisker_trie_search_node_((whisker_trie *)r, k, strlen(k), 0, false)
#define whisker_trie_set_value_str(r, k, v) whisker_trie_set_value((whisker_trie *)r, k, strlen(k), v)
#define whisker_trie_search_value_f(root, key, key_size) ({ \
    whisker_trie* matching_node = whisker_trie_search_node_((whisker_trie *)root, key, key_size, 0, false); \
    (matching_node == NULL || matching_node->value == NULL) ? NULL : matching_node->value; \
})
#define whisker_trie_set_value(root, key, key_size, val) ({ \
    whisker_trie* matching_node = whisker_trie_search_node_((whisker_trie *)root, key, key_size, 0, true); \
    (matching_node == NULL) ? false : (matching_node->value = val, true); \
})
#define whisker_trie_remove_value_str(r, k) whisker_trie_remove_value((whisker_trie *)r, k, strlen(k))
#define whisker_trie_remove_value(root, key, key_size) ({ \
    whisker_trie* matching_node = whisker_trie_search_node_((whisker_trie *)root, key, key_size, 0, false); \
    if (matching_node != NULL && matching_node->value != NULL) { \
        free(matching_node->value); \
        matching_node->value = NULL; \
        true; \
    } else { \
        false; \
    } \
})

#define whisker_trie_free_value(node) ({ \
    if (node != NULL && node->value != NULL) { \
        free(node->value); \
        node->value = NULL; \
    } \
})
#define whisker_trie_free_all(root) \
	whisker_trie_free_node_values((whisker_trie *)root); \
	whisker_trie_free_nodes((whisker_trie *)root); \
	free(root);

// short macros
#define wtrie_create_node whisker_trie_create_node
#define wtrie_create_child_node whisker_trie_create_child_node
#define wtrie_search_node whisker_trie_search_node
#define wtrie_search_node_str whisker_trie_search_node_str
#define wtrie_search_value whisker_trie_search_value
#define wtrie_search_value_str whisker_trie_search_value_str
#define wtrie_set_value_str whisker_trie_set_value
#define wtrie_free_node_values whisker_trie_free_node_values
#define wtrie_free_nodes whisker_trie_free_nodes

// trie management functions
whisker_trie *whisker_trie_search_node_(whisker_trie* root, void* key, size_t key_size, int key_position, bool create_missing_nodes);
void whisker_trie_free_nodes(whisker_trie* node);
void whisker_trie_free_node_values(whisker_trie* node);

#endif /* end of include guard WHISKER_TRIE_H */
