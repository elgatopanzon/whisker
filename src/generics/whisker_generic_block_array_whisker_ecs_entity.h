/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generic_block_array
 * @created     : Sunday Mar 02, 2025 13:47:08 CST
 */

#include "../whisker_std.h"
#include "../whisker_array.h"
#include "../whisker_block_array.h"
#include "../whisker_ecs_types_base.h"
#include "whisker_generics.h"
#include "whisker_generic_array.h"
#include "generics/whisker_generic_array_char.h"
#include "generics/whisker_generic_array_unsigned_char.h"
#include "generics/whisker_generic_array_short.h"
#include "generics/whisker_generic_array_unsigned_short.h"
#include "generics/whisker_generic_array_int.h"
#include "generics/whisker_generic_array_unsigned_int.h"
#include "generics/whisker_generic_array_long.h"
#include "generics/whisker_generic_array_unsigned_long.h"
#include "generics/whisker_generic_array_long_long.h"
#include "generics/whisker_generic_array_unsigned_long_long.h"
#include "generics/whisker_generic_array_float.h"
#include "generics/whisker_generic_array_double.h"
#include "generics/whisker_generic_array_long_double.h"
#include "generics/whisker_generic_array_void_.h"
#include "generics/whisker_generic_array_uint8_t.h"
#include "generics/whisker_generic_array_uint16_t.h"
#include "generics/whisker_generic_array_uint32_t.h"
#include "generics/whisker_generic_array_uint64_t.h"
#include "generics/whisker_generic_array_whisker_ecs_entity.h"
#include "generics/whisker_generic_array_whisker_ecs_entity_id.h"
#include "generics/whisker_generic_array_whisker_ecs_entity_index.h"
#include "generics/whisker_generic_array_whisker_ecs_entity_deferred_action.h"

#ifndef WHISKER_GENERIC_BLOCK_ARRAY_whisker_ecs_entity_H
#define WHISKER_GENERIC_BLOCK_ARRAY_whisker_ecs_entity_H

typedef struct whisker_block_arr_whisker_ecs_entity
{
	whisker_arr_void_ *blocks;
	size_t block_size;
} whisker_block_arr_whisker_ecs_entity;

E_WHISKER_BLOCK_ARR whisker_block_arr_create_whisker_ecs_entity(whisker_block_arr_whisker_ecs_entity **arr, size_t block_size);
void whisker_block_arr_free_whisker_ecs_entity(whisker_block_arr_whisker_ecs_entity *barr);
E_WHISKER_BLOCK_ARR whisker_block_arr_set_whisker_ecs_entity(whisker_block_arr_whisker_ecs_entity *barr, size_t index, whisker_ecs_entity value);
E_WHISKER_BLOCK_ARR whisker_block_arr_init_block_whisker_ecs_entity(whisker_block_arr_whisker_ecs_entity *barr, size_t block_id);
whisker_ecs_entity whisker_block_arr_get_whisker_ecs_entity(whisker_block_arr_whisker_ecs_entity *barr, size_t index);
bool whisker_block_arr_contains_whisker_ecs_entity(whisker_block_arr_whisker_ecs_entity *barr, size_t index);

#endif /* WHISKER_GENERIC_BLOCK_ARRAY_H */
