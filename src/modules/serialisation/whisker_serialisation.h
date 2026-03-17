/**
 * @author      : ElGatoPanzon
 * @file        : whisker_serialisation
 * @created     : Sunday Mar 15, 2026 21:24:35 CST
 * @description : Component serialisation hooks for world save/load and migrations
 */

#ifndef WHISKER_SERIALISATION_H
#define WHISKER_SERIALISATION_H

#include "whisker.h"

#define WM_SERIALISATION_REGISTRY_NAME "wm_serialisation_registry"
#define WM_SERIALISATION_NO_SERIALISE_TAG_NAME "wm_serialisation_exclude_"

#define WM_SERIALISATION_PARAMS_BUFFER_SIZE 1024
#define WM_SERIALISATION_BUFFER_BLOCK_SIZE 16384 

enum WM_SERIALISATION_HOOK_REGISTRY
{
	WM_SERIALISATION_HOOK_REGISTRY_COMPONENT_SERIALISE,
	WM_SERIALISATION_HOOK_REGISTRY_COMPONENT_DESERIALISE,
	WM_SERIALISATION_HOOK_REGISTRY_LIFECYCLE,

	WM_SERIALISATION_HOOK_REGISTRY_COUNT,
};


// lifecycle hooks are executed at different stages of serialisation
enum WM_SERIALISATION_LIFECYCLE_HOOK
{
	WM_SERIALISATION_LIFECYCLE_HOOK_PRE_SAVE,
	WM_SERIALISATION_LIFECYCLE_HOOK_POST_SAVE,
	WM_SERIALISATION_LIFECYCLE_HOOK_PRE_LOAD,
	WM_SERIALISATION_LIFECYCLE_HOOK_POST_LOAD,
};


// serialisation registry is the struct we create and store in the world's
// singleton registry
struct wm_serialisation_registry 
{
	struct w_hook_registry hooks[WM_SERIALISATION_HOOK_REGISTRY_COUNT];
};

// serialisation context
struct wm_serialisation_ctx 
{
	// the serialised state string
	w_array_declare(char, buffer);

	// array of persistent entities and component IDs
	w_array_declare(w_entity_id, entities);
	w_array_declare(w_entity_id, components);

	// stats
	uint32_t entities_saved;
	uint32_t components_saved;

	// result
	int err;
	char *err_message;
};

// deserialisation context
struct wm_deserialisation_ctx 
{
	// current buffer and cursor position
	char *buffer;
	char *buffer_cursor;
	size_t buffer_length;

	// restored stats
	uint32_t version;
	uint32_t entities_loaded;
	uint32_t components_loaded;

	// holds unparsed lines
	w_array_declare(char *, unparsed);

	int err;
	char *err_message;
	int err_line;
};

// context passed to the component serialisation hooks
struct wm_serialisation_component_ctx 
{
	// main context for current serialisation
	struct wm_serialisation_ctx *ctx;

	// entity-specific info
	w_entity_id entity;
	const char *entity_name;

	// component info
	w_entity_id component_entity_id;
	const char *component_name;
	uint component_type_id;
	const char *component_type_name;
	struct w_component_entry *component_entry;

	// params buffer
	char hook_params_buffer[WM_SERIALISATION_PARAMS_BUFFER_SIZE];
};

// context passed to the component deserialisation hooks
struct wm_deserialisation_component_ctx 
{
	// main context for current deserialisation
	struct wm_deserialisation_ctx *ctx;

	// entity-specific info
	w_entity_id entity;
	const char *entity_name;

	// component info
	w_entity_id component_entity_id;
	const char *component_name;
	uint component_type_id;
	const char *component_type_name;
	struct w_component_entry *component_entry;

	// params buffer
	char hook_params_buffer[WM_SERIALISATION_PARAMS_BUFFER_SIZE];
	char *value_buffer;
};

/****************
*  module api  *
****************/

// initialise serialisation module with the world
void wm_serialisation_init(struct w_ecs_world *world);

// cleanup serialisation module
void wm_serialisation_free(struct w_ecs_world *world);

// get the registry instance from the world
#define wm_serialisation_get_registry(w) w_ecs_singleton_get(world, WM_SERIALISATION_REGISTRY_NAME);

// register hooks for component serialise + deserialise
// hook fn params: world, serialisation/deserialisation context
w_pack32x2 w_serialisation_register_component_hooks(struct w_ecs_world *world, uint type_id, w_hook_fn serialise_hook, w_hook_fn deserialise_hook);

// register a serialisation lifecycle hook
uint32_t w_serialisation_register_lifecycle_hook(struct w_ecs_world *world, enum WM_SERIALISATION_LIFECYCLE_HOOK hook_type, w_hook_fn hook_fn);


/*******************
* serialisation API *
*******************/

// serialise world data to buffer
bool w_serialisation_dump_to_buffer(struct w_ecs_world *world, struct wm_serialisation_ctx *ctx);

// get current version number to write to dump
uint32_t w_serialisation_get_version(struct wm_serialisation_registry *registry, struct wm_serialisation_ctx *ctx);

// helper: allocate buffer of persistent entities for serialisation
w_entity_id *w_serialisation_get_persistent_entity_list(struct w_ecs_world *world, _Atomic size_t *entities_length, _Atomic size_t *entities_size);

// helper: allocate buffer of component IDs for serialisation
w_entity_id *w_serialisation_get_components_list(struct w_ecs_world *world, _Atomic size_t *components_length, _Atomic size_t *components_size);

// helpers: push lines to serialistion ctx buffer
void w_serialisation_push_ctx_command_(struct wm_serialisation_ctx *ctx, const char *line);
void w_serialisation_push_ctx_command_f_(struct wm_serialisation_ctx *ctx, const char *fmt, ...);


/*************************
*  deserialisation API  *
*************************/

// restore world data from buffer
bool w_serialisation_restore_from_buffer(struct w_ecs_world *world, char *buffer, size_t buf_len, struct wm_deserialisation_ctx *ctx);

// helper: get type size from type_id
size_t w_serialisation_type_size_(uint type_id);

// helpers: parse individual command types from a line
bool w_deserialisation_parse_metadata_(struct wm_deserialisation_ctx *ctx, const char *line, uint32_t *expected_entities, uint32_t *expected_components);
bool w_deserialisation_parse_entity_(struct w_ecs_world *world, struct wm_deserialisation_ctx *ctx, char *line, int line_num);
bool w_deserialisation_parse_set_(struct w_ecs_world *world, struct wm_serialisation_registry *registry, struct wm_deserialisation_ctx *ctx, char *line, int line_num);


/*****************************************
*  serialisation/deserialisation hooks  *
*****************************************/

#define WM_SERIALISE_DEFINE_SERIALIZE_HOOK(type, enum_name, serialise_block, deserialise_block) \
    static inline void wm_serialise_hook_##type##_(void *world_, void *comp_ctx_) { \
        struct w_ecs_world *world = world_; \
    	(void)world; \
        struct wm_serialisation_component_ctx *comp_ctx = comp_ctx_; \
        type *value = w_component_get_entry(comp_ctx->component_entry, comp_ctx->entity, type); \
        { \
        	serialise_block; \
        } \
    } \
	static inline void wm_deserialise_hook_##type##_(void *world_, void *comp_ctx_) { \
    	struct w_ecs_world *world = world_; \
    	(void)world; \
    	struct wm_deserialisation_component_ctx *comp_ctx = comp_ctx_; \
    	type value; \
    	char *params = comp_ctx->hook_params_buffer; \
		{ \
			deserialise_block; \
		} \
		memcpy(comp_ctx->value_buffer, &value, sizeof(value)); \
	} \
	static inline void wm_serialise_register_hooks_##type##_(struct w_ecs_world *world) { \
		w_serialisation_register_component_hooks(world, enum_name##_##type, wm_serialise_hook_##type##_, wm_deserialise_hook_##type##_); \
	} \

#define WM_SERIALISE_SIMPLE(fmt, ...) snprintf(comp_ctx->hook_params_buffer, WM_SERIALISATION_PARAMS_BUFFER_SIZE, fmt, __VA_ARGS__)

// bool
WM_SERIALISE_DEFINE_SERIALIZE_HOOK(bool, W_COMPONENT_TYPE, {
    WM_SERIALISE_SIMPLE("%s", ((*value) ? "true" : "false"));
}, {
	value = (strcmp(params, "true") == 0);
});

#endif /* WHISKER_SERIALISATION_H */
