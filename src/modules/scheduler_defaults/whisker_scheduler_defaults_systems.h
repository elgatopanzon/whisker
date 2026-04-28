/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_scheduler_defaults_systems
 * @created     : Tuesday Apr 28, 2026 14:29:03 CST
 * @description : 
 */

#include "whisker_scheduler_defaults.h"

#ifndef WHISKER_SCHEDULER_DEFAULTS_SYSTEMS_H
#define WHISKER_SCHEDULER_DEFAULTS_SYSTEMS_H

// disables WM_PHASE_ON_STARTUP after first frame
void wm_scheduler_defaults_system_disable_startup_phase_(void *ctx, double delta_time)
{
	(void)delta_time;
	struct w_ecs_world *world = ctx;
	w_ecs_set_system_phase_state(world, WM_PHASE_ON_STARTUP, false);
}

void wm_scheduler_defaults_system_enable_restart_shutdown_phases_(void *ctx, double delta_time)
{
	(void)delta_time;
	struct w_ecs_world *world = ctx;

	if (world->update_result == W_WORLD_UPDATE_RESULT_RESTART)
	{
		// run all systems registered to ON_RESTART phase directly
		w_ecs_run_phase_systems(world, WM_PHASE_ON_RESTART);
	}
	else if (world->update_result == W_WORLD_UPDATE_RESULT_SHUTDOWN)
	{
		// run all systems registered to ON_SHUTDOWN phase directly
		w_ecs_run_phase_systems(world, WM_PHASE_ON_SHUTDOWN);
	}
}

#endif /* WHISKER_SCHEDULER_DEFAULTS_SYSTEMS_H */

