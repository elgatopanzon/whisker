/**
 * @author      : ElGatoPanzon
 * @file        : test_sockets
 * @created     : Friday May 29, 2026 00:00:00 CST
 * @description : tests for generic sockets module
 */

#include "whisker_std.h"
#include "whisker_ecs_world.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/sockets/whisker_sockets.h"

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

static struct w_ecs_world g_world;
static struct w_string_table g_string_table;
static struct w_arena g_arena;

static void sockets_setup(void)
{
	w_arena_init(&g_arena, 64 * 1024);
	w_string_table_init(&g_string_table, &g_arena, 16, 64, NULL);
	w_ecs_world_init(&g_world, &g_string_table, &g_arena);

	wm_scheduler_defaults_init(&g_world, 60.0);
	wm_sockets_init(&g_world);
}

static void sockets_teardown(void)
{
	wm_sockets_free(&g_world);
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

	socket_listen_host_string_id_set_value(&g_world, e, host_id);
	socket_listen_port_set_value(&g_world, e, port);
	socket_listen_backlog_set_value(&g_world, e, 8);

	return e;
}

static void request_socket_hot(w_entity_id e)
{
	req_socket_hot_set_tag_state(&g_world, e, true);
	run_phase(WM_SOCKET_PHASE_INIT);
}

static void request_socket_cold(w_entity_id e)
{
	req_socket_cold_set_tag_state(&g_world, e, true);
	run_phase(WM_SOCKET_PHASE_CLOSE);
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

START_TEST(test_socket_hot_creates_listen_fd)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);

	request_socket_hot(e);

	ck_assert(socket_listen_fd_exists(&g_world, e));
	ck_assert_int_ge(*socket_listen_fd_get(&g_world, e), 0);
	ck_assert(!req_socket_hot_tag_exists(&g_world, e));
	ck_assert(!socket_err_exists(&g_world, e));
}
END_TEST

START_TEST(test_socket_cold_closes_listen_fd)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(e);

	int fd = *socket_listen_fd_get(&g_world, e);

	request_socket_cold(e);

	errno = 0;
	ck_assert(!socket_listen_fd_exists(&g_world, e));
	ck_assert_int_eq(fcntl(fd, F_GETFD), -1);
	ck_assert_int_eq(errno, EBADF);
}
END_TEST

START_TEST(test_socket_accept_poll_sets_ready_when_connection_pending)
{
	w_entity_id e = create_listener_entity("127.0.0.1", 0);
	request_socket_hot(e);

	int client_fd = connect_to_listener(*socket_listen_fd_get(&g_world, e));

	run_phase(WM_SOCKET_PHASE_PRE_ACCEPT);

	ck_assert(socket_accept_ready_tag_exists(&g_world, e));
	ck_assert(!socket_err_exists(&g_world, e));

	close(client_fd);
	request_socket_cold(e);
}
END_TEST

Suite *sockets_suite(void)
{
	Suite *s = suite_create("sockets");

	TCase *tc_lifecycle = tcase_create("lifecycle");
	tcase_add_checked_fixture(tc_lifecycle, sockets_setup, sockets_teardown);
	tcase_add_test(tc_lifecycle, test_socket_hot_creates_listen_fd);
	tcase_add_test(tc_lifecycle, test_socket_cold_closes_listen_fd);
	suite_add_tcase(s, tc_lifecycle);

	TCase *tc_accept = tcase_create("accept");
	tcase_add_checked_fixture(tc_accept, sockets_setup, sockets_teardown);
	tcase_add_test(tc_accept, test_socket_accept_poll_sets_ready_when_connection_pending);
	suite_add_tcase(s, tc_accept);

	return s;
}

int main(void)
{
	Suite *s = sockets_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
