/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_macros
 * @created     : Sunday Mar 30, 2025 21:29:16 CST
 * @description : string concatenation and stringification helper macros
 */

#include "whisker_std.h"

#ifndef WHISKER_MACROS_H
#define WHISKER_MACROS_H

/////////////////////
//  string macros  //
/////////////////////

// join 2 strings A and B, either literals or other macros, into a final form of
// "AB" to use as a string literal
#define MACRO_STR(A) STR(A)
#define STR(A) #A
#define JOIN_STR_NEXT(A,B) STR(A##B)
#define JOIN_STR(A, B) JOIN_STR_NEXT(A,B)

// align pointer to alignment
#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))

// math helpers
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// branch prediction hints
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// array macros
#define ARRAY_SELECT(result_type, array, length, param, cmp_expr) \
({ \
	result_type *_result = NULL; \
	for (size_t _i = 0; _i < (length); _i++) { \
		result_type *_item = &(array)[_i]; \
		if (_item->param cmp_expr) { \
			_result = _item; \
			break; \
		} \
	} \
	_result; \
})

#endif // WHISKER_MACROS_H

