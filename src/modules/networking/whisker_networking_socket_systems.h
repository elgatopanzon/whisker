/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_networking_socket_systems
 * @created     : Thursday May 28, 2026 11:40:16 CST
 * @description : systems related to listening sockets
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

#ifndef WHISKER_NETWORKING_SOCKET_SYSTEMS_H
#define WHISKER_NETWORKING_SOCKET_SYSTEMS_H

#define wm_networking_handle_addrinfo_listen_error_and_continue(module, fmt) \
	do { \
		err = errno; \
		w_log_entity_warning(module, fmt, strerror(err)); \
		if (fd >= 0) close(fd); \
		fd = -1; \
		continue; \
	} while(0)

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
	w_entity_destroy_end_of_frame(world, entity);
});

// handle requesting socket hot
// note: system is transactional, errors are set for any failure
w_ecs_system(
	wm_networking_socket_handle_request_hot,
	WM_NETWORK_PHASE_INIT,
	w_query(
		w_query_h(req_network_socket_hot),

		// socket components
		w_query_r(network_socket_listen_host_string_id),
		w_query_r(network_socket_listen_port),
		// optional default backlog
		w_query_o(network_socket_listen_backlog),

		// avoid already errored sockets
		w_query_n(network_socket_err),
	),
{
	// clear hot request
	w_set_tag(entity, req_network_socket_hot, false);

	// already hot, request satisfied
	if (network_socket_listen_fd_exists(world, entity))
	{
		continue;
	}

	char *listen_host = w_string_from_id(*w_query_get(network_socket_listen_host_string_id));
	int listen_port_int = *w_query_get(network_socket_listen_port);
	int listen_backlog = *w_query_get_opt_or_default(network_socket_listen_backlog);
	int err = -1;

	// port to string
	char listen_port[6]; // max "65535" + '\0'
  	snprintf(listen_port, sizeof(listen_port), "%u", (unsigned)listen_port_int);

	// parse host into addrinfo struct
	struct addrinfo hints = {0};
    struct addrinfo *result = NULL;

	hints.ai_family = AF_UNSPEC; // v4 or v6
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE;

	err = getaddrinfo(listen_host, listen_port, &hints, &result);

	// bail if failed to parse listen host and port
	if (err != 0)
    {
		w_log_entity_error(networking, "getaddrinfo: %s", gai_strerror(err));
		w_set_value(entity, network_socket_err, err);
        continue;
    }

	// attempt to open socket with addrinfo results
	int fd = -1;
	for (struct addrinfo *addrinfo = result; addrinfo != NULL; addrinfo = addrinfo->ai_next)
	{
		// attempt to request, bind and listen on socket
		fd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);

		if (fd < 0)
		{
			wm_networking_handle_addrinfo_listen_error_and_continue(networking, "socket request failed: %s");
		}
		int yes = 1;

		// allow reuse of existing address/port
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != 0)
		{
			wm_networking_handle_addrinfo_listen_error_and_continue(networking, "socket reuse opt failed: %s");
		}

		// set socket as non-blocking
		int flags = fcntl(fd, F_GETFL, 0);
		if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			// failed to set non-blocking flags
			wm_networking_handle_addrinfo_listen_error_and_continue(networking, "socket non-blocking flag failed: %s");
		}

		// try to bind
		err = bind(fd, addrinfo->ai_addr, addrinfo->ai_addrlen);
		if (err != 0)
		{
			wm_networking_handle_addrinfo_listen_error_and_continue(networking, "socket bind failed: %s");
		}

		// try to listen
		err = listen(fd, listen_backlog);
		if (err != 0)
		{
			wm_networking_handle_addrinfo_listen_error_and_continue(networking, "socket listen failed: %s");
		}

		// if we got this far its listening!
		break;
	}

	// cleanup addrinfo
	freeaddrinfo(result);

	// if socket listen failed set error component and continue
	if (fd == -1)
	{
		w_log_entity_error(networking, "failed to listen on socket: %s", strerror(err));
		w_set_value(entity, network_socket_err, err);
        continue;
	}

	// set socket fd as the valid listening socket
	w_set_value(entity, network_socket_listen_fd, fd);
});


// handle request socket cold
w_ecs_system(
	wm_networking_socket_handle_request_cold,
	WM_NETWORK_PHASE_CLOSE,
	w_query(
		w_query_h(req_network_socket_cold),
		w_query_o(network_socket_listen_fd),
	),
{
	w_set_tag(entity, req_network_socket_cold, false);
	w_set_tag(entity, network_socket_accept_ready, false);

	// if fd is valid, close it
	int32_t fd = *w_query_get_opt_or_default(network_socket_listen_fd);
	if (fd >= 0)
	{
		if (close(fd) != 0)
		{
			int err = errno;
			w_log_entity_error(networking, "failed to close socket: %s", strerror(err));
			w_set_value(entity, network_socket_err, err);
		}
	}

	// remove socket fd
	w_remove(entity, network_socket_listen_fd);
});


// socket accept poll, polls and sets/removes the tag indicating that this socket is ready to accept connections
w_ecs_system(
	wm_networking_socket_accept_poll,
	WM_NETWORK_PHASE_PRE_ACCEPT,
	w_query(
		// pick up valid non-error sockets
		w_query_r(network_socket_listen_fd),
		// skip errored sockets
		w_query_n(network_socket_err),
	),
{
	int32_t fd = *w_query_get(network_socket_listen_fd);

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
		w_log_entity_error(networking, "failed to poll socket for readiness: %s", strerror(err));
		w_set_value(entity, network_socket_err, err);
		w_set_tag(entity, network_socket_accept_ready, false);
		continue;
	}

	// poll may report socket error events without poll itself failing
	if (result > 0 && (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)))
	{
		w_log_entity_error(networking, "socket poll returned error event: %d", pfd.revents);
		w_set_value(entity, network_socket_err, pfd.revents);
		w_set_tag(entity, network_socket_accept_ready, false);
		continue;
	}

	// if the return event includes POLLIN and we get a positive return
	// value, we have a connection ready to accept on this listen socket
	w_set_tag(entity, network_socket_accept_ready, result > 0 && (pfd.revents & POLLIN));
});


/********************************
*  network connection systems  *
********************************/
// these systems use listen sockets to create network connection entities

// accept new connections from listening sockets
w_ecs_system(
	wm_networking_socket_accept_and_create_connections,
	WM_NETWORK_PHASE_ACCEPT,
	w_query(
		// pick up sockets with accept readiness
		w_query_r(network_socket_listen_fd),
		w_query_h(network_socket_accept_ready),
		// skip errored sockets
		w_query_n(network_socket_err),
	),
{
	int32_t listen_fd = *w_query_get(network_socket_listen_fd);

	// accept all pending connections to this socket and create a network
	// connection entity for each one
	// note: in future, look into how we'd safely thread this
	while (true)
	{
		// accept connection and get client fd
		struct sockaddr_storage remote_addr;
		socklen_t remote_addr_len = sizeof(remote_addr);

		int32_t connection_fd = accept(listen_fd, (struct sockaddr *)&remote_addr, &remote_addr_len);

		// check for accept errors
		if (connection_fd < 0)
		{
			int err = errno;

			// break out of the loop, there's nothing to get
			if (err == EAGAIN || err == EWOULDBLOCK)
			{
				break;
			}

			// the accept listened due to being interupted
			// we can try again
			if (err == EINTR)
			{
				continue;
			}


			// log the error with this socket
			w_log_entity_error(networking, "socket failed to accept connection: %s", strerror(err));
			w_set_value(entity, network_socket_err, err);
			break;
		}

		// with the connection fd lets try and set it non-blocking
		int flags = fcntl(connection_fd, F_GETFL, 0);
		if (flags == -1 || fcntl(connection_fd, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			int err = errno;
			w_log_entity_error(networking, "failed to set accepted connection non-blocking: %s", strerror(err));
			w_set_value(entity, network_socket_err, err);
			close(connection_fd);
			break;
		}

		// if we made it this far, we have a new valid connection entity!
		w_entity_id conn = w_request();
		w_set_value(conn, network_connection_fd, connection_fd);
		w_set_value(conn, network_connection_listen_socket_entity, entity);

		// prepare input and output stream buffers for this connection
		w_set_default(conn, stream_input_buffer_size);
		w_set_default(conn, stream_output_buffer_size);
		w_set_tag(conn, req_stream_input_buffer_hot, true);
		w_set_tag(conn, req_stream_output_buffer_hot, true);
	}
});


#endif /* WHISKER_NETWORKING_SOCKET_SYSTEMS_H */
