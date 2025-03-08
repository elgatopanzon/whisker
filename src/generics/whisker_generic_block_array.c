/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_block_array
 * @created     : Sunday Mar 02, 2025 13:48:44 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"

#include "whisker_generic_block_array.h"
// wT #include "whisker_generic_block_array_w_T.h"
//
E_WHISKER_BLOCK_ARR whisker_block_arr_create_w_T(whisker_block_arr_w_T **barr, size_t block_size)
{
	whisker_block_arr_w_T *ba;
	E_WHISKER_MEM err = whisker_mem_try_calloc(1, sizeof(whisker_block_arr_w_T), (void**)&ba);
	if (err != E_WHISKER_MEM_OK) { return E_WHISKER_BLOCK_ARR_MEM; }
	whisker_arr_void_ *blocks;
	whisker_arr_create_void_(&blocks, 0);
	ba->blocks = blocks;
	ba->block_size = block_size;
	*barr = ba;
	return E_WHISKER_BLOCK_ARR_OK;
}

void whisker_block_arr_free_w_T(whisker_block_arr_w_T *barr)
{
	for (size_t i = 0; i < barr->blocks->length; ++i)
	{
		if (barr->blocks->arr[i] != NULL)
		{
			whisker_arr_w_T *block = barr->blocks->arr[i];
			whisker_arr_free_w_T(block);
		}
	}
	whisker_arr_free_void_(barr->blocks);
	free(barr);
}

E_WHISKER_BLOCK_ARR whisker_block_arr_set_w_T(whisker_block_arr_w_T *barr, size_t index, wT value)
{
	size_t block_id = whisker_block_arr_get_block_id(barr->block_size, index);
	size_t block_offset = whisker_block_arr_get_block_offset(barr->block_size, index);
	if (barr->blocks->length < index + 1 || ((whisker_arr_w_T *)barr->blocks->arr[block_id])->arr == NULL) {
		E_WHISKER_BLOCK_ARR err = whisker_block_arr_init_block_w_T(barr, block_id);
		if (err != E_WHISKER_BLOCK_ARR_OK) { return err; }
	}
	((whisker_arr_w_T *)barr->blocks->arr[block_id])->arr[block_offset] = value;
	return E_WHISKER_BLOCK_ARR_OK;
}

E_WHISKER_BLOCK_ARR whisker_block_arr_init_block_w_T(whisker_block_arr_w_T *barr, size_t block_id)
{
	if (barr->blocks->length < block_id + 1) {
		E_WHISKER_ARR err = whisker_arr_resize_void_(barr->blocks, block_id + 1, true);
		if (err != E_WHISKER_ARR_OK) { return E_WHISKER_BLOCK_ARR_MEM; }
	}
	if (barr->blocks->arr[block_id] == NULL) {
		whisker_arr_w_T *block;
		E_WHISKER_ARR err = whisker_arr_create_w_T(&block, barr->block_size);
		if (err != E_WHISKER_ARR_OK) { return E_WHISKER_BLOCK_ARR_MEM; }
		barr->blocks->arr[block_id] = block;
	}
	return E_WHISKER_BLOCK_ARR_OK;
}

wT whisker_block_arr_get_w_T(whisker_block_arr_w_T *barr, size_t index)
{
	size_t block_id = whisker_block_arr_get_block_id(barr->block_size, index);
	size_t block_offset = whisker_block_arr_get_block_offset(barr->block_size, index);
	return ((whisker_arr_w_T *)barr->blocks->arr[block_id])->arr[block_offset];
}

bool whisker_block_arr_contains_w_T(whisker_block_arr_w_T *barr, size_t index)
{
	size_t block_id = whisker_block_arr_get_block_id(barr->block_size, index);
	return barr->blocks->length > block_id + 1 && ((whisker_arr_w_T *)barr->blocks->arr[block_id])->arr != NULL;
}
