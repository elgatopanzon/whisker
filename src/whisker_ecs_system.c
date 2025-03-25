/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_system
 * @created     : Thursday Feb 13, 2025 17:59:56 CST
 */


#include "whisker_std.h"
#include "whisker_debug.h"
#include "whisker_array.h"

#include "whisker_ecs_system.h"

// create and init an instance of systems container
whisker_ecs_systems *whisker_ecs_s_create_and_init_systems()
{
	whisker_ecs_systems *s = whisker_mem_xcalloc(1, sizeof(*s));
	whisker_ecs_s_init_systems(s);

	return s;
}

// init an instance of systems container
void whisker_ecs_s_init_systems(whisker_ecs_systems *s)
{
	// create array for systems
	whisker_arr_init_t(s->systems, 1);

	// create array for process phase list
	whisker_arr_init_t(s->process_phases, 1);
}

// deallocate an instance of systems container and system data
void whisker_ecs_s_free_systems_all(whisker_ecs_systems *systems)
{
	whisker_ecs_s_free_systems(systems);
	free(systems);
}

// deallocate system data in instance of systems container
void whisker_ecs_s_free_systems(whisker_ecs_systems *systems)
{
	for (int i = 0; i < systems->systems_length; ++i)
	{
		whisker_ecs_s_free_system(&systems->systems[i]);
	}

	free(systems->systems);
	free(systems->process_phases);
}


/******************************
*  system context functions  *
******************************/
// create and init an instance of a system context
whisker_ecs_system_context *whisker_ecs_s_create_and_init_system_context(whisker_ecs_system *system)
{
	whisker_ecs_system_context *c = whisker_mem_xcalloc(1, sizeof(*c));
	whisker_ecs_s_init_system_context(c, system);

	return c;
}

// init an instance of a system context
void whisker_ecs_s_init_system_context(whisker_ecs_system_context *context, whisker_ecs_system *system)
{
	// create iterators sparse set
	context->iterators = whisker_ss_create_t(whisker_ecs_system_iterator);

	// set system pointers
	context->components = system->components;
	context->entities = system->entities;
	context->process_phase_time_step = system->process_phase_time_step;
	context->system_entity_id = system->entity_id;

	context->delta_time = 0;
	context->thread_id = 0;
	context->thread_max = 0;
}

// deallocate data for system context instance
void whisker_ecs_s_free_system_context(whisker_ecs_system_context *context)
{
	if (context->iterators != NULL)
	{
		for (int i = 0; i < context->iterators->sparse_index_length; ++i)
		{
			whisker_ecs_system_iterator itor = ((whisker_ecs_system_iterator*)context->iterators->dense)[i];

			whisker_ecs_s_free_iterator(&itor);
		}

		whisker_ss_free_all(context->iterators);
	}
}

// deallocate a system context instance
void whisker_ecs_s_free_system_context_all(whisker_ecs_system_context *context)
{
	whisker_ecs_s_free_system_context(context);
	free(context);
}


/********************************
*  system operation functions  *
********************************/
// regiser a system in the systems container
whisker_ecs_system* whisker_ecs_s_register_system(whisker_ecs_systems *systems, whisker_ecs_components *components, whisker_ecs_system system)
{

	// create contexts for the provided thread count
	// set thread count to 1 when it's set to 0
	if (system.thread_count == -1)
	{
		system.thread_count = whisker_tp_system_core_count();
	}
	debug_log(DEBUG, ecs:register_system, "sys e %zu creating %zu thread contexts", system.entity_id, system.thread_count);

	// create system context array
	whisker_arr_init_t(system.thread_contexts, (system.thread_count + 1));

	// create 1 extra context, which acts as the default context or first
	// thread's context
	for (int i = 0; i < system.thread_count + 1; ++i)
	{
		size_t thread_context_idx = system.thread_contexts_length++;
		whisker_ecs_s_init_system_context(&system.thread_contexts[thread_context_idx], &system);
		system.thread_contexts[thread_context_idx].thread_id = i;
		system.thread_contexts[thread_context_idx].thread_max = system.thread_count;
	}

	// create thread pool if we want threads for this system
	if (system.thread_count > 0)
	{
		system.thread_pool = whisker_tp_create_and_init(system.thread_count, system.entities->entities[system.entity_id.index].name);
	}
	
	// add system to main systems list
	whisker_arr_ensure_alloc(systems->systems, (systems->systems_length + 1));
	systems->systems[systems->systems_length++] = system;

	return &systems->systems[systems->systems_length - 1];
}

// deallocate a system instance
void whisker_ecs_s_free_system(whisker_ecs_system *system)
{
	for (int i = 0; i < system->thread_contexts_length; ++i)
	{
		whisker_ecs_s_free_system_context(&system->thread_contexts[i]);
	}

	free(system->thread_contexts);
	if (system->thread_pool)
	{
		whisker_tp_wait_work(system->thread_pool);
		whisker_tp_free_all(system->thread_pool);
	}
}



/*****************************
*  system update functions  *
*****************************/
static void system_update_process_phase(whisker_ecs_systems *systems, whisker_ecs_entities *entities, whisker_ecs_system_process_phase *process_phase, whisker_ecs_system_context *default_context);
static int system_update_advance_process_phase_time_step(whisker_ecs_system_process_phase *process_phase);
static whisker_ecs_system_iterator* system_update_get_system_iterator(whisker_ecs_system_context *default_context, int index, whisker_ecs_entities *entities);
static void system_update_queue_system_work(whisker_ecs_system *system);
static void system_update_execute_system(whisker_ecs_system *system);
static void system_update_set_context_values(whisker_ecs_system_context *context, whisker_ecs_system *system);
static void system_update_execute_system(whisker_ecs_system *system);
static void system_update_process_phase(whisker_ecs_systems *systems, whisker_ecs_entities *entities, whisker_ecs_system_process_phase *process_phase, whisker_ecs_system_context *default_context);

// run an update on the registered systems
// note: processes all phases without deferred actions
void whisker_ecs_s_update_systems(whisker_ecs_systems *systems, whisker_ecs_entities *entities, double delta_time)
{
    whisker_ecs_system *default_system = &systems->systems[systems->system_id];
    whisker_ecs_system_context *default_context = &default_system->thread_contexts[0];

    for (int i = 0; i < systems->process_phases_length; ++i)
    {
        whisker_ecs_system_process_phase *process_phase = &systems->process_phases[i];

        system_update_process_phase(systems, entities, process_phase, default_context);
    }
}

static int system_update_advance_process_phase_time_step(whisker_ecs_system_process_phase *process_phase)
{
    return whisker_time_step_step_get_update_count(&process_phase->time_step);
}

static whisker_ecs_system_iterator* system_update_get_system_iterator(whisker_ecs_system_context *default_context, int index, whisker_ecs_entities *entities)
{
    return whisker_ecs_s_get_iterator(default_context, index, "w_ecs_system_idx", entities->entities[index].name, "");
}

static void system_update_set_context_values(whisker_ecs_system_context *context, whisker_ecs_system *system)
{
    context->process_phase_time_step = system->process_phase_time_step;
    context->delta_time = system->delta_time;
    context->system_ptr = system->system_ptr;
}

// update all systems in the given process phase
void whisker_ecs_s_update_process_phase(whisker_ecs_systems *systems, whisker_ecs_entities *entities, whisker_ecs_system_process_phase *process_phase, whisker_ecs_system_context *default_context)
{
	system_update_process_phase(systems, entities, process_phase, default_context);
}

static void system_update_process_phase(whisker_ecs_systems *systems, whisker_ecs_entities *entities, whisker_ecs_system_process_phase *process_phase, whisker_ecs_system_context *default_context)
{
    int update_count = system_update_advance_process_phase_time_step(process_phase);

    for (int ui = 0; ui < update_count; ++ui)
    {
        whisker_ecs_system_iterator *system_itor = system_update_get_system_iterator(default_context, process_phase->id.index, entities);

        while (whisker_ecs_s_iterate(system_itor))
        {
            int *system_idx = whisker_ecs_itor_get(system_itor, 0);
            whisker_ecs_system *system = &systems->systems[*system_idx];

            system->delta_time = process_phase->time_step.delta_time_fixed;
            system->process_phase_time_step = &process_phase->time_step;

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

static void system_update_execute_system(whisker_ecs_system *system)
{
    whisker_ecs_system_context *context = &system->thread_contexts[0];
    system_update_set_context_values(context, system);
    context->system_ptr(context);
}


static void system_update_queue_system_work(whisker_ecs_system *system)
{
    for (int ti = 0; ti < system->thread_count; ++ti)
    {
        whisker_ecs_system_context *context = &system->thread_contexts[ti];
        system_update_set_context_values(context, system);
        whisker_tp_queue_work(system->thread_pool, whisker_ecs_s_update_system_thread_, context);
    }
    whisker_tp_wait_work(system->thread_pool);
}


void whisker_ecs_s_update_system_thread_(void *context)
{
	whisker_ecs_system_context *system_context = context;
	system_context->system_ptr(system_context);
}

// update the provided system
void whisker_ecs_s_update_system(whisker_ecs_system *system, whisker_ecs_system_context *context)
{
	system->system_ptr(context);
}

// register a system process phase for the system scheduler
void whisker_ecs_s_register_process_phase(whisker_ecs_systems *systems, whisker_ecs_entity_id component_id, double update_rate_sec)
{
	whisker_ecs_system_process_phase process_phase = {
		.id = component_id,
		.time_step = whisker_time_step_create((update_rate_sec > 0) ? update_rate_sec : 60, (update_rate_sec > 0) ? false : true),
	};

	whisker_arr_ensure_alloc(systems->process_phases, (systems->process_phases_length + 1));
	systems->process_phases[systems->process_phases_length++] = process_phase;
}

// deregister the provided process phase by ID
// note: this will shift all process phases to fill the gap and maintain order
void whisker_ecs_s_deregister_process_phase(whisker_ecs_systems *systems, whisker_ecs_entity_id component_id)
{
	size_t index = -1;

	for (int i = 0; i < systems->process_phases_length; ++i)
	{
		if (systems->process_phases[i].id.id == component_id.id)
		{
			index = i;
		}
	}
	if (index > -1)
	{
    	for (int i = index; i < systems->process_phases_length - 1; i++)
    	{
        	systems->process_phases[i] = systems->process_phases[i + 1];
    	}

    	systems->process_phases_length--;
	}
}

// clear the current process phase list
// note: this does not remove the entities and components associated with the existing phases
void whisker_ecs_s_reset_process_phases(whisker_ecs_systems *systems)
{
    systems->process_phases_length = 0;
}


/*************************
*  iterator functions   *
*************************/
static bool itor_is_iteration_active(whisker_ecs_system_iterator *itor);
static void itor_increment_cursor(whisker_ecs_system_iterator *itor);
static void itor_set_master_components(whisker_ecs_system_iterator *itor);
static int itor_find_and_set_cursor_components(whisker_ecs_system_iterator *itor);
static void itor_update_master_index(whisker_ecs_system_iterator *itor);

// create and init an iterator instance
whisker_ecs_system_iterator *whisker_ecs_s_create_iterator()
{
	whisker_ecs_system_iterator *itor_new = whisker_mem_xcalloc_t(1, *itor_new);

	// create sparse sets for component pointers
	whisker_arr_init_t(itor_new->component_arrays, 1);
	whisker_arr_init_t(itor_new->component_arrays_cursors, 1);

	return itor_new;
}

// free instance of iterator and all data
void whisker_ecs_s_free_iterator(whisker_ecs_system_iterator *itor)
{
	free_null(itor->component_ids_rw);
	free_null(itor->component_ids_w);
	free_null(itor->component_ids_opt);
	free_null(itor->component_arrays);
	free_null(itor->component_arrays_cursors);
}

// get an iterator instance with the given itor_index
// note: this will init the iterator if one does not exist at the index
whisker_ecs_system_iterator *whisker_ecs_s_get_iterator(whisker_ecs_system_context *context, size_t itor_index, char *read_components, char *write_components, char *optional_components)
{
	whisker_ecs_system_iterator *itor;

	// check if iterator index is set
	if (!whisker_ss_contains(context->iterators, itor_index))
	{
		itor = whisker_ecs_s_create_iterator();
		whisker_ss_set(context->iterators, itor_index, itor);

		free(itor);
		itor = whisker_ss_get(context->iterators, itor_index);
		whisker_ecs_s_init_iterator(context, itor, read_components, write_components, optional_components);
	}

	itor = whisker_ss_get(context->iterators, itor_index);
	if (itor == NULL)
	{
		// TODO: panic here
		return NULL;
	}

	// reset iterator state
	itor->master_index = UINT64_MAX;
	itor->cursor = UINT64_MAX;
	itor->cursor_max = 0;
	itor->count = 0;
	itor->entity_id = whisker_ecs_e_id(UINT64_MAX);

	// find the master iterator by finding the smallest set
	for (int i = 0; i < itor->component_ids_rw_length; ++i)
	{
		whisker_sparse_set *component_array;
		if (itor->component_arrays[i] == NULL)
		{
			component_array = whisker_ecs_c_get_component_array(context->components, itor->component_ids_rw[i]);
			if (component_array == NULL)
			{
				itor->master_index = UINT64_MAX;
				break;
			}

			itor->component_arrays[i] = component_array;
		}
		else
		{
			component_array = itor->component_arrays[i];
		}

		if (component_array->sparse_index_length < itor->count || itor->count == 0)
		{
			itor->count = component_array->sparse_index_length;
			itor->cursor_max = itor->count;
			itor->master_index = i;
		}
	}

	/* debug_printf("ecs:sys:itor master selected: %zu count %zu components %zu\n", itor->master_index, itor->count, itor->component_ids->length); */

	// try to cache optional components when they are NULL
	for (int i = 0; i < itor->component_ids_opt_length; ++i)
	{
		whisker_sparse_set *component_array;
		size_t array_offset = itor->component_ids_rw_length + i;
		if (itor->component_arrays[array_offset] == NULL)
		{
			component_array = whisker_ecs_c_get_component_array(context->components, itor->component_ids_opt[i]);
			if (component_array == NULL)
			{
				continue;
			}

			itor->component_arrays[array_offset] = component_array;
		}
		itor->component_arrays_cursors[i] = 0;
	}

	// calculate cursor start and end point from thread context
	if (context->thread_max > 1)
	{
    	size_t context_thread_max = context->thread_max;
    	size_t thread_context_chunk_size = itor->count / context_thread_max;
    	itor->cursor = (context->thread_id * thread_context_chunk_size) - 1;
    	itor->cursor_max = (context->thread_id == context_thread_max - 1) ? itor->count : (context->thread_id * thread_context_chunk_size) + thread_context_chunk_size;

    	if (itor->cursor_max == itor->cursor + 1)
    	{
    		itor->count = 0;
    		itor->master_index = UINT64_MAX;
    		return itor;
    	}

    	/* if (context->thread_max > 1) { */
        /* 	debug_printf("itor: thread stats system %zu [t:%zu of %zu][cs:%zu m:%zu][c:%zu-%zu]\n", context->system_entity_id.id, context->thread_id + 1, context_thread_max, thread_context_chunk_size, itor->count, itor->cursor + 1, itor->cursor_max); */
    	/* } */
	}

	// if thread_max is uint64_max, assume we don't want to process anything
	// note: this is a hack added to allow an iterator to complete an
	// initialisation but ignore any matched entities
	else if (context->thread_max == UINT64_MAX)
	{
		itor->count = 0;
	}

	return itor;
}

// init the provided iterator and cache the given components
void whisker_ecs_s_init_iterator(whisker_ecs_system_context *context, whisker_ecs_system_iterator *itor, char *read_components, char *write_components, char *optional_components)
{
	// convert read and write component names to component sparse sets
	char *combined_components;
	combined_components = whisker_mem_xmalloc(strlen(read_components) + strlen(write_components) + 2);
	if (combined_components == NULL)
	{
		return;
	}

	strcpy(combined_components, read_components);
	if (strlen(write_components) > 0)
	{
		strcat(combined_components, ",");
		strcat(combined_components, write_components);
	}

	/* debug_printf("ecs:sys:itor init: read: %s write: %s\n", read_components, write_components); */
	/* debug_printf("ecs:sys:itor init: combined: %s\n", combined_components); */

	// rw components include read and write component IDs
	if (itor->component_ids_rw == NULL)
	{
		struct whisker_ecs_entity_id_array *e_arr = whisker_ecs_e_from_named_entities(context->entities, combined_components);
		itor->component_ids_rw = e_arr->arr;
		itor->component_ids_rw_length = e_arr->arr_length;
		itor->component_ids_rw_size = e_arr->arr_size;
		free(e_arr);

		if (itor->component_ids_rw == NULL)
		{
			// TODO: panic here
			free(combined_components);
			return;
		}

		whisker_arr_ensure_alloc(itor->component_arrays, itor->component_ids_rw_length);
		whisker_arr_ensure_alloc(itor->component_arrays_cursors, itor->component_ids_rw_length);
	}
	free(combined_components);

	// w components only includes write component IDs
	// note: these do not resize the main component arrays
	if (itor->component_ids_w == NULL)
	{
		struct whisker_ecs_entity_id_array *e_arr = whisker_ecs_e_from_named_entities(context->entities, write_components);
		itor->component_ids_w = e_arr->arr;
		itor->component_ids_w_length = e_arr->arr_length;
		itor->component_ids_w_size = e_arr->arr_size;
		free(e_arr);

		if (itor->component_ids_w == NULL)
		{
			// TODO: panic here
			return;
		}
	}

	// opt components only include optional component IDs
	// note: these belong at the end of the component arrays
	if (itor->component_ids_opt == NULL)
	{
		struct whisker_ecs_entity_id_array *e_arr = whisker_ecs_e_from_named_entities(context->entities, optional_components);
		itor->component_ids_opt = e_arr->arr;
		itor->component_ids_opt_length = e_arr->arr_length;
		itor->component_ids_opt_size = e_arr->arr_size;
		free(e_arr);

		if (itor->component_ids_opt == NULL)
		{
			// TODO: panic here
			return;
		}
		whisker_arr_ensure_alloc(itor->component_arrays, (itor->component_ids_rw_length + itor->component_ids_opt_length));
		whisker_arr_ensure_alloc(itor->component_arrays_cursors, (itor->component_ids_rw_length + itor->component_ids_opt_length));
	}
}


// step through an iterator incrementing the cursor
// note: returns false to indicate the iteration has reached the end
bool whisker_ecs_s_iterate(whisker_ecs_system_iterator *itor) 
{
    if (!itor_is_iteration_active(itor)) { return false; }

    itor_increment_cursor(itor);
    itor_set_master_components(itor);

    int cursor_state = itor_find_and_set_cursor_components(itor);
    itor_update_master_index(itor);

    return cursor_state == 0;
}

static bool itor_is_iteration_active(whisker_ecs_system_iterator *itor) {
    return (itor->master_index != UINT64_MAX && itor->count > 0);
}

static void itor_increment_cursor(whisker_ecs_system_iterator *itor) {
    itor->cursor++;
}

static void itor_set_master_components(whisker_ecs_system_iterator *itor) {
    whisker_sparse_set *master_set = itor->component_arrays[itor->master_index];
    itor->entity_id.id = master_set->sparse_index[itor->cursor];
}

static int itor_find_and_set_cursor_components(whisker_ecs_system_iterator *itor) {
    int cursor_state = 0;
    size_t rw_length = itor->component_ids_rw_length;
    for (int ci = 0; ci < rw_length; ++ci) {
        whisker_sparse_set *set = itor->component_arrays[ci];
        whisker_ecs_entity_id_raw cursor_entity = set->sparse_index[itor->cursor];

        if (cursor_entity == itor->entity_id.id) {
            itor->component_arrays_cursors[ci] = itor->cursor;
            cursor_state = 0;
            continue;
        }

        cursor_state = -1;
        size_t set_length = set->sparse_index_length;
        for (int i = itor->cursor; i < set_length; ++i) {
            cursor_entity = set->sparse_index[i];

            if (cursor_entity == itor->entity_id.id) {
            	itor->component_arrays_cursors[ci] = i;
                cursor_state = 0;
                break;
            }

            if (cursor_entity > itor->entity_id.id) {
                cursor_state = 1;
                break;
            }
        }

        if (cursor_state != 0) {
            break;
        }
    }
    return cursor_state;
}

static void itor_update_master_index(whisker_ecs_system_iterator *itor) {
    if (itor->cursor + 1 >= itor->cursor_max) {
        itor->master_index = UINT64_MAX;
    }
}
