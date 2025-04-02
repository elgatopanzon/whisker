/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_scheduler
 * @created     : Tuesday Apr 01, 2025 19:51:37 CST
 */

#include "whisker_std.h"

#include "whisker_ecs.h"

/**********************
*  system functions  *
**********************/
// register a system function with a name and desired process phase group name to execute in
// note: the process phase group has to be registered or it will not be scheduled for execution
struct w_system *w_register_system(struct w_ecs *ecs, void (*system_ptr)(struct w_sys_context*), char *system_name, char *process_phase_name, size_t thread_count)
{
	debug_log(DEBUG, ecs:register_system, "registering system %s process phase %s", system_name, process_phase_name);

	// get the entity for the process phase
	w_entity_id phase_e = w_create_named_entity_non_deferred(ecs->world, process_phase_name);

	// create an entity for this system with it's name
	w_entity_id e = w_create_named_entity_non_deferred(ecs->world, system_name);

	// add process phase component to system
	w_set_named_component(ecs->world, process_phase_name, sizeof(bool), e, &(bool){0});

	// set component of its type on itself
	w_set_named_component(ecs->world, system_name, sizeof(bool), e, &(bool){0});

	// create system struct
	struct w_system system = {
		.entity_id = e,
		.process_phase_id = phase_e,
		.system_ptr = system_ptr,
		.thread_count = thread_count,
		.world = ecs->world,
	};

	// create contexts for the provided thread count
	// set thread count to 1 when it's set to 0
	if (system.thread_count == -1)
	{
		system.thread_count = w_thread_pool_system_core_count();
	}
	debug_log(DEBUG, ecs:register_system, "sys e %zu creating %zu thread contexts", system.entity_id, system.thread_count);

	// create system context array
	w_array_init_t(system.thread_contexts, (system.thread_count + 1));

	// create 1 extra context, which acts as the default context or first
	// thread's context
	for (int i = 0; i < system.thread_count + 1; ++i)
	{
		size_t thread_context_idx = system.thread_contexts_length++;
		w_init_system_context(&system.thread_contexts[thread_context_idx], &system);
		system.thread_contexts[thread_context_idx].thread_id = i;
		system.thread_contexts[thread_context_idx].thread_max = system.thread_count;
	}

	// create thread pool if we want threads for this system
	if (system.thread_count > 0)
	{
		system.thread_pool = w_thread_pool_create_and_init(system.thread_count, system.world->entities->entities[system.entity_id.index].name);
	}
	
	// add system to main systems list
	w_array_ensure_alloc(ecs->world->systems->systems, (ecs->world->systems->systems_length + 1));
	ecs->world->systems->systems[ecs->world->systems->systems_length++] = system;

	// add the system index component to the system entity
	w_set_named_component(ecs->world, "struct w_ecs_system_idx", sizeof(int), e, &(int){ecs->world->systems->systems_length - 1});

	return &ecs->world->systems->systems[ecs->world->systems->systems_length - 1];
}

// register a process phase time step to use when registering a process phase
size_t w_register_process_phase_time_step(struct w_ecs *ecs, w_time_step time_step)
{
	struct w_process_phase_time_step time_step_container = {
		.time_step = time_step,
		.update_count = 0,
		.updated = false,
	};

	size_t idx = ecs->world->systems->process_phase_time_steps_length++;
	w_array_ensure_alloc(ecs->world->systems->process_phase_time_steps, (idx + 1));

	ecs->world->systems->process_phase_time_steps[idx] = time_step_container;
	return idx;
}

// register a process phase for use by the system scheduler
// note: update_rate_sec set to 0 = uncapped processing with variable delta time
w_entity_id w_register_process_phase(struct w_ecs *ecs, char *phase_name, size_t time_step_id)
{
	w_entity_id component_id = w_create_named_entity_non_deferred(ecs->world, phase_name);

	// add component ID to system's process phase list
	struct w_process_phase process_phase = {
		.id = component_id,
		.time_step_id = time_step_id,
	};

	w_array_ensure_alloc(ecs->world->systems->process_phases, (ecs->world->systems->process_phases_length + 1));
	ecs->world->systems->process_phases[ecs->world->systems->process_phases_length++] = process_phase;

	return component_id;
}

// set the order of process phases in the form of a char ** array of names
// note: if a phase is specified which has not been created it will use the
// W_PHASE_DEFAULT_RATE for the update rate
void w_set_process_phase_order(struct w_ecs *ecs, char **phase_names, size_t phase_count)
{
	// backup existing process phase array
	w_array_declare(struct w_process_phase, process_phases_backup);
	process_phases_backup = ecs->world->systems->process_phases;
	process_phases_backup_length = ecs->world->systems->process_phases_length;
	process_phases_backup_size = ecs->world->systems->process_phases_size;

	// reinit the process phases array
	w_array_init_t(ecs->world->systems->process_phases, phase_count);
	ecs->world->systems->process_phases_length = 0;
	
	for (int i = 0; i < phase_count; ++i)
	{
		w_entity_id component_id = w_create_named_entity_non_deferred(ecs->world, phase_names[i]);

		bool exists = false;

		// find existing phase in old list
		for (int pi = 0; pi < process_phases_backup_length; ++pi)
		{
			if (process_phases_backup[pi].id.id == component_id.id)
			{
				// re-register the process phase
				debug_log(DEBUG, ecs:set_process_phase_order, "re-registering phase %s", phase_names[i]);

				size_t idx = ecs->world->systems->process_phases_length++;
				ecs->world->systems->process_phases[idx].id = component_id;
				ecs->world->systems->process_phases[idx].time_step_id = process_phases_backup[pi].time_step_id;

				exists = true;
				break;
			}
		}

		// if it doesn't exist, create it using the defaults
		if (!exists)
		{
			debug_log(DEBUG, ecs:set_process_phase_order, "registering new phase %s", phase_names[i]);
			w_register_process_phase(ecs, phase_names[i], 0);
		}
	}

	// insert managed process phases
	// note: this assumes that the pre/post phase timestep is the same as the
	// reserved one
	w_register_process_phase(ecs, W_PHASE_RESERVED, process_phases_backup[ecs->process_phase_pre_idx].time_step_id);

	w_register_process_phase(ecs, W_PHASE_PRE_PHASE_, process_phases_backup[ecs->process_phase_pre_idx].time_step_id);
	ecs->process_phase_pre_idx = ecs->world->systems->process_phases_length - 1;
	ecs->world->systems->process_phases[ecs->process_phase_pre_idx].manual_scheduling = true;
	w_register_process_phase(ecs, W_PHASE_POST_PHASE_, process_phases_backup[ecs->process_phase_pre_idx].time_step_id);
	ecs->process_phase_post_idx = ecs->world->systems->process_phases_length - 1;
	ecs->world->systems->process_phases[ecs->process_phase_post_idx].manual_scheduling = true;

	// free old process phases list
	free(process_phases_backup);
}

/*****************************
*  system update functions  *
*****************************/
// issue an update of all registered systems on all matching world entities
void w_scheduler_update(struct w_ecs *ecs, double delta_time)
{
	// update each system phase then run deferred actions
    struct w_sys_context *update_context = &ecs->system_update_context;

    for (int i = 0; i < ecs->world->systems->process_phases_length; ++i)
    {
    	if (ecs->world->systems->process_phases[i].manual_scheduling) { continue; }

    	// run pre_phase process phase
        w_scheduler_update_process_phase(ecs->world, &ecs->world->systems->process_phases[ecs->process_phase_pre_idx], update_context);

        w_scheduler_update_process_phase(ecs->world, &ecs->world->systems->process_phases[i], update_context);

    	// run post_phase process phase
        w_scheduler_update_process_phase(ecs->world, &ecs->world->systems->process_phases[ecs->process_phase_post_idx], update_context);

		w_process_deferred_actions_(ecs);
    }

	// reset process phase time steps to allow next frame to advance
	w_scheduler_reset_process_phase_time_steps_(ecs->world->systems);
}


// create and init an instance of systems container
struct w_systems *wcreate_and_init_systems_container()
{
	struct w_systems *s = w_mem_xcalloc(1, sizeof(*s));
	w_init_systems_container(s);

	return s;
}

// init an instance of systems container
void w_init_systems_container(struct w_systems *s)
{
	// create array for systems
	w_array_init_t(s->systems, 1);

	// create array for process phase list
	w_array_init_t(s->process_phases, 1);

	// create array for process phase time steps
	w_array_init_t(s->process_phase_time_steps, 1);
}

// deallocate an instance of systems container and system data
void w_free_systems_container_all(struct w_systems *systems)
{
	w_free_systems_container(systems);
	free(systems);
}

// deallocate system data in instance of systems container
void w_free_systems_container(struct w_systems *systems)
{
	for (int i = 0; i < systems->systems_length; ++i)
	{
		w_free_system(&systems->systems[i]);
	}

	free(systems->systems);
	free(systems->process_phases);
	free(systems->process_phase_time_steps);
}


/******************************
*  system context functions  *
******************************/
// create and init an instance of a system context
struct w_sys_context *w_create_and_init_system_context(struct w_system *system)
{
	struct w_sys_context *c = w_mem_xcalloc(1, sizeof(*c));
	w_init_system_context(c, system);

	return c;
}

// init an instance of a system context
void w_init_system_context(struct w_sys_context *context, struct w_system *system)
{
	// create iterators sparse set
	context->iterators = w_sparse_set_create_t(struct w_iterator);

	// set system pointers
	context->world = system->world;
	context->process_phase_time_step = system->process_phase_time_step;
	context->system_entity_id = system->entity_id;

	context->delta_time = 0;
	context->thread_id = 0;
	context->thread_max = 0;
}

// deallocate data for system context instance
void w_free_system_context(struct w_sys_context *context)
{
	if (context->iterators != NULL)
	{
		for (int i = 0; i < context->iterators->sparse_index_length; ++i)
		{
			struct w_iterator itor = ((struct w_iterator*)context->iterators->dense)[i];

			w_free_iterator(&itor);
		}

		w_sparse_set_free_all(context->iterators);
	}
}

// deallocate a system context instance
void w_free_system_context_all(struct w_sys_context *context)
{
	w_free_system_context(context);
	free(context);
}



/********************************
*  system operation functions  *
********************************/

// deallocate a system instance
void w_free_system(struct w_system *system)
{
	for (int i = 0; i < system->thread_contexts_length; ++i)
	{
		w_free_system_context(&system->thread_contexts[i]);
	}

	free(system->thread_contexts);
	if (system->thread_pool)
	{
		w_thread_pool_wait_work(system->thread_pool);
		w_thread_pool_free_all(system->thread_pool);
	}
}



/*****************************
*  system update functions  *
*****************************/
static int system_update_advance_process_phase_time_step(struct w_process_phase_time_step *time_step_container);
static struct w_iterator* system_update_get_system_iterator(struct w_sys_context *default_context, int index, struct w_entities *entities);
static void system_update_queue_system_work(struct w_system *system);
static void system_update_execute_system(struct w_system *system);
static void system_update_set_context_values(struct w_sys_context *context, struct w_system *system);
static void system_update_execute_system(struct w_system *system);
static void system_update_process_phase(struct w_world *world, struct w_process_phase *process_phase, struct w_sys_context *default_context);


static int system_update_advance_process_phase_time_step(struct w_process_phase_time_step *time_step_container)
{
	if (!time_step_container->updated)
	{
		time_step_container->update_count = w_time_step_advance(&time_step_container->time_step);
		time_step_container->updated = true;
	}

	return time_step_container->update_count;
}

static struct w_iterator* system_update_get_system_iterator(struct w_sys_context *default_context, int index, struct w_entities *entities)
{
    return w_query(default_context, index, "struct w_ecs_system_idx", entities->entities[index].name, "");
}

static void system_update_set_context_values(struct w_sys_context *context, struct w_system *system)
{
    context->process_phase_time_step = system->process_phase_time_step;
    context->delta_time = system->delta_time;
    context->system_ptr = system->system_ptr;
}

// reset updated state on all time steps
// this should be done at the end of the frame to allow triggering an advance
void w_scheduler_reset_process_phase_time_steps_(struct w_systems *systems)
{
	for (int i = 0; i < systems->process_phase_time_steps_length; ++i)
	{
		systems->process_phase_time_steps[i].updated = false;
	}
}

// update all systems in the given process phase
void w_scheduler_update_process_phase(struct w_world *world, struct w_process_phase *process_phase, struct w_sys_context *default_context)
{
	system_update_process_phase(world, process_phase, default_context);
}

static void system_update_process_phase(struct w_world *world, struct w_process_phase *process_phase, struct w_sys_context *default_context)
{
    if (w_get_entity(world, process_phase->id)->unmanaged)
    {
    	return;
    }

    struct w_process_phase_time_step *time_step_container = &world->systems->process_phase_time_steps[process_phase->time_step_id];

    int update_count = system_update_advance_process_phase_time_step(time_step_container);

    for (int ui = 0; ui < update_count; ++ui)
    {
        struct w_iterator *system_itor = system_update_get_system_iterator(default_context, process_phase->id.index, world->entities);

        while (w_iterate(system_itor))
        {
            int *system_idx = w_itor_get(system_itor, 0);
            struct w_system *system = &world->systems->systems[*system_idx];

            system->delta_time = time_step_container->time_step.delta_time_fixed;
            system->process_phase_time_step = &time_step_container->time_step;

            if (system->thread_count > 0)
            {
                system_update_queue_system_work(system);
            }
            else
            {
                system_update_execute_system(system);
            }
        }
    }
}

static void system_update_execute_system(struct w_system *system)
{
    struct w_sys_context *context = &system->thread_contexts[0];
    system_update_set_context_values(context, system);
    context->system_ptr(context);
}


static void system_update_queue_system_work(struct w_system *system)
{
    for (int ti = 0; ti < system->thread_count; ++ti)
    {
        struct w_sys_context *context = &system->thread_contexts[ti];
        system_update_set_context_values(context, system);
        w_thread_pool_queue_work(system->thread_pool, w_scheduler_update_system_thread_func_, context);
    }
    w_thread_pool_wait_work(system->thread_pool);
}


void w_scheduler_update_system_thread_func_(void *context, w_thread_pool_context *t)
{
	struct w_sys_context *system_context = context;
	system_context->system_ptr(system_context);
}

// update the provided system
void w_scheduler_update_system(struct w_system *system, struct w_sys_context *context)
{
	system->system_ptr(context);
}
