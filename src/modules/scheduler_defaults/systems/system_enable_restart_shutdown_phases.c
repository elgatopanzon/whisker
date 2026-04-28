/**
 * @author      : ElGatoPanzon
 * @file        : system_enable_restart_shutdown_phases
 * @created     : Sunday Apr 27, 2026 16:30:00 CST
 * @description : System that runs restart/shutdown phase systems directly when triggered
 */

#include "whisker_ecs_world.h"
#include "system_enable_restart_shutdown_phases.h"
#include "../whisker_scheduler_defaults.h"

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
