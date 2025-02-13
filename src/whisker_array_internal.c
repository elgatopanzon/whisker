/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_array_internal
 * @created     : Wednesday Feb 05, 2025 20:42:59 CST
 */

#include "whisker_array.h"
#include "whisker_array_internal.h"

// resize the array only if the stored size doesn't allow for the new size
// otherwise, increase the managed length to the new size in the header
E_WHISKER_ARR whisker_arr_try_resize_if_required_(void** arr, size_t elements)
{
	whisker_array_header_t* header = whisker_arr_header(*arr);
	if (elements * header->element_size > header->size)
	{
		E_WHISKER_ARR err = whisker_arr_resize_f(arr, elements);
		if (err != E_WHISKER_ARR_OK)
		{
			return err;
		}
	}
	else
	{
		header->length++;
	}

	return E_WHISKER_ARR_OK;
}
