/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_dispatch_buffer
 * @created     : Friday May 01, 2026 00:27:23 CST
 * @description : type-id based dispatch buffer for deferred processing
 */

#include "whisker_std.h"
#include "whisker_hook_registry.h"
#include "whisker_array.h"

#ifndef WHISKER_DISPATCH_BUFFER_H
#define WHISKER_DISPATCH_BUFFER_H

#ifndef W_DISPATCH_BUFFER_DATA_REALLOC_BLOCK_SIZE
#define W_DISPATCH_BUFFER_DATA_REALLOC_BLOCK_SIZE 16384
#endif /* ifndef W_DISPATCH_BUFFER_DATA_REALLOC_BLOCK_SIZE */

#define w_dispatch_buffer_register_handler(buf, cmd_id, handler_ptr) \
	w_hook_registry_register_hook(&buf->handlers, cmd_id, (w_hook_fn)handler_ptr)

#define w_dispatch_buffer_run_handler(buf, cmd_id, ctx, data) \
	w_hook_registry_run_hooks(&buf->handlers, cmd_id, ctx, data)

// struct for each buffered dispatch entry
struct w_dispatch_entry
{
	int type_id;
	int priority;
	size_t payload_offset;
	size_t payload_size;
};

struct w_dispatch_buffer
{
	w_array_declare(struct w_dispatch_entry, entries);
	w_array_declare(uint8_t, payload_data);
	size_t read_index; // current read position for pop
	struct w_hook_registry handlers; // handlers for dispatch calls
};

#define w_dispatch_buffer_push_value(buf, cmd, priority, payload_type, ...) \
	((void)sizeof(cmd), (void)sizeof(payload_type), w_dispatch_buffer_push(buf, cmd, priority, &((payload_type){__VA_ARGS__}), sizeof(payload_type)));

// init dispatch buffer
void w_dispatch_buffer_init(struct w_dispatch_buffer *buffer);
// free dispatch buffer entries and payload data
void w_dispatch_buffer_free(struct w_dispatch_buffer *buffer);

// push a dispatch entry to the buffer
void w_dispatch_buffer_push(struct w_dispatch_buffer *buffer, int type_id, int priority, void *payload, size_t payload_size);

// pop next entry from buffer, returns NULL if empty
// payload pointer set to payload data location
struct w_dispatch_entry* w_dispatch_buffer_pop(struct w_dispatch_buffer *buffer, void **payload);

// peek at entry by index without advancing read position
// returns NULL if index out of range
struct w_dispatch_entry* w_dispatch_buffer_peek(struct w_dispatch_buffer *buffer, size_t index, void **payload);

// compare function for sorting entries
// returns negative if A < B, 0 if equal, positive if A > B
typedef int (*w_dispatch_compare_fn)(
	struct w_dispatch_buffer *buffer,
	struct w_dispatch_entry *entry_a, void *payload_a,
	struct w_dispatch_entry *entry_b, void *payload_b
);

// sort unread entries using a custom compare function
void w_dispatch_buffer_sort(struct w_dispatch_buffer *buffer, w_dispatch_compare_fn compare_fn);

// default comparator: ascending by priority field
int w_dispatch_buffer_compare_by_priority(
	struct w_dispatch_buffer *buffer,
	struct w_dispatch_entry *entry_a, void *payload_a,
	struct w_dispatch_entry *entry_b, void *payload_b);

// convenience: sort unread entries ascending by priority
void w_dispatch_buffer_sort_by_priority(struct w_dispatch_buffer *buffer);

// check if buffer has entries to read
bool w_dispatch_buffer_has_entries(struct w_dispatch_buffer *buffer);

// get count of unread entries
size_t w_dispatch_buffer_count(struct w_dispatch_buffer *buffer);

// clear all entries and reset buffer
void w_dispatch_buffer_clear(struct w_dispatch_buffer *buffer);

#endif /* WHISKER_DISPATCH_BUFFER_H */
