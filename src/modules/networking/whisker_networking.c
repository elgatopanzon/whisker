/**
 * @author      : ElGatoPanzon
 * @file        : whisker_networking
 * @created     : Thursday May 28, 2026 11:08:36 CST
 * @description : Module providing networking functionality using the streams module
 */

#include "whisker_networking.h"
#include "whisker_networking_socket_systems.h"
#include "whisker_networking_connection_systems.h"
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"

void wm_networking_init(struct w_ecs_world *world)
{
	// register custom phases
	struct w_scheduler_phase phase_network_pre_init = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "NETWORK_PRE_INIT"};
	struct w_scheduler_phase phase_network_init = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "NETWORK_INIT"};
	struct w_scheduler_phase phase_network_post_init = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "NETWORK_POST_INIT"};
	struct w_scheduler_phase phase_network_pre_accept = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "NETWORK_PRE_ACCEPT"};
	struct w_scheduler_phase phase_network_accept = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "NETWORK_ACCEPT"};
	struct w_scheduler_phase phase_network_post_accept = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "NETWORK_POST_ACCEPT"};
	struct w_scheduler_phase phase_network_pre_read = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "NETWORK_PRE_READ"};
	struct w_scheduler_phase phase_network_read = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "NETWORK_READ"};
	struct w_scheduler_phase phase_network_post_read = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "NETWORK_POST_READ"};
	struct w_scheduler_phase phase_network_pre_protocol = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "NETWORK_PRE_PROTOCOL"};
	struct w_scheduler_phase phase_network_protocol = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "NETWORK_PROTOCOL"};
	struct w_scheduler_phase phase_network_post_protocol = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT, .name = "NETWORK_POST_PROTOCOL"};
	struct w_scheduler_phase phase_network_pre_write = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_POST, .name = "NETWORK_PRE_WRITE"};
	struct w_scheduler_phase phase_network_write = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_POST, .name = "NETWORK_WRITE"};
	struct w_scheduler_phase phase_network_post_write = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_POST, .name = "NETWORK_POST_WRITE"};
	struct w_scheduler_phase phase_network_pre_close = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_POST, .name = "NETWORK_PRE_CLOSE"};
	struct w_scheduler_phase phase_network_close = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_POST, .name = "NETWORK_CLOSE"};
	struct w_scheduler_phase phase_network_post_close = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_POST, .name = "NETWORK_POST_CLOSE"};
	struct w_scheduler_phase phase_network_pre_dispose = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_POST, .name = "NETWORK_PRE_DISPOSE"};
	struct w_scheduler_phase phase_network_dispose = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_POST, .name = "NETWORK_DISPOSE"};
	struct w_scheduler_phase phase_network_post_dispose = {.enabled = true, .time_step_id = WM_TIMESTEP_DEFAULT_POST, .name = "NETWORK_POST_DISPOSE"};

	w_ecs_register_system_phase_at(world, &phase_network_pre_init, WM_NETWORK_PHASE_PRE_INIT);
	w_ecs_register_system_phase_at(world, &phase_network_init, WM_NETWORK_PHASE_INIT);
	w_ecs_register_system_phase_at(world, &phase_network_post_init, WM_NETWORK_PHASE_POST_INIT);
	w_ecs_register_system_phase_at(world, &phase_network_pre_accept, WM_NETWORK_PHASE_PRE_ACCEPT);
	w_ecs_register_system_phase_at(world, &phase_network_accept, WM_NETWORK_PHASE_ACCEPT);
	w_ecs_register_system_phase_at(world, &phase_network_post_accept, WM_NETWORK_PHASE_POST_ACCEPT);
	w_ecs_register_system_phase_at(world, &phase_network_pre_read, WM_NETWORK_PHASE_PRE_READ);
	w_ecs_register_system_phase_at(world, &phase_network_read, WM_NETWORK_PHASE_READ);
	w_ecs_register_system_phase_at(world, &phase_network_post_read, WM_NETWORK_PHASE_POST_READ);
	w_ecs_register_system_phase_at(world, &phase_network_pre_protocol, WM_NETWORK_PHASE_PRE_PROTOCOL);
	w_ecs_register_system_phase_at(world, &phase_network_protocol, WM_NETWORK_PHASE_PROTOCOL);
	w_ecs_register_system_phase_at(world, &phase_network_post_protocol, WM_NETWORK_PHASE_POST_PROTOCOL);
	w_ecs_register_system_phase_at(world, &phase_network_pre_write, WM_NETWORK_PHASE_PRE_WRITE);
	w_ecs_register_system_phase_at(world, &phase_network_write, WM_NETWORK_PHASE_WRITE);
	w_ecs_register_system_phase_at(world, &phase_network_post_write, WM_NETWORK_PHASE_POST_WRITE);
	w_ecs_register_system_phase_at(world, &phase_network_pre_close, WM_NETWORK_PHASE_PRE_CLOSE);
	w_ecs_register_system_phase_at(world, &phase_network_close, WM_NETWORK_PHASE_CLOSE);
	w_ecs_register_system_phase_at(world, &phase_network_post_close, WM_NETWORK_PHASE_POST_CLOSE);
	w_ecs_register_system_phase_at(world, &phase_network_pre_dispose, WM_NETWORK_PHASE_PRE_DISPOSE);
	w_ecs_register_system_phase_at(world, &phase_network_dispose, WM_NETWORK_PHASE_DISPOSE);
	w_ecs_register_system_phase_at(world, &phase_network_post_dispose, WM_NETWORK_PHASE_POST_DISPOSE);

	// assign phases order
	w_ecs_set_phase_chain(world,
		WM_PHASE_PRE_LOAD,
		WM_NETWORK_PHASE_PRE_INIT,
		WM_SOCKET_PHASE_INIT,
		WM_NETWORK_PHASE_INIT,
		WM_NETWORK_PHASE_POST_INIT,
		WM_SOCKET_PHASE_PRE_ACCEPT,
		WM_NETWORK_PHASE_PRE_ACCEPT,
		WM_NETWORK_PHASE_ACCEPT,
		WM_NETWORK_PHASE_POST_ACCEPT,
	);

	w_ecs_set_phase_chain(world,
		WM_PHASE_ON_LOAD,
		WM_NETWORK_PHASE_PRE_READ,
		WM_NETWORK_PHASE_READ,
		WM_NETWORK_PHASE_POST_READ,
	);

	w_ecs_set_phase_chain(world,
		WM_PHASE_POST_LOAD,
		WM_NETWORK_PHASE_PRE_PROTOCOL,
		WM_NETWORK_PHASE_PROTOCOL,
		WM_NETWORK_PHASE_POST_PROTOCOL,
	);

	w_ecs_set_phase_chain(world,
		WM_PHASE_POST_UPDATE,
		WM_NETWORK_PHASE_PRE_WRITE,
		WM_NETWORK_PHASE_WRITE,
		WM_NETWORK_PHASE_POST_WRITE,
	);

	w_ecs_set_phase_chain(world,
		WM_PHASE_FINAL,
		WM_NETWORK_PHASE_PRE_CLOSE,
		WM_SOCKET_PHASE_CLOSE,
		WM_NETWORK_PHASE_CLOSE,
		WM_NETWORK_PHASE_POST_CLOSE,
		WM_NETWORK_PHASE_PRE_DISPOSE,
		WM_NETWORK_PHASE_DISPOSE,
		WM_NETWORK_PHASE_POST_DISPOSE,
	);

	// register systems
	wm_networking_socket_project_to_socket_register(world);
	wm_networking_socket_project_close_request_register(world);
	wm_networking_socket_handle_request_destroyed_register(world);
	wm_networking_socket_sync_after_init_register(world);
	wm_networking_socket_sync_after_accept_poll_register(world);
	wm_networking_socket_sync_after_close_register(world);
	wm_networking_socket_accept_and_create_connections_register(world);

	wm_networking_connection_handle_destroyed_request_register(world);
	wm_networking_connection_handle_close_request_register(world);
	wm_networking_connection_read_poll_register(world);
	wm_networking_connection_receive_bytes_register(world);
	wm_networking_connection_write_poll_register(world);
	wm_networking_connection_send_bytes_register(world);
	wm_networking_connection_handle_closed_after_write_request_register(world);
}

void wm_networking_free(struct w_ecs_world *world)
{
	(void)world;
}
