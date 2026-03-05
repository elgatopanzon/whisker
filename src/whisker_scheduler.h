/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_scheduler
 * @created     : Wednesday Mar 04, 2026 12:15:55 CST
 * @description : Job scheduler with phases and timesteps
 */

#include "whisker_std.h"
#include "whisker_time.h"
#include "whisker_arena.h"
#include "whisker_array.h"

#ifndef WHISKER_SCHEDULER_H
#define WHISKER_SCHEDULER_H

#ifndef W_SCHEDULER_TIMESTEPS_REALLOC_BLOCK_SIZE
#define W_SCHEDULER_TIMESTEPS_REALLOC_BLOCK_SIZE 16
#endif /* ifndef W_SCHEDULER_TIMESTEPS_REALLOC_BLOCK_SIZE */

#ifndef W_SCHEDULER_PHASES_REALLOC_BLOCK_SIZE
#define W_SCHEDULER_PHASES_REALLOC_BLOCK_SIZE 32
#endif /* ifndef W_SCHEDULER_PHASES_REALLOC_BLOCK_SIZE */

#ifndef W_SCHEDULER_SCHEDULE_REALLOC_BLOCK_SIZE
#define W_SCHEDULER_SCHEDULE_REALLOC_BLOCK_SIZE 32
#endif /* ifndef W_SCHEDULER_SCHEDULE_REALLOC_BLOCK_SIZE */

enum W_SCHEDULER_ACTIONS
{ 
	W_SCHEDULER_ACTIONS_NOOP = 0,
	W_SCHEDULER_ACTIONS_SCHEDULE_BEGIN = 1,
	W_SCHEDULER_ACTIONS_SCHEDULE_END = 2,
	W_SCHEDULER_ACTIONS_TIMESTEP_BEGIN = 3,
	W_SCHEDULER_ACTIONS_TIMESTEP_END = 4,
	W_SCHEDULER_ACTIONS_PHASE_BEGIN = 5,
	W_SCHEDULER_ACTIONS_PHASE_END = 6,
	W_SCHEDULER_ACTIONS_DISPATCH = 7,
};

// scheduler phases allow grouping jobs
struct w_scheduler_phase 
{
	bool enabled;
	size_t id;
	size_t time_step_id;
};

// scheduler timesteps isolate phase update frequency
struct w_scheduler_time_step 
{
	bool enabled;
	size_t id;
	struct whisker_time_step time_step;
};

// scheduler returns action structs containing actions to be processed
struct w_scheduler_action 
{
	enum W_SCHEDULER_ACTIONS action;
	size_t phase_id;
	struct whisker_time_step *time_step;
	size_t job_idx;
};

struct w_scheduler_job
{
	size_t job_id;
	size_t phase_id;
};

struct w_scheduler_schedule 
{
	w_array_declare(struct w_scheduler_action, items);
	bool schedule_dirty;
	uint64_t rebuild_count;
};

struct w_scheduler 
{
	w_array_declare(struct w_scheduler_time_step, time_steps);
	w_array_declare(struct w_scheduler_phase, phases);
	w_array_declare(size_t, time_steps_order);
	w_array_declare(size_t, phases_order);

	struct w_scheduler_schedule schedule;
};


// init a w_scheduler
void w_scheduler_init(struct w_scheduler *scheduler);

// free a w_scheduler's timesteps, phases and schedule
void w_scheduler_free(struct w_scheduler *scheduler);

// get the schedule
struct w_scheduler_schedule *w_scheduler_get_schedule(struct w_scheduler *scheduler, struct w_scheduler_job *jobs, size_t jobs_count);

// phase management: register a phase
size_t w_scheduler_register_phase(struct w_scheduler *scheduler, struct w_scheduler_phase *phase);
// phase management: get phase
struct w_scheduler_phase *w_scheduler_get_phase(struct w_scheduler *scheduler, size_t phase_id);
// phase management: set phase enabled/disabled
void w_scheduler_set_phase_state(struct w_scheduler *scheduler, size_t phase_id, bool state);
// phase management: reset and clear all registered phases
void w_scheduler_reset_phases(struct w_scheduler *scheduler);
// phase management: set phase runs before
void w_scheduler_set_phase_runs_before(struct w_scheduler *scheduler, size_t phase_id, size_t runs_before_phase_id);
// phase management: set phase runs after
void w_scheduler_set_phase_runs_after(struct w_scheduler *scheduler, size_t phase_id, size_t runs_after_phase_id);

// timestep management: register a timestep
size_t w_scheduler_register_time_step(struct w_scheduler *scheduler, struct w_scheduler_time_step *time_step);
// timestep management: get timestep
struct w_scheduler_time_step *w_scheduler_get_time_step(struct w_scheduler *scheduler, size_t time_step_id);
// timestep management: set timestep enabled/disabled
void w_scheduler_set_time_step_state(struct w_scheduler *scheduler, size_t time_step_id, bool state);
// timestep management: reset and clear all registered timesteps
void w_scheduler_reset_time_steps(struct w_scheduler *scheduler);
// timestep management: set timestep runs before
void w_scheduler_set_time_step_runs_before(struct w_scheduler *scheduler, size_t time_step_id, size_t runs_before_time_step_id);
// timestep management: set timestep runs after
void w_scheduler_set_time_step_runs_after(struct w_scheduler *scheduler, size_t time_step_id, size_t runs_after_time_step_id);

#endif /* WHISKER_SCHEDULER_H */
