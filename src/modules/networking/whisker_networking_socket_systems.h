/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_networking_socket_systems
 * @created     : Thursday May 28, 2026 11:40:16 CST
 * @description : network socket facade and connection spawning systems
 */

#include "whisker_networking.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/utilities/whisker_utilities_entity_lifecycle.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef WHISKER_NETWORKING_SOCKET_SYSTEMS_H
#define WHISKER_NETWORKING_SOCKET_SYSTEMS_H

static inline w_entity_id wm_networking_socket_ensure_listen_entity(struct w_ecs_world *world, w_entity_id entity)
{
	w_entity_id socket_entity = wm_networking_socket_get_listen_entity(world, entity);
	if (socket_entity == W_ENTITY_INVALID)
	{
		socket_entity = w_ecs_request_entity(world);
		network_socket_listen_entity_set_value(world, entity, socket_entity);
	}

	return socket_entity;
}

static inline void wm_networking_socket_sync_state(struct w_ecs_world *world, w_entity_id entity, w_entity_id socket_entity)
{
	if (socket_entity == W_ENTITY_INVALID)
	{
		network_socket_accept_ready_set_tag_state(world, entity, false);
		network_socket_listen_fd_remove(world, entity);
		return;
	}

	if (socket_listen_fd_exists(world, socket_entity))
	{
		network_socket_listen_fd_set_value(world, entity, *socket_listen_fd_get(world, socket_entity));
	}
	else
	{
		network_socket_listen_fd_remove(world, entity);
	}

	network_socket_accept_ready_set_tag_state(world, entity, socket_accept_ready_tag_exists(world, socket_entity));

	if (socket_err_exists(world, socket_entity))
	{
		network_socket_err_set_value(world, entity, *socket_err_get(world, socket_entity));
	}
}

// project network socket configuration and lifecycle requests to the backing
// generic socket entity before socket lifecycle systems run.
w_ecs_system(
	wm_networking_socket_project_to_socket,
	WM_NETWORK_PHASE_PRE_INIT,
	w_query(
		w_query_r(network_socket_listen_host_string_id),
		w_query_r(network_socket_listen_port),
		w_query_o(network_socket_listen_backlog),
	),
{
	w_entity_id socket_entity = wm_networking_socket_ensure_listen_entity(world, entity);

	socket_listen_host_string_id_set_value(world, socket_entity, *w_query_get(network_socket_listen_host_string_id));
	socket_listen_port_set_value(world, socket_entity, *w_query_get(network_socket_listen_port));
	socket_listen_backlog_set_value(world, socket_entity, *w_query_get_opt_or_default(network_socket_listen_backlog));

	if (req_network_socket_hot_tag_exists(world, entity))
	{
		req_network_socket_hot_set_tag_state(world, entity, false);
		req_socket_hot_set_tag_state(world, socket_entity, true);
	}
});

// project close requests late in the frame so backing sockets are closed by the
// generic socket close phase.
w_ecs_system(
	wm_networking_socket_project_close_request,
	WM_NETWORK_PHASE_PRE_CLOSE,
	w_query(
		w_query_h(req_network_socket_cold),
		w_query_o(network_socket_listen_entity),
	),
{
	w_entity_id socket_entity = *w_query_get_opt_or_default(network_socket_listen_entity);
	req_network_socket_cold_set_tag_state(world, entity, false);
	network_socket_accept_ready_set_tag_state(world, entity, false);
	network_socket_listen_fd_remove(world, entity);

	if (socket_entity != W_ENTITY_INVALID)
	{
		req_socket_cold_set_tag_state(world, socket_entity, true);
	}
});

// handle request socket destroyed
w_ecs_system(
	wm_networking_socket_handle_request_destroyed,
	WM_PHASE_PRE,
	w_query(
		w_query_h(req_network_socket_destroyed),
	),
{
	w_set_tag(entity, req_network_socket_destroyed, false);
	w_set_tag(entity, req_network_socket_cold, true);

	w_entity_id socket_entity = wm_networking_socket_get_listen_entity(world, entity);
	if (socket_entity != W_ENTITY_INVALID)
	{
		req_socket_cold_set_tag_state(world, socket_entity, true);
		w_entity_destroy_end_of_frame(world, socket_entity);
	}

	w_entity_destroy_end_of_frame(world, entity);
});

// sync backing socket fd/error state after the generic socket init phase.
w_ecs_system(
	wm_networking_socket_sync_after_init,
	WM_NETWORK_PHASE_POST_INIT,
	w_query(
		w_query_r(network_socket_listen_entity),
	),
{
	wm_networking_socket_sync_state(world, entity, *w_query_get(network_socket_listen_entity));
});

// sync backing socket readiness after the generic socket accept poll phase.
w_ecs_system(
	wm_networking_socket_sync_after_accept_poll,
	WM_NETWORK_PHASE_PRE_ACCEPT,
	w_query(
		w_query_r(network_socket_listen_entity),
	),
{
	wm_networking_socket_sync_state(world, entity, *w_query_get(network_socket_listen_entity));
});

// sync backing socket state after the generic socket close phase.
w_ecs_system(
	wm_networking_socket_sync_after_close,
	WM_NETWORK_PHASE_POST_CLOSE,
	w_query(
		w_query_r(network_socket_listen_entity),
	),
{
	wm_networking_socket_sync_state(world, entity, *w_query_get(network_socket_listen_entity));
});

/********************************
*  network connection systems  *
********************************/
// these systems use listen sockets to create network connection entities

// accept new connections from backing listening sockets
w_ecs_system(
	wm_networking_socket_accept_and_create_connections,
	WM_NETWORK_PHASE_ACCEPT,
	w_query(
		w_query_r(network_socket_listen_entity),
		w_query_h(network_socket_accept_ready),
		w_query_n(network_socket_err),
	),
{
	w_entity_id socket_entity = *w_query_get(network_socket_listen_entity);
	if (socket_entity == W_ENTITY_INVALID || !socket_listen_fd_exists(world, socket_entity))
	{
		continue;
	}

	int32_t listen_fd = *socket_listen_fd_get(world, socket_entity);

	// accept all pending connections to this socket and create a network
	// connection entity for each one
	// note: in future, look into how we'd safely thread this
	while (true)
	{
		struct sockaddr_storage remote_addr;
		socklen_t remote_addr_len = sizeof(remote_addr);

		int32_t connection_fd = accept(listen_fd, (struct sockaddr *)&remote_addr, &remote_addr_len);

		if (connection_fd < 0)
		{
			int err = errno;

			if (err == EAGAIN || err == EWOULDBLOCK)
			{
				break;
			}

			if (err == EINTR)
			{
				continue;
			}

			w_log_entity_error(networking, "socket failed to accept connection: %s", strerror(err));
			w_set_value(entity, network_socket_err, err);
			socket_err_set_value(world, socket_entity, err);
			break;
		}

		int flags = fcntl(connection_fd, F_GETFL, 0);
		if (flags == -1 || fcntl(connection_fd, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			int err = errno;
			w_log_entity_error(networking, "failed to set accepted connection non-blocking: %s", strerror(err));
			w_set_value(entity, network_socket_err, err);
			socket_err_set_value(world, socket_entity, err);
			close(connection_fd);
			break;
		}

		w_entity_id conn = w_request();
		w_set_value(conn, network_connection_fd, connection_fd);
		w_set_value(conn, network_connection_listen_socket_entity, entity);

		w_entity_id input_stream = w_request();
		w_set_default(input_stream, stream_buffer_size);
		w_set_tag(input_stream, stream_auto_compact, true);
		w_set_tag(input_stream, req_stream_buffer_hot, true);
		w_set_value(conn, network_connection_input_stream_entity, input_stream);

		w_entity_id output_stream = w_request();
		w_set_default(output_stream, stream_buffer_size);
		w_set_tag(output_stream, stream_auto_compact, true);
		w_set_tag(output_stream, req_stream_buffer_hot, true);
		w_set_value(conn, network_connection_output_stream_entity, output_stream);
	}
});

#endif /* WHISKER_NETWORKING_SOCKET_SYSTEMS_H */
