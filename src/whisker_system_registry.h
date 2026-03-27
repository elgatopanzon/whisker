/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_system_registry
 * @created     : Wednesday Mar 04, 2026 19:42:43 CST
 * @description : holds system function pointers and their configurations
 */

#include "whisker_std.h"
#include "whisker_array.h"
#include "whisker_memory.h"
#include "whisker_hashmap.h"

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

w_hashmap_t_declare(char*, size_t, w_system_name_map);
struct w_system_registry
{
	// list of system entries
	w_array_declare(struct w_system, systems);

	// system names to ID map
	struct w_system_name_map system_names;

	// arena for hashmap allocation
	struct w_arena *arena;
};

// init system registry
void w_system_registry_init(struct w_system_registry *registry, struct w_arena *arena);
// free system registry job list and systems
void w_system_registry_free(struct w_system_registry *registry);

// register a system entry with name, returns system ID
size_t w_system_register_system(struct w_system_registry *registry, char *name, struct w_system *system);
// set system entry state
void w_system_set_system_state(struct w_system_registry *registry, size_t system_id, bool state);
// get system entry
struct w_system *w_system_get_system_entry(struct w_system_registry *registry, size_t system_id);

// get system ID by name (returns pointer to size_t, or NULL if not found)
#define w_system_get_id_by_name(registry, name) \
	({ \
		size_t *_out; \
		w_hashmap_t_get(&(registry)->system_names, (name), _out); \
		_out; \
	})

// get system entry by name
#define w_system_get_system_entry_str(registry, name) \
	({ \
		size_t *_id = w_system_get_id_by_name(registry, name); \
		_id ? w_system_get_system_entry(registry, *_id) : NULL; \
	})

// set system state by name
#define w_system_set_system_state_str(registry, name, state) \
	do { \
		size_t *_id = w_system_get_id_by_name(registry, name); \
		if (_id) w_system_set_system_state(registry, *_id, state); \
	} while (0)

#endif /* WHISKER_SYSTEM_REGISTRY_H */

