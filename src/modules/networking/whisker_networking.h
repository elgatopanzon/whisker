/**
 * @author      : ElGatoPanzon
 * @file        : whisker_networking
 * @created     : Thursday May 28, 2026 11:08:36 CST
 * @description : Module providing networking functionality using the streams module
 */

#ifndef WHISKER_NETWORKING_H
#define WHISKER_NETWORKING_H

#include "whisker.h"
#include "modules/managed_alloc/whisker_managed_alloc.h"
#include "modules/sockets/whisker_sockets.h"
#include "modules/streams/whisker_streams.h"

enum WM_NETWORK_PHASE 
{ 
	WM_NETWORK_PHASE_PRE_INIT = 210,
	WM_NETWORK_PHASE_INIT = 211,
	WM_NETWORK_PHASE_POST_INIT = 212,
	WM_NETWORK_PHASE_PRE_ACCEPT = 213,
	WM_NETWORK_PHASE_ACCEPT = 214,
	WM_NETWORK_PHASE_POST_ACCEPT = 215,
	WM_NETWORK_PHASE_PRE_READ = 216,
	WM_NETWORK_PHASE_READ = 217,
	WM_NETWORK_PHASE_POST_READ = 218,
	WM_NETWORK_PHASE_PRE_PROTOCOL = 219,
	WM_NETWORK_PHASE_PROTOCOL = 220,
	WM_NETWORK_PHASE_POST_PROTOCOL = 221,
	WM_NETWORK_PHASE_PRE_WRITE = 222,
	WM_NETWORK_PHASE_WRITE = 223,
	WM_NETWORK_PHASE_POST_WRITE = 224,
	WM_NETWORK_PHASE_PRE_CLOSE = 225,
	WM_NETWORK_PHASE_CLOSE = 226,
	WM_NETWORK_PHASE_POST_CLOSE = 227,
	WM_NETWORK_PHASE_PRE_DISPOSE = 228,
	WM_NETWORK_PHASE_DISPOSE = 229,
	WM_NETWORK_PHASE_POST_DISPOSE = 230,
};

/*******************************************************************************
*                                 components                                  *
*******************************************************************************/

/***********************
*  socket components  *
***********************/
// these components make up a valid listen socket

// the listen host is a string stored in the string table
w_ecs_define_component(w_string_table_id, network_socket_listen_host_string_id, W_STRING_TABLE_INVALID_ID);

// default port being 0 means random, in reality this would always be set to
// something valid
w_ecs_define_component(uint16_t, network_socket_listen_port, 0);

// size of the backlog we pass when requesting the socket from the OS
w_ecs_define_component(int32_t, network_socket_listen_backlog, 128);

// the OS file descriptor for the socket
// note: having a fd means the socket is valid and usable
w_ecs_define_component(int32_t, network_socket_listen_fd, -1);

// custom ID for any network socket errors
w_ecs_define_component(int, network_socket_err, -1);

// backing generic socket entity used by this network socket facade
w_ecs_define_component(w_entity_id, network_socket_listen_entity, W_ENTITY_INVALID);

// indicates the socket is ready to accept connections
w_ecs_define_tag(network_socket_accept_ready);


/*********************************
*  socket lifecycle components  *
*********************************/
// request components to control socket lifecycle actions

// request a socket listening with valid fd
// note: this activate and put the socket in working state
w_ecs_define_tag(req_network_socket_hot);

// request a socket to stop listening, close fd and remove fd component
// note: this will deactivate and put the socket in non-active state
w_ecs_define_tag(req_network_socket_cold);

// ensure socket cold and destroy socket entity
w_ecs_define_tag(req_network_socket_destroyed);


/***********************************
*  network connection components  *
***********************************/
// these components are used for client connections

// client connection fd
w_ecs_define_component(int32_t, network_connection_fd, -1);

// entity of the listen socket where this connection came from
w_ecs_define_component(w_entity_id, network_connection_listen_socket_entity, W_ENTITY_INVALID);

// remote host managed alloc of the remote connection
// (managed alloc instead of string table so we dont pollute it)
w_ecs_define_component(uint64_t, network_connection_remote_host_string_handle, WM_MANAGED_ALLOC_INVALID_HANDLE);

// remote port where this connection came from
w_ecs_define_component(uint16_t, network_connection_remote_port, 0);

// custom error ID for any network connection errors
w_ecs_define_component(int, network_connection_err, -1);

// stream entities used for bytes received from/sent to this connection
w_ecs_define_component(w_entity_id, network_connection_input_stream_entity, W_ENTITY_INVALID);
w_ecs_define_component(w_entity_id, network_connection_output_stream_entity, W_ENTITY_INVALID);

// indicates this client fd is ready to read from
w_ecs_define_tag(network_connection_read_ready);

// indicates this client fd is ready to write to
w_ecs_define_tag(network_connection_write_ready);


/*********************************************
*  network connection lifecycle components  *
*********************************************/
// request components co control connection lifecycle actions

// request closing of this client connection
w_ecs_define_tag(req_network_connection_closed);

// request closing and destroying of this connection
w_ecs_define_tag(req_network_connection_destroyed);

// shortcut to close connection after write is complete
w_ecs_define_tag(req_network_connection_closed_after_write);

static inline w_entity_id wm_networking_socket_get_listen_entity(struct w_ecs_world *world, w_entity_id entity)
{
	w_entity_id *socket_entity = network_socket_listen_entity_get(world, entity);
	return socket_entity ? *socket_entity : W_ENTITY_INVALID;
}

static inline w_entity_id wm_networking_connection_get_input_stream_entity(struct w_ecs_world *world, w_entity_id entity)
{
	w_entity_id *stream_entity = network_connection_input_stream_entity_get(world, entity);
	return stream_entity ? *stream_entity : W_ENTITY_INVALID;
}

static inline w_entity_id wm_networking_connection_get_output_stream_entity(struct w_ecs_world *world, w_entity_id entity)
{
	w_entity_id *stream_entity = network_connection_output_stream_entity_get(world, entity);
	return stream_entity ? *stream_entity : W_ENTITY_INVALID;
}

static inline void *wm_networking_connection_get_input_buffer(struct w_ecs_world *world, w_entity_id entity)
{
	w_entity_id stream_entity = wm_networking_connection_get_input_stream_entity(world, entity);
	return stream_entity == W_ENTITY_INVALID ? NULL : wm_streams_get_buffer(world, stream_entity);
}

static inline void *wm_networking_connection_get_output_buffer(struct w_ecs_world *world, w_entity_id entity)
{
	w_entity_id stream_entity = wm_networking_connection_get_output_stream_entity(world, entity);
	return stream_entity == W_ENTITY_INVALID ? NULL : wm_streams_get_buffer(world, stream_entity);
}


// initialize the networking module
void wm_networking_init(struct w_ecs_world *world);

// cleanup the networking module
void wm_networking_free(struct w_ecs_world *world);

#endif /* WHISKER_NETWORKING_H */
