/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_system_registry
 * @created     : Wednesday Mar 04, 2026 19:42:43 CST
 * @description : holds system function pointers and their configurations
 */

#include "whisker_std.h"
#include "whisker_array.h"
#include "whisker_memory.h"

#ifndef WHISKER_SYSTEM_REGISTRY_H
#define WHISKER_SYSTEM_REGISTRY_H

struct w_system 
{
	size_t phase_id;
	bool enabled;
	void (*update)(void *ctx, double delta_time);
	uint64_t last_update_ticks;
	uint64_t update_frequency;
};

struct w_system_registry 
{
	// list of system entries
	w_array_declare(struct w_system, systems);
};

// init system registry
void w_system_registry_init(struct w_system_registry *registry);
// free system registry job list and systems
void w_system_registry_free(struct w_system_registry *registry);

// register a system entry, returns system ID
size_t w_system_register_system(struct w_system_registry *registry, struct w_system *system);
// set system entry state
void w_system_set_system_state(struct w_system_registry *registry, size_t system_id, bool state);
// get system entry
struct w_system *w_system_get_system_entry(struct w_system_registry *registry, size_t system_id);

#endif /* WHISKER_SYSTEM_REGISTRY_H */

