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

enum W_SPARSE_SET_MUTATION_TYPE
{ 
	WHISKER_SPARSE_SET_MUTATION_TYPE_ADD,
	WHISKER_SPARSE_SET_MUTATION_TYPE_REMOVE,
	WHISKER_SPARSE_SET_MUTATION_TYPE_SWAP,
};

struct w_sparse_set_mutation
{
	uint64_t mutated_index;
	uint64_t mutated_sparse_index;
	enum W_SPARSE_SET_MUTATION_TYPE mutation_type;
};

typedef struct w_sparse_set
{
	w_array_declare(uint64_t, sparse);
	w_array_declare(uint64_t, sparse_index);
	w_array_declare(void, dense);
	w_array_declare(struct w_sparse_set_mutation, mutations);
	void *swap_buffer;
	size_t element_size;
	_Atomic size_t *length;
} w_sparse_set;

#define w_sparse_set_create_t(t) w_sparse_set_create_and_init_f(sizeof(t))
#define w_sparse_set_create_s(s) w_sparse_set_create_and_init_f(s)

// management functions
w_sparse_set *w_sparse_set_create_and_init_f(size_t element_size);
void w_sparse_set_init_f(w_sparse_set *ss, size_t element_size);
w_sparse_set *w_sparse_set_create_f();
void w_sparse_set_free(w_sparse_set *ss);
void w_sparse_set_free_all(w_sparse_set *ss);

// operation functions
void w_sparse_set_set(w_sparse_set *ss, uint64_t index, void *value);
void* w_sparse_set_get(w_sparse_set *ss, uint64_t index);
void w_sparse_set_remove(w_sparse_set *ss, uint64_t index);
bool w_sparse_set_contains(w_sparse_set *ss, uint64_t index);
void w_sparse_set_mutation(w_sparse_set *ss, uint64_t index_mutated, uint64_t sparse_index_mutated, enum W_SPARSE_SET_MUTATION_TYPE mutation_type);

void w_sparse_set_set_dense_index_(w_sparse_set *ss, uint64_t index, uint64_t dense_index);
void w_sparse_set_init_dense_index_(w_sparse_set *ss, uint64_t index);
uint64_t w_sparse_set_get_dense_index_(w_sparse_set *ss, uint64_t index);

void w_sparse_set_sort(w_sparse_set *ss);

#endif /* WHISKER_SPARSE_SET_H */

