/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_sparse_bitset
 * @created     : Monday Mar 02, 2026 13:57:46 CST
 */

#include "whisker_std.h"

#include "whisker_sparse_bitset.h"

#include <immintrin.h>

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

#pragma GCC push_options
#pragma GCC target("avx2")

// batch allocation block size - check capacity every N elements
#define INTERSECT_ALLOC_BLOCK 1024

uint64_t w_sparse_bitset_intersect_cache_stale(struct w_sparse_bitset_intersect_cache *intersect_cache)
{
	// compute cached generation from bitsets
	uint64_t bitsets_generation = 0;
	for (uint64_t i = 0; i < intersect_cache->bitsets_length; i++)
	{
		bitsets_generation += intersect_cache->bitsets[i]->generation;
	}

	// if generation didn't change, cache already exists
	if (bitsets_generation == intersect_cache->cache_generation) return UINT64_MAX;

	return bitsets_generation;
}

uint64_t w_sparse_bitset_intersect(struct w_sparse_bitset_intersect_cache *intersect_cache)
{
	// init cache if we need to
	if (!intersect_cache->indexes) {
		w_array_init_t(intersect_cache->indexes, 0);
		intersect_cache->indexes_length = 0;
		intersect_cache->cache_generation = UINT64_MAX;
	}

	// if bitsets is invalid or 0 then early out
	if (!intersect_cache->bitsets || intersect_cache->bitsets_length == 0) return 0;

	// compute cached generation from bitsets
	uint64_t bitsets_generation = w_sparse_bitset_intersect_cache_stale(intersect_cache);
	if (bitsets_generation == UINT64_MAX) return intersect_cache->indexes_length;

	// pre-allocate initial capacity
	w_array_ensure_alloc_block_size(intersect_cache->indexes, INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);

	// local count for hot path - no per-element capacity check
	uint64_t count = 0;
	uint64_t capacity = intersect_cache->indexes_size / sizeof(uint64_t);
	uint64_t *indexes = intersect_cache->indexes;

	// single bitset: return all set bits
	if (intersect_cache->bitsets_length == 1)
	{
		struct w_sparse_bitset *bs = intersect_cache->bitsets[0];
		for (uint64_t li = 0; li < bs->lookup_pages_length; li++)
		{
			uint64_t lword = bs->lookup_pages[li];
			while (lword)
			{
				int lbit = __builtin_ctzll(lword);
				lword &= lword - 1;
				uint64_t page_index = li * 64 + (uint64_t)lbit;
				if (page_index >= bs->pages_length) continue;
				struct w_sparse_bitset_page *page = &bs->pages[page_index];
				if (!page->bits) continue;
				for (uint32_t w = page->first_set; w <= page->last_set; w++)
				{
					uint64_t word = page->bits[w];
					while (word)
					{
						int bit = __builtin_ctzll(word);
						word &= word - 1;
						if (count >= capacity) {
							w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
							indexes = intersect_cache->indexes;
							capacity = intersect_cache->indexes_size / sizeof(uint64_t);
						}
						indexes[count++] = (page_index * bs->page_size_ + w) * 64 + (uint64_t)bit;
					}
				}
			}
		}
		intersect_cache->indexes_length = count;
		intersect_cache->cache_generation = bitsets_generation;
		return count;
	}

	// find minimum lookup_pages_length across all bitsets
	uint64_t max_li = intersect_cache->bitsets[0]->lookup_pages_length;
	for (uint64_t i = 1; i < intersect_cache->bitsets_length; i++)
	{
		if (intersect_cache->bitsets[i]->lookup_pages_length < max_li)
			max_li = intersect_cache->bitsets[i]->lookup_pages_length;
	}

	// fast path for 2-way intersection (most common case)
	if (intersect_cache->bitsets_length == 2)
	{
		struct w_sparse_bitset *a = intersect_cache->bitsets[0];
		struct w_sparse_bitset *b = intersect_cache->bitsets[1];
		uint64_t page_size = a->page_size_;

		for (uint64_t li = 0; li < max_li; li++)
		{
			uint64_t lword = a->lookup_pages[li] & b->lookup_pages[li];
			while (lword)
			{
				int lbit = __builtin_ctzll(lword);
				lword &= lword - 1;
				uint64_t page_index = li * 64 + (uint64_t)lbit;
				if (page_index >= a->pages_length || page_index >= b->pages_length) continue;
				struct w_sparse_bitset_page *pa = &a->pages[page_index];
				struct w_sparse_bitset_page *pb = &b->pages[page_index];
				if (!pa->bits || !pb->bits) continue;

				uint32_t first = pa->first_set > pb->first_set ? pa->first_set : pb->first_set;
				uint32_t last = pa->last_set < pb->last_set ? pa->last_set : pb->last_set;
				if (first > last || first == UINT32_MAX) continue;

				uint32_t simd_first = (first + 3) & ~3u;
				uint32_t simd_last = (last + 1) & ~3u;

				// prefix scalar
				for (uint32_t w = first; w < simd_first && w <= last; w++)
				{
					uint64_t word = pa->bits[w] & pb->bits[w];
					while (word)
					{
						int bit = __builtin_ctzll(word);
						word &= word - 1;
						if (count >= capacity) {
							w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
							indexes = intersect_cache->indexes;
							capacity = intersect_cache->indexes_size / sizeof(uint64_t);
						}
						indexes[count++] = (page_index * page_size + w) * 64 + (uint64_t)bit;
					}
				}

				// SIMD loop
				for (uint32_t w = simd_first; w < simd_last; w += 4)
				{
					__m256i va = _mm256_loadu_si256((__m256i*)&pa->bits[w]);
					__m256i vb = _mm256_loadu_si256((__m256i*)&pb->bits[w]);
					__m256i vr = _mm256_and_si256(va, vb);

					uint64_t r0 = _mm256_extract_epi64(vr, 0);
					uint64_t r1 = _mm256_extract_epi64(vr, 1);
					uint64_t r2 = _mm256_extract_epi64(vr, 2);
					uint64_t r3 = _mm256_extract_epi64(vr, 3);

					if (r0) {
						uint32_t ww = w;
						while (r0) {
							int bit = __builtin_ctzll(r0);
							r0 &= r0 - 1;
							if (count >= capacity) {
								w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
								indexes = intersect_cache->indexes;
								capacity = intersect_cache->indexes_size / sizeof(uint64_t);
							}
							indexes[count++] = (page_index * page_size + ww) * 64 + (uint64_t)bit;
						}
					}
					if (r1) {
						uint32_t ww = w + 1;
						while (r1) {
							int bit = __builtin_ctzll(r1);
							r1 &= r1 - 1;
							if (count >= capacity) {
								w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
								indexes = intersect_cache->indexes;
								capacity = intersect_cache->indexes_size / sizeof(uint64_t);
							}
							indexes[count++] = (page_index * page_size + ww) * 64 + (uint64_t)bit;
						}
					}
					if (r2) {
						uint32_t ww = w + 2;
						while (r2) {
							int bit = __builtin_ctzll(r2);
							r2 &= r2 - 1;
							if (count >= capacity) {
								w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
								indexes = intersect_cache->indexes;
								capacity = intersect_cache->indexes_size / sizeof(uint64_t);
							}
							indexes[count++] = (page_index * page_size + ww) * 64 + (uint64_t)bit;
						}
					}
					if (r3) {
						uint32_t ww = w + 3;
						while (r3) {
							int bit = __builtin_ctzll(r3);
							r3 &= r3 - 1;
							if (count >= capacity) {
								w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
								indexes = intersect_cache->indexes;
								capacity = intersect_cache->indexes_size / sizeof(uint64_t);
							}
							indexes[count++] = (page_index * page_size + ww) * 64 + (uint64_t)bit;
						}
					}
				}

				// suffix scalar (start at max of simd_last, simd_first to avoid prefix overlap)
				uint32_t suffix_start = simd_last >= simd_first ? simd_last : simd_first;
				for (uint32_t w = suffix_start; w <= last; w++)
				{
					uint64_t word = pa->bits[w] & pb->bits[w];
					while (word)
					{
						int bit = __builtin_ctzll(word);
						word &= word - 1;
						if (count >= capacity) {
							w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
							indexes = intersect_cache->indexes;
							capacity = intersect_cache->indexes_size / sizeof(uint64_t);
						}
						indexes[count++] = (page_index * page_size + w) * 64 + (uint64_t)bit;
					}
				}
			}
		}

		intersect_cache->indexes_length = count;
		intersect_cache->cache_generation = bitsets_generation;
		return count;
	}

	// n-way intersection (3+ bitsets)
	for (uint64_t li = 0; li < max_li; li++)
	{
		// AND all lookup pages together
		uint64_t lword = intersect_cache->bitsets[0]->lookup_pages[li];
		for (uint64_t i = 1; i < intersect_cache->bitsets_length && lword; i++)
		{
			lword &= intersect_cache->bitsets[i]->lookup_pages[li];
		}

		while (lword)
		{
			int lbit = __builtin_ctzll(lword);
			lword &= lword - 1;
			uint64_t page_index = li * 64 + (uint64_t)lbit;

			// check all bitsets have this page
			bool valid = true;
			for (uint64_t i = 0; i < intersect_cache->bitsets_length; i++)
			{
				if (page_index >= intersect_cache->bitsets[i]->pages_length ||
				    !intersect_cache->bitsets[i]->pages[page_index].bits)
				{
					valid = false;
					break;
				}
			}
			if (!valid) continue;

			// find tightest bounds: max of first_set, min of last_set
			uint32_t first = intersect_cache->bitsets[0]->pages[page_index].first_set;
			uint32_t last = intersect_cache->bitsets[0]->pages[page_index].last_set;
			for (uint64_t i = 1; i < intersect_cache->bitsets_length; i++)
			{
				struct w_sparse_bitset_page *p = &intersect_cache->bitsets[i]->pages[page_index];
				if (p->first_set > first) first = p->first_set;
				if (p->last_set < last) last = p->last_set;
			}
			if (first > last || first == UINT32_MAX) continue;

			uint64_t page_size = intersect_cache->bitsets[0]->page_size_;
			uint32_t simd_first = (first + 3) & ~3u;
			uint32_t simd_last = (last + 1) & ~3u;

			// prefix scalar
			for (uint32_t w = first; w < simd_first && w <= last; w++)
			{
				uint64_t word = intersect_cache->bitsets[0]->pages[page_index].bits[w];
				for (uint64_t i = 1; i < intersect_cache->bitsets_length; i++)
				{
					word &= intersect_cache->bitsets[i]->pages[page_index].bits[w];
				}
				while (word)
				{
					int bit = __builtin_ctzll(word);
					word &= word - 1;
					if (count >= capacity) {
						w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
						indexes = intersect_cache->indexes;
						capacity = intersect_cache->indexes_size / sizeof(uint64_t);
					}
					indexes[count++] = (page_index * page_size + w) * 64 + (uint64_t)bit;
				}
			}

			// SIMD loop
			for (uint32_t w = simd_first; w < simd_last; w += 4)
			{
				__m256i vr = _mm256_loadu_si256((__m256i*)&intersect_cache->bitsets[0]->pages[page_index].bits[w]);
				for (uint64_t i = 1; i < intersect_cache->bitsets_length; i++)
				{
					__m256i vi = _mm256_loadu_si256((__m256i*)&intersect_cache->bitsets[i]->pages[page_index].bits[w]);
					vr = _mm256_and_si256(vr, vi);
				}

				uint64_t r0 = _mm256_extract_epi64(vr, 0);
				uint64_t r1 = _mm256_extract_epi64(vr, 1);
				uint64_t r2 = _mm256_extract_epi64(vr, 2);
				uint64_t r3 = _mm256_extract_epi64(vr, 3);

				if (r0) {
					uint32_t ww = w;
					while (r0) {
						int bit = __builtin_ctzll(r0);
						r0 &= r0 - 1;
						if (count >= capacity) {
							w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
							indexes = intersect_cache->indexes;
							capacity = intersect_cache->indexes_size / sizeof(uint64_t);
						}
						indexes[count++] = (page_index * page_size + ww) * 64 + (uint64_t)bit;
					}
				}
				if (r1) {
					uint32_t ww = w + 1;
					while (r1) {
						int bit = __builtin_ctzll(r1);
						r1 &= r1 - 1;
						if (count >= capacity) {
							w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
							indexes = intersect_cache->indexes;
							capacity = intersect_cache->indexes_size / sizeof(uint64_t);
						}
						indexes[count++] = (page_index * page_size + ww) * 64 + (uint64_t)bit;
					}
				}
				if (r2) {
					uint32_t ww = w + 2;
					while (r2) {
						int bit = __builtin_ctzll(r2);
						r2 &= r2 - 1;
						if (count >= capacity) {
							w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
							indexes = intersect_cache->indexes;
							capacity = intersect_cache->indexes_size / sizeof(uint64_t);
						}
						indexes[count++] = (page_index * page_size + ww) * 64 + (uint64_t)bit;
					}
				}
				if (r3) {
					uint32_t ww = w + 3;
					while (r3) {
						int bit = __builtin_ctzll(r3);
						r3 &= r3 - 1;
						if (count >= capacity) {
							w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
							indexes = intersect_cache->indexes;
							capacity = intersect_cache->indexes_size / sizeof(uint64_t);
						}
						indexes[count++] = (page_index * page_size + ww) * 64 + (uint64_t)bit;
					}
				}
			}

			// suffix scalar
			uint32_t suffix_start = simd_last >= simd_first ? simd_last : simd_first;
			for (uint32_t w = suffix_start; w <= last; w++)
			{
				uint64_t word = intersect_cache->bitsets[0]->pages[page_index].bits[w];
				for (uint64_t i = 1; i < intersect_cache->bitsets_length; i++)
				{
					word &= intersect_cache->bitsets[i]->pages[page_index].bits[w];
				}
				while (word)
				{
					int bit = __builtin_ctzll(word);
					word &= word - 1;
					if (count >= capacity) {
						w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
						indexes = intersect_cache->indexes;
						capacity = intersect_cache->indexes_size / sizeof(uint64_t);
					}
					indexes[count++] = (page_index * page_size + w) * 64 + (uint64_t)bit;
				}
			}
		}
	}

	intersect_cache->indexes_length = count;
	intersect_cache->cache_generation = bitsets_generation;
	return count;
}

#pragma GCC pop_options

void w_sparse_bitset_intersect_free_cache(struct w_sparse_bitset_intersect_cache *intersect_cache)
{
	free_null(intersect_cache->bitsets);
	free_null(intersect_cache->indexes);
	intersect_cache->cache_generation = 0;
}
