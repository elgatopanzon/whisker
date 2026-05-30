/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_sockets_systems
 * @created     : Friday May 29, 2026 00:00:00 CST
 * @description : systems related to generic socket entities
 */

#include "whisker_sockets.h"
#include "modules/utilities/whisker_utilities_entity_lifecycle.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef WHISKER_SOCKETS_SYSTEMS_H
#define WHISKER_SOCKETS_SYSTEMS_H

#define wm_sockets_handle_addrinfo_listen_error_and_continue(module, fmt) \
	do { \
		err = errno; \
		w_log_entity_warning(module, fmt, strerror(err)); \
		if (fd >= 0) close(fd); \
		fd = -1; \
		continue; \
	} while(0)

w_ecs_system(
	wm_sockets_handle_request_destroyed,
	WM_PHASE_PRE,
	w_query(
		w_query_h(req_socket_destroyed),
	),
{
	w_set_tag(entity, req_socket_destroyed, false);
	w_set_tag(entity, req_socket_cold, true);
	w_entity_destroy_end_of_frame(world, entity);
});

w_ecs_system(
	wm_sockets_handle_request_hot,
	WM_SOCKET_PHASE_INIT,
	w_query(
		w_query_h(req_socket_hot),
		w_query_r(socket_listen_host_string_id),
		w_query_r(socket_listen_port),
		w_query_o(socket_listen_backlog),
		w_query_n(socket_err),
	),
{
	w_set_tag(entity, req_socket_hot, false);

	if (socket_listen_fd_exists(world, entity))
	{
		continue;
	}

	char *listen_host = w_string_from_id(*w_query_get(socket_listen_host_string_id));
	int listen_port_int = *w_query_get(socket_listen_port);
	int listen_backlog = *w_query_get_opt_or_default(socket_listen_backlog);
	int err = -1;

	char listen_port[6];
	snprintf(listen_port, sizeof(listen_port), "%u", (unsigned)listen_port_int);

	struct addrinfo hints = {0};
	struct addrinfo *result = NULL;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	err = getaddrinfo(listen_host, listen_port, &hints, &result);
	if (err != 0)
	{
		w_log_entity_error(sockets, "getaddrinfo: %s", gai_strerror(err));
		w_set_value(entity, socket_err, err);
		continue;
	}

	int fd = -1;
	for (struct addrinfo *addrinfo = result; addrinfo != NULL; addrinfo = addrinfo->ai_next)
	{
		fd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
		if (fd < 0)
		{
			wm_sockets_handle_addrinfo_listen_error_and_continue(sockets, "socket request failed: %s");
		}

		int yes = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != 0)
		{
			wm_sockets_handle_addrinfo_listen_error_and_continue(sockets, "socket reuse opt failed: %s");
		}

		int flags = fcntl(fd, F_GETFL, 0);
		if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			wm_sockets_handle_addrinfo_listen_error_and_continue(sockets, "socket non-blocking flag failed: %s");
		}

		err = bind(fd, addrinfo->ai_addr, addrinfo->ai_addrlen);
		if (err != 0)
		{
			wm_sockets_handle_addrinfo_listen_error_and_continue(sockets, "socket bind failed: %s");
		}

		err = listen(fd, listen_backlog);
		if (err != 0)
		{
			wm_sockets_handle_addrinfo_listen_error_and_continue(sockets, "socket listen failed: %s");
		}

		break;
	}

	freeaddrinfo(result);

	if (fd == -1)
	{
		w_log_entity_error(sockets, "failed to listen on socket: %s", strerror(err));
		w_set_value(entity, socket_err, err);
		continue;
	}

	w_set_value(entity, socket_listen_fd, fd);
});

w_ecs_system(
	wm_sockets_handle_request_cold,
	WM_SOCKET_PHASE_CLOSE,
	w_query(
		w_query_h(req_socket_cold),
		w_query_o(socket_listen_fd),
	),
{
	w_set_tag(entity, req_socket_cold, false);
	w_set_tag(entity, socket_accept_ready, false);

	int32_t fd = *w_query_get_opt_or_default(socket_listen_fd);
	if (fd >= 0)
	{
		if (close(fd) != 0)
		{
			int err = errno;
			w_log_entity_error(sockets, "failed to close socket: %s", strerror(err));
			w_set_value(entity, socket_err, err);
		}
	}

	w_remove(entity, socket_listen_fd);
});

w_ecs_system(
	wm_sockets_accept_poll,
	WM_SOCKET_PHASE_PRE_ACCEPT,
	w_query(
		w_query_r(socket_listen_fd),
		w_query_n(socket_err),
	),
{
	int32_t fd = *w_query_get(socket_listen_fd);

	struct pollfd pfd = {0};
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int result = poll(&pfd, 1, 0);

	if (result == -1)
	{
		int err = errno;
		w_log_entity_error(sockets, "failed to poll socket for readiness: %s", strerror(err));
		w_set_value(entity, socket_err, err);
		w_set_tag(entity, socket_accept_ready, false);
		continue;
	}

	if (result > 0 && (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)))
	{
		w_log_entity_error(sockets, "socket poll returned error event: %d", pfd.revents);
		w_set_value(entity, socket_err, pfd.revents);
		w_set_tag(entity, socket_accept_ready, false);
		continue;
	}

	w_set_tag(entity, socket_accept_ready, result > 0 && (pfd.revents & POLLIN));
});

#endif /* WHISKER_SOCKETS_SYSTEMS_H */
