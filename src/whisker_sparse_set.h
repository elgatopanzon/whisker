/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_sparse_set
 * @created     : Tuesday Feb 25, 2025 18:32:05 CST
 */

#include "whisker_std.h"
#include "whisker_array.h"
#include "whisker_trie.h"

#ifndef WHISKER_SPARSE_SET_H
#define WHISKER_SPARSE_SET_H

#define WHISKER_SPARSE_SET_BLOCK_SIZE 2048
#define WHISKER_SPARSE_SET_AUTOSORT false

#define WHISKER_SPARSE_SET_RESIZE_RATIO 1.61803398875
#define WHISKER_SPARSE_SET_SPARSE_BLOCK_SIZE (1024 / sizeof(uint64_t))
#define WHISKER_SPARSE_SET_SPARSE_INDEX_BLOCK_SIZE (1024 / sizeof(uint64_t))
#define WHISKER_SPARSE_SET_DENSE_REALLOC_BLOCK_SIZE_MULTIPLIER 16384

enum WHISKER_SPARSE_SET_MUTATION_TYPE
{ 
	WHISKER_SPARSE_SET_MUTATION_TYPE_ADD,
	WHISKER_SPARSE_SET_MUTATION_TYPE_REMOVE,
	WHISKER_SPARSE_SET_MUTATION_TYPE_SWAP,
};

struct whisker_sparse_set_mutation
{
	uint64_t mutated_index;
	uint64_t mutated_sparse_index;
	enum WHISKER_SPARSE_SET_MUTATION_TYPE mutation_type;
};

typedef struct whisker_sparse_set
{
	whisker_arr_declare(uint64_t, sparse);
	whisker_arr_declare(uint64_t, sparse_index);
	whisker_arr_declare(void, dense);
	whisker_arr_declare(struct whisker_sparse_set_mutation, mutations);
	void *swap_buffer;
	size_t element_size;
	_Atomic size_t *length;
} whisker_sparse_set;

#define whisker_ss_create_t(t) whisker_ss_create_and_init_f(sizeof(t))
#define whisker_ss_create_s(s) whisker_ss_create_and_init_f(s)

// short macros
#define wss_create_t whisker_ss_create_t
#define wss_create_s whisker_ss_create_s
#define wss_free whisker_ss_free
#define wss_set whisker_ss_set
#define wss_get whisker_ss_get
#define wss_remove whisker_ss_remove
#define wss_contains whisker_ss_contains

// management functions
whisker_sparse_set *whisker_ss_create_and_init_f(size_t element_size);
void whisker_ss_init_f(whisker_sparse_set *ss, size_t element_size);
whisker_sparse_set *whisker_ss_create_f();
void whisker_ss_free(whisker_sparse_set *ss);
void whisker_ss_free_all(whisker_sparse_set *ss);

// operation functions
void whisker_ss_set(whisker_sparse_set *ss, uint64_t index, void *value);
void* whisker_ss_get(whisker_sparse_set *ss, uint64_t index);
void whisker_ss_remove(whisker_sparse_set *ss, uint64_t index);
bool whisker_ss_contains(whisker_sparse_set *ss, uint64_t index);
void whisker_ss_record_mutation(whisker_sparse_set *ss, uint64_t index_mutated, uint64_t sparse_index_mutated, enum WHISKER_SPARSE_SET_MUTATION_TYPE mutation_type);

void whisker_ss_set_dense_index(whisker_sparse_set *ss, uint64_t index, uint64_t dense_index);
void whisker_ss_init_dense_index(whisker_sparse_set *ss, uint64_t index);
uint64_t whisker_ss_get_dense_index(whisker_sparse_set *ss, uint64_t index);

void whisker_ss_sort(whisker_sparse_set *ss);

#endif /* WHISKER_SPARSE_SET_H */

