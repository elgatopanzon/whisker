/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_sparse_set
 * @created     : Tuesday Feb 25, 2025 18:32:05 CST
 */

#include "whisker_std.h"
#include "whisker_array.h"
// #include "whisker_block_array.h"
#include "whisker_trie.h"

#ifndef WHISKER_SPARSE_SET_H
#define WHISKER_SPARSE_SET_H

#define WHISKER_SPARSE_SET_BLOCK_SIZE 2048
#define WHISKER_SPARSE_SET_AUTOSORT true

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
	uint64_t *sparse;
	size_t sparse_length;
	void *dense;
	void *swap_buffer;
	uint64_t *sparse_index;
	whisker_trie *sparse_trie;
	size_t length;
	size_t element_size;
} whisker_sparse_set;

#define whisker_ss_create_t(ss, t) whisker_ss_create_f(ss, sizeof(t))
#define whisker_ss_create_s(ss, s) whisker_ss_create_f(ss, s)

// short macros
#define wss_create_t whisker_ss_create_t
#define wss_create_s whisker_ss_create_s
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
void* whisker_ss_get(whisker_sparse_set *ss, uint64_t index);
E_WHISKER_SS whisker_ss_remove(whisker_sparse_set *ss, uint64_t index);
bool whisker_ss_contains(whisker_sparse_set *ss, uint64_t index);

E_WHISKER_SS whisker_ss_set_dense_index(whisker_sparse_set *ss, uint64_t index, uint64_t dense_index);
E_WHISKER_SS whisker_ss_init_dense_index(whisker_sparse_set *ss, uint64_t index);
uint64_t whisker_ss_get_dense_index(whisker_sparse_set *ss, uint64_t index);

void whisker_ss_sort(whisker_sparse_set *ss);

#endif /* WHISKER_SPARSE_SET_H */

