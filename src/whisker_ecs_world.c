/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_world
 * @created     : Wednesday Mar 04, 2026 20:31:35 CST
 */

#include "whisker_std.h"

#include "whisker_ecs_world.h"

static inline void w_ecs_update_hook_flush_command_buffer_(void *world, void *action);

void w_ecs_world_init(struct w_ecs_world *world, struct w_string_table *string_table, struct w_arena *arena)
{
	world->arena = arena;
	world->string_table = string_table;

	w_entity_registry_init(&world->entities, string_table);
	w_component_registry_init(&world->components, arena, &world->entities);
	w_system_registry_init(&world->systems);

	w_scheduler_init(&world->scheduler);
	w_array_init_t(world->scheduler_jobs, 16);

	w_hook_registry_init(&world->hooks);

	w_command_buffer_init(&world->command_buffer);
	world->buffering_enabled = false;

	world->scheduler_jobs_dirty = true;
	world->update_result = W_WORLD_UPDATE_RESULT_CONTINUE;

	// register command buffer flush hook
	w_hook_registry_register_hook(&world->hooks, W_WORLD_HOOK_UPDATE_PHASE_END, w_ecs_update_hook_flush_command_buffer_);
}

void w_ecs_world_free(struct w_ecs_world *world)
{
	w_entity_registry_free(&world->entities);
	w_component_registry_free(&world->components);
	w_system_registry_free(&world->systems);
	w_scheduler_free(&world->scheduler);
	w_hook_registry_free(&world->hooks);
	w_command_buffer_free(&world->command_buffer);
	free_null(world->scheduler_jobs);
}

/**************
*  core API  *
**************/

static inline void w_ecs_rebuild_scheduler_jobs_(struct w_ecs_world *world)
{
	world->scheduler_jobs_length = 0;

	// ensure scheduler jobs can fit all registered systems
	w_array_ensure_alloc_block_size(
		world->scheduler_jobs,
		world->systems.systems_length,
		16
	);

	// build scheduler jobs list
	for (size_t i = 0; i < world->systems.systems_length; ++i)
	{
		if (!world->systems.systems[i].enabled) continue;

		size_t job_idx = world->scheduler_jobs_length++;
		world->scheduler_jobs[job_idx].job_id = i;
		world->scheduler_jobs[job_idx].phase_id = world->systems.systems[i].phase_id;
	}
}

static inline void w_ecs_update_hook_flush_command_buffer_(void *world_, void *action_)
{
	struct w_ecs_world *world = world_;
	w_command_buffer_flush(&world->command_buffer);
}

enum W_WORLD_UPDATE_RESULT w_ecs_update(struct w_ecs_world *world)
{
	// check if jobs need rebuilding
	if (world->scheduler_jobs_dirty)
	{
		w_ecs_rebuild_scheduler_jobs_(world);
		world->scheduler_jobs_dirty = false;
		world->scheduler.schedule.schedule_dirty = true;
	}

	// force enable buffering for safety
	world->buffering_enabled = true;

	// process the scheduler's schedule
	struct w_scheduler_schedule *schedule = w_scheduler_get_schedule(&world->scheduler, world->scheduler_jobs, world->scheduler_jobs_length);

	size_t count = schedule->items_length;
	struct w_scheduler_action *schedule_items = schedule->items;
	struct w_system *systems = world->systems.systems;

	for (size_t i = 0; i < count; ++i)
	{
		struct w_scheduler_action *action = &schedule_items[i];

		switch (action->action) {
			case W_SCHEDULER_ACTIONS_NOOP:
				break;
			case W_SCHEDULER_ACTIONS_SCHEDULE_BEGIN:
				w_hook_registry_run_hooks(&world->hooks, W_WORLD_HOOK_UPDATE_BEGIN, world, action);
				break;
			case W_SCHEDULER_ACTIONS_SCHEDULE_END:
				w_hook_registry_run_hooks(&world->hooks, W_WORLD_HOOK_UPDATE_END, world, action);
				break;
			case W_SCHEDULER_ACTIONS_TIMESTEP_BEGIN:
				w_hook_registry_run_hooks(&world->hooks, W_WORLD_HOOK_UPDATE_TIMESTEP_BEGIN, world, action);
				break;
			case W_SCHEDULER_ACTIONS_TIMESTEP_END:
				w_hook_registry_run_hooks(&world->hooks, W_WORLD_HOOK_UPDATE_TIMESTEP_END, world, action);
				break;
			case W_SCHEDULER_ACTIONS_PHASE_BEGIN:
				w_hook_registry_run_hooks(&world->hooks, W_WORLD_HOOK_UPDATE_PHASE_BEGIN, world, action);
				break;
			case W_SCHEDULER_ACTIONS_PHASE_END:
				w_hook_registry_run_hooks(&world->hooks, W_WORLD_HOOK_UPDATE_PHASE_END, world, action);
				break;
			case W_SCHEDULER_ACTIONS_DISPATCH:
				struct w_system *system = &systems[action->job_idx];

				// frequency is 0, use timestep delta time directly
				if (system->update_frequency == 0) 
				{
					system->update(NULL, action->time_step->delta_time_fixed);
					continue;
				}

				// per-system frequency check
				uint64_t current_tick = action->time_step->tick_count;
				uint64_t ticks_elapsed = current_tick - system->last_update_ticks;

				// not enough ticks have passed, skip this system
				if (ticks_elapsed < system->update_frequency)
					break;

				// calculate delta time based on actual ticks elapsed
				double delta_time = ticks_elapsed * action->time_step->delta_time_fixed;

				// update last_update_ticks after running
				system->last_update_ticks = current_tick;

				system->update(NULL, delta_time);

				break;
			default:
				break;
		}
	}

	return world->update_result;
}


/****************
*  entity API  *
****************/

w_entity_id w_ecs_request_entity(struct w_ecs_world *world)
{
	return w_entity_request(&world->entities);
}

w_entity_id w_ecs_request_entity_with_name(struct w_ecs_world *world, char *name)
{
	w_entity_id existing = w_entity_lookup_by_name(&world->entities, name);
	if (existing != W_ENTITY_INVALID)
		return existing;

	w_entity_id entity = w_entity_request(&world->entities);
	w_entity_set_name(&world->entities, entity, name);
	return entity;
}

void w_ecs_return_entity(struct w_ecs_world *world, w_entity_id entity)
{
	w_entity_return(&world->entities, entity);
}

void w_ecs_set_entity_name(struct w_ecs_world *world, w_entity_id entity, char *name)
{
	w_entity_set_name(&world->entities, entity, name);
}

void w_ecs_clear_entity_name(struct w_ecs_world *world, w_entity_id entity)
{
	w_entity_clear_name(&world->entities, entity);
}

char *w_ecs_get_entity_name(struct w_ecs_world *world, w_entity_id entity)
{
	return w_entity_get_name(&world->entities, entity);
}

w_entity_id w_ecs_get_entity_by_name(struct w_ecs_world *world, char *name)
{
	return w_entity_lookup_by_name(&world->entities, name);
}


/*******************
*  component API  *
*******************/

void *w_ecs_set_component_(struct w_ecs_world *world, uint type_id, w_entity_id type_entity_id, w_entity_id entity_id, void *data, size_t data_size)
{
	return w_component_set_(&world->components, type_id, type_entity_id, entity_id, data, data_size);
}

void *w_ecs_get_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id)
{
	return w_component_get_(&world->components, type_entity_id, entity_id);
}

void w_ecs_remove_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id)
{
	w_component_remove_(&world->components, type_entity_id, entity_id);
}

bool w_ecs_has_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id)
{
	return w_component_has_(&world->components, type_entity_id, entity_id);
}

w_entity_id w_ecs_get_component_by_name(struct w_ecs_world *world, char *name)
{
	return w_component_get_id(&world->components, name);
}

char *w_ecs_get_component_name(struct w_ecs_world *world, w_entity_id type_entity_id)
{
	return w_component_get_name(&world->components, type_entity_id);
}

void *w_ecs_unsafe_set_component_(struct w_ecs_world *world, uint type_id, w_entity_id type_entity_id, w_entity_id entity_id, void *data, size_t data_size)
{
	return w_component_set_unsafe_(&world->components, type_id, type_entity_id, entity_id, data, data_size);
}

void *w_ecs_unsafe_get_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id)
{
	return w_component_get_unsafe_(&world->components, type_entity_id, entity_id);
}

bool w_ecs_unsafe_has_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id)
{
	return w_component_has_unsafe_(&world->components, type_entity_id, entity_id);
}

struct w_component_entry *w_ecs_get_component_entry(struct w_ecs_world *world, w_entity_id type_entity_id)
{
	return w_component_registry_get_entry(&world->components, type_entity_id);
}


/****************
*  system API  *
****************/

size_t w_ecs_register_system(struct w_ecs_world *world, struct w_system *system)
{
	world->scheduler_jobs_dirty = true;
	return w_system_register_system(&world->systems, system);
}

size_t w_ecs_set_system_state(struct w_ecs_world *world, size_t system_id, bool system_state)
{
	world->scheduler_jobs_dirty = true;
	w_system_set_system_state(&world->systems, system_id, system_state);
	return system_id;
}

struct w_system *w_ecs_get_system_entry(struct w_ecs_world *world, size_t system_id)
{
	return w_system_get_system_entry(&world->systems, system_id);
}

size_t w_ecs_register_system_phase(struct w_ecs_world *world, struct w_scheduler_phase *phase)
{
	return w_scheduler_register_phase(&world->scheduler, phase);
}

struct w_scheduler_phase *w_ecs_get_system_phase(struct w_ecs_world *world, size_t phase_id)
{
	return w_scheduler_get_phase(&world->scheduler, phase_id);
}

void w_ecs_set_system_phase_state(struct w_ecs_world *world, size_t phase_id, bool state)
{
	w_scheduler_set_phase_state(&world->scheduler, phase_id, state);
}

void w_ecs_set_system_phase_runs_before(struct w_ecs_world *world, size_t phase_id, size_t runs_before_phase_id)
{
	w_scheduler_set_phase_runs_before(&world->scheduler, phase_id, runs_before_phase_id);
}

void w_ecs_set_system_phase_runs_after(struct w_ecs_world *world, size_t phase_id, size_t runs_after_phase_id)
{
	w_scheduler_set_phase_runs_after(&world->scheduler, phase_id, runs_after_phase_id);
}

void w_ecs_reset_system_phases(struct w_ecs_world *world)
{
	w_scheduler_reset_phases(&world->scheduler);
}

size_t w_ecs_register_system_time_step(struct w_ecs_world *world, struct w_scheduler_time_step *time_step)
{
	return w_scheduler_register_time_step(&world->scheduler, time_step);
}

struct w_scheduler_time_step *w_ecs_get_system_time_step(struct w_ecs_world *world, size_t time_step_id)
{
	return w_scheduler_get_time_step(&world->scheduler, time_step_id);
}

void w_ecs_set_system_time_step_state(struct w_ecs_world *world, size_t time_step_id, bool state)
{
	w_scheduler_set_time_step_state(&world->scheduler, time_step_id, state);
}

void w_ecs_set_system_time_step_runs_before(struct w_ecs_world *world, size_t time_step_id, size_t runs_before_time_step_id)
{
	w_scheduler_set_time_step_runs_before(&world->scheduler, time_step_id, runs_before_time_step_id);
}

void w_ecs_set_system_time_step_runs_after(struct w_ecs_world *world, size_t time_step_id, size_t runs_after_time_step_id)
{
	w_scheduler_set_time_step_runs_after(&world->scheduler, time_step_id, runs_after_time_step_id);
}

void w_ecs_reset_system_time_steps(struct w_ecs_world *world)
{
	w_scheduler_reset_time_steps(&world->scheduler);
}
