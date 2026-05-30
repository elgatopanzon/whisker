/**
 * @author      : ElGatoPanzon
 * @file        : whisker_sockets
 * @created     : Friday May 29, 2026 00:00:00 CST
 * @description : Generic socket entity lifecycle and polling module
 */

#ifndef WHISKER_SOCKETS_H
#define WHISKER_SOCKETS_H

#include "whisker.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

enum WM_SOCKET_PHASE
{
	WM_SOCKET_PHASE_INIT = 206,
	WM_SOCKET_PHASE_PRE_ACCEPT = 207,
	WM_SOCKET_PHASE_CLOSE = 208,
};

/****************
*  components  *
****************/

w_ecs_define_component(w_string_table_id, socket_listen_host_string_id, W_STRING_TABLE_INVALID_ID);
w_ecs_define_component(uint16_t, socket_listen_port, 0);
w_ecs_define_component(int32_t, socket_listen_backlog, 128);
w_ecs_define_component(int32_t, socket_listen_fd, -1);
w_ecs_define_component(int, socket_err, -1);

w_ecs_define_tag(socket_accept_ready);

w_ecs_define_tag(req_socket_hot);
w_ecs_define_tag(req_socket_cold);
w_ecs_define_tag(req_socket_destroyed);

void wm_sockets_init(struct w_ecs_world *world);
void wm_sockets_free(struct w_ecs_world *world);

#endif /* WHISKER_SOCKETS_H */
