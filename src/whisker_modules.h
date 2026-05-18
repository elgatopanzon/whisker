/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_modules
 * @created     : Sunday May 17, 2026 18:28:40 CST
 * @description : header to include all whisker modules
 */

#include "whisker.h"

#ifndef WHISKER_MODULES_H
#define WHISKER_MODULES_H

// utilities and tools
#include "modules/scheduler_defaults/whisker_scheduler_defaults.h"
#include "modules/buffers/whisker_buffers.h"
#include "modules/utilities/whisker_utilities.h"
#include "modules/managed_alloc/whisker_managed_alloc.h"

// events and reactivity
#include "modules/events/whisker_events.h"
#include "modules/component_events/whisker_component_events.h"
#include "modules/system_groups/whisker_system_group.h"
#include "modules/relationships/whisker_relationships.h"

// files and data
#include "modules/serialisation/whisker_serialisation.h"
#include "modules/resources/whisker_resources.h"

// specialised
#include "modules/assets/whisker_assets.h"
#include "modules/rendering/whisker_rendering.h"

// per-module enable flags
struct w_modules_config {
	bool scheduler_defaults;
	bool buffers;
	bool utilities;
	bool managed_alloc;
	bool events;
	bool component_events;
	bool system_groups;
	bool relationships;
	bool serialisation;
	bool resources;
	bool assets;
	bool rendering;
};

// init payloads: world pointer plus module-specific configs
struct w_modules_payloads {
	// scheduler_defaults
	double scheduler_fixed_update_rate;
	// rendering
	struct w_rendering_display_config *rendering_display_config;
	struct w_rendering_render_config *rendering_render_config;
};

// init all enabled modules (order matches include order)
static inline void w_modules_init_all(struct w_ecs_world *world, struct w_modules_config *config, struct w_modules_payloads *payloads)
{
	struct w_modules_config *cfg = config;
	struct w_modules_config all_enabled;
	if (!cfg) {
		memset(&all_enabled, 1, sizeof(all_enabled));
		cfg = &all_enabled;
	}

	if (cfg->scheduler_defaults)
		wm_scheduler_defaults_init(world, payloads->scheduler_fixed_update_rate);
	if (cfg->buffers)
		w_buffers_init(world);
	if (cfg->utilities)
		wm_utils_init(world);
	if (cfg->managed_alloc)
		wm_managed_alloc_init(world);
	if (cfg->events)
		wm_events_init(world);
	if (cfg->component_events)
		wm_component_events_init(world);
	if (cfg->system_groups)
		wm_system_group_init(world);
	if (cfg->relationships)
		wm_relationships_init(world);
	if (cfg->serialisation)
		wm_serialisation_init(world);
	if (cfg->resources)
		wm_resources_init(world);
	if (cfg->assets)
		wm_assets_init(world);
	if (cfg->rendering)
		wm_rendering_init(world, payloads->rendering_display_config,
			payloads->rendering_render_config);
}

// free all enabled modules (reverse init order)
static inline void w_modules_free_all(struct w_ecs_world *world, struct w_modules_config *config)
{
	struct w_modules_config *cfg = config;
	struct w_modules_config all_enabled;
	if (!cfg) {
		memset(&all_enabled, 1, sizeof(all_enabled));
		cfg = &all_enabled;
	}

	if (cfg->rendering)
		wm_rendering_free(world);
	if (cfg->assets)
		wm_assets_free(world);
	if (cfg->resources)
		wm_resources_free(world);
	if (cfg->serialisation)
		wm_serialisation_free(world);
	if (cfg->relationships)
		wm_relationships_free(world);
	if (cfg->system_groups)
		wm_system_group_free(world);
	if (cfg->component_events)
		wm_component_events_free(world);
	if (cfg->events)
		wm_events_free(world);
	if (cfg->managed_alloc)
		wm_managed_alloc_free(world);
	if (cfg->utilities)
		wm_utils_free(world);
	if (cfg->buffers)
		w_buffers_free(world);
	if (cfg->scheduler_defaults)
		wm_scheduler_defaults_free(world);
}

#endif /* WHISKER_MODULES_H */

