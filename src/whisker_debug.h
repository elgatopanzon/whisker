/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_debug
 * @created     : Friday Feb 14, 2025 19:13:56 CST
 */

#include "whisker_std.h"

#ifndef WHISKER_DEBUG_H
#define WHISKER_DEBUG_H

#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

#define debug_printf(fmt, ...) \
            do { if (DEBUG_TEST) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

void whisker_debug_print_value_layout(void *obj, size_t len, const char *name);

#endif /* WHISKER_DEBUG_H */

