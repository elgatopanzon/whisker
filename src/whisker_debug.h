/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_debug
 * @created     : Friday Feb 14, 2025 19:13:56 CST
 */

#include "whisker_std.h"

#ifndef WHISKER_DEBUG_H
#define WHISKER_DEBUG_H

#ifndef NDEBUG
#define debug_printf(fmt, ...) \
    fprintf(stderr, fmt, ##__VA_ARGS__)
#define trace_printf(fmt, ...) \
    fprintf(stderr, "[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug_printf(fmt, ...)
#define trace_printf(fmt, ...)
#endif

void whisker_debug_print_value_layout(void *obj, size_t len, const char *name);

#endif /* WHISKER_DEBUG_H */

