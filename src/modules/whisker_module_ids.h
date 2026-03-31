/**
 * @author      : ElGatoPanzon
 * @file        : whisker_module_ids
 * @created     : Tuesday Mar 31, 2026 12:32:08 CST
 * @description : Centralized module resource ID registry to prevent conflicts
 */

#ifndef WHISKER_MODULE_IDS_H
#define WHISKER_MODULE_IDS_H

/* Each module gets a base ID (multiples of 100).
 * Resources within a module are base + sequential index (0, 1, 2...).
 * Adding a new module: pick the next free hundred block. */


/****************************
*  module base IDs          *
****************************/

#define WM_MODULE_RESOURCE_COMPONENT_EVENTS_ID 100
#define WM_MODULE_RESOURCE_SYSTEM_GROUPS_ID    200
#define WM_MODULE_RESOURCE_RENDERING_ID        300

/* Expand to (WM_MODULE_RESOURCE_{NAME}_ID + idx) via token pasting */
#define WM_MODULE_RESOURCE_ID(name, idx) (WM_MODULE_RESOURCE_##name##_ID + idx)

#endif /* WHISKER_MODULE_IDS_H */
