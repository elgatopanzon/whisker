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
	WM_SERIALISATION_LIFECYCLE_HOOK_MIGRATION, // version-aware, skip first N where N = saved version
	WM_SERIALISATION_LIFECYCLE_HOOK_POST_LOAD, // always run all, no version skipping
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

// type = both the C type and the identifier for function names and enum lookup
#define WM_SERIALISE_DEFINE_SERIALISE_HOOK(type, serialise_block, deserialise_block) \
    static inline void wm_serialise_hook_##type##_(void *world_, void *comp_ctx_) { \
        struct w_ecs_world *world = world_; \
    	(void)world; \
        struct wm_serialisation_component_ctx *comp_ctx = comp_ctx_; \
        type *value = (type *)w_component_get_entry(comp_ctx->component_entry, comp_ctx->entity, type); \
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
		w_serialisation_register_component_hooks(world, W_COMPONENT_TYPE_##type, wm_serialise_hook_##type##_, wm_deserialise_hook_##type##_); \
	}

#define WM_SERIALISE_SIMPLE(fmt, ...) snprintf(comp_ctx->hook_params_buffer, WM_SERIALISATION_PARAMS_BUFFER_SIZE, fmt, __VA_ARGS__)


/*****************************************
*  built-in hooks: primitives            *
*****************************************/

// id 10 = bool
WM_SERIALISE_DEFINE_SERIALISE_HOOK(bool, {
    WM_SERIALISE_SIMPLE("%s", ((*value) ? "true" : "false"));
}, {
	value = (strcmp(params, "true") == 0);
});

// id 11 = char (serialised as integer to handle non-printable chars)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(char, {
    WM_SERIALISE_SIMPLE("%d", (int)*value);
}, {
	int tmp = 0;
	sscanf(params, "%d", &tmp);
	value = (char)tmp;
});

// id 0 = int8_t
WM_SERIALISE_DEFINE_SERIALISE_HOOK(int8_t, {
    WM_SERIALISE_SIMPLE("%d", (int)*value);
}, {
	int tmp = 0;
	sscanf(params, "%d", &tmp);
	value = (int8_t)tmp;
});

// id 1 = int16_t
WM_SERIALISE_DEFINE_SERIALISE_HOOK(int16_t, {
    WM_SERIALISE_SIMPLE("%d", (int)*value);
}, {
	int tmp = 0;
	sscanf(params, "%d", &tmp);
	value = (int16_t)tmp;
});

// id 2 = int32_t
WM_SERIALISE_DEFINE_SERIALISE_HOOK(int32_t, {
    WM_SERIALISE_SIMPLE("%d", (int)*value);
}, {
	sscanf(params, "%d", &value);
});

// id 3 = int64_t
WM_SERIALISE_DEFINE_SERIALISE_HOOK(int64_t, {
    WM_SERIALISE_SIMPLE("%" PRId64, *value);
}, {
	sscanf(params, "%" SCNd64, &value);
});

// id 4 = uint8_t
WM_SERIALISE_DEFINE_SERIALISE_HOOK(uint8_t, {
    WM_SERIALISE_SIMPLE("%u", (unsigned)*value);
}, {
	unsigned tmp = 0;
	sscanf(params, "%u", &tmp);
	value = (uint8_t)tmp;
});

// id 5 = uint16_t
WM_SERIALISE_DEFINE_SERIALISE_HOOK(uint16_t, {
    WM_SERIALISE_SIMPLE("%u", (unsigned)*value);
}, {
	unsigned tmp = 0;
	sscanf(params, "%u", &tmp);
	value = (uint16_t)tmp;
});

// id 6 = uint32_t
WM_SERIALISE_DEFINE_SERIALISE_HOOK(uint32_t, {
    WM_SERIALISE_SIMPLE("%u", *value);
}, {
	sscanf(params, "%u", &value);
});

// id 7 = uint64_t
WM_SERIALISE_DEFINE_SERIALISE_HOOK(uint64_t, {
    WM_SERIALISE_SIMPLE("%" PRIu64, *value);
}, {
	sscanf(params, "%" SCNu64, &value);
});

// id 8 = float
WM_SERIALISE_DEFINE_SERIALISE_HOOK(float, {
    WM_SERIALISE_SIMPLE("%.9g", (double)*value);
}, {
	sscanf(params, "%f", &value);
});

// id 9 = double
WM_SERIALISE_DEFINE_SERIALISE_HOOK(double, {
    WM_SERIALISE_SIMPLE("%.17g", *value);
}, {
	sscanf(params, "%lf", &value);
});


/*****************************************
*  built-in hooks: vectors               *
*****************************************/

// id 20 = w_vec2 (float x, y)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_vec2, {
    WM_SERIALISE_SIMPLE("%.9g %.9g", (double)value->x, (double)value->y);
}, {
	sscanf(params, "%f %f", &value.x, &value.y);
});

// id 21 = w_vec2i (int x, y)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_vec2i, {
    WM_SERIALISE_SIMPLE("%d %d", value->x, value->y);
}, {
	sscanf(params, "%d %d", &value.x, &value.y);
});

// id 22 = w_vec2u (uint x, y)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_vec2u, {
    WM_SERIALISE_SIMPLE("%u %u", value->x, value->y);
}, {
	sscanf(params, "%u %u", &value.x, &value.y);
});

// id 23 = w_vec3 (float x, y, z)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_vec3, {
    WM_SERIALISE_SIMPLE("%.9g %.9g %.9g", (double)value->x, (double)value->y, (double)value->z);
}, {
	sscanf(params, "%f %f %f", &value.x, &value.y, &value.z);
});

// id 24 = w_vec3i (int x, y, z)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_vec3i, {
    WM_SERIALISE_SIMPLE("%d %d %d", value->x, value->y, value->z);
}, {
	sscanf(params, "%d %d %d", &value.x, &value.y, &value.z);
});

// id 25 = w_vec3u (uint x, y, z)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_vec3u, {
    WM_SERIALISE_SIMPLE("%u %u %u", value->x, value->y, value->z);
}, {
	sscanf(params, "%u %u %u", &value.x, &value.y, &value.z);
});

// id 26 = w_vec4 (float x, y, z, w)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_vec4, {
    WM_SERIALISE_SIMPLE("%.9g %.9g %.9g %.9g", (double)value->x, (double)value->y, (double)value->z, (double)value->w);
}, {
	sscanf(params, "%f %f %f %f", &value.x, &value.y, &value.z, &value.w);
});

// id 27 = w_vec4i (int x, y, z, w)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_vec4i, {
    WM_SERIALISE_SIMPLE("%d %d %d %d", value->x, value->y, value->z, value->w);
}, {
	sscanf(params, "%d %d %d %d", &value.x, &value.y, &value.z, &value.w);
});

// id 28 = w_vec4u (uint x, y, z, w)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_vec4u, {
    WM_SERIALISE_SIMPLE("%u %u %u %u", value->x, value->y, value->z, value->w);
}, {
	sscanf(params, "%u %u %u %u", &value.x, &value.y, &value.z, &value.w);
});


/*****************************************
*  built-in hooks: matrices              *
*****************************************/

// id 29 = w_mat2 (float m[4])
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_mat2, {
    WM_SERIALISE_SIMPLE("%.9g %.9g %.9g %.9g",
        (double)value->m[0], (double)value->m[1],
        (double)value->m[2], (double)value->m[3]);
}, {
	sscanf(params, "%f %f %f %f",
        &value.m[0], &value.m[1], &value.m[2], &value.m[3]);
});

// id 30 = w_mat3 (float m[9])
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_mat3, {
    WM_SERIALISE_SIMPLE("%.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g",
        (double)value->m[0], (double)value->m[1], (double)value->m[2],
        (double)value->m[3], (double)value->m[4], (double)value->m[5],
        (double)value->m[6], (double)value->m[7], (double)value->m[8]);
}, {
	sscanf(params, "%f %f %f %f %f %f %f %f %f",
        &value.m[0], &value.m[1], &value.m[2],
        &value.m[3], &value.m[4], &value.m[5],
        &value.m[6], &value.m[7], &value.m[8]);
});

// id 31 = w_mat4 (float m[16])
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_mat4, {
    WM_SERIALISE_SIMPLE(
        "%.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g "
        "%.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g",
        (double)value->m[0],  (double)value->m[1],  (double)value->m[2],  (double)value->m[3],
        (double)value->m[4],  (double)value->m[5],  (double)value->m[6],  (double)value->m[7],
        (double)value->m[8],  (double)value->m[9],  (double)value->m[10], (double)value->m[11],
        (double)value->m[12], (double)value->m[13], (double)value->m[14], (double)value->m[15]);
}, {
	sscanf(params,
        "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
        &value.m[0],  &value.m[1],  &value.m[2],  &value.m[3],
        &value.m[4],  &value.m[5],  &value.m[6],  &value.m[7],
        &value.m[8],  &value.m[9],  &value.m[10], &value.m[11],
        &value.m[12], &value.m[13], &value.m[14], &value.m[15]);
});


/*****************************************
*  built-in hooks: color types           *
*****************************************/

// id 32 = w_color (float r, g, b, a)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_color, {
    WM_SERIALISE_SIMPLE("%.9g %.9g %.9g %.9g",
        (double)value->r, (double)value->g, (double)value->b, (double)value->a);
}, {
	sscanf(params, "%f %f %f %f", &value.r, &value.g, &value.b, &value.a);
});

// id 33 = w_color8 (uint8_t r, g, b, a)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_color8, {
    WM_SERIALISE_SIMPLE("%u %u %u %u",
        (unsigned)value->r, (unsigned)value->g, (unsigned)value->b, (unsigned)value->a);
}, {
	unsigned r = 0; unsigned g = 0; unsigned b = 0; unsigned a = 0;
	sscanf(params, "%u %u %u %u", &r, &g, &b, &a);
	value.r = (uint8_t)r; value.g = (uint8_t)g; value.b = (uint8_t)b; value.a = (uint8_t)a;
});


/*****************************************
*  built-in hooks: geometry types        *
*****************************************/

// id 34 = w_rect (float x, y, width, height)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_rect, {
    WM_SERIALISE_SIMPLE("%.9g %.9g %.9g %.9g",
        (double)value->x, (double)value->y, (double)value->width, (double)value->height);
}, {
	sscanf(params, "%f %f %f %f", &value.x, &value.y, &value.width, &value.height);
});

// id 35 = w_recti (uint x, y, width, height)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_recti, {
    WM_SERIALISE_SIMPLE("%u %u %u %u", value->x, value->y, value->width, value->height);
}, {
	sscanf(params, "%u %u %u %u", &value.x, &value.y, &value.width, &value.height);
});

// id 36 = w_aabb2 (w_vec2 min, max)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_aabb2, {
    WM_SERIALISE_SIMPLE("%.9g %.9g %.9g %.9g",
        (double)value->min.x, (double)value->min.y,
        (double)value->max.x, (double)value->max.y);
}, {
	sscanf(params, "%f %f %f %f",
        &value.min.x, &value.min.y, &value.max.x, &value.max.y);
});

// id 37 = w_aabb3 (w_vec3 min, max)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_aabb3, {
    WM_SERIALISE_SIMPLE("%.9g %.9g %.9g %.9g %.9g %.9g",
        (double)value->min.x, (double)value->min.y, (double)value->min.z,
        (double)value->max.x, (double)value->max.y, (double)value->max.z);
}, {
	sscanf(params, "%f %f %f %f %f %f",
        &value.min.x, &value.min.y, &value.min.z,
        &value.max.x, &value.max.y, &value.max.z);
});

// id 38 = w_ray2 (w_vec2 origin, direction_norm)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_ray2, {
    WM_SERIALISE_SIMPLE("%.9g %.9g %.9g %.9g",
        (double)value->origin.x, (double)value->origin.y,
        (double)value->direction_norm.x, (double)value->direction_norm.y);
}, {
	sscanf(params, "%f %f %f %f",
        &value.origin.x, &value.origin.y,
        &value.direction_norm.x, &value.direction_norm.y);
});

// id 39 = w_ray3 (w_vec3 origin, direction_norm)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_ray3, {
    WM_SERIALISE_SIMPLE("%.9g %.9g %.9g %.9g %.9g %.9g",
        (double)value->origin.x, (double)value->origin.y, (double)value->origin.z,
        (double)value->direction_norm.x, (double)value->direction_norm.y, (double)value->direction_norm.z);
}, {
	sscanf(params, "%f %f %f %f %f %f",
        &value.origin.x, &value.origin.y, &value.origin.z,
        &value.direction_norm.x, &value.direction_norm.y, &value.direction_norm.z);
});


/*****************************************
*  built-in hooks: ECS types             *
*****************************************/

// id 40 = w_entity_id (uint32_t)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_entity_id, {
    WM_SERIALISE_SIMPLE("%u", *value);
}, {
	sscanf(params, "%u", &value);
});


/*****************************************
*  built-in hooks: pack unions           *
*****************************************/

// id 41 = w_pack16x2 (packed uint32_t)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_pack16x2, {
    WM_SERIALISE_SIMPLE("%u", value->packed);
}, {
	sscanf(params, "%u", &value.packed);
});

// id 42 = w_pack16x4 (packed uint64_t)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_pack16x4, {
    WM_SERIALISE_SIMPLE("%" PRIu64, value->packed);
}, {
	sscanf(params, "%" SCNu64, &value.packed);
});

// id 43 = w_pack8x4 (packed uint32_t)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_pack8x4, {
    WM_SERIALISE_SIMPLE("%u", value->packed);
}, {
	sscanf(params, "%u", &value.packed);
});

// id 44 = w_pack8x8 (packed uint64_t)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_pack8x8, {
    WM_SERIALISE_SIMPLE("%" PRIu64, value->packed);
}, {
	sscanf(params, "%" SCNu64, &value.packed);
});

// id 45 = w_pack32x2 (packed uint64_t)
WM_SERIALISE_DEFINE_SERIALISE_HOOK(w_pack32x2, {
    WM_SERIALISE_SIMPLE("%" PRIu64, value->packed);
}, {
	sscanf(params, "%" SCNu64, &value.packed);
});

#endif /* WHISKER_SERIALISATION_H */
