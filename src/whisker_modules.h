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
#include "modules/rendering/whisker_rendering.h"

#endif /* WHISKER_MODULES_H */

