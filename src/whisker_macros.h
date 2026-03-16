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

// integer packing macros (left in high bits, right in low bits)

// pack two uint16_t into uint32_t
#define w_pack_u16_u16(left, right) \
	(((uint32_t)(left) << 16) | ((uint32_t)(right) & 0xFFFF))
#define w_unpack_u16_left(packed)  ((uint16_t)(((packed) >> 16) & 0xFFFF))
#define w_unpack_u16_right(packed) ((uint16_t)((packed) & 0xFFFF))

// pack two uint32_t into uint64_t
#define w_pack_u32_u32(left, right) \
	(((uint64_t)(left) << 32) | ((uint64_t)(right) & 0xFFFFFFFF))
#define w_unpack_u32_left(packed)  ((uint32_t)(((packed) >> 32) & 0xFFFFFFFF))
#define w_unpack_u32_right(packed) ((uint32_t)((packed) & 0xFFFFFFFF))

// pack four uint8_t into uint32_t (a=highest, d=lowest)
#define w_pack_u8_u8_u8_u8(a, b, c, d) \
	(((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | \
	 ((uint32_t)(c) << 8) | ((uint32_t)(d) & 0xFF))
#define w_unpack_u8_0(packed) ((uint8_t)(((packed) >> 24) & 0xFF))
#define w_unpack_u8_1(packed) ((uint8_t)(((packed) >> 16) & 0xFF))
#define w_unpack_u8_2(packed) ((uint8_t)(((packed) >> 8) & 0xFF))
#define w_unpack_u8_3(packed) ((uint8_t)((packed) & 0xFF))

// pack two uint8_t into uint16_t
#define w_pack_u8_u8(left, right) \
	(((uint16_t)(left) << 8) | ((uint16_t)(right) & 0xFF))
#define w_unpack_u8_left(packed)  ((uint8_t)(((packed) >> 8) & 0xFF))
#define w_unpack_u8_right(packed) ((uint8_t)((packed) & 0xFF))

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

