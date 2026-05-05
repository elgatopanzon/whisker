/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_sparse_bitset
 * @created     : Monday Mar 02, 2026 13:57:46 CST
 */

#include "whisker_std.h"

#include "whisker_sparse_bitset.h"

#if defined(__AVX2__) && !defined(__EMSCRIPTEN__)
#include <immintrin.h>
#endif

void w_sparse_bitset_init(struct w_sparse_bitset *bitset, struct w_arena *arena, uint8_t page_shift)
{
	bitset->page_shift_ = page_shift;
	bitset->page_mask_ = (1ULL << page_shift) - 1;
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
	uint64_t page_index = w_sparse_bitset_page_index(word_index, bitset->page_shift_);
	uint64_t page_lookup_index = w_sparse_bitset_page_index(page_index, 6);
	
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
	uint64_t page_index = w_sparse_bitset_page_index(word_index, bitset->page_shift_);
	uint64_t page_lookup_index = w_sparse_bitset_page_index(page_index, 6);

	w_sparse_bitset_ensure_capacity_(bitset, index);

	// proceed to set bits
	uint32_t local_word = w_sparse_bitset_local_word(word_index, bitset->page_mask_);
	struct w_sparse_bitset_page *page = &bitset->pages[page_index];

	// allocate page if page is fresh
	if (!page->bits)
	{
		page->bits = w_arena_calloc(bitset->arena, (1ULL << bitset->page_shift_) * sizeof(*page->bits));
	}

	// set actual bits - only increment generation when state actually changes
	uint64_t mask = w_sparse_bitset_bit_mask(index);
	bool was_clear = !(page->bits[local_word] & mask);
	page->bits[local_word] |= mask;

	// update page metadata
	if (local_word < page->first_set) page->first_set = local_word;
	if (local_word > page->last_set) page->last_set = local_word;

	// set lookup bit
	bitset->lookup_pages[page_lookup_index] |= w_sparse_bitset_bit_mask(page_index);
	if (was_clear) bitset->generation++;
}

void w_sparse_bitset_clear(struct w_sparse_bitset *bitset, uint64_t index)
{
	uint64_t word_index = w_sparse_bitset_word_index(index);
	uint64_t page_index = w_sparse_bitset_page_index(word_index, bitset->page_shift_);

	// early out
	if (page_index >= bitset->pages_length) return;

	struct w_sparse_bitset_page *page = &bitset->pages[page_index];

	if (!page->bits) return;

	uint32_t local_word = w_sparse_bitset_local_word(word_index, bitset->page_mask_);

	// only increment generation when state actually changes
	bool was_set = (page->bits[local_word] & w_sparse_bitset_bit_mask(index)) != 0;
	page->bits[local_word] &= w_sparse_bitset_bit_clear_mask(index);

	// clear lookup bit if page is empty
	bool page_empty = true;
	for (uint32_t i = page->first_set; i <= page->last_set; i++)
	{
		if (page->bits[i]) { page_empty = false; break; }
	}
	if (page_empty)
	{
		uint64_t page_lookup_index = w_sparse_bitset_page_index(page_index, 6);
		bitset->lookup_pages[page_lookup_index] &= w_sparse_bitset_bit_clear_mask(page_index);
		page->first_set = UINT32_MAX;
		page->last_set = 0;
	}
	if (was_set) bitset->generation++;
}

bool w_sparse_bitset_get(struct w_sparse_bitset *bitset, uint64_t index)
{
	uint64_t word_index = w_sparse_bitset_word_index(index);
	uint64_t page_index = w_sparse_bitset_page_index(word_index, bitset->page_shift_);

	// early out
	if (page_index >= bitset->pages_length) return false;

	struct w_sparse_bitset_page *page = &bitset->pages[page_index];

	if (!page->bits) return false;

	uint32_t local_word = w_sparse_bitset_local_word(word_index, bitset->page_mask_);

	return (page->bits[local_word] & w_sparse_bitset_bit_mask(index)) != 0;
}

#if defined(__AVX2__) && !defined(__EMSCRIPTEN__)
#pragma GCC push_options
#pragma GCC target("avx2")
#endif

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

	// include exclude bitsets in generation tracking
	for (uint64_t i = 0; i < intersect_cache->exclude_bitsets_length; i++)
	{
		if (intersect_cache->exclude_bitsets[i])
			bitsets_generation += intersect_cache->exclude_bitsets[i]->generation;
	}

	// if generation didn't change, cache already exists
	if (bitsets_generation == intersect_cache->cache_generation) return UINT64_MAX;

	return bitsets_generation;
}

// apply exclude bitsets (AND-NOT) to a word at a given page and word index
static inline uint64_t w_sparse_bitset_apply_excludes_(
	struct w_sparse_bitset_intersect_cache *cache,
	uint64_t word, uint64_t page_index, uint32_t w)
{
	for (uint64_t i = 0; i < cache->exclude_bitsets_length && word; i++)
	{
		struct w_sparse_bitset *ex = cache->exclude_bitsets[i];
		if (!ex) continue;
		if (page_index >= ex->pages_length) continue;
		struct w_sparse_bitset_page *page = &ex->pages[page_index];
		if (!page->bits) continue;
		word &= ~page->bits[w];
	}
	return word;
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

	// if any input bitset is NULL, clear result and return 0
	for (uint64_t i = 0; i < intersect_cache->bitsets_length; i++)
	{
		if (!intersect_cache->bitsets[i])
		{
			intersect_cache->indexes_length = 0;
			intersect_cache->cache_generation = 0;
			return 0;
		}
	}

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
					word = w_sparse_bitset_apply_excludes_(intersect_cache, word, page_index, w);
					while (word)
					{
						int bit = __builtin_ctzll(word);
						word &= word - 1;
						if (count >= capacity) {
							w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
							indexes = intersect_cache->indexes;
							capacity = intersect_cache->indexes_size / sizeof(uint64_t);
						}
						indexes[count++] = ((page_index << bs->page_shift_) + w) * 64 + (uint64_t)bit;
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
		uint8_t page_shift = a->page_shift_;

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

	#if defined(__AVX2__) && !defined(__EMSCRIPTEN__)
				uint32_t simd_first = (first + 3) & ~3u;
				uint32_t simd_last = (last + 1) & ~3u;

				// prefix scalar
				for (uint32_t w = first; w < simd_first && w <= last; w++)
				{
					uint64_t word = pa->bits[w] & pb->bits[w];
					word = w_sparse_bitset_apply_excludes_(intersect_cache, word, page_index, w);
					while (word)
					{
						int bit = __builtin_ctzll(word);
						word &= word - 1;
						if (count >= capacity) {
							w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
							indexes = intersect_cache->indexes;
							capacity = intersect_cache->indexes_size / sizeof(uint64_t);
						}
						indexes[count++] = ((page_index << page_shift) + w) * 64 + (uint64_t)bit;
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

					r0 = w_sparse_bitset_apply_excludes_(intersect_cache, r0, page_index, w);
					r1 = w_sparse_bitset_apply_excludes_(intersect_cache, r1, page_index, w + 1);
					r2 = w_sparse_bitset_apply_excludes_(intersect_cache, r2, page_index, w + 2);
					r3 = w_sparse_bitset_apply_excludes_(intersect_cache, r3, page_index, w + 3);

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
							indexes[count++] = ((page_index << page_shift) + ww) * 64 + (uint64_t)bit;
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
							indexes[count++] = ((page_index << page_shift) + ww) * 64 + (uint64_t)bit;
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
							indexes[count++] = ((page_index << page_shift) + ww) * 64 + (uint64_t)bit;
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
							indexes[count++] = ((page_index << page_shift) + ww) * 64 + (uint64_t)bit;
						}
					}
				}

				// suffix scalar (start at max of simd_last, simd_first to avoid prefix overlap)
				uint32_t suffix_start = simd_last >= simd_first ? simd_last : simd_first;
				for (uint32_t w = suffix_start; w <= last; w++)
#else
				// scalar fallback for non-AVX2 platforms
				for (uint32_t w = first; w <= last; w++)
#endif
				{
					uint64_t word = pa->bits[w] & pb->bits[w];
					word = w_sparse_bitset_apply_excludes_(intersect_cache, word, page_index, w);
					while (word)
					{
						int bit = __builtin_ctzll(word);
						word &= word - 1;
						if (count >= capacity) {
							w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
							indexes = intersect_cache->indexes;
							capacity = intersect_cache->indexes_size / sizeof(uint64_t);
						}
						indexes[count++] = ((page_index << page_shift) + w) * 64 + (uint64_t)bit;
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

			uint8_t page_shift = intersect_cache->bitsets[0]->page_shift_;
#if defined(__AVX2__) && !defined(__EMSCRIPTEN__)
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
				word = w_sparse_bitset_apply_excludes_(intersect_cache, word, page_index, w);
				while (word)
				{
					int bit = __builtin_ctzll(word);
					word &= word - 1;
					if (count >= capacity) {
						w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
						indexes = intersect_cache->indexes;
						capacity = intersect_cache->indexes_size / sizeof(uint64_t);
					}
					indexes[count++] = ((page_index << page_shift) + w) * 64 + (uint64_t)bit;
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

				r0 = w_sparse_bitset_apply_excludes_(intersect_cache, r0, page_index, w);
				r1 = w_sparse_bitset_apply_excludes_(intersect_cache, r1, page_index, w + 1);
				r2 = w_sparse_bitset_apply_excludes_(intersect_cache, r2, page_index, w + 2);
				r3 = w_sparse_bitset_apply_excludes_(intersect_cache, r3, page_index, w + 3);

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
						indexes[count++] = ((page_index << page_shift) + ww) * 64 + (uint64_t)bit;
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
						indexes[count++] = ((page_index << page_shift) + ww) * 64 + (uint64_t)bit;
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
						indexes[count++] = ((page_index << page_shift) + ww) * 64 + (uint64_t)bit;
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
						indexes[count++] = ((page_index << page_shift) + ww) * 64 + (uint64_t)bit;
					}
				}
			}

			// suffix scalar
			uint32_t suffix_start = simd_last >= simd_first ? simd_last : simd_first;
			for (uint32_t w = suffix_start; w <= last; w++)
#else
			// scalar fallback for non-AVX2 platforms
			for (uint32_t w = first; w <= last; w++)
#endif
			{
				uint64_t word = intersect_cache->bitsets[0]->pages[page_index].bits[w];
				for (uint64_t i = 1; i < intersect_cache->bitsets_length; i++)
				{
					word &= intersect_cache->bitsets[i]->pages[page_index].bits[w];
				}
				word = w_sparse_bitset_apply_excludes_(intersect_cache, word, page_index, w);
				while (word)
				{
					int bit = __builtin_ctzll(word);
					word &= word - 1;
					if (count >= capacity) {
						w_array_ensure_alloc_block_size(intersect_cache->indexes, count + INTERSECT_ALLOC_BLOCK, INTERSECT_ALLOC_BLOCK);
						indexes = intersect_cache->indexes;
						capacity = intersect_cache->indexes_size / sizeof(uint64_t);
					}
					indexes[count++] = ((page_index << page_shift) + w) * 64 + (uint64_t)bit;
				}
			}
		}
	}

	intersect_cache->indexes_length = count;
	intersect_cache->cache_generation = bitsets_generation;
	return count;
}

#if defined(__AVX2__) && !defined(__EMSCRIPTEN__)
#pragma GCC pop_options
#endif

void w_sparse_bitset_set_contiguous_range(struct w_sparse_bitset *bitset, uint64_t start, uint64_t count)
{
	if (count == 0) return;

	uint64_t end = start + count;

	// ensure capacity for the last bit
	w_sparse_bitset_ensure_capacity_(bitset, end - 1);

	for (uint64_t pos = start; pos < end; )
	{
		uint64_t word_index = w_sparse_bitset_word_index(pos);
		uint64_t page_index = w_sparse_bitset_page_index(word_index, bitset->page_shift_);
		uint64_t page_lookup_index = w_sparse_bitset_page_index(page_index, 6);
		uint32_t local_word = w_sparse_bitset_local_word(word_index, bitset->page_mask_);
		struct w_sparse_bitset_page *page = &bitset->pages[page_index];

		// allocate page if fresh
		if (!page->bits)
			page->bits = w_arena_calloc(bitset->arena, (1ULL << bitset->page_shift_) * sizeof(*page->bits));

		// compute mask for bits within this word
		uint32_t bit_start = w_sparse_bitset_bit_index(pos);
		uint64_t remaining_in_word = 64 - bit_start;
		uint64_t bits_to_set = end - pos;
		if (bits_to_set > remaining_in_word) bits_to_set = remaining_in_word;

		uint64_t mask;
		if (bits_to_set == 64) {
			mask = ~0ULL;
		} else {
			mask = ((1ULL << bits_to_set) - 1) << bit_start;
		}

		page->bits[local_word] |= mask;

		// update page metadata
		if (local_word < page->first_set) page->first_set = local_word;
		if (local_word > page->last_set) page->last_set = local_word;

		// set lookup bit
		bitset->lookup_pages[page_lookup_index] |= w_sparse_bitset_bit_mask(page_index);

		pos += bits_to_set;
	}

	bitset->generation++;
}

void w_sparse_bitset_clear_contiguous_range(struct w_sparse_bitset *bitset, uint64_t start, uint64_t count)
{
	if (count == 0) return;

	uint64_t end = start + count;

	for (uint64_t pos = start; pos < end; )
	{
		uint64_t word_index = w_sparse_bitset_word_index(pos);
		uint64_t page_index = w_sparse_bitset_page_index(word_index, bitset->page_shift_);

		if (page_index >= bitset->pages_length) break;

		struct w_sparse_bitset_page *page = &bitset->pages[page_index];
		if (!page->bits) {
			// skip to next word boundary
			uint32_t bit_start = w_sparse_bitset_bit_index(pos);
			pos += 64 - bit_start;
			continue;
		}

		uint32_t local_word = w_sparse_bitset_local_word(word_index, bitset->page_mask_);
		uint32_t bit_start = w_sparse_bitset_bit_index(pos);
		uint64_t remaining_in_word = 64 - bit_start;
		uint64_t bits_to_clear = end - pos;
		if (bits_to_clear > remaining_in_word) bits_to_clear = remaining_in_word;

		uint64_t mask;
		if (bits_to_clear == 64) {
			mask = ~0ULL;
		} else {
			mask = ((1ULL << bits_to_clear) - 1) << bit_start;
		}

		page->bits[local_word] &= ~mask;

		pos += bits_to_clear;
	}

	// recompute page metadata for affected pages
	for (uint64_t pos = start; pos < end; )
	{
		uint64_t word_index = w_sparse_bitset_word_index(pos);
		uint64_t page_index = w_sparse_bitset_page_index(word_index, bitset->page_shift_);

		if (page_index >= bitset->pages_length) break;

		struct w_sparse_bitset_page *page = &bitset->pages[page_index];
		if (!page->bits) {
			pos += 64;
			continue;
		}

		// check if page is empty
		bool page_empty = true;
		for (uint32_t i = page->first_set; i <= page->last_set && page->first_set != UINT32_MAX; i++)
		{
			if (page->bits[i]) { page_empty = false; break; }
		}

		if (page_empty)
		{
			uint64_t page_lookup_index = w_sparse_bitset_page_index(page_index, 6);
			bitset->lookup_pages[page_lookup_index] &= w_sparse_bitset_bit_clear_mask(page_index);
			page->first_set = UINT32_MAX;
			page->last_set = 0;
		}

		// advance to next page
		uint64_t page_start_word = page_index << bitset->page_shift_;
		uint64_t next_page_start_bit = (page_start_word + (1ULL << bitset->page_shift_)) * 64;
		if (next_page_start_bit <= pos) next_page_start_bit = pos + 64;
		pos = next_page_start_bit;
	}

	bitset->generation++;
}

uint64_t w_sparse_bitset_find_contiguous_clear(struct w_sparse_bitset *bitset, uint64_t count, uint64_t max_index)
{
	if (count == 0) return 0;

	uint64_t run_start = 0;
	uint64_t run_length = 0;

	for (uint64_t i = 0; i < max_index; i++)
	{
		if (w_sparse_bitset_get(bitset, i))
		{
			run_start = i + 1;
			run_length = 0;
		}
		else
		{
			run_length++;
			if (run_length >= count)
				return run_start;
		}
	}

	return UINT64_MAX;
}

void w_sparse_bitset_intersect_free_cache(struct w_sparse_bitset_intersect_cache *intersect_cache)
{
	free_null(intersect_cache->bitsets);
	free_null(intersect_cache->exclude_bitsets);
	free_null(intersect_cache->indexes);
	intersect_cache->cache_generation = 0;
}
