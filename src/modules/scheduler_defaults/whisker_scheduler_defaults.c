/**
 * @author      : ElGatoPanzon
 * @file        : whisker_scheduler_defaults
 * @created     : Saturday Mar 07, 2026 11:27:59 CST
 * @description : Register default timesteps and phases on the ECS scheduler
 */

#include "whisker_scheduler_defaults.h"
#include "systems/system_disable_startup_phase.h"

void wm_scheduler_defaults_init(struct w_ecs_world *world, double fixed_update_rate)
{
	if (fixed_update_rate <= 0) {
		fixed_update_rate = WM_SCHEDULER_DEFAULTS_FIXED_UPDATE_RATE;
	}

	// clear existing timesteps and phases so we can set the defaults
	w_scheduler_reset_time_steps(&world->scheduler);
	w_scheduler_reset_phases(&world->scheduler);

	// register timesteps (rate 0 + uncapped = runs every frame)
	struct w_scheduler_time_step ts_pre            = {.enabled = true, .time_step = w_time_step_create(
		0,
		1,
		true,
		WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
	)};
	struct w_scheduler_time_step ts_default        = {.enabled = true, .time_step = w_time_step_create(
		0,
		1,
		true,
		WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
	)};
	struct w_scheduler_time_step ts_default_fixed  = {.enabled = true, .time_step = w_time_step_create(
		fixed_update_rate,
		1,
		false,
		WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
	)};
	struct w_scheduler_time_step ts_default_post   = {.enabled = true, .time_step = w_time_step_create(
		0,
		1,
		true,
		WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
	)};
	struct w_scheduler_time_step ts_default_render = {.enabled = true, .time_step = w_time_step_create(
		0,
		1,
		true,
		WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
	)};
	struct w_scheduler_time_step ts_reserved       = {.enabled = true, .time_step = w_time_step_create(
		0,
		1,
		true,
		WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
	)};
	struct w_scheduler_time_step ts_post           = {.enabled = true, .time_step = w_time_step_create(
		0,
		1,
		true,
		WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
	)};

	size_t ts_pre_id            = w_ecs_register_system_time_step(world, &ts_pre);
	size_t ts_default_id        = w_ecs_register_system_time_step(world, &ts_default);
	size_t ts_default_fixed_id  = w_ecs_register_system_time_step(world, &ts_default_fixed);
	size_t ts_default_post_id   = w_ecs_register_system_time_step(world, &ts_default_post);
	size_t ts_default_render_id = w_ecs_register_system_time_step(world, &ts_default_render);
	size_t ts_reserved_id       = w_ecs_register_system_time_step(world, &ts_reserved);
	size_t ts_post_id           = w_ecs_register_system_time_step(world, &ts_post);

	// timestep ordering: PRE -> DEFAULT -> FIXED -> POST -> RENDER -> RESERVED -> POST
	w_ecs_set_system_time_step_runs_before(world, ts_pre_id, ts_default_id);
	w_ecs_set_system_time_step_runs_before(world, ts_default_id, ts_default_fixed_id);
	w_ecs_set_system_time_step_runs_before(world, ts_default_fixed_id, ts_default_post_id);
	w_ecs_set_system_time_step_runs_before(world, ts_default_post_id, ts_default_render_id);
	w_ecs_set_system_time_step_runs_before(world, ts_default_render_id, ts_reserved_id);
	w_ecs_set_system_time_step_runs_before(world, ts_reserved_id, ts_post_id);

	// register phases with timestep assignments
	struct w_scheduler_phase p_pre              = {.enabled = true, .time_step_id = ts_pre_id};
	struct w_scheduler_phase p_on_startup       = {.enabled = true, .time_step_id = ts_default_id};
	struct w_scheduler_phase p_pre_load         = {.enabled = true, .time_step_id = ts_default_id};
	struct w_scheduler_phase p_on_load          = {.enabled = true, .time_step_id = ts_default_id};
	struct w_scheduler_phase p_post_load        = {.enabled = true, .time_step_id = ts_default_id};
	struct w_scheduler_phase p_pre_update       = {.enabled = true, .time_step_id = ts_default_id};
	struct w_scheduler_phase p_on_update        = {.enabled = true, .time_step_id = ts_default_id};
	struct w_scheduler_phase p_post_update      = {.enabled = true, .time_step_id = ts_default_post_id};
	struct w_scheduler_phase p_pre_fixed_update  = {.enabled = true, .time_step_id = ts_default_fixed_id};
	struct w_scheduler_phase p_on_fixed_update   = {.enabled = true, .time_step_id = ts_default_fixed_id};
	struct w_scheduler_phase p_post_fixed_update = {.enabled = true, .time_step_id = ts_default_fixed_id};
	struct w_scheduler_phase p_final_fixed      = {.enabled = true, .time_step_id = ts_default_fixed_id};
	struct w_scheduler_phase p_post_fixed       = {.enabled = true, .time_step_id = ts_default_fixed_id};
	struct w_scheduler_phase p_final            = {.enabled = true, .time_step_id = ts_default_post_id};
	struct w_scheduler_phase p_pre_render       = {.enabled = true, .time_step_id = ts_default_render_id};
	struct w_scheduler_phase p_on_render        = {.enabled = true, .time_step_id = ts_default_render_id};
	struct w_scheduler_phase p_post_render      = {.enabled = true, .time_step_id = ts_default_render_id};
	struct w_scheduler_phase p_final_render     = {.enabled = true, .time_step_id = ts_default_render_id};
	struct w_scheduler_phase p_reserved         = {.enabled = true, .time_step_id = ts_reserved_id};
	struct w_scheduler_phase p_post             = {.enabled = true, .time_step_id = ts_post_id};

	size_t id_pre              = w_ecs_register_system_phase(world, &p_pre);
	size_t id_on_startup       = w_ecs_register_system_phase(world, &p_on_startup);
	size_t id_pre_load         = w_ecs_register_system_phase(world, &p_pre_load);
	size_t id_on_load          = w_ecs_register_system_phase(world, &p_on_load);
	size_t id_post_load        = w_ecs_register_system_phase(world, &p_post_load);
	size_t id_pre_update       = w_ecs_register_system_phase(world, &p_pre_update);
	size_t id_on_update        = w_ecs_register_system_phase(world, &p_on_update);
	size_t id_post_update      = w_ecs_register_system_phase(world, &p_post_update);
	size_t id_pre_fixed_update  = w_ecs_register_system_phase(world, &p_pre_fixed_update);
	size_t id_on_fixed_update   = w_ecs_register_system_phase(world, &p_on_fixed_update);
	size_t id_post_fixed_update = w_ecs_register_system_phase(world, &p_post_fixed_update);
	size_t id_final_fixed      = w_ecs_register_system_phase(world, &p_final_fixed);
	size_t id_post_fixed       = w_ecs_register_system_phase(world, &p_post_fixed);
	size_t id_final            = w_ecs_register_system_phase(world, &p_final);
	size_t id_pre_render       = w_ecs_register_system_phase(world, &p_pre_render);
	size_t id_on_render        = w_ecs_register_system_phase(world, &p_on_render);
	size_t id_post_render      = w_ecs_register_system_phase(world, &p_post_render);
	size_t id_final_render     = w_ecs_register_system_phase(world, &p_final_render);
	size_t id_reserved         = w_ecs_register_system_phase(world, &p_reserved);
	size_t id_post             = w_ecs_register_system_phase(world, &p_post);

	// phase ordering within WM_TIMESTEP_DEFAULT
	w_ecs_set_system_phase_runs_before(world, id_on_startup, id_pre_load);
	w_ecs_set_system_phase_runs_before(world, id_pre_load, id_on_load);
	w_ecs_set_system_phase_runs_before(world, id_on_load, id_post_load);
	w_ecs_set_system_phase_runs_before(world, id_post_load, id_pre_update);
	w_ecs_set_system_phase_runs_before(world, id_pre_update, id_on_update);

	// phase ordering within WM_TIMESTEP_DEFAULT_POST
	w_ecs_set_system_phase_runs_before(world, id_post_update, id_final);

	// phase ordering within WM_TIMESTEP_DEFAULT_FIXED
	w_ecs_set_system_phase_runs_before(world, id_pre_fixed_update, id_on_fixed_update);
	w_ecs_set_system_phase_runs_before(world, id_on_fixed_update, id_post_fixed_update);
	w_ecs_set_system_phase_runs_before(world, id_post_fixed_update, id_final_fixed);
	w_ecs_set_system_phase_runs_before(world, id_final_fixed, id_post_fixed);

	// phase ordering within WM_TIMESTEP_DEFAULT_RENDER
	w_ecs_set_system_phase_runs_before(world, id_pre_render, id_on_render);
	w_ecs_set_system_phase_runs_before(world, id_on_render, id_post_render);
	w_ecs_set_system_phase_runs_before(world, id_post_render, id_final_render);

	// suppress unused warnings for single-phase timesteps
	(void)id_pre;
	(void)id_reserved;
	(void)id_post;

	// startup-disable system: disables WM_PHASE_ON_STARTUP after first frame
	struct w_system startup_disable = {
		.phase_id = id_reserved,
		.enabled = true,
		.update = wm_scheduler_defaults_system_disable_startup_phase_,
	};
	w_ecs_register_system(world, &startup_disable);
}

void wm_scheduler_defaults_free(struct w_ecs_world *world)
{
	(void)world;
}
