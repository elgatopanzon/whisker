/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_sparse_set
 * @created     : Tuesday Feb 25, 2025 18:32:05 CST
 */

#include "whisker_std.h"
#include "whisker_array.h"
#include "whisker_block_array.h"
#include "whisker_trie.h"

#ifndef WHISKER_SPARSE_SET_H
#define WHISKER_SPARSE_SET_H

#define WHISKER_SPARSE_SET_BLOCK_SIZE 2048

// errors
typedef enum E_WHISKER_SS  
{
	E_WHISKER_SS_OK = 0,
	E_WHISKER_SS_UNKNOWN = 1,
	E_WHISKER_SS_MEM = 2,
	E_WHISKER_SS_ARR = 3,
} E_WHISKER_SS;
extern const char* E_WHISKER_SS_STR[];

typedef struct whisker_sparse_set
{
	whisker_block_array *sparse;
	void *dense;
	uint64_t *dense_index;
	whisker_trie *sparse_trie;
} whisker_sparse_set;

#define whisker_ss_create(ss, t) whisker_ss_create_f(ss, sizeof(t))

// short macros
#define wss_create whisker_ss_create
#define wss_free whisker_ss_free
#define wss_set whisker_ss_set
#define wss_get whisker_ss_get
#define wss_remove whisker_ss_remove
#define wss_contains whisker_ss_contains

// management functions
E_WHISKER_SS whisker_ss_create_f(whisker_sparse_set **ss, size_t element_size);
void whisker_ss_free(whisker_sparse_set *ss);

// operation functions
E_WHISKER_SS whisker_ss_set(whisker_sparse_set *ss, uint64_t index, void *value);
void* whisker_ss_get(whisker_sparse_set *ss, uint64_t index, bool create);
E_WHISKER_SS whisker_ss_remove(whisker_sparse_set *ss, uint64_t index);
bool whisker_ss_contains(whisker_sparse_set *ss, uint64_t index);

E_WHISKER_SS whisker_ss_set_dense_index(whisker_sparse_set *ss, uint64_t index, uint64_t dense_index);
uint64_t whisker_ss_get_dense_index(whisker_sparse_set *ss, uint64_t index);

#endif /* WHISKER_SPARSE_SET_H */

