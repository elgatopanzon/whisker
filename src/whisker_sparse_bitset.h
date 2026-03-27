/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_sparse_bitset
 * @created     : Monday Mar 02, 2026 13:57:20 CST
 * @description : bitset optimised for sparsely set bits
 */

#include "whisker_std.h"
#include "whisker_arena.h"
#include "whisker_array.h"
#include "whisker_memory.h"

#ifndef WHISKER_SPARSE_BITSET_H
#define WHISKER_SPARSE_BITSET_H

#define W_SPARSE_BITSET_WORD_BITS 64
#define W_SPARSE_BITSET_PAGE_SHIFT 8
#define W_SPARSE_BITSET_PAGE_REALLOC_BLOCK_SIZE 16

// each page points to an array of words
// page size is (1 << page_shift_) words
struct w_sparse_bitset_page 
{
	// first/last set allow specifying the first/last non-zero WORD
	uint32_t first_set;
	uint32_t last_set;
	uint64_t *bits;
};

// main bitset
struct w_sparse_bitset
{
	uint8_t page_shift_;
	uint64_t page_mask_;
	w_array_declare(struct w_sparse_bitset_page, pages);
	w_array_declare(uint64_t, lookup_pages);
	struct w_arena *arena;
	uint64_t generation;
};

// intersect cache, holds a list of indexes set across bitsets
struct w_sparse_bitset_intersect_cache
{
	w_array_declare(struct w_sparse_bitset *, bitsets);
	w_array_declare(struct w_sparse_bitset *, exclude_bitsets);
	w_array_declare(uint64_t, indexes);
	uint64_t cache_generation;
};

// iterate all set bits in a sparse bitset; provides uint64_t i as the current bit index
#define w_sparse_bitset_for_each(bs) \
    for (uint64_t _sb_li = 0; _sb_li < (bs)->lookup_pages_length; _sb_li++) \
    for (uint64_t _sb_lw = (bs)->lookup_pages[_sb_li]; _sb_lw; _sb_lw &= _sb_lw - 1) \
    for (uint64_t _sb_pi = _sb_li * 64ULL + (uint64_t)__builtin_ctzll(_sb_lw), \
             _sb_pg = (_sb_pi < (bs)->pages_length && (bs)->pages[_sb_pi].bits) ? 1ULL : 0ULL; \
         _sb_pg; _sb_pg = 0) \
    for (uint32_t _sb_w = (bs)->pages[_sb_pi].first_set; _sb_w <= (bs)->pages[_sb_pi].last_set; _sb_w++) \
    for (uint64_t _sb_wd = (bs)->pages[_sb_pi].bits[_sb_w]; _sb_wd; _sb_wd &= _sb_wd - 1) \
    for (uint64_t i = ((_sb_pi << (bs)->page_shift_) + (uint64_t)_sb_w) * 64ULL + (uint64_t)__builtin_ctzll(_sb_wd), \
             _sb_d = 0; !_sb_d; _sb_d = 1)

// math macros
#define w_sparse_bitset_word_index(i) ((i) >> 6)
#define w_sparse_bitset_page_index(i, shift) ((i) >> (shift))
#define w_sparse_bitset_bit_index(i) ((i) & 63)
#define w_sparse_bitset_local_word(w, mask) ((w) & (mask))
#define w_sparse_bitset_bit_mask(i) (1ULL << ((i) & 63))
#define w_sparse_bitset_bit_clear_mask(i) (~(1ULL << ((i) & 63)))

// init sparse bitset with page shift (log2 of page size)
void w_sparse_bitset_init(struct w_sparse_bitset *bitset, struct w_arena *arena, uint8_t page_shift);

// free bitset page lists
void w_sparse_bitset_free(struct w_sparse_bitset *bitset);

// set bit index
void w_sparse_bitset_set(struct w_sparse_bitset *bitset, uint64_t index);

// clear bit index
void w_sparse_bitset_clear(struct w_sparse_bitset *bitset, uint64_t index);

// check if bit index is set
bool w_sparse_bitset_get(struct w_sparse_bitset *bitset, uint64_t index);

// write intersecting IDs to a result struct from multiple intersecting bitsets
// writes into a cache struct, will init if not already setup
// returns total count of intersections found, 0 for none or failed
uint64_t w_sparse_bitset_intersect(struct w_sparse_bitset_intersect_cache *intersect_cache);

// free indexes and reset counts
void w_sparse_bitset_intersect_free_cache(struct w_sparse_bitset_intersect_cache *intersect_cache);

// check if bitset intersect cache is stale
uint64_t w_sparse_bitset_intersect_cache_stale(struct w_sparse_bitset_intersect_cache *intersect_cache);

// set a contiguous range of bits [start, start+count)
void w_sparse_bitset_set_contiguous_range(struct w_sparse_bitset *bitset, uint64_t start, uint64_t count);

// clear a contiguous range of bits [start, start+count)
void w_sparse_bitset_clear_contiguous_range(struct w_sparse_bitset *bitset, uint64_t start, uint64_t count);

// find first contiguous run of cleared bits with length >= count
// returns start index of the run, or UINT64_MAX if not found
// searches from 0 up to max_index (exclusive)
uint64_t w_sparse_bitset_find_contiguous_clear(struct w_sparse_bitset *bitset, uint64_t count, uint64_t max_index);

#endif /* WHISKER_SPARSE_BITSET_H */

