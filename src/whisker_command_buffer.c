/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_command_buffer
 * @created     : Thursday Mar 05, 2026 14:51:30 CST
 */

#include "whisker_std.h"

#include "whisker_command_buffer.h"

void w_command_buffer_init(struct w_command_buffer *buffer)
{
	w_array_init_t(buffer->commands, W_COMMAND_BUFFER_DATA_REALLOC_BLOCK_SIZE);
	w_array_init_t(buffer->payload_data, W_COMMAND_BUFFER_DATA_REALLOC_BLOCK_SIZE);
	buffer->commands_length = 0;
	buffer->payload_data_length = 0;
}
void w_command_buffer_free(struct w_command_buffer *buffer)
{
	free_null(buffer->commands);
	free_null(buffer->payload_data);
	buffer->commands_length = 0;
	buffer->payload_data_length = 0;
}

void w_command_buffer_queue(struct w_command_buffer *buffer, w_command_fn command_fn, void *ctx, void *payload, size_t payload_size)
{
	w_array_ensure_alloc_block_size(
		buffer->commands,
		buffer->commands_length + 1,
		W_COMMAND_BUFFER_DATA_REALLOC_BLOCK_SIZE
	);

	w_array_ensure_alloc_block_size(
		buffer->payload_data,
		buffer->payload_data_length + payload_size,
		W_COMMAND_BUFFER_DATA_REALLOC_BLOCK_SIZE
	);

	// set command
	buffer->commands[buffer->commands_length].command_fn = command_fn;
	buffer->commands[buffer->commands_length].ctx = ctx;
	buffer->commands[buffer->commands_length].payload_offset = buffer->payload_data_length;
	buffer->commands[buffer->commands_length].payload_size = payload_size;

	// copy payload data
	memcpy(buffer->payload_data + buffer->payload_data_length, payload, payload_size);
	buffer->payload_data_length += payload_size;
	buffer->commands_length++;
}

void w_command_buffer_flush(struct w_command_buffer *buffer)
{
	for (size_t i = 0; i < buffer->commands_length; ++i)
	{
		struct w_command_entry *command = &buffer->commands[i];
		command->command_fn(command->ctx, buffer->payload_data + command->payload_offset);
	}

	buffer->commands_length = 0;
	buffer->payload_data_length = 0;
}
