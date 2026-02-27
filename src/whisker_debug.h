/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_debug
 * @created     : Friday Feb 14, 2025 19:13:56 CST
 * @description : debug logging, tracing, and typed assertion macros
 */

#include "whisker_std.h"

#ifndef WHISKER_DEBUG_H
#define WHISKER_DEBUG_H

#ifndef NDEBUG
#define trace_printf(fmt, ...) \
    fprintf(stderr, "[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define debug_printf(fmt, ...) \
	debug_log(DEBUG, _, fmt, ##__VA_ARGS__)

#define debug_log(log_level, log_module, log_format, ...) \
    fprintf(stderr, "%s | %s | " log_format "\n", #log_level, #log_module, ##__VA_ARGS__)
#define trace_log(log_level, log_module, log_format, ...) \
    fprintf(stderr, "%s | %s | " log_format, #log_level, #log_module, ##__VA_ARGS__); \
    fprintf(stderr, " [%s:%d]\n", __FILE__, __LINE__)

#define w_assert(exp) \
	w_assert_eq(exp, true)
#define w_assert_eq(a, b) \
	if (a != b) { trace_log(FATAL, ASSERT, "Assertion failed: %s == %s", #a, #b); exit(2); }
#define w_assert_ne(a, b) \
	if (a == b) { trace_log(FATAL, ASSERT, "Assertion failed: %s != %s", #a, #b); exit(2); }
#define w_assert_gt(a, b) \
	if (a <= b) { trace_log(FATAL, ASSERT, "Assertion failed: %s > %s", #a, #b); exit(2); }
#define w_assert_lt(a, b) \
	if (a >= b) { trace_log(FATAL, ASSERT, "Assertion failed: %s < %s", #a, #b); exit(2); }
#define w_assert_ge(a, b) \
	if (a < b) { trace_log(FATAL, ASSERT, "Assertion failed: %s >= %s", #a, #b); exit(2); }
#define w_assert_le(a, b) \
	if (a > b) { trace_log(FATAL, ASSERT, "Assertion failed: %s <= %s", #a, #b); exit(2); }

#define w_assert_int_eq(a, b) \
	if ((int)(a) != (int)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (int): %s (%d) == %s (%d)", #a, (int)(a), #b, (int)(b)); exit(2); }
#define w_assert_int_ne(a, b) \
	if ((int)(a) == (int)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (int): %s (%d) != %s (%d)", #a, (int)(a), #b, (int)(b)); exit(2); }
#define w_assert_int_lt(a, b) \
	if ((int)(a) >= (int)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (int): %s (%d) < %s (%d)", #a, (int)(a), #b, (int)(b)); exit(2); }
#define w_assert_int_gt(a, b) \
	if ((int)(a) <= (int)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (int): %s (%d) > %s (%d)", #a, (int)(a), #b, (int)(b)); exit(2); }
#define w_assert_int_le(a, b) \
	if ((int)(a) > (int)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (int): %s (%d) <= %s (%d)", #a, (int)(a), #b, (int)(b)); exit(2); }
#define w_assert_int_ge(a, b) \
	if ((int)(a) < (int)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (int): %s (%d) >= %s (%d)", #a, (int)(a), #b, (int)(b)); exit(2); }

#define w_assert_uint_eq(a, b) \
	if ((unsigned int)(a) != (unsigned int)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (uint): %s (%u) == %s (%u)", #a, (unsigned int)(a), #b, (unsigned int)(b)); exit(2); }
#define w_assert_uint_ne(a, b) \
	if ((unsigned int)(a) == (unsigned int)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (uint): %s (%u) != %s (%u)", #a, (unsigned int)(a), #b, (unsigned int)(b)); exit(2); }
#define w_assert_uint_lt(a, b) \
	if ((unsigned int)(a) >= (unsigned int)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (uint): %s (%u) < %s (%u)", #a, (unsigned int)(a), #b, (unsigned int)(b)); exit(2); }
#define w_assert_uint_gt(a, b) \
	if ((unsigned int)(a) <= (unsigned int)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (uint): %s (%u) > %s (%u)", #a, (unsigned int)(a), #b, (unsigned int)(b)); exit(2); }
#define w_assert_uint_le(a, b) \
	if ((unsigned int)(a) > (unsigned int)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (uint): %s (%u) <= %s (%u)", #a, (unsigned int)(a), #b, (unsigned int)(b)); exit(2); }
#define w_assert_uint_ge(a, b) \
	if ((unsigned int)(a) < (unsigned int)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (uint): %s (%u) >= %s (%u)", #a, (unsigned int)(a), #b, (unsigned int)(b)); exit(2); }

#define w_assert_long_eq(a, b) \
	if ((long)(a) != (long)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (long): %s (%ld) == %s (%ld)", #a, (long)(a), #b, (long)(b)); exit(2); }
#define w_assert_long_ne(a, b) \
	if ((long)(a) == (long)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (long): %s (%ld) != %s (%ld)", #a, (long)(a), #b, (long)(b)); exit(2); }
#define w_assert_long_lt(a, b) \
	if ((long)(a) >= (long)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (long): %s (%ld) < %s (%ld)", #a, (long)(a), #b, (long)(b)); exit(2); }
#define w_assert_long_gt(a, b) \
	if ((long)(a) <= (long)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (long): %s (%ld) > %s (%ld)", #a, (long)(a), #b, (long)(b)); exit(2); }
#define w_assert_long_le(a, b) \
	if ((long)(a) > (long)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (long): %s (%ld) <= %s (%ld)", #a, (long)(a), #b, (long)(b)); exit(2); }
#define w_assert_long_ge(a, b) \
	if ((long)(a) < (long)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (long): %s (%ld) >= %s (%ld)", #a, (long)(a), #b, (long)(b)); exit(2); }

#define w_assert_ulong_eq(a, b) \
	if ((unsigned long)(a) != (unsigned long)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (ulong): %s (%lu) == %s (%lu)", #a, (unsigned long)(a), #b, (unsigned long)(b)); exit(2); }
#define w_assert_ulong_ne(a, b) \
	if ((unsigned long)(a) == (unsigned long)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (ulong): %s (%lu) != %s (%lu)", #a, (unsigned long)(a), #b, (unsigned long)(b)); exit(2); }
#define w_assert_ulong_lt(a, b) \
	if ((unsigned long)(a) >= (unsigned long)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (ulong): %s (%lu) < %s (%lu)", #a, (unsigned long)(a), #b, (unsigned long)(b)); exit(2); }
#define w_assert_ulong_gt(a, b) \
	if ((unsigned long)(a) <= (unsigned long)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (ulong): %s (%lu) > %s (%lu)", #a, (unsigned long)(a), #b, (unsigned long)(b)); exit(2); }
#define w_assert_ulong_le(a, b) \
	if ((unsigned long)(a) > (unsigned long)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (ulong): %s (%lu) <= %s (%lu)", #a, (unsigned long)(a), #b, (unsigned long)(b)); exit(2); }
#define w_assert_ulong_ge(a, b) \
	if ((unsigned long)(a) < (unsigned long)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (ulong): %s (%lu) >= %s (%lu)", #a, (unsigned long)(a), #b, (unsigned long)(b)); exit(2); }

#define w_assert_float_eq(a, b) \
	if ((float)(a) != (float)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (float): %s (%f) == %s (%f)", #a, (float)(a), #b, (float)(b)); exit(2); }
#define w_assert_float_ne(a, b) \
	if ((float)(a) == (float)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (float): %s (%f) != %s (%f)", #a, (float)(a), #b, (float)(b)); exit(2); }
#define w_assert_float_lt(a, b) \
	if ((float)(a) >= (float)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (float): %s (%f) < %s (%f)", #a, (float)(a), #b, (float)(b)); exit(2); }
#define w_assert_float_gt(a, b) \
	if ((float)(a) <= (float)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (float): %s (%f) > %s (%f)", #a, (float)(a), #b, (float)(b)); exit(2); }
#define w_assert_float_le(a, b) \
	if ((float)(a) > (float)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (float): %s (%f) <= %s (%f)", #a, (float)(a), #b, (float)(b)); exit(2); }
#define w_assert_float_ge(a, b) \
	if ((float)(a) < (float)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (float): %s (%f) >= %s (%f)", #a, (float)(a), #b, (float)(b)); exit(2); }

#define w_assert_double_eq(a, b) \
	if ((double)(a) != (double)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (double): %s (%lf) == %s (%lf)", #a, (double)(a), #b, (double)(b)); exit(2); }
#define w_assert_double_ne(a, b) \
	if ((double)(a) == (double)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (double): %s (%lf) != %s (%lf)", #a, (double)(a), #b, (double)(b)); exit(2); }
#define w_assert_double_lt(a, b) \
	if ((double)(a) >= (double)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (double): %s (%lf) < %s (%lf)", #a, (double)(a), #b, (double)(b)); exit(2); }
#define w_assert_double_gt(a, b) \
	if ((double)(a) <= (double)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (double): %s (%lf) > %s (%lf)", #a, (double)(a), #b, (double)(b)); exit(2); }
#define w_assert_double_le(a, b) \
	if ((double)(a) > (double)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (double): %s (%lf) <= %s (%lf)", #a, (double)(a), #b, (double)(b)); exit(2); }
#define w_assert_double_ge(a, b) \
	if ((double)(a) < (double)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (double): %s (%lf) >= %s (%lf)", #a, (double)(a), #b, (double)(b)); exit(2); }

#define w_assert_char_eq(a, b) \
	if ((char)(a) != (char)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (char): %c (%s) == %c (%s)", (char)(a), #a, (char)(b), #b); exit(2); }
#define w_assert_char_ne(a, b) \
	if ((char)(a) == (char)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (char): %c (%s) != %c (%s)", (char)(a), #a, (char)(b), #b); exit(2); }
#define w_assert_str_eq(a, b) \
	if (strcmp((a), (b)) != 0) { trace_log(FATAL, ASSERT, "Assertion failed (str): %s (%s) == %s (%s)", (a), #a, (b), #b); exit(2); }
#define w_assert_str_ne(a, b) \
	if (strcmp((a), (b)) == 0) { trace_log(FATAL, ASSERT, "Assertion failed (str): %s (%s) != %s (%s)", (a), #a, (b), #b); exit(2); }
#define w_assert_mem_eq(a, b, size) \
	if (memcmp((a), (b), (size)) != 0) { trace_log(FATAL, ASSERT, "Assertion failed (mem): %p (%s) == %p (%s)", (a), #a, (b), #b); exit(2); }
#define w_assert_mem_ne(a, b, size) \
	if (memcmp((a), (b), (size)) == 0) { trace_log(FATAL, ASSERT, "Assertion failed (mem): %p (%s) != %p (%s)", (a), #a, (b), #b); exit(2); }
#define w_assert_ptr_eq(a, b) \
	if ((void*)(a) != (void*)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (ptr): %p (%s) == %p (%s)", (void*)(a), #a, (void*)(b), #b); exit(2); }
#define w_assert_ptr_ne(a, b) \
	if ((void*)(a) == (void*)(b)) { trace_log(FATAL, ASSERT, "Assertion failed (ptr): %p (%s) != %p (%s)", (void*)(a), #a, (void*)(b), #b); exit(2); }
#else
#define debug_printf(fmt, ...)
#define trace_printf(fmt, ...)
#define debug_log(log_level, log_module, log_format, ...)
#define trace_log(log_level, log_module, log_format, ...)
#define w_assert(exp)
#define w_assert_eq(a, b)
#define w_assert_ne(a, b)
#define w_assert_gt(a, b)
#define w_assert_lt(a, b)
#define w_assert_ge(a, b)
#define w_assert_le(a, b)
#define w_assert_int_eq(a, b)
#define w_assert_int_lt(a, b)
#define w_assert_int_gt(a, b)
#define w_assert_int_le(a, b)
#define w_assert_int_ge(a, b)
#define w_assert_uint_eq(a, b)
#define w_assert_uint_lt(a, b)
#define w_assert_uint_gt(a, b)
#define w_assert_uint_le(a, b)
#define w_assert_uint_ge(a, b)
#define w_assert_long_eq(a, b)
#define w_assert_long_lt(a, b)
#define w_assert_long_gt(a, b)
#define w_assert_long_le(a, b)
#define w_assert_long_ge(a, b)
#define w_assert_ulong_eq(a, b)
#define w_assert_ulong_lt(a, b)
#define w_assert_ulong_gt(a, b)
#define w_assert_ulong_le(a, b)
#define w_assert_ulong_ge(a, b)
#define w_assert_float_eq(a, b)
#define w_assert_float_lt(a, b)
#define w_assert_float_gt(a, b)
#define w_assert_float_le(a, b)
#define w_assert_float_ge(a, b)
#define w_assert_double_eq(a, b)
#define w_assert_double_lt(a, b)
#define w_assert_double_gt(a, b)
#define w_assert_double_le(a, b)
#define w_assert_double_ge(a, b)
#define w_assert_char_eq(a, b)
#define w_assert_char_ne(a, b)
#define w_assert_str_eq(a, b)
#define w_assert_str_ne(a, b)
#define w_assert_mem_eq(a, b, size)
#define w_assert_mem_ne(a, b, size)
#define w_assert_ptr_eq(a, b)
#define w_assert_ptr_ne(a, b)
#endif

void w_debug_print_value_layout(void *obj, size_t len, const char *name);

#endif // WHISKER_DEBUG_H

