/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_command_buffer
 * @created     : Thursday Mar 05, 2026 14:38:28 CST
 * @description : generic command buffer for func+payload
 */

#include "whisker_std.h"
#include "whisker_array.h"

#ifndef WHISKER_COMMAND_BUFFER_H
#define WHISKER_COMMAND_BUFFER_H

#ifndef W_COMMAND_BUFFER_DATA_REALLOC_BLOCK_SIZE
#define W_COMMAND_BUFFER_DATA_REALLOC_BLOCK_SIZE 16384
#endif /* ifndef W_COMMAND_BUFFER_DATA_REALLOC_BLOCK_SIZE */

// each command accepts ctx and payload
typedef void (*w_command_fn)(void *ctx, void *payload);

// struct for each buffered command
struct w_command_entry 
{
	w_command_fn command_fn;
	void *ctx;
	size_t payload_offset;
	size_t payload_size;
};

struct w_command_buffer 
{
	w_array_declare(struct w_command_entry, commands);
	w_array_declare(uint8_t, payload_data);
};

// init command buffer
void w_command_buffer_init(struct w_command_buffer *buffer);
// free command buffer commands and payload data
void w_command_buffer_free(struct w_command_buffer *buffer);

// queue a command function to the command buffer
void w_command_buffer_queue(struct w_command_buffer *buffer, w_command_fn command_fn, void *ctx, void *payload, size_t payload_size);

// process queued commands and clear
void w_command_buffer_flush(struct w_command_buffer *buffer);

#endif /* WHISKER_COMMAND_BUFFER_H */

