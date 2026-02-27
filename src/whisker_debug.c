/**
 * @author      : ElGatoPanzon
 * @file        : whisker_debug
 * @created     : Thursday Feb 26, 2026 14:01:00 CST
 * @description : debug utility functions for whisker
 */

#include "whisker_debug.h"

void w_debug_print_value_layout(void *obj, size_t len, const char *name)
{
    unsigned char *bytes = (unsigned char *)obj;
    fprintf(stderr, "Layout of '%s' [%zu bytes]:", name, len);
    for (size_t i = 0; i < len; i++) {
        fprintf(stderr, " 0x%02x", bytes[i]);
    }
    fprintf(stderr, "\n");
}
