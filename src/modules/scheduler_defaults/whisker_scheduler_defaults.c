/**
 * @author      : ElGatoPanzon
 * @file        : whisker_scheduler_defaults
 * @created     : Saturday Mar 07, 2026 11:27:59 CST
 * @description : Register default timesteps and phases on the ECS scheduler
 */

#include "whisker_scheduler_defaults.h"
#include "whisker_scheduler_defaults_systems.h"

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
	), .name = "PRE"};
	struct w_scheduler_time_step ts_default        = {.enabled = true, .time_step = w_time_step_create(
		0,
		1,
		true,
		WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
	), .name = "DEFAULT"};
	struct w_scheduler_time_step ts_default_fixed  = {.enabled = true, .time_step = w_time_step_create(
		fixed_update_rate,
		1,
		false,
		WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
	), .name = "DEFAULT_FIXED"};
	struct w_scheduler_time_step ts_default_post   = {.enabled = true, .time_step = w_time_step_create(
		0,
		1,
		true,
		WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
	), .name = "DEFAULT_POST"};
	struct w_scheduler_time_step ts_default_render = {.enabled = true, .time_step = w_time_step_create(
		0,
		1,
		true,
		WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
	), .name = "DEFAULT_RENDER"};
	struct w_scheduler_time_step ts_reserved       = {.enabled = true, .time_step = w_time_step_create(
		0,
		1,
		true,
		WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
	), .name= "RESERVED"};
	struct w_scheduler_time_step ts_post           = {.enabled = true, .time_step = w_time_step_create(
		0,
		1,
		true,
		WM_SCHEDULER_DEFAULTS_DELTA_CLAMP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_SNAP_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_AVG_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_ENABLED,
		WM_SCHEDULER_DEFAULTS_DELTA_ACC_CLAMP_ENABLED
	), .name = "POST"};

	size_t ts_pre_id            = w_ecs_register_system_time_step_at(world, &ts_pre, WM_TIMESTEP_PRE);
	size_t ts_default_id        = w_ecs_register_system_time_step_at(world, &ts_default, WM_TIMESTEP_DEFAULT);
	size_t ts_default_fixed_id  = w_ecs_register_system_time_step_at(world, &ts_default_fixed, WM_TIMESTEP_DEFAULT_FIXED);
	size_t ts_default_post_id   = w_ecs_register_system_time_step_at(world, &ts_default_post, WM_TIMESTEP_DEFAULT_POST);
	size_t ts_default_render_id = w_ecs_register_system_time_step_at(world, &ts_default_render, WM_TIMESTEP_DEFAULT_RENDER);
	size_t ts_reserved_id       = w_ecs_register_system_time_step_at(world, &ts_reserved, WM_TIMESTEP_RESERVED);
	size_t ts_post_id           = w_ecs_register_system_time_step_at(world, &ts_post, WM_TIMESTEP_POST);

	// timestep ordering: PRE -> DEFAULT -> FIXED -> POST -> RENDER -> RESERVED -> POST
	w_ecs_set_system_time_step_runs_before(world, ts_pre_id, ts_default_id);
	w_ecs_set_system_time_step_runs_before(world, ts_default_id, ts_default_fixed_id);
	w_ecs_set_system_time_step_runs_before(world, ts_default_fixed_id, ts_default_post_id);
	w_ecs_set_system_time_step_runs_before(world, ts_default_post_id, ts_default_render_id);
	w_ecs_set_system_time_step_runs_before(world, ts_default_render_id, ts_reserved_id);
	w_ecs_set_system_time_step_runs_before(world, ts_reserved_id, ts_post_id);

	// register phases with timestep assignments
	struct w_scheduler_phase p_pre              = {.enabled = true, .time_step_id = ts_pre_id, .name = "PRE"};
	struct w_scheduler_phase p_on_startup       = {.enabled = true, .time_step_id = ts_default_id, .name = "ON_STARTUP"};
	struct w_scheduler_phase p_pre_load         = {.enabled = true, .time_step_id = ts_default_id, .name = "PRE_LOAD"};
	struct w_scheduler_phase p_on_load          = {.enabled = true, .time_step_id = ts_default_id, .name = "ON_LOAD"};
	struct w_scheduler_phase p_post_load        = {.enabled = true, .time_step_id = ts_default_id, .name = "POST_LOAD"};
	struct w_scheduler_phase p_pre_update       = {.enabled = true, .time_step_id = ts_default_id, .name = "PRE_UPDATE"};
	struct w_scheduler_phase p_on_update        = {.enabled = true, .time_step_id = ts_default_id, .name = "ON_UPDATE"};
	struct w_scheduler_phase p_post_update      = {.enabled = true, .time_step_id = ts_default_post_id, .name = "POST_UPDATE"};
	struct w_scheduler_phase p_pre_fixed_update  = {.enabled = true, .time_step_id = ts_default_fixed_id, .name = "PRE_FIXED_UPDATE"};
	struct w_scheduler_phase p_on_fixed_update   = {.enabled = true, .time_step_id = ts_default_fixed_id, .name = "ON_FIXED_UPDATE"};
	struct w_scheduler_phase p_post_fixed_update = {.enabled = true, .time_step_id = ts_default_fixed_id, .name = "POST_FIXED_UPDATE"};
	struct w_scheduler_phase p_final_fixed      = {.enabled = true, .time_step_id = ts_default_fixed_id, .name = "FINAL_FIXED"};
	struct w_scheduler_phase p_post_fixed       = {.enabled = true, .time_step_id = ts_default_fixed_id, .name = "POST_FIXED"};
	struct w_scheduler_phase p_final            = {.enabled = true, .time_step_id = ts_default_post_id, .name = "FINAL"};
	struct w_scheduler_phase p_pre_render       = {.enabled = true, .time_step_id = ts_default_render_id, .name = "PRE_RENDER"};
	struct w_scheduler_phase p_on_render        = {.enabled = true, .time_step_id = ts_default_render_id, .name = "ON_RENDER"};
	struct w_scheduler_phase p_post_render      = {.enabled = true, .time_step_id = ts_default_render_id, .name = "POST_RENDER"};
	struct w_scheduler_phase p_final_render     = {.enabled = true, .time_step_id = ts_default_render_id, .name = "FINAL_RENDER"};
	struct w_scheduler_phase p_reserved         = {.enabled = true, .time_step_id = ts_reserved_id, .name = "RESERVED"};
	struct w_scheduler_phase p_post             = {.enabled = true, .time_step_id = ts_post_id, .name = "POST"};

	// restart and shutdown phases disabled by default (on-demand)
	struct w_scheduler_phase p_on_restart       = {.enabled = false, .time_step_id = ts_default_id, .name = "ON_RESTART"};
	struct w_scheduler_phase p_on_shutdown      = {.enabled = false, .time_step_id = ts_default_id, .name = "ON_SHUTDOWN"};

	size_t id_pre              = w_ecs_register_system_phase_at(world, &p_pre, WM_PHASE_PRE);
	size_t id_on_startup       = w_ecs_register_system_phase_at(world, &p_on_startup, WM_PHASE_ON_STARTUP);
	size_t id_pre_load         = w_ecs_register_system_phase_at(world, &p_pre_load, WM_PHASE_PRE_LOAD);
	size_t id_on_load          = w_ecs_register_system_phase_at(world, &p_on_load, WM_PHASE_ON_LOAD);
	size_t id_post_load        = w_ecs_register_system_phase_at(world, &p_post_load, WM_PHASE_POST_LOAD);
	size_t id_pre_update       = w_ecs_register_system_phase_at(world, &p_pre_update, WM_PHASE_PRE_UPDATE);
	size_t id_on_update        = w_ecs_register_system_phase_at(world, &p_on_update, WM_PHASE_ON_UPDATE);
	size_t id_post_update      = w_ecs_register_system_phase_at(world, &p_post_update, WM_PHASE_POST_UPDATE);
	size_t id_pre_fixed_update  = w_ecs_register_system_phase_at(world, &p_pre_fixed_update, WM_PHASE_PRE_FIXED_UPDATE);
	size_t id_on_fixed_update   = w_ecs_register_system_phase_at(world, &p_on_fixed_update, WM_PHASE_ON_FIXED_UPDATE);
	size_t id_post_fixed_update = w_ecs_register_system_phase_at(world, &p_post_fixed_update, WM_PHASE_POST_FIXED_UPDATE);
	size_t id_final_fixed      = w_ecs_register_system_phase_at(world, &p_final_fixed, WM_PHASE_FINAL_FIXED);
	size_t id_post_fixed       = w_ecs_register_system_phase_at(world, &p_post_fixed, WM_PHASE_POST_FIXED);
	size_t id_final            = w_ecs_register_system_phase_at(world, &p_final, WM_PHASE_FINAL);
	size_t id_pre_render       = w_ecs_register_system_phase_at(world, &p_pre_render, WM_PHASE_PRE_RENDER);
	size_t id_on_render        = w_ecs_register_system_phase_at(world, &p_on_render, WM_PHASE_ON_RENDER);
	size_t id_post_render      = w_ecs_register_system_phase_at(world, &p_post_render, WM_PHASE_POST_RENDER);
	size_t id_final_render     = w_ecs_register_system_phase_at(world, &p_final_render, WM_PHASE_FINAL_RENDER);
	size_t id_reserved         = w_ecs_register_system_phase_at(world, &p_reserved, WM_PHASE_RESERVED);
	size_t id_post             = w_ecs_register_system_phase_at(world, &p_post, WM_PHASE_POST);
	size_t id_on_restart       = w_ecs_register_system_phase_at(world, &p_on_restart, WM_PHASE_ON_RESTART);
	size_t id_on_shutdown       = w_ecs_register_system_phase_at(world, &p_on_shutdown, WM_PHASE_ON_SHUTDOWN);

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

	// restart and shutdown phases
	w_ecs_set_system_phase_runs_after(world, id_on_restart, id_post);
	w_ecs_set_system_phase_runs_after(world, id_on_shutdown, id_post);

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
	w_ecs_register_system(world, "wm_scheduler_defaults_system_disable_startup_phase_", &startup_disable);

	// restart/shutdown phase enabler: enables ON_RESTART or ON_SHUTDOWN phases
	// when world->update_result signals restart or shutdown, then forces scheduler
	// rebuild to append those phases to the current update iteration
	struct w_system restart_shutdown_enable = {
		.phase_id = id_post,
		.enabled = true,
		.update = wm_scheduler_defaults_system_enable_restart_shutdown_phases_,
	};
	w_ecs_register_system(world, "wm_scheduler_defaults_system_enable_restart_shutdown_phases_", &restart_shutdown_enable);
}

void wm_scheduler_defaults_free(struct w_ecs_world *world)
{
	(void)world;
}
