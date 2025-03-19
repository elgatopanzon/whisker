/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_trie
 * @created     : Thursday Feb 06, 2025 15:14:58 CST
 */

#include <string.h>
#include <stdio.h>
#include "whisker_trie.h"
#include "whisker_debug.h"

// internal function to recursively traverse the tree looking for a matching
// node for a given key, accepting position
whisker_trie *whisker_trie_search_node_(whisker_trie* root, void* key, size_t key_size, int key_position, bool create_missing_nodes)
{
	// loop over the key starting at the current key position
	// each part of the loop checks for the node's matching index
	for (size_t i = key_position; i < key_size; ++i)
	{
		int key_index = ((unsigned char*)key)[i];

		// if the key index is null, the search ends here if
		// create_missing_nodes is false
		if (root->nodes[key_index] == NULL && !create_missing_nodes)
		{
			return NULL;
		}

		// if create_missing_nodes is true, we create this node and move on!
		if (root->nodes[key_index] == NULL && create_missing_nodes)
		{
			whisker_trie* missing_node = whisker_mem_xcalloc_t(1, whisker_trie);

			root->nodes[key_index] = missing_node;
		}

		return whisker_trie_search_node_(root->nodes[key_index], key, key_size, i + 1, create_missing_nodes);
	}

	// check if this is the end of the key
	// if it is, assign the current node to it because that means it's a match
	if (key_position >= key_size)
	{
		return root;
	}

	return NULL;
}

// free a trie node and all of it's linked children
void whisker_trie_free_nodes(whisker_trie* node)
{
	// recursive free of child nodes
	whisker_trie** nodes = node->nodes;
	for (int i = 0; i < WHISKER_TRIE_NODE_CAPACITY; ++i)
	{
		if (nodes[i] != NULL)
		{
			whisker_trie_free_nodes(nodes[i]);
			free(nodes[i]);
		}
	}
}

// call free on all the node value pointers
void whisker_trie_free_node_values(whisker_trie* node)
{
	whisker_trie** nodes = node->nodes;
	for (int i = 0; i < WHISKER_TRIE_NODE_CAPACITY; ++i)
	{
		if (nodes[i] != NULL)
		{
			whisker_trie_free_node_values(nodes[i]);
		}
	}

	free_null(node->value);
}
