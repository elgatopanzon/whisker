/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs
 * @created     : Thursday Feb 13, 2025 17:49:29 CST
 */

#include "whisker_std.h"

#include "whisker_debug.h"
#include "whisker_array.h"
#include "whisker_ecs.h"

/*********
*  ECS  *
*********/

// create and initialise an instance of whisker_ecs to hold the ECS's world
// state (entities, components, systems)
whisker_ecs *whisker_ecs_create()
{
	whisker_ecs *new = whisker_mem_xcalloc(1, sizeof(*new));
	//
	// create world object
	new->world = whisker_ecs_world_create(new);

	new->world->entities = whisker_ecs_create_and_init_entities_container_();
	new->world->components = whisker_ecs_create_and_init_components_container();
	new->world->systems = whisker_ecs_create_and_init_systems_container();


	// reserve 1 entity for system use
	whisker_ecs_create_entity(new->world);

	// create a dummy system to use by the system context
	whisker_ecs_system system = {
		.entity_id = 0,
		.process_phase_id = 0,
		.system_ptr = NULL,
		.thread_id = 0,
		.last_update = 0,
		.delta_time = 0,
		.world = new->world,
	};
	// init the system update context
	whisker_ecs_init_system_context(&new->system_update_context, &system);

	// register default system process phases to allow bundled systems and
	// modules and a standard default processing phase group set
	whisker_time_step default_time_step = whisker_time_step_create(
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_RATE,
			1,
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_UNCAPPED,
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_CLAMP,
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_SNAP,
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_AVERAGE,
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_ACCUMULATION,
			WHISKER_ECS_PROCESS_PHASE_DEFAULT_DELTA_ACCUMULATION_CLAMP
		);
	whisker_time_step fixed_update_time_step = whisker_time_step_create(
			WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_RATE,
			0,
			false,
			WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_CLAMP,
			WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_SNAP,
			WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_AVERAGE,
			WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_ACCUMULATION,
			WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE_DELTA_ACCUMULATION_CLAMP
		);
	whisker_time_step rendering_phase_time_step = whisker_time_step_create(
			0,
			0,
			WHISKER_ECS_PROCESS_PHASE_ON_RENDER_UNCAPPED,
			false,
			false,
			false,
			false,
			false
		);
	whisker_time_step reserved_time_step = whisker_time_step_create(
			0,
			0,
			true,
			false,
			false,
			false,
			false,
			false
		);
	size_t default_time_step_id = whisker_ecs_register_process_phase_time_step(new, default_time_step);
	size_t fixed_update_time_step_id = whisker_ecs_register_process_phase_time_step(new, fixed_update_time_step);
	size_t rendering_phase_time_step_id = whisker_ecs_register_process_phase_time_step(new, rendering_phase_time_step);
	size_t reserved_time_step_id = whisker_ecs_register_process_phase_time_step(new, reserved_time_step);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_ON_STARTUP, default_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_LOAD, default_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_UPDATE, default_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_FIXED_UPDATE, fixed_update_time_step_id);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_ON_UPDATE, default_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_POST_UPDATE, default_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_FINAL, default_time_step_id);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_RENDER, rendering_phase_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_ON_RENDER, rendering_phase_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_POST_RENDER, rendering_phase_time_step_id);
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_FINAL_RENDER, rendering_phase_time_step_id);

	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_RESERVED, reserved_time_step_id);

	// register 2 special process phases PRE_PHASE and POST_PHASE
	// then set them to externally managed so they don't get scheduled
	// TODO: implement this into the register process phase functions
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_PRE_PHASE_, reserved_time_step_id);
	new->process_phase_pre_idx = new->world->systems->process_phases_length - 1;
	new->world->systems->process_phases[new->process_phase_pre_idx].manual_scheduling = true;
	whisker_ecs_register_process_phase(new, WHISKER_ECS_PROCESS_PHASE_POST_PHASE_, reserved_time_step_id);
	new->process_phase_post_idx = new->world->systems->process_phases_length - 1;
	new->world->systems->process_phases[new->process_phase_post_idx].manual_scheduling = true;

	// register built-in systems
	whisker_ecs_register_system(new, whisker_ecs_system_deregister_startup_phase, "wecs_system_deregister_startup_phase", WHISKER_ECS_PROCESS_PHASE_FINAL, WHISKER_ECS_PROCESS_THREADED_MAIN_THREAD);

	// create thread pool for general work tasks
	new->general_thread_pool = whisker_tp_create_and_init(0, "ecs_general_tasks");
	whisker_arr_init_t(new->component_sort_requests, 32);

	return new;
}

// deallocate an instance of whisker_ecs and it's complete state
void whisker_ecs_free(whisker_ecs *ecs)
{
	// free ecs state
	whisker_ecs_free_entities_all_(ecs->world->entities);
	whisker_ecs_free_components_container_all(ecs->world->components);
	whisker_ecs_free_systems_container_all(ecs->world->systems);

	whisker_ecs_free_system_context(&ecs->system_update_context);

	// free thread pool
	whisker_tp_free_all(ecs->general_thread_pool);
	free(ecs->component_sort_requests);

	free(ecs->world);
	free(ecs);
}



/**********************
*  built-in systems  *
**********************/

// this system ensures the process phase WHISKER_ECS_PROCESS_PHASE_ON_STARTUP
// gets disabled after the first frame along with this system
void whisker_ecs_system_deregister_startup_phase(whisker_ecs_system_context *context)
{
	// destroy the process phase entity, and destroy this system's entity
	whisker_ecs_entity *e = whisker_ecs_get_named_entity(context->world, WHISKER_ECS_PROCESS_PHASE_ON_STARTUP);

	if (e)
	{
		debug_log(DEBUG, ecs:system_deregister_startup_phase, "de-registering startup phase entity %d", e->id.index);

		whisker_ecs_set_entity_unmanaged(context->world, e->id);

		// destroy system entity to prevent running it again
		whisker_ecs_set_entity_unmanaged(context->world, context->system_entity_id);
	}
}
