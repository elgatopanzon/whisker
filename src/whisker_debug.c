/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_debug
 * @created     : Friday Feb 14, 2025 19:14:40 CST
 */

#include "whisker_std.h"

#include "whisker_debug.h"

// take an object and it's size, print the bytes representation
void w_debug_print_value_layout(void *obj, size_t len, const char *name)
{
	unsigned char *bytes = (unsigned char *) obj;
	printf("%s representation: 0x", name);
	while (len--)
		printf("%02x", *bytes++);
	printf("\n");
}
