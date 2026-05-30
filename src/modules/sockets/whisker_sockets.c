/**
 * @author      : ElGatoPanzon
 * @file        : whisker_sockets
 * @created     : Friday May 29, 2026 00:00:00 CST
 * @description : Generic socket entity lifecycle and polling module
 */

#include "whisker_sockets.h"
#include "whisker_sockets_systems.h"

void wm_sockets_init(struct w_ecs_world *world)
{
	struct w_scheduler_phase phase_socket_init = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "SOCKET_INIT"};
	struct w_scheduler_phase phase_socket_pre_accept = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "SOCKET_PRE_ACCEPT"};
	struct w_scheduler_phase phase_socket_close = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_POST, .name = "SOCKET_CLOSE"};

	w_ecs_register_system_phase_at(world, &phase_socket_init, WM_SOCKET_PHASE_INIT);
	w_ecs_register_system_phase_at(world, &phase_socket_pre_accept, WM_SOCKET_PHASE_PRE_ACCEPT);
	w_ecs_register_system_phase_at(world, &phase_socket_close, WM_SOCKET_PHASE_CLOSE);

	w_ecs_set_phase_chain(world,
		WM_PHASE_PRE_LOAD,
		WM_SOCKET_PHASE_INIT,
		WM_SOCKET_PHASE_PRE_ACCEPT,
	);

	w_ecs_set_phase_chain(world,
		WM_PHASE_FINAL,
		WM_SOCKET_PHASE_CLOSE,
	);

	wm_sockets_handle_request_destroyed_register(world);
	wm_sockets_handle_request_hot_register(world);
	wm_sockets_handle_request_cold_register(world);
	wm_sockets_accept_poll_register(world);
}

void wm_sockets_free(struct w_ecs_world *world)
{
	(void)world;
}
