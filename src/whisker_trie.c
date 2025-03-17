/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_trie
 * @created     : Thursday Feb 06, 2025 15:14:58 CST
 */

#include <string.h>
#include <stdio.h>
#include "whisker_memory.h"
#include "whisker_trie.h"
#include "whisker_debug.h"

const char* E_WHISKER_TRIE_STR[] = {
	[E_WHISKER_TRIE_OK]="OK",
	[E_WHISKER_TRIE_UNKNOWN]="Unknown error",
	[E_WHISKER_TRIE_MEM]="Memory error during operation",
	[E_WHISKER_TRIE_SEARCH_MISSING_ALL]="Search prefix doesn't match any node",
	[E_WHISKER_TRIE_SEARCH_MISSING_NODE]="Search doesn't match all nodes",
	[E_WHISKER_TRIE_SEARCH_MISSING_VALUE]="Search matches a node, but it has no value",
};

// creating a root node is the same as creating a child node
// 1. create and allocate struct instance
// 2. create and allocate array of children to map to offset IDs
E_WHISKER_TRIE whisker_trie_create_node(whisker_trie** node)
{
	whisker_trie *node_new;
	node_new = whisker_mem_xcalloc_t(1, *node_new);
	*node = node_new;

	return E_WHISKER_TRIE_OK;
}


// this allows to create the node instance for a node's specific child index
E_WHISKER_TRIE whisker_trie_create_child_node(whisker_trie* node, char byte, whisker_trie** child)
{
	E_WHISKER_TRIE err = whisker_trie_create_node(child);
	if (err != E_WHISKER_TRIE_OK)
	{
		return err;
	}

	// assign child to node
	whisker_trie_set_child_node(node, byte, *child);

	return E_WHISKER_TRIE_OK;
}

// set the child node instance with key
void whisker_trie_set_child_node(whisker_trie* node, char byte, whisker_trie* child)
{
	node->nodes[byte] = child;
}

// internal function to recursively traverse the tree looking for a matching
// node for a given key, accepting position
E_WHISKER_TRIE whisker_trie_search_node_(whisker_trie* root, void* key, size_t key_size, int key_position, bool create_missing_nodes, whisker_trie** match)
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
			return (key_position == 0) ? E_WHISKER_TRIE_SEARCH_MISSING_ALL : E_WHISKER_TRIE_SEARCH_MISSING_NODE;
		}

		// if create_missing_nodes is true, we create this node and move on!
		if (root->nodes[key_index] == NULL && create_missing_nodes)
		{
			whisker_trie* missing_node;
			E_WHISKER_TRIE create_err = whisker_trie_create_node(&missing_node);
			if (create_err != E_WHISKER_TRIE_OK)
			{
				return create_err;
			}

			root->nodes[key_index] = missing_node;
		}

		return whisker_trie_search_node_(root->nodes[key_index], key, key_size, i + 1, create_missing_nodes, match);
	}

	// check if this is the end of the key
	// if it is, assign the current node to it because that means it's a match
	if (key_position >= key_size)
	{
		*match = root;		

		return E_WHISKER_TRIE_OK;
	}

	return E_WHISKER_TRIE_SEARCH_MISSING_NODE;
}

// this should get the pointer to the matching node 
// this will be useful when wanting to store just the node without wanting to
// traverse the tree, but still be able to update the array pointer if required
// it will stop searching when encountering an invalid node, or when finding the
// end node
E_WHISKER_TRIE whisker_trie_search_node_str(whisker_trie* root, char* key, whisker_trie** match)
{
	return whisker_trie_search_node_(root, key, strlen(key), 0, false, match);
}

// uses search_node to traverse the tree and returns the node's value instead
E_WHISKER_TRIE whisker_trie_search_value_str_f(whisker_trie* root, char* key, void** match)
{
	return whisker_trie_search_value_f(root, key, strlen(key), match);
}

E_WHISKER_TRIE whisker_trie_search_value_f(whisker_trie* root, void* key, size_t key_size, void** match)
{
	whisker_trie* matching_node;
	E_WHISKER_TRIE search_err = whisker_trie_search_node_(root, key, key_size, 0, false, &matching_node);

	// if the node isn't a match, we certainly dont have a value
	if (search_err != E_WHISKER_TRIE_OK)
	{
		return search_err;;
	}

	// if the value is null, we also don't have a value
	if (matching_node->value == NULL)
	{
		return E_WHISKER_TRIE_SEARCH_MISSING_VALUE;
	}

	// yay, we have a value
	*match = matching_node->value;

	return E_WHISKER_TRIE_OK;
}


// setting data requires traversing the tree, creating missing nodes until the
// end of the key, then setting the value to the desired node
// loop:
// 1. check for null pointer in nodes array for adjusted ID of the char
// 2. if exists, move into it and advance the key array
//    if it doesn't exist, allocate the node with create_node(), and assign the
//    pointer to the current node, then advance into it
// 3. when the end of the key is reached, assign the node's pointer to provided
// value pointer
E_WHISKER_TRIE whisker_trie_set_value_str(whisker_trie** root, char* key, void* value)
{
	return whisker_trie_set_value(root, key, strlen(key), value);
}

E_WHISKER_TRIE whisker_trie_set_value(whisker_trie** root, void* key, size_t key_size, void* value)
{
	whisker_trie* matching_node;
	E_WHISKER_TRIE search_err = whisker_trie_search_node_(*root, key, key_size, 0, true, &matching_node);

	// if the node isn't a match, we cannot set the value
	if (search_err != E_WHISKER_TRIE_OK)
	{
		return search_err;;
	}

	matching_node->value = value;
	
	return E_WHISKER_TRIE_OK;
}

// free a trie node and all of it's linked children
E_WHISKER_TRIE whisker_trie_free_node(whisker_trie* node, bool free_values)
{
	// recursive free of child nodes
	whisker_trie** nodes = node->nodes;
	for (int i = 0; i < WHISKER_TRIE_NODE_CAPACITY; ++i)
	{
		if (nodes[i] != NULL)
		{
			E_WHISKER_TRIE err = whisker_trie_free_node(nodes[i], free_values);
			if (err != E_WHISKER_TRIE_OK)
			{
				return err;
			}
		}
	}

	if (free_values)
	{
		free(node->value);
	}
	free(node);

	return E_WHISKER_TRIE_OK;
}
