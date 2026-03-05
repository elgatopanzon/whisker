/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_scheduler
 * @created     : Wednesday Mar 04, 2026 12:58:37 CST
 */

#include "whisker_std.h"

#include "whisker_scheduler.h"

void w_scheduler_init(struct w_scheduler *scheduler)
{
	w_array_init_t(
			scheduler->time_steps, 
			W_SCHEDULER_TIMESTEPS_REALLOC_BLOCK_SIZE
	);
	scheduler->time_steps_length = 0;
	w_array_init_t(
			scheduler->time_steps_order, 
			W_SCHEDULER_TIMESTEPS_REALLOC_BLOCK_SIZE
	);
	scheduler->time_steps_order_length = 0;

	w_array_init_t(
			scheduler->phases, 
			W_SCHEDULER_PHASES_REALLOC_BLOCK_SIZE
	);
	scheduler->phases_length = 0;
	w_array_init_t(
			scheduler->phases_order, 
			W_SCHEDULER_PHASES_REALLOC_BLOCK_SIZE
	);
	scheduler->phases_order_length = 0;

	w_array_init_t(
			scheduler->schedule.items, 
			W_SCHEDULER_SCHEDULE_REALLOC_BLOCK_SIZE
	);
	scheduler->schedule.items_length = 0;
	scheduler->schedule.schedule_dirty = true;
}

void w_scheduler_free(struct w_scheduler *scheduler)
{
	free_null(scheduler->time_steps);
	free_null(scheduler->phases);
	free_null(scheduler->time_steps_order);
	free_null(scheduler->phases_order);
	free_null(scheduler->schedule.items);
}



static inline void w_scheduler_push_schedule_action_(struct w_scheduler *scheduler, struct w_scheduler_action *action)
{
	// ensure schedule items long enough
	w_array_ensure_alloc_block_size(
		scheduler->schedule.items,
		scheduler->schedule.items_length + 1,
		W_SCHEDULER_SCHEDULE_REALLOC_BLOCK_SIZE
	);

	memcpy(&scheduler->schedule.items[scheduler->schedule.items_length++], action, sizeof(*action));
}


static inline void w_scheduler_rebuild_schedule_(struct w_scheduler *scheduler, struct w_scheduler_job *jobs, size_t jobs_count)
{
	// clear scheduler actions
	scheduler->schedule.items_length = 0;

	// temp action, will write to this one and push each time
	struct w_scheduler_action action;
	action.action = W_SCHEDULER_ACTIONS_SCHEDULE_BEGIN;
	action.phase_id = 0;
	action.time_step_id = 0;
	action.job_id = 0;

	// write schedule start action
	action.action = W_SCHEDULER_ACTIONS_SCHEDULE_BEGIN;
	w_scheduler_push_schedule_action_(scheduler, &action);

	// compute the schedule from the job ID list, starting with timesteps then
	// phases orders
	for (size_t tsi = 0; tsi < scheduler->time_steps_order_length; tsi++)
	{
		size_t time_step_id = scheduler->time_steps_order[tsi];
		struct w_scheduler_time_step *time_step = w_scheduler_get_time_step(scheduler, time_step_id);

		// skip disable time steps
		if (!time_step->enabled) continue;

		// push time step start action
		action.action = W_SCHEDULER_ACTIONS_TIMESTEP_BEGIN;
		action.time_step_id = time_step_id;
		w_scheduler_push_schedule_action_(scheduler, &action);

		for (size_t pi = 0; pi < scheduler->phases_order_length; pi++)
		{
			size_t phase_id = scheduler->phases_order[pi];
			struct w_scheduler_phase *phase = w_scheduler_get_phase(scheduler, phase_id);

			// skip if disable phase or non-matching phase
			if (!phase->enabled || phase->time_step_id != time_step_id) continue;

			// push phase start action
			action.action = W_SCHEDULER_ACTIONS_PHASE_BEGIN;
			action.phase_id = phase_id;
			w_scheduler_push_schedule_action_(scheduler, &action);


			for (size_t ji = 0; ji < jobs_count; ji++)
			{
				// push the job if it matches current phase ID
				if (jobs[ji].phase_id == phase_id)
				{
					action.action = W_SCHEDULER_ACTIONS_DISPATCH;
					action.job_id = jobs[ji].job_id;
					w_scheduler_push_schedule_action_(scheduler, &action);
				}
			}

			// push phase end action
			action.action = W_SCHEDULER_ACTIONS_PHASE_END;
			w_scheduler_push_schedule_action_(scheduler, &action);
		}

		// push timestep end action
		action.action = W_SCHEDULER_ACTIONS_TIMESTEP_END;
		w_scheduler_push_schedule_action_(scheduler, &action);
	}

	// write schedule end action
	action.action = W_SCHEDULER_ACTIONS_SCHEDULE_END;
	action.phase_id = 0;
	action.time_step_id = 0;
	action.job_id = 0;

	w_scheduler_push_schedule_action_(scheduler, &action);
}

struct w_scheduler_schedule *w_scheduler_get_schedule(struct w_scheduler *scheduler, struct w_scheduler_job *jobs, size_t jobs_count)
{
	if (scheduler->schedule.schedule_dirty)
	{
		w_scheduler_rebuild_schedule_(scheduler, jobs, jobs_count);
		scheduler->schedule.schedule_dirty = false;
	}

	return &scheduler->schedule;
}


/********************************
*  phase management functions  *
********************************/

size_t w_scheduler_register_phase(struct w_scheduler *scheduler, struct w_scheduler_phase *phase)
{
	// ensure phase array large enough
	w_array_ensure_alloc_block_size(
		scheduler->phases,
		scheduler->phases_length + 1,
		W_SCHEDULER_PHASES_REALLOC_BLOCK_SIZE
	);

	// ensure phase order large enough
	w_array_ensure_alloc_block_size(
		scheduler->phases_order,
		scheduler->phases_order_length + 1,
		W_SCHEDULER_PHASES_REALLOC_BLOCK_SIZE
	);

	size_t id = ++scheduler->phases_length;
	scheduler->phases[scheduler->phases_length - 1] = *phase;
	scheduler->phases[scheduler->phases_length - 1].id = id;

	// append id to phase order list
	scheduler->phases_order[scheduler->phases_order_length++] = id;

	scheduler->schedule.schedule_dirty = true;

	return id;
}
struct w_scheduler_phase *w_scheduler_get_phase(struct w_scheduler *scheduler, size_t phase_id)
{
	return ARRAY_SELECT(struct w_scheduler_phase, scheduler->phases, scheduler->phases_length, id, == phase_id);
}
void w_scheduler_set_phase_state(struct w_scheduler *scheduler, size_t phase_id, bool state)
{
	struct w_scheduler_phase *phase = w_scheduler_get_phase(scheduler, phase_id);
	if (phase)
	{
		if (phase->enabled != state)
		{
			scheduler->schedule.schedule_dirty = true;
		}
		phase->enabled = state;
	}
}
void w_scheduler_reset_phases(struct w_scheduler *scheduler)
{
	scheduler->phases_length = 0;
	scheduler->phases_order_length = 0;
	scheduler->schedule.schedule_dirty = true;
}
static inline void w_scheduler_move_phase_(struct w_scheduler *scheduler, size_t phase_id, size_t target_phase_id, bool runs_before)
{
	struct w_scheduler_phase *phase = w_scheduler_get_phase(scheduler, phase_id);
	struct w_scheduler_phase *target_phase = w_scheduler_get_phase(scheduler, target_phase_id);

	if (!phase || !target_phase)
		return;

	// get current indexes
	size_t phase_id_idx = SIZE_MAX;
	size_t target_phase_id_idx = SIZE_MAX;
	for (size_t i = 0; i < scheduler->phases_order_length; i++)
	{
		if (scheduler->phases_order[i] == phase_id)
		{
			phase_id_idx = i;
			break;
		}
	}
	for (size_t i = 0; i < scheduler->phases_order_length; i++)
	{
		if (scheduler->phases_order[i] == target_phase_id)
		{
			target_phase_id_idx = i;
			break;
		}
	}

	// early out if invalid indexes
	if (phase_id_idx == SIZE_MAX || target_phase_id_idx == SIZE_MAX || phase_id_idx == target_phase_id_idx)
		return;

	// remove and insert phase_id using in-place swaps
	size_t from = phase_id_idx;
	size_t to = runs_before ? target_phase_id_idx : target_phase_id_idx + 1;
	if (from < to) {
		// move forward, shift left while swapping
		for (size_t i = from; i < to - 1; i++)
			scheduler->phases_order[i] = scheduler->phases_order[i + 1];
		scheduler->phases_order[to - 1] = phase_id;
	} else if (from > to) {
		// move backward, shift right while swapping
		for (size_t i = from; i > to; i--)
			scheduler->phases_order[i] = scheduler->phases_order[i - 1];
		scheduler->phases_order[to] = phase_id;
	}

	scheduler->schedule.schedule_dirty = true;
}
void w_scheduler_set_phase_runs_before(struct w_scheduler *scheduler, size_t phase_id, size_t runs_before_phase_id)
{
	w_scheduler_move_phase_(scheduler, phase_id, runs_before_phase_id, true);
}
void w_scheduler_set_phase_runs_after(struct w_scheduler *scheduler, size_t phase_id, size_t runs_after_phase_id)
{
	w_scheduler_move_phase_(scheduler, phase_id, runs_after_phase_id, false);
}


/***********************************
*  timestep management functions  *
***********************************/

size_t w_scheduler_register_time_step(struct w_scheduler *scheduler, struct w_scheduler_time_step *time_step)
{
	// ensure time_step array large enough
	w_array_ensure_alloc_block_size(
		scheduler->time_steps,
		scheduler->time_steps_length + 1,
		W_SCHEDULER_TIMESTEPS_REALLOC_BLOCK_SIZE
	);

	// ensure time_step order large enough
	w_array_ensure_alloc_block_size(
		scheduler->time_steps_order,
		scheduler->time_steps_order_length + 1,
		W_SCHEDULER_TIMESTEPS_REALLOC_BLOCK_SIZE
	);

	size_t id = ++scheduler->time_steps_length;
	scheduler->time_steps[scheduler->time_steps_length - 1] = *time_step;
	scheduler->time_steps[scheduler->time_steps_length - 1].id = id;

	// append id to time_step order list
	scheduler->time_steps_order[scheduler->time_steps_order_length++] = id;

	scheduler->schedule.schedule_dirty = true;

	return id;
}
struct w_scheduler_time_step *w_scheduler_get_time_step(struct w_scheduler *scheduler, size_t time_step_id)
{
	return ARRAY_SELECT(struct w_scheduler_time_step, scheduler->time_steps, scheduler->time_steps_length, id, == time_step_id);
}
void w_scheduler_set_time_step_state(struct w_scheduler *scheduler, size_t time_step_id, bool state)
{
	struct w_scheduler_time_step *time_step = w_scheduler_get_time_step(scheduler, time_step_id);
	if (time_step)
	{
		if (time_step->enabled != state)
		{
			scheduler->schedule.schedule_dirty = true;
		}
		time_step->enabled = state;
	}
}
void w_scheduler_reset_time_steps(struct w_scheduler *scheduler)
{
	scheduler->time_steps_length = 0;
	scheduler->time_steps_order_length = 0;
	scheduler->schedule.schedule_dirty = true;
}
static inline void w_scheduler_move_time_step_(struct w_scheduler *scheduler, size_t time_step_id, size_t target_time_step_id, bool runs_before)
{
	struct w_scheduler_time_step *time_step = w_scheduler_get_time_step(scheduler, time_step_id);
	struct w_scheduler_time_step *target_time_step = w_scheduler_get_time_step(scheduler, target_time_step_id);

	if (!time_step || !target_time_step)
		return;

	// get current indexes
	size_t time_step_id_idx = SIZE_MAX;
	size_t target_time_step_id_idx = SIZE_MAX;
	for (size_t i = 0; i < scheduler->time_steps_order_length; i++)
	{
		if (scheduler->time_steps_order[i] == time_step_id)
		{
			time_step_id_idx = i;
			break;
		}
	}
	for (size_t i = 0; i < scheduler->time_steps_order_length; i++)
	{
		if (scheduler->time_steps_order[i] == target_time_step_id)
		{
			target_time_step_id_idx = i;
			break;
		}
	}

	// early out if invalid indexes
	if (time_step_id_idx == SIZE_MAX || target_time_step_id_idx == SIZE_MAX || time_step_id_idx == target_time_step_id_idx)
		return;

	// remove and insert time_step_id using in-place swaps
	size_t from = time_step_id_idx;
	size_t to = runs_before ? target_time_step_id_idx : target_time_step_id_idx + 1;
	if (from < to) {
		// move forward, shift left while swapping
		for (size_t i = from; i < to - 1; i++)
			scheduler->time_steps_order[i] = scheduler->time_steps_order[i + 1];
		scheduler->time_steps_order[to - 1] = time_step_id;
	} else if (from > to) {
		// move backward, shift right while swapping
		for (size_t i = from; i > to; i--)
			scheduler->time_steps_order[i] = scheduler->time_steps_order[i - 1];
		scheduler->time_steps_order[to] = time_step_id;
	}

	scheduler->schedule.schedule_dirty = true;
}
void w_scheduler_set_time_step_runs_before(struct w_scheduler *scheduler, size_t time_step_id, size_t runs_before_time_step_id)
{
	w_scheduler_move_time_step_(scheduler, time_step_id, runs_before_time_step_id, true);
}
void w_scheduler_set_time_step_runs_after(struct w_scheduler *scheduler, size_t time_step_id, size_t runs_after_time_step_id)
{
	w_scheduler_move_time_step_(scheduler, time_step_id, runs_after_time_step_id, false);
}
