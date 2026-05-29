/**
 * @author      : ElGatoPanzon
 * @file        : test_networking
 * @created     : Thursday May 28, 2026 17:10:00 CST
 * @description : tests for networking module socket lifecycle
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/managed_alloc/whisker_managed_alloc.h"
#include "modules/streams/whisker_streams.h"
#include "modules/utilities/whisker_utilities.h"
#include "modules/utilities/whisker_utilities_entity_lifecycle.h"
#include "modules/networking/whisker_networking.h"

#include <arpa/inet.h>
#include <check.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


/*****************************
*  fixture                   *
*****************************/

static struct w_ecs_world g_world;
static struct w_string_table g_string_table;
static struct w_arena g_arena;

static void networking_setup(void)
{
	w_arena_init(&g_arena, 64 * 1024);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);

	wm_scheduler_defaults_init(&g_world, 60.0);
	wm_managed_alloc_init(&g_world);
	wm_streams_init(&g_world);
	wm_utils_init(&g_world);
	wm_networking_init(&g_world);
}

static void networking_teardown(void)
{
	wm_networking_free(&g_world);
	wm_utils_free(&g_world);
	wm_streams_free(&g_world);
	wm_managed_alloc_free(&g_world);
	wm_scheduler_defaults_free(&g_world);
	w_ecs_world_free(&g_world);
	w_string_table_free(&g_string_table);
	w_arena_free(&g_arena);
}

static void run_phase(size_t phase_id)
{
	w_ecs_run_phase_systems(&g_world, phase_id);
}

static w_entity_id create_listener_entity(const char *host, uint16_t port)
{
	w_entity_id e = w_ecs_request_entity(&g_world);
	w_string_table_id host_id = w_string_table_intern_str(&g_string_table, host);

	network_socket_listen_host_string_id_set_value(&g_world, e, host_id);
	network_socket_listen_port_set_value(&g_world, e, port);
	network_socket_listen_backlog_set_value(&g_world, e, 8);

	return e;
}

static void request_socket_hot(w_entity_id e)
{
	req_network_socket_hot_set_tag_state(&g_world, e, true);
	run_phase(WM_NETWORK_PHASE_INIT);
}

static void request_socket_cold(w_entity_id e)
{
	req_network_socket_cold_set_tag_state(&g_world, e, true);
	run_phase(WM_NETWORK_PHASE_CLOSE);
}

static int32_t listen_fd(w_entity_id e)
{
	ck_assert(network_socket_listen_fd_exists(&g_world, e));
	return *network_socket_listen_fd_get(&g_world, e);
}

static uint16_t socket_bound_port(int fd)
{
	struct sockaddr_storage addr;
	socklen_t len = sizeof(addr);

	ck_assert_int_eq(getsockname(fd, (struct sockaddr *)&addr, &len), 0);

	if (addr.ss_family == AF_INET)
	{
		struct sockaddr_in *in = (struct sockaddr_in *)&addr;
		return ntohs(in->sin_port);
	}

	if (addr.ss_family == AF_INET6)
	{
		struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)&addr;
		return ntohs(in6->sin6_port);
	}

	ck_abort_msg("unexpected socket address family: %d", addr.ss_family);
	return 0;
}

static int connect_to_listener(int listen_fd)
{
	uint16_t port = socket_bound_port(listen_fd);
	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	ck_assert_int_ge(client_fd, 0);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	ck_assert_int_eq(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr), 1);

	ck_assert_int_eq(connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)), 0);
	return client_fd;
}

static w_entity_id find_connection_for_socket(w_entity_id socket)
{
	char *query_str = w_query(
		w_query_r(network_connection_fd),
		w_query_r(network_connection_listen_socket_entity),
	);
	struct w_query *q = w_ecs_get_query(&g_world, query_str);
	w_query_rebuild_cache(&g_world.queries, q);

	w_entity_id connection = W_ENTITY_INVALID;
	int count = 0;
	w_query_for_each(&g_world, query_str, {
		w_entity_id *listen_socket = network_connection_listen_socket_entity_get(&g_world, itor.entity_id);
		if (*listen_socket == socket)
		{
			connection = itor.entity_id;
			count++;
		}
	});

	ck_assert_int_eq(count, 1);
	return connection;
}

static w_entity_id accept_one_connection(w_entity_id socket, int *client_fd)
{
	*client_fd = connect_to_listener(listen_fd(socket));

	run_phase(WM_NETWORK_PHASE_PRE_ACCEPT);
	ck_assert(network_socket_accept_ready_tag_exists(&g_world, socket));

	run_phase(WM_NETWORK_PHASE_ACCEPT);
	ck_assert(!network_socket_err_exists(&g_world, socket));

	return find_connection_for_socket(socket);
}

static void prepare_connection_stream_buffers(w_entity_id connection)
{
	run_phase(WM_STREAM_PHASE_INPUT_PRE_CONSUME);
	run_phase(WM_STREAM_PHASE_OUTPUT_PRE_CONSUME);

	ck_assert(stream_input_buffer_handle_exists(&g_world, connection));
	ck_assert(stream_output_buffer_handle_exists(&g_world, connection));
}

static void send_client_bytes(int client_fd, const char *bytes, size_t length)
{
	ck_assert_int_eq(send(client_fd, bytes, length, 0), length);
}

static void receive_pending_connection_bytes(w_entity_id connection)
{
	run_phase(WM_NETWORK_PHASE_PRE_READ);
	ck_assert(network_connection_read_ready_tag_exists(&g_world, connection));

	run_phase(WM_NETWORK_PHASE_READ);
	ck_assert(!network_connection_err_exists(&g_world, connection));
}

static void write_connection_output(w_entity_id connection, const char *bytes, size_t length, uint64_t offset)
{
	uint8_t *buffer = wm_streams_get_output_buffer(&g_world, connection);
	ck_assert_ptr_nonnull(buffer);

	memcpy(buffer, bytes, length);
	stream_output_buffer_offset_set_value(&g_world, connection, offset);
	stream_output_buffer_length_set_value(&g_world, connection, length);
	stream_output_available_set_tag_state(&g_world, connection, true);
}

static void recv_client_bytes(int client_fd, char *buffer, size_t length)
{
	size_t received = 0;

	while (received < length)
	{
		struct pollfd pfd = {0};
		pfd.fd = client_fd;
		pfd.events = POLLIN;
		pfd.revents = 0;

		ck_assert_int_eq(poll(&pfd, 1, 1000), 1);
		ck_assert((pfd.revents & POLLIN) != 0);

		ssize_t n = recv(client_fd, buffer + received, length - received, 0);
		ck_assert_int_gt(n, 0);
		received += (size_t)n;
	}
}

static void send_pending_connection_bytes(w_entity_id connection)
{
	run_phase(WM_NETWORK_PHASE_PRE_WRITE);
	ck_assert(network_connection_write_ready_tag_exists(&g_world, connection));

	run_phase(WM_NETWORK_PHASE_WRITE);
	ck_assert(!network_connection_write_ready_tag_exists(&g_world, connection));
	ck_assert(!network_connection_err_exists(&g_world, connection));
}

static void request_connection_closed(w_entity_id connection)
{
	req_network_connection_closed_set_tag_state(&g_world, connection, true);
	run_phase(WM_NETWORK_PHASE_CLOSE);
}


/*****************************
*  socket hot                *
*****************************/

START_TEST(test_socket_hot_creates_listen_fd)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);

	request_socket_hot(e);

	ck_assert(network_socket_listen_fd_exists(&g_world, e));
	ck_assert_int_ge(*network_socket_listen_fd_get(&g_world, e), 0);
	ck_assert(!req_network_socket_hot_tag_exists(&g_world, e));
	ck_assert(!network_socket_err_exists(&g_world, e));
}
END_TEST

START_TEST(test_socket_hot_sets_fd_nonblocking)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);

	request_socket_hot(e);

	int fd = listen_fd(e);
	int flags = fcntl(fd, F_GETFL, 0);
	ck_assert_int_ne(flags, -1);
	ck_assert((flags & O_NONBLOCK) != 0);
}
END_TEST

START_TEST(test_socket_hot_is_idempotent_when_fd_exists)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(e);

	int fd = listen_fd(e);

	request_socket_hot(e);

	ck_assert_int_eq(listen_fd(e), fd);
	ck_assert(!req_network_socket_hot_tag_exists(&g_world, e));
	ck_assert(!network_socket_err_exists(&g_world, e));
}
END_TEST

START_TEST(test_socket_hot_binds_ephemeral_port)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);

	request_socket_hot(e);

	int fd = listen_fd(e);
	ck_assert_uint_ne(socket_bound_port(fd), 0);
}
END_TEST

START_TEST(test_socket_hot_listens_for_connections)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);

	request_socket_hot(e);

	int fd = listen_fd(e);
	int client_fd = connect_to_listener(fd);

	close(client_fd);
}
END_TEST

START_TEST(test_socket_hot_failure_sets_error_without_fd)
{
	w_entity_id bound = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(bound);

	uint16_t port = socket_bound_port(listen_fd(bound));
	w_entity_id duplicate = create_listener_entity("127.0.0.1", port);

	request_socket_hot(duplicate);

	ck_assert(!network_socket_listen_fd_exists(&g_world, duplicate));
	ck_assert(network_socket_err_exists(&g_world, duplicate));
	ck_assert(!req_network_socket_hot_tag_exists(&g_world, duplicate));
}
END_TEST


/*****************************
*  socket cold               *
*****************************/

START_TEST(test_socket_cold_removes_listen_fd)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(e);

	network_socket_accept_ready_set_tag_state(&g_world, e, true);
	request_socket_cold(e);

	ck_assert(!network_socket_listen_fd_exists(&g_world, e));
	ck_assert(!network_socket_accept_ready_tag_exists(&g_world, e));
	ck_assert(!req_network_socket_cold_tag_exists(&g_world, e));
}
END_TEST

START_TEST(test_socket_cold_closes_listen_fd)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(e);

	int fd = listen_fd(e);

	request_socket_cold(e);

	errno = 0;
	ck_assert_int_eq(fcntl(fd, F_GETFD), -1);
	ck_assert_int_eq(errno, EBADF);
}
END_TEST

START_TEST(test_socket_cold_is_idempotent_without_fd)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);

	req_network_socket_cold_set_tag_state(&g_world, e, true);
	network_socket_accept_ready_set_tag_state(&g_world, e, true);
	run_phase(WM_NETWORK_PHASE_CLOSE);

	ck_assert(!network_socket_listen_fd_exists(&g_world, e));
	ck_assert(!network_socket_accept_ready_tag_exists(&g_world, e));
	ck_assert(!req_network_socket_cold_tag_exists(&g_world, e));
}
END_TEST


/*****************************
*  socket destroyed          *
*****************************/

START_TEST(test_socket_destroyed_closes_then_destroys_end_of_frame)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(e);

	w_ecs_set_tag_str(&g_world, "networking_destroy_marker", e);
	req_network_socket_destroyed_set_tag_state(&g_world, e, true);

	run_phase(WM_PHASE_PRE);

	ck_assert(!req_network_socket_destroyed_tag_exists(&g_world, e));
	ck_assert(req_network_socket_cold_tag_exists(&g_world, e));
	ck_assert(req_destroy_tag_exists(&g_world, e));
	ck_assert(req_destroy_end_of_frame_tag_exists(&g_world, e));

	run_phase(WM_NETWORK_PHASE_CLOSE);
	run_phase(WM_NETWORK_PHASE_DISPOSE);

	ck_assert(w_ecs_has_tag_str(&g_world, "networking_destroy_marker", e));
	ck_assert(!network_socket_listen_fd_exists(&g_world, e));

	w_ecs_update(&g_world);

	ck_assert(!w_ecs_has_tag_str(&g_world, "networking_destroy_marker", e));
}
END_TEST


/*****************************
*  socket accept poll        *
*****************************/

START_TEST(test_socket_accept_poll_clears_ready_when_no_connection_pending)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(e);

	network_socket_accept_ready_set_tag_state(&g_world, e, true);

	run_phase(WM_NETWORK_PHASE_PRE_ACCEPT);

	ck_assert(!network_socket_accept_ready_tag_exists(&g_world, e));
	ck_assert(!network_socket_err_exists(&g_world, e));
}
END_TEST

START_TEST(test_socket_accept_poll_sets_ready_when_connection_pending)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(e);

	int client_fd = connect_to_listener(listen_fd(e));

	run_phase(WM_NETWORK_PHASE_PRE_ACCEPT);

	ck_assert(network_socket_accept_ready_tag_exists(&g_world, e));
	ck_assert(!network_socket_err_exists(&g_world, e));

	close(client_fd);
}
END_TEST


/*****************************
*  socket accept             *
*****************************/

START_TEST(test_socket_accept_creates_connection_entity)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);

	ck_assert(network_connection_fd_exists(&g_world, connection));
	ck_assert_int_ge(*network_connection_fd_get(&g_world, connection), 0);
	ck_assert(network_connection_listen_socket_entity_exists(&g_world, connection));
	ck_assert_int_eq(*network_connection_listen_socket_entity_get(&g_world, connection), socket);

	close(client_fd);
	request_connection_closed(connection);
}
END_TEST

START_TEST(test_socket_accept_sets_connection_fd_nonblocking)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);

	int fd = *network_connection_fd_get(&g_world, connection);
	int flags = fcntl(fd, F_GETFL, 0);
	ck_assert_int_ne(flags, -1);
	ck_assert((flags & O_NONBLOCK) != 0);

	close(client_fd);
	request_connection_closed(connection);
}
END_TEST


/*****************************
*  connection closed         *
*****************************/

START_TEST(test_connection_closed_request_closes_fd_and_removes_component)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);

	network_connection_read_ready_set_tag_state(&g_world, connection, true);
	network_connection_write_ready_set_tag_state(&g_world, connection, true);
	int fd = *network_connection_fd_get(&g_world, connection);

	request_connection_closed(connection);

	ck_assert(!network_connection_fd_exists(&g_world, connection));
	ck_assert(!network_connection_read_ready_tag_exists(&g_world, connection));
	ck_assert(!network_connection_write_ready_tag_exists(&g_world, connection));
	ck_assert(!req_network_connection_closed_tag_exists(&g_world, connection));

	errno = 0;
	ck_assert_int_eq(fcntl(fd, F_GETFD), -1);
	ck_assert_int_eq(errno, EBADF);

	close(client_fd);
}
END_TEST


/*****************************
*  connection destroyed      *
*****************************/

START_TEST(test_connection_destroyed_request_closes_then_destroys_end_of_frame)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);

	w_ecs_set_tag_str(&g_world, "networking_connection_destroy_marker", connection);
	req_network_connection_destroyed_set_tag_state(&g_world, connection, true);

	run_phase(WM_PHASE_PRE);

	ck_assert(!req_network_connection_destroyed_tag_exists(&g_world, connection));
	ck_assert(req_network_connection_closed_tag_exists(&g_world, connection));
	ck_assert(req_destroy_tag_exists(&g_world, connection));
	ck_assert(req_destroy_end_of_frame_tag_exists(&g_world, connection));

	run_phase(WM_NETWORK_PHASE_CLOSE);
	run_phase(WM_NETWORK_PHASE_DISPOSE);

	ck_assert(w_ecs_has_tag_str(&g_world, "networking_connection_destroy_marker", connection));
	ck_assert(!network_connection_fd_exists(&g_world, connection));

	w_ecs_update(&g_world);

	ck_assert(!w_ecs_has_tag_str(&g_world, "networking_connection_destroy_marker", connection));

	close(client_fd);
}
END_TEST


/*****************************
*  connection read poll      *
*****************************/

START_TEST(test_connection_read_poll_clears_ready_when_no_bytes_pending)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);

	network_connection_read_ready_set_tag_state(&g_world, connection, true);

	run_phase(WM_NETWORK_PHASE_PRE_READ);

	ck_assert(!network_connection_read_ready_tag_exists(&g_world, connection));
	ck_assert(!network_connection_err_exists(&g_world, connection));

	close(client_fd);
	request_connection_closed(connection);
}
END_TEST

START_TEST(test_connection_read_poll_sets_ready_when_bytes_pending)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);

	char bytes[] = "hello";
	ck_assert_int_eq(send(client_fd, bytes, sizeof(bytes) - 1, 0), sizeof(bytes) - 1);

	run_phase(WM_NETWORK_PHASE_PRE_READ);

	ck_assert(network_connection_read_ready_tag_exists(&g_world, connection));
	ck_assert(!network_connection_err_exists(&g_world, connection));

	close(client_fd);
	request_connection_closed(connection);
}
END_TEST


/*****************************
*  connection receive        *
*****************************/

START_TEST(test_connection_receive_appends_bytes_to_stream_input)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);
	prepare_connection_stream_buffers(connection);

	char bytes[] = "hello";
	send_client_bytes(client_fd, bytes, sizeof(bytes) - 1);

	receive_pending_connection_bytes(connection);

	uint8_t *buffer = wm_streams_get_input_buffer(&g_world, connection);
	ck_assert_ptr_nonnull(buffer);
	ck_assert_uint_eq(*stream_input_buffer_length_get(&g_world, connection), sizeof(bytes) - 1);
	ck_assert_int_eq(memcmp(buffer, bytes, sizeof(bytes) - 1), 0);
	ck_assert(stream_input_available_tag_exists(&g_world, connection));

	close(client_fd);
	request_connection_closed(connection);
}
END_TEST

START_TEST(test_connection_receive_preserves_partial_data_across_receives)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);
	prepare_connection_stream_buffers(connection);

	char first[] = "hello ";
	char second[] = "world";
	send_client_bytes(client_fd, first, sizeof(first) - 1);
	receive_pending_connection_bytes(connection);

	send_client_bytes(client_fd, second, sizeof(second) - 1);
	receive_pending_connection_bytes(connection);

	char expected[] = "hello world";
	uint8_t *buffer = wm_streams_get_input_buffer(&g_world, connection);
	ck_assert_ptr_nonnull(buffer);
	ck_assert_uint_eq(*stream_input_buffer_length_get(&g_world, connection), sizeof(expected) - 1);
	ck_assert_int_eq(memcmp(buffer, expected, sizeof(expected) - 1), 0);
	ck_assert(stream_input_available_tag_exists(&g_world, connection));

	close(client_fd);
	request_connection_closed(connection);
}
END_TEST

START_TEST(test_connection_receive_eof_requests_connection_closed)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);
	prepare_connection_stream_buffers(connection);

	close(client_fd);
	network_connection_read_ready_set_tag_state(&g_world, connection, true);

	run_phase(WM_NETWORK_PHASE_READ);

	ck_assert(req_network_connection_closed_tag_exists(&g_world, connection));
	ck_assert(!network_connection_err_exists(&g_world, connection));
	ck_assert_uint_eq(*stream_input_buffer_length_get(&g_world, connection), 0);
	ck_assert(!stream_input_available_tag_exists(&g_world, connection));

	request_connection_closed(connection);
}
END_TEST


/*****************************
*  connection write poll     *
*****************************/

START_TEST(test_connection_write_poll_sets_ready_when_output_available)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);
	prepare_connection_stream_buffers(connection);

	char bytes[] = "hello";
	write_connection_output(connection, bytes, sizeof(bytes) - 1, 0);

	run_phase(WM_NETWORK_PHASE_PRE_WRITE);

	ck_assert(network_connection_write_ready_tag_exists(&g_world, connection));
	ck_assert(!network_connection_err_exists(&g_world, connection));

	close(client_fd);
	request_connection_closed(connection);
}
END_TEST


/*****************************
*  connection send           *
*****************************/

START_TEST(test_connection_send_writes_output_bytes_and_advances_offset)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);
	prepare_connection_stream_buffers(connection);

	char bytes[] = "hello";
	write_connection_output(connection, bytes, sizeof(bytes) - 1, 0);

	send_pending_connection_bytes(connection);

	char received[sizeof(bytes) - 1] = {0};
	recv_client_bytes(client_fd, received, sizeof(received));

	ck_assert_int_eq(memcmp(received, bytes, sizeof(received)), 0);
	ck_assert_uint_eq(*stream_output_buffer_offset_get(&g_world, connection), sizeof(bytes) - 1);
	ck_assert_uint_eq(*stream_output_buffer_length_get(&g_world, connection), sizeof(bytes) - 1);

	close(client_fd);
	request_connection_closed(connection);
}
END_TEST

START_TEST(test_connection_send_preserves_unsent_bytes_after_partial_send)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);
	prepare_connection_stream_buffers(connection);

	char bytes[] = "hello";
	write_connection_output(connection, bytes, sizeof(bytes) - 1, 2);

	send_pending_connection_bytes(connection);

	char expected[] = "llo";
	char received[sizeof(expected) - 1] = {0};
	recv_client_bytes(client_fd, received, sizeof(received));

	ck_assert_int_eq(memcmp(received, expected, sizeof(received)), 0);
	ck_assert_uint_eq(*stream_output_buffer_offset_get(&g_world, connection), sizeof(bytes) - 1);
	ck_assert_uint_eq(*stream_output_buffer_length_get(&g_world, connection), sizeof(bytes) - 1);

	close(client_fd);
	request_connection_closed(connection);
}
END_TEST


/*****************************
*  closed after write        *
*****************************/

START_TEST(test_connection_closed_after_write_waits_while_output_pending)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);
	prepare_connection_stream_buffers(connection);

	char bytes[] = "hello";
	write_connection_output(connection, bytes, sizeof(bytes) - 1, 0);
	req_network_connection_closed_after_write_set_tag_state(&g_world, connection, true);

	run_phase(WM_NETWORK_PHASE_POST_WRITE);

	ck_assert(req_network_connection_closed_after_write_tag_exists(&g_world, connection));
	ck_assert(!req_network_connection_closed_tag_exists(&g_world, connection));

	close(client_fd);
	request_connection_closed(connection);
}
END_TEST

START_TEST(test_connection_closed_after_write_requests_close_when_no_pending_io)
{
	w_entity_id socket = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(socket);

	int client_fd = -1;
	w_entity_id connection = accept_one_connection(socket, &client_fd);
	prepare_connection_stream_buffers(connection);

	stream_input_buffer_offset_set_value(&g_world, connection, 0);
	stream_input_buffer_length_set_value(&g_world, connection, 0);
	stream_output_buffer_offset_set_value(&g_world, connection, 5);
	stream_output_buffer_length_set_value(&g_world, connection, 5);
	req_network_connection_closed_after_write_set_tag_state(&g_world, connection, true);

	run_phase(WM_NETWORK_PHASE_POST_WRITE);

	ck_assert(!req_network_connection_closed_after_write_tag_exists(&g_world, connection));
	ck_assert(req_network_connection_closed_tag_exists(&g_world, connection));

	close(client_fd);
	request_connection_closed(connection);
}
END_TEST


/*****************************
*  suite + runner            *
*****************************/

Suite *networking_suite(void)
{
	Suite *s = suite_create("networking");

	TCase *tc_socket_hot = tcase_create("socket_hot");
	tcase_add_checked_fixture(tc_socket_hot, networking_setup, networking_teardown);
	tcase_add_test(tc_socket_hot, test_socket_hot_creates_listen_fd);
	tcase_add_test(tc_socket_hot, test_socket_hot_sets_fd_nonblocking);
	tcase_add_test(tc_socket_hot, test_socket_hot_is_idempotent_when_fd_exists);
	tcase_add_test(tc_socket_hot, test_socket_hot_binds_ephemeral_port);
	tcase_add_test(tc_socket_hot, test_socket_hot_listens_for_connections);
	tcase_add_test(tc_socket_hot, test_socket_hot_failure_sets_error_without_fd);
	suite_add_tcase(s, tc_socket_hot);

	TCase *tc_socket_cold = tcase_create("socket_cold");
	tcase_add_checked_fixture(tc_socket_cold, networking_setup, networking_teardown);
	tcase_add_test(tc_socket_cold, test_socket_cold_removes_listen_fd);
	tcase_add_test(tc_socket_cold, test_socket_cold_closes_listen_fd);
	tcase_add_test(tc_socket_cold, test_socket_cold_is_idempotent_without_fd);
	suite_add_tcase(s, tc_socket_cold);

	TCase *tc_socket_destroyed = tcase_create("socket_destroyed");
	tcase_add_checked_fixture(tc_socket_destroyed, networking_setup, networking_teardown);
	tcase_add_test(tc_socket_destroyed, test_socket_destroyed_closes_then_destroys_end_of_frame);
	suite_add_tcase(s, tc_socket_destroyed);

	TCase *tc_socket_accept_poll = tcase_create("socket_accept_poll");
	tcase_add_checked_fixture(tc_socket_accept_poll, networking_setup, networking_teardown);
	tcase_add_test(tc_socket_accept_poll, test_socket_accept_poll_clears_ready_when_no_connection_pending);
	tcase_add_test(tc_socket_accept_poll, test_socket_accept_poll_sets_ready_when_connection_pending);
	suite_add_tcase(s, tc_socket_accept_poll);

	TCase *tc_socket_accept = tcase_create("socket_accept");
	tcase_add_checked_fixture(tc_socket_accept, networking_setup, networking_teardown);
	tcase_add_test(tc_socket_accept, test_socket_accept_creates_connection_entity);
	tcase_add_test(tc_socket_accept, test_socket_accept_sets_connection_fd_nonblocking);
	suite_add_tcase(s, tc_socket_accept);

	TCase *tc_connection_closed = tcase_create("connection_closed");
	tcase_add_checked_fixture(tc_connection_closed, networking_setup, networking_teardown);
	tcase_add_test(tc_connection_closed, test_connection_closed_request_closes_fd_and_removes_component);
	suite_add_tcase(s, tc_connection_closed);

	TCase *tc_connection_destroyed = tcase_create("connection_destroyed");
	tcase_add_checked_fixture(tc_connection_destroyed, networking_setup, networking_teardown);
	tcase_add_test(tc_connection_destroyed, test_connection_destroyed_request_closes_then_destroys_end_of_frame);
	suite_add_tcase(s, tc_connection_destroyed);

	TCase *tc_connection_read_poll = tcase_create("connection_read_poll");
	tcase_add_checked_fixture(tc_connection_read_poll, networking_setup, networking_teardown);
	tcase_add_test(tc_connection_read_poll, test_connection_read_poll_clears_ready_when_no_bytes_pending);
	tcase_add_test(tc_connection_read_poll, test_connection_read_poll_sets_ready_when_bytes_pending);
	suite_add_tcase(s, tc_connection_read_poll);

	TCase *tc_connection_receive = tcase_create("connection_receive");
	tcase_add_checked_fixture(tc_connection_receive, networking_setup, networking_teardown);
	tcase_add_test(tc_connection_receive, test_connection_receive_appends_bytes_to_stream_input);
	tcase_add_test(tc_connection_receive, test_connection_receive_preserves_partial_data_across_receives);
	tcase_add_test(tc_connection_receive, test_connection_receive_eof_requests_connection_closed);
	suite_add_tcase(s, tc_connection_receive);

	TCase *tc_connection_write_poll = tcase_create("connection_write_poll");
	tcase_add_checked_fixture(tc_connection_write_poll, networking_setup, networking_teardown);
	tcase_add_test(tc_connection_write_poll, test_connection_write_poll_sets_ready_when_output_available);
	suite_add_tcase(s, tc_connection_write_poll);

	TCase *tc_connection_send = tcase_create("connection_send");
	tcase_add_checked_fixture(tc_connection_send, networking_setup, networking_teardown);
	tcase_add_test(tc_connection_send, test_connection_send_writes_output_bytes_and_advances_offset);
	tcase_add_test(tc_connection_send, test_connection_send_preserves_unsent_bytes_after_partial_send);
	suite_add_tcase(s, tc_connection_send);

	TCase *tc_connection_closed_after_write = tcase_create("connection_closed_after_write");
	tcase_add_checked_fixture(tc_connection_closed_after_write, networking_setup, networking_teardown);
	tcase_add_test(tc_connection_closed_after_write, test_connection_closed_after_write_waits_while_output_pending);
	tcase_add_test(tc_connection_closed_after_write, test_connection_closed_after_write_requests_close_when_no_pending_io);
	suite_add_tcase(s, tc_connection_closed_after_write);

	return s;
}

int main(void)
{
	Suite *s = networking_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
