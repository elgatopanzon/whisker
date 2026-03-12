/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_world
 * @created     : Wednesday Mar 04, 2026 20:31:35 CST
 */

#include "whisker_std.h"

#include "whisker_ecs_world.h"

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

	w_query_registry_init(&world->queries, world->string_table, &world->components, world->arena);

	w_singleton_registry_init(&world->singletons, arena);

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
	w_query_registry_free(&world->queries);
	w_singleton_registry_free(&world->singletons);
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

	// timestep loop tracking
	size_t timestep_begin_idx = 0;
	int timestep_iterations_remaining = 0;

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
			{
				// if update_time_target is 0 (uninitialized), treat as single update
				int n = (action->time_step->update_time_target == 0) ? 1 : w_time_step_advance(action->time_step);
				if (n <= 0)
				{
					// skip entire timestep - find the matching TIMESTEP_END
					size_t depth = 1;
					while (++i < count && depth > 0)
					{
						if (schedule_items[i].action == W_SCHEDULER_ACTIONS_TIMESTEP_BEGIN)
							depth++;
						else if (schedule_items[i].action == W_SCHEDULER_ACTIONS_TIMESTEP_END)
							depth--;
					}
					// back up so the main loop increment lands on TIMESTEP_END
					i--;
					break;
				}
				timestep_begin_idx = i;
				timestep_iterations_remaining = n - 1; // first iteration runs now
				w_hook_registry_run_hooks(&world->hooks, W_WORLD_HOOK_UPDATE_TIMESTEP_BEGIN, world, action);
				break;
			}
			case W_SCHEDULER_ACTIONS_TIMESTEP_END:
				w_hook_registry_run_hooks(&world->hooks, W_WORLD_HOOK_UPDATE_TIMESTEP_END, world, action);
				// check if more iterations needed
				if (timestep_iterations_remaining > 0)
				{
					timestep_iterations_remaining--;
					i = timestep_begin_idx; // jump back (loop will increment to BEGIN+1)
				}
				break;
			case W_SCHEDULER_ACTIONS_PHASE_BEGIN:
				w_hook_registry_run_hooks(&world->hooks, W_WORLD_HOOK_UPDATE_PHASE_BEGIN, world, action);
				break;
			case W_SCHEDULER_ACTIONS_PHASE_END:
				w_hook_registry_run_hooks(&world->hooks, W_WORLD_HOOK_UPDATE_PHASE_END, world, action);
				break;
			case W_SCHEDULER_ACTIONS_DISPATCH:
			{
				struct w_system *system = &systems[action->job_idx];

				// frequency is 0, use timestep delta time directly
				if (system->update_frequency == 0)
				{
					system->update(world, action->time_step->delta_time_fixed);
					break;
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

				system->update(world, delta_time);

				break;
			}
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
	// get and return existing named entity
	// note: needs thread-safe testing
	w_entity_id existing = w_entity_lookup_by_name(&world->entities, name);
	if (existing != W_ENTITY_INVALID)
		return existing;

	// request new entity
	w_entity_id entity = w_entity_request(&world->entities);

	w_ecs_set_entity_name(world, entity, name);

	return entity;
}

void w_ecs_return_entity(struct w_ecs_world *world, w_entity_id entity)
{
	if (!world->buffering_enabled)
		w_entity_return(&world->entities, entity);
	else
		w_command_buffer_queue(&world->command_buffer, w_ecs_cmd_return_entity, world, &entity, sizeof(entity));
}

void w_ecs_set_entity_name(struct w_ecs_world *world, w_entity_id entity, char *name)
{
	// unbuffered
	if (!world->buffering_enabled)
		w_entity_set_name(&world->entities, entity, name);

	// buffered
	else
	{
		size_t name_len = strlen(name) + 1;
		size_t payload_size = name_len + sizeof(entity);
		uint8_t payload[payload_size];

		memcpy(payload, &entity, sizeof(entity));
		memcpy(payload + sizeof(entity), name, name_len);

		w_command_buffer_queue(&world->command_buffer, w_ecs_cmd_set_entity_name, world, payload, payload_size);
	}
}

void w_ecs_clear_entity_name(struct w_ecs_world *world, w_entity_id entity)
{
	if (!world->buffering_enabled)
		w_entity_clear_name(&world->entities, entity);
	else
		w_command_buffer_queue(&world->command_buffer, w_ecs_cmd_clear_entity_name, world, &entity, sizeof(entity));
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
	if (!world->buffering_enabled)
		return w_component_set_(&world->components, type_id, type_entity_id, entity_id, data, data_size);

	// payload: type_id + type_entity_id + entity_id + data_size + data
	size_t payload_size = sizeof(type_id) + sizeof(type_entity_id) + sizeof(entity_id) + sizeof(data_size) + data_size;
	uint8_t payload[payload_size];
	size_t offset = 0;

	memcpy(payload + offset, &type_id, sizeof(type_id));
	offset += sizeof(type_id);
	memcpy(payload + offset, &type_entity_id, sizeof(type_entity_id));
	offset += sizeof(type_entity_id);
	memcpy(payload + offset, &entity_id, sizeof(entity_id));
	offset += sizeof(entity_id);
	memcpy(payload + offset, &data_size, sizeof(data_size));
	offset += sizeof(data_size);
	memcpy(payload + offset, data, data_size);

	w_command_buffer_queue(&world->command_buffer, w_ecs_cmd_set_component, world, payload, payload_size);
	return NULL;
}

void *w_ecs_get_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id)
{
	return w_component_get_(&world->components, type_entity_id, entity_id);
}

void w_ecs_remove_component_(struct w_ecs_world *world, w_entity_id type_entity_id, w_entity_id entity_id)
{
	if (!world->buffering_enabled)
	{
		w_component_remove_(&world->components, type_entity_id, entity_id);
		return;
	}

	// payload: type_entity_id + entity_id
	size_t payload_size = sizeof(type_entity_id) + sizeof(entity_id);
	uint8_t payload[payload_size];

	memcpy(payload, &type_entity_id, sizeof(type_entity_id));
	memcpy(payload + sizeof(type_entity_id), &entity_id, sizeof(entity_id));

	w_command_buffer_queue(&world->command_buffer, w_ecs_cmd_remove_component, world, payload, payload_size);
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


/*****************
*  queries API  *
*****************/

struct w_query *w_ecs_get_query(struct w_ecs_world *world, char *query)
{
	return w_query_registry_get_query(&world->queries, query);
}

/***********
*  hooks  *
***********/

void w_ecs_cmd_set_entity_name(void *w, void *entity_name_payload)
{
	struct w_ecs_world *world = w;
	uint8_t *payload = entity_name_payload;
	w_entity_id e = *(w_entity_id *)payload;
	char *name = (char *)(payload + sizeof(e));

	w_ecs_world_do_unbuffered(world, {
		w_ecs_set_entity_name(world, e, name);
	});
}

void w_ecs_cmd_return_entity(void *w, void *entity_payload)
{
	struct w_ecs_world *world = w;
	w_entity_id e = *(w_entity_id *)entity_payload;

	w_ecs_world_do_unbuffered(world, {
		w_ecs_return_entity(world, e);
	});
}

void w_ecs_cmd_clear_entity_name(void *w, void *entity_payload)
{
	struct w_ecs_world *world = w;
	w_entity_id e = *(w_entity_id *)entity_payload;

	w_ecs_world_do_unbuffered(world, {
		w_ecs_clear_entity_name(world, e);
	});
}

void w_ecs_cmd_set_component(void *w, void *component_payload)
{
	struct w_ecs_world *world = w;
	uint8_t *payload = component_payload;
	size_t offset = 0;

	uint type_id = *(uint *)(payload + offset);
	offset += sizeof(type_id);
	w_entity_id type_entity_id = *(w_entity_id *)(payload + offset);
	offset += sizeof(type_entity_id);
	w_entity_id entity_id = *(w_entity_id *)(payload + offset);
	offset += sizeof(entity_id);
	size_t data_size = *(size_t *)(payload + offset);
	offset += sizeof(data_size);
	void *data = payload + offset;

	w_ecs_world_do_unbuffered(world, {
		w_ecs_set_component_(world, type_id, type_entity_id, entity_id, data, data_size);
	});
}

void w_ecs_cmd_remove_component(void *w, void *component_payload)
{
	struct w_ecs_world *world = w;
	uint8_t *payload = component_payload;

	w_entity_id type_entity_id = *(w_entity_id *)payload;
	w_entity_id entity_id = *(w_entity_id *)(payload + sizeof(type_entity_id));

	w_ecs_world_do_unbuffered(world, {
		w_ecs_remove_component_(world, type_entity_id, entity_id);
	});
}
