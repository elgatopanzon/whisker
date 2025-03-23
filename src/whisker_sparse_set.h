/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_sparse_set
 * @created     : Tuesday Feb 25, 2025 18:32:05 CST
 */

#include "whisker_std.h"
#include "whisker_array.h"
#include "generics/whisker_generic_array_uint64_t.h"
#include "whisker_trie.h"

#ifndef WHISKER_SPARSE_SET_H
#define WHISKER_SPARSE_SET_H

#define WHISKER_SPARSE_SET_BLOCK_SIZE 2048
#define WHISKER_SPARSE_SET_AUTOSORT false

#define WHISKER_SPARSE_SET_RESIZE_RATIO 1.61803398875
#define WHISKER_SPARSE_SET_SPARSE_BLOCK_SIZE (1024 / sizeof(uint64_t))
#define WHISKER_SPARSE_SET_SPARSE_INDEX_BLOCK_SIZE (1024 / sizeof(uint64_t))
#define WHISKER_SPARSE_SET_DENSE_REALLOC_BLOCK_SIZE_MULTIPLIER 16384

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
	whisker_arr_declare(uint64_t, sparse);
	whisker_arr_declare(uint64_t, sparse_index);
	whisker_trie *sparse_trie;
	whisker_arr_declare(void, dense);
	void *swap_buffer;
	size_t element_size;
	size_t *length;
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

