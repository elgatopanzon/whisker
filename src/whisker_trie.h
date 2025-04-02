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

#define W_NODE_CAPACITY 256

// struct to hold data for a trie node
typedef struct w_trie_node
{
	// pointer to child nodes array
	struct w_trie_node *nodes[W_NODE_CAPACITY];

	// pointer to target data
	void* value;
} w_trie_node;

// macros
#define w_trie_create_child_node(node, byte) node->nodes[byte] = w_mem_xcalloc_t(1, struct w_trie_node)
#define w_trie_search_value_key(r, k, t) w_trie_search_value_f((struct w_trie_node *)r, k, sizeof(t))
#define w_trie_search_value_str(r, k) w_trie_search_value_f((struct w_trie_node *)r, k, strlen(k))
#define w_trie_search_node_str(r, k) w_trie_search_node_((struct w_trie_node *)r, k, strlen(k), 0, false)
#define w_trie_set_value_str(r, k, v) w_trie_set_value((struct w_trie_node *)r, k, strlen(k), v)
#define w_trie_search_value_f(root, key, key_size) ({ \
    struct w_trie_node* matching_node = w_trie_search_node_((struct w_trie_node *)root, key, key_size, 0, false); \
    (matching_node == NULL || matching_node->value == NULL) ? NULL : matching_node->value; \
})
#define w_trie_set_value(root, key, key_size, val) ({ \
    struct w_trie_node* matching_node = w_trie_search_node_((struct w_trie_node *)root, key, key_size, 0, true); \
    (matching_node == NULL) ? false : (matching_node->value = val, true); \
})
#define w_trie_remove_value_str(r, k) w_trie_remove_value((struct w_trie_node *)r, k, strlen(k))
#define w_trie_remove_value(root, key, key_size) ({ \
    struct w_trie_node* matching_node = w_trie_search_node_((struct w_trie_node *)root, key, key_size, 0, false); \
    if (matching_node != NULL && matching_node->value != NULL) { \
        free(matching_node->value); \
        matching_node->value = NULL; \
        true; \
    } else { \
        false; \
    } \
})

#define w_trie_free_value(node) ({ \
    if (node != NULL && node->value != NULL) { \
        free(node->value); \
        node->value = NULL; \
    } \
})
#define w_trie_free_all(root) \
	w_trie_free_node_values((struct w_trie_node *)root); \
	w_trie_free_nodes((struct w_trie_node *)root); \
	free(root);

// trie management functions
w_trie_node *w_trie_search_node_(w_trie_node* root, void* key, size_t key_size, int key_position, bool create_missing_nodes);
void w_trie_free_nodes(w_trie_node* node);
void w_trie_free_node_values(w_trie_node* node);

#endif /* end of include guard WHISKER_TRIE_H */
