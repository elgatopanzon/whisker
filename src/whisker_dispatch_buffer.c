/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_dispatch_buffer
 * @created     : Friday May 01, 2026 00:27:23 CST
 * @description : type-id based dispatch buffer for deferred processing
 */

#include "whisker_std.h"

#include "whisker_dispatch_buffer.h"

void w_dispatch_buffer_init(struct w_dispatch_buffer *buffer)
{
	w_array_init_t(buffer->entries, W_DISPATCH_BUFFER_DATA_REALLOC_BLOCK_SIZE);
	w_array_init_t(buffer->payload_data, W_DISPATCH_BUFFER_DATA_REALLOC_BLOCK_SIZE);
	buffer->entries_length = 0;
	buffer->payload_data_length = 0;
	buffer->read_index = 0;
}

void w_dispatch_buffer_free(struct w_dispatch_buffer *buffer)
{
	free_null(buffer->entries);
	free_null(buffer->payload_data);
	buffer->entries_length = 0;
	buffer->payload_data_length = 0;
	buffer->read_index = 0;
}

void w_dispatch_buffer_push(struct w_dispatch_buffer *buffer, int type_id, int priority, void *payload, size_t payload_size)
{
	w_array_ensure_alloc_block_size(
		buffer->entries,
		buffer->entries_length + 1,
		W_DISPATCH_BUFFER_DATA_REALLOC_BLOCK_SIZE
	);

	w_array_ensure_alloc_block_size(
		buffer->payload_data,
		buffer->payload_data_length + payload_size,
		W_DISPATCH_BUFFER_DATA_REALLOC_BLOCK_SIZE
	);

	// set entry
	buffer->entries[buffer->entries_length].type_id = type_id;
	buffer->entries[buffer->entries_length].priority = priority;
	buffer->entries[buffer->entries_length].payload_offset = buffer->payload_data_length;
	buffer->entries[buffer->entries_length].payload_size = payload_size;

	// copy payload data
	memcpy(buffer->payload_data + buffer->payload_data_length, payload, payload_size);
	buffer->payload_data_length += payload_size;
	buffer->entries_length++;
}

struct w_dispatch_entry* w_dispatch_buffer_pop(struct w_dispatch_buffer *buffer, void **payload)
{
	if (buffer->read_index >= buffer->entries_length) return NULL;

	struct w_dispatch_entry *entry = &buffer->entries[buffer->read_index];
	if (payload != NULL)
	{
		*payload = buffer->payload_data + entry->payload_offset;
	}
	buffer->read_index++;
	return entry;
}

struct w_dispatch_entry* w_dispatch_buffer_peek(struct w_dispatch_buffer *buffer, size_t index, void **payload)
{
	if (index >= buffer->entries_length) return NULL;

	struct w_dispatch_entry *entry = &buffer->entries[index];
	if (payload != NULL)
	{
		*payload = buffer->payload_data + entry->payload_offset;
	}
	return entry;
}

bool w_dispatch_buffer_has_entries(struct w_dispatch_buffer *buffer)
{
	return buffer->read_index < buffer->entries_length;
}

size_t w_dispatch_buffer_count(struct w_dispatch_buffer *buffer)
{
	return buffer->entries_length - buffer->read_index;
}

void w_dispatch_buffer_clear(struct w_dispatch_buffer *buffer)
{
	buffer->entries_length = 0;
	buffer->payload_data_length = 0;
	buffer->read_index = 0;
}
