/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_dispatch_buffer
 * @created     : Friday May 01, 2026 00:27:23 CST
 * @description : type-id based dispatch buffer for deferred processing
 */

#include "whisker_dispatch_buffer.h"

void w_dispatch_buffer_init(struct w_dispatch_buffer *buffer)
{
	w_array_init_t(buffer->entries, W_DISPATCH_BUFFER_DATA_REALLOC_BLOCK_SIZE);
	w_array_init_t(buffer->payload_data, W_DISPATCH_BUFFER_DATA_REALLOC_BLOCK_SIZE);
	buffer->entries_length = 0;
	buffer->payload_data_length = 0;
	buffer->read_index = 0;
	w_hook_registry_init(&buffer->handlers);
}

void w_dispatch_buffer_free(struct w_dispatch_buffer *buffer)
{
	free_null(buffer->entries);
	free_null(buffer->payload_data);
	buffer->entries_length = 0;
	buffer->payload_data_length = 0;
	buffer->read_index = 0;
	w_hook_registry_free(&buffer->handlers);
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

void w_dispatch_buffer_sort(struct w_dispatch_buffer *buffer, w_dispatch_compare_fn compare_fn)
{
	size_t start = buffer->read_index;
	size_t end = buffer->entries_length;
	if (end - start < 2) return;

	// insertion sort for stability (preserves order of equal entries)
	for (size_t i = start + 1; i < end; i++)
	{
		struct w_dispatch_entry key = buffer->entries[i];
		void *key_payload = buffer->payload_data + key.payload_offset;
		size_t j = i;

		while (j > start)
		{
			struct w_dispatch_entry *prev = &buffer->entries[j - 1];
			void *prev_payload = buffer->payload_data + prev->payload_offset;

			if (compare_fn(buffer, prev, prev_payload, &key, key_payload) <= 0)
				break;

			buffer->entries[j] = buffer->entries[j - 1];
			j--;
		}

		buffer->entries[j] = key;
	}
}

int w_dispatch_buffer_compare_by_priority(
	struct w_dispatch_buffer *buffer,
	struct w_dispatch_entry *entry_a, void *payload_a,
	struct w_dispatch_entry *entry_b, void *payload_b)
{
	(void)buffer;
	(void)payload_a;
	(void)payload_b;
	return entry_a->priority - entry_b->priority;
}

void w_dispatch_buffer_sort_by_priority(struct w_dispatch_buffer *buffer)
{
	w_dispatch_buffer_sort(buffer, w_dispatch_buffer_compare_by_priority);
}

void w_dispatch_buffer_clear(struct w_dispatch_buffer *buffer)
{
	buffer->entries_length = 0;
	buffer->payload_data_length = 0;
	buffer->read_index = 0;
}
