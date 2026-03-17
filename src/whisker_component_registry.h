/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_component_registry
 * @created     : Monday Mar 02, 2026 21:39:30 CST
 * @description : ID based component storage registry
 */

#include "whisker_std.h"
#include "whisker_array.h"
#include "whisker_arena.h"
#include "whisker_memory.h"
#include "whisker_sparse_bitset.h"
#include "whisker_entity_registry.h"
#include "whisker_types.h"

#ifndef WHISKER_COMPONENT_REGISTRY_H
#define WHISKER_COMPONENT_REGISTRY_H

#ifndef W_COMPONENT_REGISTRY_ENTRY_REALLOC_BLOCK_SIZE
#define W_COMPONENT_REGISTRY_ENTRY_REALLOC_BLOCK_SIZE 64
#endif /* ifndef W_COMPONENT_REGISTRY_ENTRY_REALLOC_BLOCK_SIZE */

#ifndef W_COMPONENT_REGISTRY_ENTRY_BITSET_PAGE_SIZE
#define W_COMPONENT_REGISTRY_ENTRY_BITSET_PAGE_SIZE 64
#endif /* ifndef W_COMPONENT_REGISTRY_ENTRY_BITSET_PAGE_SIZE */

#ifndef W_COMPONENT_REGISTRY_DATA_BITSET_PAGE_SIZE
#define W_COMPONENT_REGISTRY_DATA_BITSET_PAGE_SIZE 256
#endif /* ifndef W_COMPONENT_REGISTRY_DATA_BITSET_PAGE_SIZE */

#ifndef W_COMPONENT_REGISTRY_DATA_REALLOC_BLOCK_SIZE_BASE
#define W_COMPONENT_REGISTRY_DATA_REALLOC_BLOCK_SIZE_BASE 64
#endif /* ifndef W_COMPONENT_REGISTRY_DATA_REALLOC_BLOCK_SIZE_BASE */

// these component types are supported by the component registry
enum W_COMPONENT_TYPE { 
	// basic primitives
	W_COMPONENT_TYPE_int8_t = 0,

	W_COMPONENT_TYPE_int16_t = 1,
	W_COMPONENT_TYPE_short = 1,

	W_COMPONENT_TYPE_int32_t = 2,
	W_COMPONENT_TYPE_int = 2,

	W_COMPONENT_TYPE_int64_t = 3,
	W_COMPONENT_TYPE_long = 3,

	W_COMPONENT_TYPE_uint8_t = 4,

	W_COMPONENT_TYPE_uint16_t = 5,
	W_COMPONENT_TYPE_ushort = 5,

	W_COMPONENT_TYPE_uint32_t = 6,
	W_COMPONENT_TYPE_uint = 6,

	W_COMPONENT_TYPE_uint64_t = 7,
	W_COMPONENT_TYPE_ulong = 7,

	W_COMPONENT_TYPE_float = 8,
	W_COMPONENT_TYPE_double = 9,

	W_COMPONENT_TYPE_bool = 10,
	W_COMPONENT_TYPE_char = 11,
	W_COMPONENT_TYPE_void = 12,

	// specials
	W_COMPONENT_TYPE_char_ptr = 18,
	W_COMPONENT_TYPE_void_ptr = 19,

	// engine types
	W_COMPONENT_TYPE_vec2 = 20,
	W_COMPONENT_TYPE_vec2i = 21,
	W_COMPONENT_TYPE_vec2u = 22,
	W_COMPONENT_TYPE_vec3 = 23,
	W_COMPONENT_TYPE_vec3i = 24,
	W_COMPONENT_TYPE_vec3u = 25,
	W_COMPONENT_TYPE_vec4 = 26,
	W_COMPONENT_TYPE_vec4i = 27,
	W_COMPONENT_TYPE_vec4u = 28,

	W_COMPONENT_TYPE_mat2 = 29,
	W_COMPONENT_TYPE_mat3 = 30,
	W_COMPONENT_TYPE_mat4 = 31,

	W_COMPONENT_TYPE_color = 32,
	W_COMPONENT_TYPE_color8 = 33,

	W_COMPONENT_TYPE_rect = 34,
	W_COMPONENT_TYPE_recti = 35,
	W_COMPONENT_TYPE_aabb2 = 36,
	W_COMPONENT_TYPE_aabb3 = 37,
	W_COMPONENT_TYPE_ray2 = 38,
	W_COMPONENT_TYPE_ray3 = 39,

	// ECS types
	W_COMPONENT_TYPE_w_entity_id = 40,

	// pack unions
	W_COMPONENT_TYPE_w_pack16x2 = 41,
	W_COMPONENT_TYPE_w_pack16x4 = 42,
	W_COMPONENT_TYPE_w_pack8x4 = 43,
	W_COMPONENT_TYPE_w_pack8x8 = 44,
	W_COMPONENT_TYPE_w_pack32x2 = 45,

	W_COMPONENT_TYPE_COUNT = 46,
};

// static array of canonical type names keyed by enum ID
static const char *w_component_type_names[W_COMPONENT_TYPE_COUNT] = {
	[W_COMPONENT_TYPE_int8_t]      = "int8_t",
	[W_COMPONENT_TYPE_int16_t]     = "int16_t",
	[W_COMPONENT_TYPE_int32_t]     = "int32_t",
	[W_COMPONENT_TYPE_int64_t]     = "int64_t",
	[W_COMPONENT_TYPE_uint8_t]     = "uint8_t",
	[W_COMPONENT_TYPE_uint16_t]    = "uint16_t",
	[W_COMPONENT_TYPE_uint32_t]    = "uint32_t",
	[W_COMPONENT_TYPE_uint64_t]    = "uint64_t",
	[W_COMPONENT_TYPE_float]       = "float",
	[W_COMPONENT_TYPE_double]      = "double",
	[W_COMPONENT_TYPE_bool]        = "bool",
	[W_COMPONENT_TYPE_char]        = "char",
	[W_COMPONENT_TYPE_void]        = "void",
	[W_COMPONENT_TYPE_char_ptr]    = "char_ptr",
	[W_COMPONENT_TYPE_void_ptr]    = "void_ptr",
	[W_COMPONENT_TYPE_vec2]        = "vec2",
	[W_COMPONENT_TYPE_vec2i]       = "vec2i",
	[W_COMPONENT_TYPE_vec2u]       = "vec2u",
	[W_COMPONENT_TYPE_vec3]        = "vec3",
	[W_COMPONENT_TYPE_vec3i]       = "vec3i",
	[W_COMPONENT_TYPE_vec3u]       = "vec3u",
	[W_COMPONENT_TYPE_vec4]        = "vec4",
	[W_COMPONENT_TYPE_vec4i]       = "vec4i",
	[W_COMPONENT_TYPE_vec4u]       = "vec4u",
	[W_COMPONENT_TYPE_mat2]        = "mat2",
	[W_COMPONENT_TYPE_mat3]        = "mat3",
	[W_COMPONENT_TYPE_mat4]        = "mat4",
	[W_COMPONENT_TYPE_color]       = "color",
	[W_COMPONENT_TYPE_color8]      = "color8",
	[W_COMPONENT_TYPE_rect]        = "rect",
	[W_COMPONENT_TYPE_recti]       = "recti",
	[W_COMPONENT_TYPE_aabb2]       = "aabb2",
	[W_COMPONENT_TYPE_aabb3]       = "aabb3",
	[W_COMPONENT_TYPE_ray2]        = "ray2",
	[W_COMPONENT_TYPE_ray3]        = "ray3",
	[W_COMPONENT_TYPE_w_entity_id] = "w_entity_id",
	[W_COMPONENT_TYPE_w_pack16x2]  = "w_pack16x2",
	[W_COMPONENT_TYPE_w_pack16x4]  = "w_pack16x4",
	[W_COMPONENT_TYPE_w_pack8x4]   = "w_pack8x4",
	[W_COMPONENT_TYPE_w_pack8x8]   = "w_pack8x8",
	[W_COMPONENT_TYPE_w_pack32x2]  = "w_pack32x2",
};

static const size_t w_component_type_sizes[W_COMPONENT_TYPE_COUNT] = {
	[W_COMPONENT_TYPE_int8_t]      = sizeof(int8_t),
	[W_COMPONENT_TYPE_int16_t]     = sizeof(int16_t),
	[W_COMPONENT_TYPE_int32_t]     = sizeof(int32_t),
	[W_COMPONENT_TYPE_int64_t]     = sizeof(int64_t),
	[W_COMPONENT_TYPE_uint8_t]     = sizeof(uint8_t),
	[W_COMPONENT_TYPE_uint16_t]    = sizeof(uint16_t),
	[W_COMPONENT_TYPE_uint32_t]    = sizeof(uint32_t),
	[W_COMPONENT_TYPE_uint64_t]    = sizeof(uint64_t),
	[W_COMPONENT_TYPE_float]       = sizeof(float),
	[W_COMPONENT_TYPE_double]      = sizeof(double),
	[W_COMPONENT_TYPE_bool]        = sizeof(bool),
	[W_COMPONENT_TYPE_char]        = sizeof(char),
	[W_COMPONENT_TYPE_void]        = 0,
	[W_COMPONENT_TYPE_char_ptr]    = sizeof(char *),
	[W_COMPONENT_TYPE_void_ptr]    = sizeof(void *),
	[W_COMPONENT_TYPE_vec2]        = sizeof(struct vec2),
	[W_COMPONENT_TYPE_vec2i]       = sizeof(struct vec2i),
	[W_COMPONENT_TYPE_vec2u]       = sizeof(struct vec2u),
	[W_COMPONENT_TYPE_vec3]        = sizeof(struct vec3),
	[W_COMPONENT_TYPE_vec3i]       = sizeof(struct vec3i),
	[W_COMPONENT_TYPE_vec3u]       = sizeof(struct vec3u),
	[W_COMPONENT_TYPE_vec4]        = sizeof(struct vec4),
	[W_COMPONENT_TYPE_vec4i]       = sizeof(struct vec4i),
	[W_COMPONENT_TYPE_vec4u]       = sizeof(struct vec4u),
	[W_COMPONENT_TYPE_mat2]        = sizeof(struct mat2),
	[W_COMPONENT_TYPE_mat3]        = sizeof(struct mat3),
	[W_COMPONENT_TYPE_mat4]        = sizeof(struct mat4),
	[W_COMPONENT_TYPE_color]       = sizeof(struct color),
	[W_COMPONENT_TYPE_color8]      = sizeof(struct color8),
	[W_COMPONENT_TYPE_rect]        = sizeof(struct rect),
	[W_COMPONENT_TYPE_recti]       = sizeof(struct recti),
	[W_COMPONENT_TYPE_aabb2]       = sizeof(struct aabb2),
	[W_COMPONENT_TYPE_aabb3]       = sizeof(struct aabb3),
	[W_COMPONENT_TYPE_ray2]        = sizeof(struct ray2),
	[W_COMPONENT_TYPE_ray3]        = sizeof(struct ray3),
	[W_COMPONENT_TYPE_w_entity_id] = sizeof(w_entity_id),
	[W_COMPONENT_TYPE_w_pack16x2]  = sizeof(w_pack16x2),
	[W_COMPONENT_TYPE_w_pack16x4]  = sizeof(w_pack16x4),
	[W_COMPONENT_TYPE_w_pack8x4]   = sizeof(w_pack8x4),
	[W_COMPONENT_TYPE_w_pack8x8]   = sizeof(w_pack8x8),
	[W_COMPONENT_TYPE_w_pack32x2]  = sizeof(w_pack32x2),
};

// get string name for a component type enum ID, NULL if invalid
#define W_COMPONENT_TYPE_NAME(type_id) \
	(((type_id) >= W_COMPONENT_TYPE_COUNT) ? NULL : w_component_type_names[(type_id)])

#define W_COMPONENT_TYPE_SIZE(type_id) \
	(((type_id) >= W_COMPONENT_TYPE_COUNT) ? 0 : w_component_type_sizes[(type_id)])

// find enum ID from string name, UINT32_MAX if not found
#define W_COMPONENT_TYPE_FROM_NAME(name) ({ \
	uint32_t _result = UINT32_MAX; \
	for (uint32_t _i = 0; _i < W_COMPONENT_TYPE_COUNT; _i++) { \
		if (w_component_type_names[_i] && strcmp(w_component_type_names[_i], (name)) == 0) { \
			_result = _i; \
			break; \
		} \
	} \
	_result; \
})

// a component storage entry
struct w_component_entry 
{
	// component data pointer
	w_array_declare(unsigned char, data);

	// bitset holds which components are set
	struct w_sparse_bitset data_bitset;

	// enum type id
	uint type_id;

	// type data size
	uint64_t type_size;
};

// main component registry struct
struct w_component_registry 
{
	// entities registry for component IDs and names
	struct w_entity_registry *entities;

	// arena, passed to bitsets on init
	struct w_arena *arena;

	// component data storage entries + bitset of active components
	w_array_declare(struct w_component_entry, entries);
	struct w_sparse_bitset entries_bitset;
};

// use the macros for set/get/remove/has
#define w_component_set_ex(r, t, tt, te, e, d) w_component_set_(r, tt##_##t, te, e, (t*)d, sizeof(t));
#define w_component_set_str_ex(r, t, tt, n, e, d) w_component_set_ex(r, t, tt, w_component_get_id(r, n), e, d);
#define w_component_set(r, t, te, e, d) w_component_set_ex(r, t, W_COMPONENT_TYPE, te, e, d);
#define w_component_set_str(r, t, n, e, d) w_component_set(r, t, w_component_get_id(r, n), e, d);

#define w_component_get(r, t, te, e) (t *)w_component_get_(r, te, e);
#define w_component_get_str(r, t, n, e) w_component_get(r, t, w_component_get_id(r, n), e);

#define w_component_remove(r, te, e) w_component_remove_(r, te, e);
#define w_component_remove_str(r, n, e) w_component_remove(r, w_component_get_id(r, n), e);

#define w_component_has(r, te, e) w_component_has_(r, te, e)
#define w_component_has_str(r, n, e) w_component_has(r, w_component_get_id(r, n), e)

// unsafe variants (skip bounds checks, caller must ensure validity)
#define w_component_set_unsafe_ex(r, t, tt, te, e, d) w_component_set_unsafe_(r, tt##_##t, te, e, (t *)d, sizeof(t));
#define w_component_set_unsafe(r, t, te, e, d) w_component_set_unsafe_ex(r, t, W_COMPONENT_TYPE, te, e, d);

#define w_component_get_unsafe(r, t, te, e) (t *)w_component_get_unsafe_(r, te, e);

#define w_component_has_unsafe(r, te, e) w_component_has_unsafe_(r, te, e)

// entry-based unsafe macros (caller provides pre-fetched entry pointer, maximum speed)
#define w_component_set_entry(ent, eid, src, type) (((type *)(ent)->data)[eid] = *(src))
#define w_component_get_entry(ent, eid, type) ((type *)((ent)->data + ((eid) * (ent)->type_size)))
#define w_component_has_entry(ent, eid)                                       \
    ({                                                                        \
        uint64_t word_index = w_sparse_bitset_word_index((eid));              \
        uint64_t page_index = w_sparse_bitset_page_index(word_index, (ent)->data_bitset.page_size_); \
        struct w_sparse_bitset_page *page = &(ent)->data_bitset.pages[page_index]; \
        uint32_t local_word = w_sparse_bitset_local_word(word_index, (ent)->data_bitset.page_size_); \
        (page->bits[local_word] & w_sparse_bitset_bit_mask((eid))); \
    })
#define w_component_remove_entry(ent, eid) do { \
	uint64_t _word_index = w_sparse_bitset_word_index(eid); \
	uint64_t _page_index = w_sparse_bitset_page_index(_word_index, (ent)->data_bitset.page_size_); \
	struct w_sparse_bitset_page *_page = &((ent)->data_bitset.pages[_page_index]); \
	uint32_t _local_word = w_sparse_bitset_local_word(_word_index, (ent)->data_bitset.page_size_); \
	_page->bits[_local_word] &= w_sparse_bitset_bit_clear_mask(eid); \
	bool _page_empty = true; \
	for (uint32_t _i = _page->first_set; _i <= _page->last_set; _i++) \
		if (_page->bits[_i]) { _page_empty = false; break; } \
	if (_page_empty) { \
		uint64_t _page_lookup_index = w_sparse_bitset_page_index(_page_index, W_SPARSE_BITSET_WORD_BITS); \
		(ent)->data_bitset.lookup_pages[_page_lookup_index] &= w_sparse_bitset_bit_clear_mask(_page_index); \
		_page->first_set = UINT32_MAX; \
		_page->last_set = 0; \
	} \
} while(0)

// init a component registry
void w_component_registry_init(struct w_component_registry *registry, struct w_arena *arena, struct w_entity_registry *entities);

// free component data and bitsets
void w_component_registry_free(struct w_component_registry *registry);

// get component entry
struct w_component_entry *w_component_registry_get_entry(struct w_component_registry *registry, w_entity_id entity_type_id);
// check if registry has entry
bool w_component_registry_has_entry(struct w_component_registry *registry, w_entity_id entity_type_id);


// set a component using type entity ID (not thread-safe!)
void *w_component_set_(struct w_component_registry *registry, uint type_id, w_entity_id type_entity_id, w_entity_id entity_id, void *data, size_t data_size);
// get a component using type entity ID
void *w_component_get_(struct w_component_registry *registry, w_entity_id type_entity_id, w_entity_id entity_id);
// remove a component using type entity ID
void w_component_remove_(struct w_component_registry *registry, w_entity_id type_entity_id, w_entity_id entity_id);
// check if entity has component set
bool w_component_has_(struct w_component_registry *registry, w_entity_id type_entity_id, w_entity_id entity_id);

// unsafe variants (skip bounds checks, caller must ensure entry exists and entity is valid)
void *w_component_set_unsafe_(struct w_component_registry *registry, uint type_id, w_entity_id type_entity_id, w_entity_id entity_id, void *data, size_t data_size);
void *w_component_get_unsafe_(struct w_component_registry *registry, w_entity_id type_entity_id, w_entity_id entity_id);
bool w_component_has_unsafe_(struct w_component_registry *registry, w_entity_id type_entity_id, w_entity_id entity_id);

// get component entity ID from name (not thread-safe)
w_entity_id w_component_get_id(struct w_component_registry *registry, char *name);
// get component name from type entity ID
char *w_component_get_name(struct w_component_registry *registry, w_entity_id type_entity_id);

#endif /* WHISKER_COMPONENT_REGISTRY_H */

