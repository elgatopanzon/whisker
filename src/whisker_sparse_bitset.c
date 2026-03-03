/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_sparse_bitset
 * @created     : Monday Mar 02, 2026 13:57:46 CST
 */

#include "whisker_std.h"

#include "whisker_sparse_bitset.h"

void w_sparse_bitset_init(struct w_sparse_bitset *bitset, struct w_arena *arena, uint64_t page_size_)
{
	bitset->page_size_ = page_size_;
	bitset->arena = arena;

	w_array_init_t(bitset->pages, 0);
	bitset->pages_length = 0;

	w_array_init_t(bitset->lookup_pages, 0);
	bitset->lookup_pages_length = 0;
}

void w_sparse_bitset_free(struct w_sparse_bitset *bitset)
{
	free_null(bitset->pages);
	free_null(bitset->lookup_pages);
	bitset->arena = NULL;
	bitset->pages_length = 0;
	bitset->lookup_pages_length = 0;
}

static inline void w_sparse_bitset_ensure_capacity_(struct w_sparse_bitset *bitset, uint64_t index)
{
	// get indexes
	uint64_t word_index = w_sparse_bitset_word_index(index);
	uint64_t page_index = w_sparse_bitset_page_index(word_index, bitset->page_size_);
	uint64_t page_lookup_index = w_sparse_bitset_page_index(page_index, W_SPARSE_BITSET_WORD_BITS);
	
	// ensure pages and lookup pages capacity
	w_array_ensure_alloc_block_size(
			bitset->pages,
			page_index + 1,
			W_SPARSE_BITSET_PAGE_REALLOC_BLOCK_SIZE
		);
	if (page_index + 1 > bitset->pages_length)
	{
		for (uint64_t i = bitset->pages_length; i <= page_index; i++)
		{
			bitset->pages[i].bits = NULL;
			bitset->pages[i].first_set = UINT32_MAX;
			bitset->pages[i].last_set = 0;
		}
		bitset->pages_length = page_index + 1;
	}

	w_array_ensure_alloc_block_size(
			bitset->lookup_pages,
			page_lookup_index + 1,
			W_SPARSE_BITSET_PAGE_REALLOC_BLOCK_SIZE
		);
	if (page_lookup_index + 1 > bitset->lookup_pages_length)
	{
		for (uint64_t i = bitset->lookup_pages_length; i <= page_lookup_index; i++)
		{
			bitset->lookup_pages[i] = 0;
		}
		bitset->lookup_pages_length = page_lookup_index + 1;
	}
}

void w_sparse_bitset_set(struct w_sparse_bitset *bitset, uint64_t index)
{
	// get indexes
	uint64_t word_index = w_sparse_bitset_word_index(index);
	uint64_t page_index = w_sparse_bitset_page_index(word_index, bitset->page_size_);
	uint64_t page_lookup_index = w_sparse_bitset_page_index(page_index, W_SPARSE_BITSET_WORD_BITS);

	w_sparse_bitset_ensure_capacity_(bitset, index);

	// proceed to set bits
	uint32_t local_word = w_sparse_bitset_local_word(word_index, bitset->page_size_);
	struct w_sparse_bitset_page *page = &bitset->pages[page_index];

	// allocate page if page is fresh
	if (!page->bits)
	{
		page->bits = w_arena_calloc(bitset->arena, bitset->page_size_ * sizeof(*page->bits));
	}

	// set actual bits
	page->bits[local_word] |= w_sparse_bitset_bit_mask(index);

	// update page metadata
	if (local_word < page->first_set) page->first_set = local_word;
	if (local_word > page->last_set) page->last_set = local_word;

	// set lookup bit
	bitset->lookup_pages[page_lookup_index] |= w_sparse_bitset_bit_mask(page_index);
}

void w_sparse_bitset_clear(struct w_sparse_bitset *bitset, uint64_t index)
{
	uint64_t word_index = w_sparse_bitset_word_index(index);
	uint64_t page_index = w_sparse_bitset_page_index(word_index, bitset->page_size_);

	// early out
	if (page_index >= bitset->pages_length) return;

	struct w_sparse_bitset_page *page = &bitset->pages[page_index];

	if (!page->bits) return;

	uint32_t local_word = w_sparse_bitset_local_word(word_index, bitset->page_size_);
	page->bits[local_word] &= w_sparse_bitset_bit_clear_mask(index);

	// clear lookup bit if page is empty
	bool page_empty = true;
	for (uint32_t i = page->first_set; i <= page->last_set; i++)
	{
		if (page->bits[i]) { page_empty = false; break; }
	}
	if (page_empty)
	{
		uint64_t page_lookup_index = w_sparse_bitset_page_index(page_index, W_SPARSE_BITSET_WORD_BITS);
		bitset->lookup_pages[page_lookup_index] &= w_sparse_bitset_bit_clear_mask(page_index);
		page->first_set = UINT32_MAX;
		page->last_set = 0;
	}
}

bool w_sparse_bitset_get(struct w_sparse_bitset *bitset, uint64_t index)
{
	uint64_t word_index = w_sparse_bitset_word_index(index);
	uint64_t page_index = w_sparse_bitset_page_index(word_index, bitset->page_size_);

	// early out
	if (page_index >= bitset->pages_length) return false;

	struct w_sparse_bitset_page *page = &bitset->pages[page_index];

	if (!page->bits) return false;

	uint32_t local_word = w_sparse_bitset_local_word(word_index, bitset->page_size_);

	return (page->bits[local_word] & w_sparse_bitset_bit_mask(index)) != 0;
}
