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

	// request buffers to be freed
	w_set_tag(entity, req_stream_input_buffer_cold, true);
	w_set_tag(entity, req_stream_output_buffer_cold, true);

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

		w_query_r(stream_input_buffer_handle),
		w_query_r(stream_input_buffer_size),
		w_query_r(stream_input_buffer_length),

		w_query_n(network_connection_err),
	),
{
	int32_t fd = *w_query_get(network_connection_fd);
	uint64_t size = *w_query_get(stream_input_buffer_size);
	uint64_t *length = w_query_get(stream_input_buffer_length);

	uint8_t *buffer = wm_managed_alloc_resolve_handle(world, stream_input_buffer_handle, entity);

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
			w_set_tag(entity, stream_input_available, true);
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
		w_query_h(stream_output_available),
		w_query_n(network_connection_err),
	),
{
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

		w_query_r(stream_output_buffer_handle),
		w_query_r(stream_output_buffer_offset),
		w_query_r(stream_output_buffer_length),

		w_query_n(network_connection_err),
	),
{
	int32_t fd = *w_query_get(network_connection_fd);
	uint64_t *offset = w_query_get(stream_output_buffer_offset);
	uint64_t *length = w_query_get(stream_output_buffer_length);

	uint8_t *buffer = wm_streams_get_output_buffer(world, entity);

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
		w_query_o(stream_input_buffer_offset),
		w_query_o(stream_input_buffer_length),
		w_query_o(stream_output_buffer_offset),
		w_query_o(stream_output_buffer_length),
	),
{
	uint64_t input_offset = *w_query_get_opt_or_default(stream_input_buffer_offset);
	uint64_t input_length = *w_query_get_opt_or_default(stream_input_buffer_length);
	uint64_t output_offset = *w_query_get_opt_or_default(stream_output_buffer_offset);
	uint64_t output_length = *w_query_get_opt_or_default(stream_output_buffer_length);

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
