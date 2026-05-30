/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_networking_connection_systems
 * @created     : Thursday May 28, 2026 11:40:36 CST
 * @description : systems related to handling client connections
 */

#include "whisker_networking.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/utilities/whisker_utilities_entity_lifecycle.h"
#include "modules/streams/whisker_streams.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef WHISKER_NETWORKING_CONNECTION_SYSTEMS_H
#define WHISKER_NETWORKING_CONNECTION_SYSTEMS_H

#ifdef MSG_NOSIGNAL
#define WM_NETWORKING_SEND_FLAGS MSG_NOSIGNAL
#else
#define WM_NETWORKING_SEND_FLAGS 0
#endif

// handle destroy connection request
w_ecs_system(
	wm_networking_connection_handle_destroyed_request,
	WM_PHASE_PRE,
	w_query(
		w_query_h(req_network_connection_destroyed),
	),
{
	w_set_tag(entity, req_network_connection_destroyed, false);
	w_set_tag(entity, req_network_connection_closed, true);
	w_entity_destroy_end_of_frame(world, entity);
});

// handle close connection request
w_ecs_system(
	wm_networking_connection_handle_close_request,
	WM_NETWORK_PHASE_CLOSE,
	w_query(
		w_query_r(network_connection_fd),
		w_query_h(req_network_connection_closed),
	),
{
	// remove connection lifecycle and state tags
	w_set_tag(entity, req_network_connection_closed, false);
	w_set_tag(entity, network_connection_read_ready, false);
	w_set_tag(entity, network_connection_write_ready, false);

	// request stream buffers to be freed
	w_entity_id input_stream = wm_networking_connection_get_input_stream_entity(world, entity);
	if (input_stream != W_ENTITY_INVALID)
	{
		w_set_tag(input_stream, req_stream_buffer_cold, true);
		w_entity_destroy_end_of_frame(world, input_stream);
		w_remove(entity, network_connection_input_stream_entity);
	}

	w_entity_id output_stream = wm_networking_connection_get_output_stream_entity(world, entity);
	if (output_stream != W_ENTITY_INVALID)
	{
		w_set_tag(output_stream, req_stream_buffer_cold, true);
		w_entity_destroy_end_of_frame(world, output_stream);
		w_remove(entity, network_connection_output_stream_entity);
	}

	int32_t fd = *w_query_get(network_connection_fd);

	// attempt to close the client fd
	if (close(fd) != 0)
	{
		int err = errno;
		w_log_entity_error(networking, "failed to close connection: %s", strerror(err));
		w_set_value(entity, network_connection_err, err);
	}

	// remove client fd
	w_remove(entity, network_connection_fd);
});


// poll connection for read readiness
w_ecs_system(
	wm_networking_connection_read_poll,
	WM_NETWORK_PHASE_PRE_READ,
	w_query(
		w_query_r(network_connection_fd),
		w_query_n(network_connection_err),
	),
{
	int32_t fd = *w_query_get(network_connection_fd);

	// poll socket to check for data to be read
	struct pollfd pfd = {0};
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int result = poll(&pfd, 1, 0);

	// handle poll error and set err
	if (result == -1)
	{
		int err = errno;
		w_log_entity_error(networking, "failed to poll connection for readiness: %s", strerror(err));
		w_set_value(entity, network_connection_err, err);
		w_set_tag(entity, network_connection_read_ready, false);
		continue;
	}

	// poll may report socket error events without poll itself failing
	if (result > 0 && (pfd.revents & (POLLERR | POLLNVAL)))
	{
		w_log_entity_error(networking, "connection poll returned error event: %d", pfd.revents);
		w_set_value(entity, network_connection_err, pfd.revents);
		w_set_tag(entity, network_connection_read_ready, false);
		continue;
	}

	// if the return event includes POLLIN and we get a positive return
	// value, we have data ready to read on this connection
	w_set_tag(entity, network_connection_read_ready, result > 0 && (pfd.revents & POLLIN));
});


// read bytes from network connection into input buffer
w_ecs_system(
	wm_networking_connection_receive_bytes,
	WM_NETWORK_PHASE_READ,
	w_query(
		w_query_r(network_connection_fd),
		w_query_h(network_connection_read_ready),

		w_query_n(network_connection_err),
	),
{
	int32_t fd = *w_query_get(network_connection_fd);
	w_entity_id input_stream = wm_networking_connection_get_input_stream_entity(world, entity);
	if (input_stream == W_ENTITY_INVALID || !stream_buffer_handle_exists(world, input_stream))
	{
		continue;
	}

	uint64_t size = *stream_buffer_size_get(world, input_stream);
	uint64_t *length = stream_buffer_length_get(world, input_stream);

	uint8_t *buffer = wm_streams_get_buffer(world, input_stream);

	// recv from the socket into the buffer up to max size
	while (*length < size) 
	{
		uint64_t available = size - *length;
		ssize_t n = recv(fd, buffer + *length, available, 0);

		// check if we received any bytes
		// if so, set input available on the buffer
		if (n > 0)
		{
			*length += (uint64_t) n;
			stream_available_set_tag_state(world, input_stream, true);
			continue;
		}

		// if we got nothing, then connection is closed cleanly
		if (n == 0)
		{
			w_set_tag(entity, req_network_connection_closed, true);
			break;
		}

		// errors!
		int err = errno;

		if (err == EAGAIN || err == EWOULDBLOCK)
		{
			break;
		}

		if (err == EINTR)
		{
			continue;
		}

		// record error and break
		w_log_entity_error(networking, "failed to receive connection bytes: %s", strerror(err));
		w_set_value(entity, network_connection_err, err);
		w_set_tag(entity, req_network_connection_closed, true);

		break;
	}
});


// poll connection for write readiness
w_ecs_system(
	wm_networking_connection_write_poll,
	WM_NETWORK_PHASE_PRE_WRITE,
	w_query(
		w_query_r(network_connection_fd),
		w_query_n(network_connection_err),
	),
{
	w_entity_id output_stream = wm_networking_connection_get_output_stream_entity(world, entity);
	if (output_stream == W_ENTITY_INVALID || !stream_available_tag_exists(world, output_stream))
	{
		continue;
	}

	int32_t fd = *w_query_get(network_connection_fd);

	struct pollfd pfd = {0};
	pfd.fd = fd;
	pfd.events = POLLOUT;
	pfd.revents = 0;

	int result = poll(&pfd, 1, 0);

	if (result == -1)
	{
		int err = errno;
		w_log_entity_error(networking, "failed to poll connection for write readiness: %s", strerror(err));
		w_set_value(entity, network_connection_err, err);
		w_set_tag(entity, network_connection_write_ready, false);
		continue;
	}

	if (result > 0 && (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)))
	{
		w_log_entity_error(networking, "connection write poll returned error event: %d", pfd.revents);
		w_set_value(entity, network_connection_err, pfd.revents);
		w_set_tag(entity, network_connection_write_ready, false);
		continue;
	}

	w_set_tag(entity, network_connection_write_ready, result > 0 && (pfd.revents & POLLOUT));
});


// write bytes from output buffer to network connection
w_ecs_system(
	wm_networking_connection_send_bytes,
	WM_NETWORK_PHASE_WRITE,
	w_query(
		w_query_r(network_connection_fd),
		w_query_h(network_connection_write_ready),

		w_query_n(network_connection_err),
	),
{
	int32_t fd = *w_query_get(network_connection_fd);
	w_entity_id output_stream = wm_networking_connection_get_output_stream_entity(world, entity);
	if (output_stream == W_ENTITY_INVALID || !stream_buffer_handle_exists(world, output_stream))
	{
		w_set_tag(entity, network_connection_write_ready, false);
		continue;
	}

	uint64_t *offset = stream_buffer_offset_get(world, output_stream);
	uint64_t *length = stream_buffer_length_get(world, output_stream);

	uint8_t *buffer = wm_streams_get_buffer(world, output_stream);

	while (*offset < *length)
	{
		uint64_t pending = *length - *offset;
		ssize_t n = send(fd, buffer + *offset, pending, WM_NETWORKING_SEND_FLAGS);

		if (n > 0)
		{
			*offset += (uint64_t)n;
			continue;
		}

		if (n == 0)
		{
			break;
		}

		int err = errno;

		if (err == EAGAIN || err == EWOULDBLOCK)
		{
			break;
		}

		if (err == EINTR)
		{
			continue;
		}

		w_log_entity_error(networking, "failed to send connection bytes: %s", strerror(err));
		w_set_value(entity, network_connection_err, err);
		w_set_tag(entity, req_network_connection_closed, true);
		break;
	}

	w_set_tag(entity, network_connection_write_ready, false);
});


// handle close-after-write request
w_ecs_system(
	wm_networking_connection_handle_closed_after_write_request,
	WM_NETWORK_PHASE_POST_WRITE,
	w_query(
		w_query_h(req_network_connection_closed_after_write),
	),
{
	w_entity_id input_stream = wm_networking_connection_get_input_stream_entity(world, entity);
	w_entity_id output_stream = wm_networking_connection_get_output_stream_entity(world, entity);

	uint64_t input_offset = 0;
	uint64_t input_length = 0;
	uint64_t output_offset = 0;
	uint64_t output_length = 0;

	if (input_stream != W_ENTITY_INVALID)
	{
		uint64_t *offset = stream_buffer_offset_get(world, input_stream);
		uint64_t *length = stream_buffer_length_get(world, input_stream);
		input_offset = offset ? *offset : 0;
		input_length = length ? *length : 0;
	}

	if (output_stream != W_ENTITY_INVALID)
	{
		uint64_t *offset = stream_buffer_offset_get(world, output_stream);
		uint64_t *length = stream_buffer_length_get(world, output_stream);
		output_offset = offset ? *offset : 0;
		output_length = length ? *length : 0;
	}

	bool input_pending = input_offset < input_length;
	bool output_pending = output_offset < output_length;

	if (input_pending || output_pending)
	{
		continue;
	}

	w_set_tag(entity, req_network_connection_closed_after_write, false);
	w_set_tag(entity, req_network_connection_closed, true);
});

#endif /* WHISKER_NETWORKING_CONNECTION_SYSTEMS_H */
