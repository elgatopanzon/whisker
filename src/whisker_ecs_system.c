/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_ecs_system
 * @created     : Thursday Feb 13, 2025 17:59:56 CST
 */

#include "whisker_std.h"
#include "whisker_debug.h"
#include "whisker_array.h"
#include "whisker_string.h"
#include "whisker_dict.h"

#include "whisker_ecs_system.h"

// create and init an instance of systems container
E_WHISKER_ECS_SYS whisker_ecs_s_create_systems(whisker_ecs_systems **systems)
{
	whisker_ecs_systems *s = calloc(1, sizeof(*s));
	if (s == NULL)
	{
		return E_WHISKER_ECS_SYS_MEM;
	}

	// create array for systems
	if (warr_create(whisker_ecs_system, 0, &s->systems) != E_WHISKER_ARR_OK)
		return E_WHISKER_ECS_SYS_ARR;
	s->systems_length = 0;

	// create array for process phase list
	E_WHISKER_ARR err = whisker_arr_create_whisker_ecs_system_process_phase(&s->process_phases, 0);
	if (err != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_ECS_SYS_ARR;
	}

	*systems = s;

	return E_WHISKER_ECS_SYS_OK;
}

// deallocate an instance of systems container
void whisker_ecs_s_free_systems(whisker_ecs_systems *systems)
{
	for (int i = 0; i < systems->systems_length; ++i)
	{
		whisker_ecs_s_free_system(&systems->systems[i]);
	}

	warr_free(systems->systems);
	whisker_arr_free_whisker_ecs_system_process_phase(systems->process_phases);

	free(systems);
}

// regiser a system in the systems container
whisker_ecs_system* whisker_ecs_s_register_system(whisker_ecs_systems *systems, whisker_ecs_components *components, whisker_ecs_system system)
{
	// create iterators sparse set
	E_WHISKER_SS itors_err = whisker_ss_create_t(&system.iterators, whisker_ecs_iterator);
	if (itors_err != E_WHISKER_SS_OK)
	{
		return NULL;
	}


	// add system to main systems list
	E_WHISKER_ARR push_err = warr_push(&systems->systems, &system);
	if (push_err != E_WHISKER_ARR_OK)
	{
		return NULL;
	}
	systems->systems_length++;

	return &systems->systems[systems->systems_length - 1];
}

// deallocate a system instance
void whisker_ecs_s_free_system(whisker_ecs_system *system)
{
	if (system->iterators != NULL)
	{
		for (int i = 0; i < system->iterators->sparse_index->length; ++i)
		{
			whisker_ecs_iterator itor = ((whisker_ecs_iterator*)system->iterators->dense)[i];
			if (itor.read != NULL)
			{
				whisker_arr_free_void_(itor.read);
			}
			if (itor.write != NULL)
			{
				whisker_arr_free_void_(itor.write);
			}
			if (itor.opt != NULL)
			{
				whisker_arr_free_void_(itor.opt);
			}

			if (itor.component_ids_rw != NULL)
			{
				whisker_arr_free_whisker_ecs_entity_id(itor.component_ids_rw);
			}
			if (itor.component_ids_w != NULL)
			{
				whisker_arr_free_whisker_ecs_entity_id(itor.component_ids_w);
			}
			if (itor.component_ids_opt != NULL)
			{
				whisker_arr_free_whisker_ecs_entity_id(itor.component_ids_opt);
			}
			if (itor.component_arrays != NULL)
			{
				whisker_arr_free_void_(itor.component_arrays);
			}
		}

		whisker_ss_free(system->iterators);
	}
}

// run an update on the registered systems
E_WHISKER_ECS_SYS whisker_ecs_s_update_systems(whisker_ecs_systems *systems, whisker_ecs_entities *entities, double delta_time)
{
	whisker_ecs_system *system = &systems->systems[systems->system_id];

	// loop over each registered process phase
	for (int i = 0; i < systems->process_phases->length; ++i)
	{
		whisker_ecs_system_process_phase *process_phase = &systems->process_phases->arr[i];

		// advance the process phase time step
		int update_count = whisker_time_step_step_get_update_count(&process_phase->time_step);

		/* printf("update count for phase: %d delta %.10f ms/f %s\n", update_count, process_phase->time_step.delta_time_variable * 1000, entities->entities->arr[process_phase->id.index].name); */

		// update all systems in the process phase by update count
		for (int ui = 0; ui < update_count; ++ui)
		{
			whisker_ecs_iterator *system_itor = whisker_ecs_s_get_iterator(system, process_phase->id.index, "w_ecs_system_idx", entities->entities->arr[process_phase->id.index].name, "");

			while (whisker_ecs_s_iterate(system, system_itor)) 
			{
				int *system_idx = system_itor->read->arr[0];
				whisker_ecs_system *system = &systems->systems[*system_idx];

				system->delta_time = process_phase->time_step.delta_time_fixed;
				system->process_phase_time_step = &process_phase->time_step;
				E_WHISKER_ECS_SYS update_err = whisker_ecs_s_update_system(system);			
				if (update_err != E_WHISKER_ECS_SYS_OK)
				{
					// TODO: panic here if a system failed to execute for any
					// reason
					// for now just continue the loop as normal
					continue;
				}
			}
		}
	}

	return E_WHISKER_ECS_SYS_OK;
}

// update the provided system
E_WHISKER_ECS_SYS whisker_ecs_s_update_system(whisker_ecs_system *system)
{
	system->system_ptr(system);

	return E_WHISKER_ECS_SYS_OK;
}

// register a system process phase for the system scheduler
E_WHISKER_ECS_SYS whisker_ecs_s_register_process_phase(whisker_ecs_systems *systems, whisker_ecs_entity_id component_id, double update_rate_sec)
{
	whisker_ecs_system_process_phase process_phase = {
		.id = component_id,
		.time_step = whisker_time_step_create((update_rate_sec > 0) ? update_rate_sec : 60, (update_rate_sec > 0) ? false : true),
	};

	E_WHISKER_ARR err = whisker_arr_push_whisker_ecs_system_process_phase(systems->process_phases, process_phase);
	if (err != E_WHISKER_ARR_OK)
	{
		return E_WHISKER_ECS_SYS_ARR;
	}

	return E_WHISKER_ECS_SYS_OK;
}

// deregister the provided process phase by ID
// note: this will shift all process phases to fill the gap and maintain order
E_WHISKER_ECS_SYS whisker_ecs_s_deregister_process_phase(whisker_ecs_systems *systems, whisker_ecs_entity_id component_id)
{
	size_t index = -1;

	for (int i = 0; i < systems->process_phases->length; ++i)
	{
		if (systems->process_phases->arr[i].id.id == component_id.id)
		{
			index = i;
		}
	}
	if (index > -1)
	{
    	for (int i = index; i < systems->process_phases->length - 1; i++)
    	{
        	systems->process_phases->arr[i] = systems->process_phases->arr[i + 1];
    	}

    	systems->process_phases->length--;
	}

	return E_WHISKER_ECS_SYS_OK;
}

// clear the current process phase list
// note: this does not remove the entities and components associated with the existing phases
E_WHISKER_ECS_SYS whisker_ecs_s_reset_process_phases(whisker_ecs_systems *systems)
{
    systems->process_phases->length = 0;

    return E_WHISKER_ECS_SYS_OK;
}


/*************************
*  iterator functions   *
*************************/
// create and init an iterator instance
E_WHISKER_ECS_SYS whisker_ecs_s_create_iterator(whisker_ecs_iterator **itor)
{
	whisker_ecs_iterator *itor_new;
	E_WHISKER_MEM err = whisker_mem_try_calloc(1, sizeof(*itor_new), (void**)&itor_new);
	if (err != E_WHISKER_MEM_OK)
	{
		return E_WHISKER_ECS_SYS_MEM;
	}

	// create sparse sets for component pointers
	if (whisker_arr_create_void_(&itor_new->read, 0) != E_WHISKER_ARR_OK) return E_WHISKER_ECS_SYS_ARR;
	if (whisker_arr_create_void_(&itor_new->write, 0) != E_WHISKER_ARR_OK) return E_WHISKER_ECS_SYS_ARR;
	if (whisker_arr_create_void_(&itor_new->opt, 0) != E_WHISKER_ARR_OK) return E_WHISKER_ECS_SYS_ARR;
	if (whisker_arr_create_void_(&itor_new->component_arrays, 0) != E_WHISKER_ARR_OK) return E_WHISKER_ECS_SYS_ARR;

	*itor = itor_new;

	return E_WHISKER_ECS_SYS_OK;
}

// get an iterator instance with the given itor_index
// note: this will init the iterator if one does not exist at the index
whisker_ecs_iterator *whisker_ecs_s_get_iterator(whisker_ecs_system *system, size_t itor_index, char *read_components, char *write_components, char *optional_components)
{
	whisker_ecs_iterator *itor;

	// check if iterator index is set
	if (!wss_contains(system->iterators, itor_index))
	{
		E_WHISKER_ECS_SYS itor_err = whisker_ecs_s_create_iterator(&itor);
		if (itor_err != E_WHISKER_ECS_SYS_OK)
		{
			// TODO: panic here
			return NULL;
		}
		E_WHISKER_SS set_err = wss_set(system->iterators, itor_index, itor);
		if (set_err != E_WHISKER_SS_OK)
		{
			// TODO: panic here
			return NULL;
		}

		free(itor);
		itor = wss_get(system->iterators, itor_index);
		itor_err = whisker_ecs_s_init_iterator(system, itor, read_components, write_components, optional_components);
		if (itor_err != E_WHISKER_ECS_SYS_OK)
		{
			// TODO: panic here
			return NULL;
		}
	}

	itor = wss_get(system->iterators, itor_index);
	if (itor == NULL)
	{
		// TODO: panic here
		return NULL;
	}

	// reset iterator state
	itor->master_index = UINT64_MAX;
	itor->cursor = UINT64_MAX;
	itor->count = 0;
	itor->entity_id = whisker_ecs_e_id(UINT64_MAX);

	// find the master iterator by finding the smallest set
	for (int i = 0; i < itor->component_ids_rw->length; ++i)
	{
		whisker_sparse_set *component_array;
		if (itor->component_arrays->arr[i] == NULL)
		{
			whisker_ecs_c_get_component_array(system->components, itor->component_ids_rw->arr[i], &component_array);
			if (component_array == NULL)
			{
				itor->master_index = UINT64_MAX;
				break;
			}

			itor->component_arrays->arr[i] = component_array;
		}
		else
		{
			component_array = itor->component_arrays->arr[i];
		}

		if (component_array->sparse_index->length < itor->count || itor->count == 0)
		{
			itor->count = component_array->sparse_index->length;
			itor->master_index = i;
		}
	}

	/* debug_printf("ecs:sys:itor master selected: %zu count %zu components %zu\n", itor->master_index, itor->count, itor->component_ids->length); */

	// try to cache optional components when they are NULL
	for (int i = 0; i < itor->component_ids_opt->length; ++i)
	{
		whisker_sparse_set *component_array;
		size_t array_offset = itor->component_ids_rw->length + i;
		if (itor->component_arrays->arr[array_offset] == NULL)
		{
			whisker_ecs_c_get_component_array(system->components, itor->component_ids_opt->arr[i], &component_array);
			if (component_array == NULL)
			{
				continue;
			}

			itor->component_arrays->arr[array_offset] = component_array;
		}
	}

	return itor;
}

// init the provided iterator and cache the given components
E_WHISKER_ECS_SYS whisker_ecs_s_init_iterator(whisker_ecs_system *system, whisker_ecs_iterator *itor, char *read_components, char *write_components, char *optional_components)
{
	// convert read and write component names to component sparse sets
	char *combined_components;
	combined_components = malloc(strlen(read_components) + strlen(write_components) + 2);
	if (combined_components == NULL)
	{
		return E_WHISKER_ECS_SYS_MEM;
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
		itor->component_ids_rw = whisker_ecs_e_from_named_entities(system->entities, combined_components);
		if (itor->component_ids_rw == NULL)
		{
			// TODO: panic here
			free(combined_components);
			return E_WHISKER_ECS_SYS_ARR;
		}
		if (whisker_arr_resize_void_(itor->read, itor->component_ids_rw->length, true) != E_WHISKER_ARR_OK) {
			free(combined_components);
    		return E_WHISKER_ECS_SYS_ARR;
		}
		if (whisker_arr_resize_void_(itor->write, itor->component_ids_rw->length, true) != E_WHISKER_ARR_OK) {
			free(combined_components);
    		return E_WHISKER_ECS_SYS_ARR;
		}
		if (whisker_arr_resize_void_(itor->component_arrays, itor->component_ids_rw->length, true) != E_WHISKER_ARR_OK) {
			free(combined_components);
    		return E_WHISKER_ECS_SYS_ARR;
		}
	}
	free(combined_components);

	// w components only includes write component IDs
	// note: these do not resize the main component arrays
	if (itor->component_ids_w == NULL)
	{
		itor->component_ids_w = whisker_ecs_e_from_named_entities(system->entities, write_components);
		if (itor->component_ids_w == NULL)
		{
			// TODO: panic here
			return E_WHISKER_ECS_SYS_ARR;
		}
	}

	// opt components only include optional component IDs
	// note: these belong at the end of the component arrays
	if (itor->component_ids_opt == NULL)
	{
		itor->component_ids_opt = whisker_ecs_e_from_named_entities(system->entities, optional_components);
		if (itor->component_ids_opt == NULL)
		{
			// TODO: panic here
			return E_WHISKER_ECS_SYS_ARR;
		}
		if (whisker_arr_resize_void_(itor->opt, itor->component_ids_opt->length, true) != E_WHISKER_ARR_OK) {
    		return E_WHISKER_ECS_SYS_ARR;
		}
		if (whisker_arr_resize_void_(itor->component_arrays, itor->component_ids_rw->length + itor->component_ids_opt->length, true) != E_WHISKER_ARR_OK) {
    		return E_WHISKER_ECS_SYS_ARR;
		}
	}

	return E_WHISKER_ECS_SYS_OK;
}

// iterate one step with the provided iterator
// note: iteration returns true while the iteration is still on-going
bool whisker_ecs_s_iterate(whisker_ecs_system *system, whisker_ecs_iterator *itor)
{
	// if the master index is invalid then there is nothing to iterate
	bool iteration_active = (itor->master_index != UINT64_MAX && itor->count > 0);
	if (!iteration_active)
	{
		return iteration_active;
	}

	itor->cursor++;

	// set the master components
	whisker_sparse_set *master_set = itor->component_arrays->arr[itor->master_index];
	itor->entity_id.id = master_set->sparse_index->arr[itor->cursor];
	/* itor->read->arr[itor->master_index] = master_set->dense + (itor->cursor * master_set->element_size); */
	/* itor->write->arr[itor->master_index] = master_set->dense + (itor->cursor * master_set->element_size); */

	/* debug_printf("ecs:sys:itor master: index %zu entity %zu cursor [%zu/%zu]\n", itor->master_index, master_entity.id, itor->cursor, itor->count - 1); */
    /*  */
	/* // print out all component entities */
	/* for (int ci = 0; ci < itor->component_arrays->length; ++ci) */
	/* { */
	/* 	whisker_sparse_set *set = itor->component_arrays->arr[ci]; */
	/* 	debug_printf("ecs:sys:itor [%zu/%zu] component %d entities: ", itor->cursor, itor->count - 1, ci); */
	/* 	for (int i = 0; i < set->sparse_index->length; ++i) */
	/* 	{ */
	/* 		debug_printf("%zu ", set->sparse_index->arr[i]); */
	/* 	} */
	/* 	debug_printf("\n"); */
	/* } */

	// find and set the current cursor's components
	int cursor_state = 0;
	size_t rw_length = itor->component_ids_rw->length;
	for (int ci = 0; ci < rw_length; ++ci)
	{
		// get current set for component
		whisker_sparse_set *set = itor->component_arrays->arr[ci];
		whisker_ecs_entity_id_raw cursor_entity = set->sparse_index->arr[itor->cursor];

		// check if cursor entity matches, if so set the component data
		if (cursor_entity == itor->entity_id.id)
		{
			/* debug_printf("ecs:sys:itor [%zu/%zu] component %d cursor entity %zu == master entity %zu\n", itor->cursor, itor->count - 1, ci, cursor_entity.id, master_entity.id); */

			itor->read->arr[ci] = set->dense + (itor->cursor * set->element_size);
			itor->write->arr[ci] = set->dense + (itor->cursor * set->element_size);

			cursor_state = 0;

			continue;
		}

		// reset current cursor state
		cursor_state = -1;

		/* debug_printf("ecs:sys:itor [%zu/%zu] component %d entity: ", itor->cursor, itor->count - 1, ci); */
		/* for (int i = 0; i < set->sparse_index->length; ++i) */
		/* { */
		/* 	debug_printf("%zu ", set->sparse_index->arr[i]); */
		/* } */
		/* debug_printf("\n"); */

		// look for the next matching entity
		// note: it's possible we need a need check for coming to the end of a
		// list to also set the state invalid
		size_t set_length = set->sparse_index->length;
		for (int i = itor->cursor; i < set_length; ++i)
		{
			cursor_entity = set->sparse_index->arr[i];

			// check if cursor entity matches, if so set the component data
			if (cursor_entity == itor->entity_id.id)
			{
				/* debug_printf("ecs:sys:itor [%zu/%zu] component %d cursor entity %zu == master entity %zu\n", itor->cursor, itor->count - 1, ci, cursor_entity.id, master_entity.id); */

				itor->read->arr[ci] = set->dense + (i * set->element_size);
				itor->write->arr[ci] = set->dense + (i * set->element_size);

				// set cursor state to 0 to indicate success
				cursor_state = 0;

				break;
			}

			// if the cursor entity is > than master, then we can early out
			if (cursor_entity > itor->entity_id.id)
			{
				/* debug_printf("ecs:sys:itor [%zu/%zu] component %d cursor entity %zu > master entity %zu\n", itor->cursor, itor->count, ci, cursor_entity.id, master_entity.id); */
				cursor_state = 1;
				break;
			}
		}

		// if the state is anything other than 0, then end the loop early
		if (cursor_state != 0)
		{
			break;
		}

	}

	/* debug_printf("ecs:sys:itor cursor %zu state %d\n", itor->cursor, cursor_state); */

	if (itor->cursor + 1 >= master_set->sparse_index->length)
	{
		/* debug_printf("ecs:sys:itor reached end of master %zu with entity %zu\n", itor->master_index, itor->entity_id); */
		itor->master_index = UINT64_MAX;
	}

	return cursor_state == 0;
}
