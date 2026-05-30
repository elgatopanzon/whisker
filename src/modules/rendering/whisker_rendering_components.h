/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_rendering_components
 * @created     : Wednesday May 13, 2026 16:29:47 CST
 * @description : components used by the rendering module
 */

#include "whisker.h"
#include "modules/managed_alloc/whisker_managed_alloc.h"

#ifndef WHISKER_RENDERING_COMPONENTS_H
#define WHISKER_RENDERING_COMPONENTS_H

/*************************
*  components and tags  *
*************************/

// handle to shape verts used by procedural shapes
w_ecs_define_managed_component(shape_verts_handle);

// hash of shape components used to invalididate verts
w_ecs_define_component(uint64_t, shape_verts_hash, UINT64_MAX);

// number of verts in the managed vert data
w_ecs_define_component(int, shape_verts_count, 0);


// phase as render layer
w_ecs_define_component(int, render_layer, 0);

// enable billboard on Y or all axies
enum W_RENDERING_BILLBOARD
{
	W_RENDERING_BILLBOARD_Y,
	W_RENDERING_BILLBOARD_ALL,
};
w_ecs_define_component(int, render_billboard, W_RENDERING_BILLBOARD_Y);

// render at screen scale
w_ecs_define_tag(render_scale_screen);


/*********************
*  text components  *
*********************/
enum W_RENDERING_TEXT_ALIGN
{
	W_RENDERING_TEXT_ALIGN_LEFT,
	W_RENDERING_TEXT_ALIGN_CENTER,
	W_RENDERING_TEXT_ALIGN_RIGHT,
};

w_ecs_define_component(int, font_size, 10);
w_ecs_define_component(int, font_spacing, 1);
w_ecs_define_component(float, font_line_height, 0.0f);
w_ecs_define_component(w_color8, font_color, W_COLOR8_BLACK.r, W_COLOR8_BLACK.g, W_COLOR8_BLACK.b, W_COLOR8_BLACK.a);
w_ecs_define_component(w_vec2, font_shadow, 0.0f, 0.0f);
w_ecs_define_component(w_color8, font_shadow_color, 0.0f, 0.0f, 0.0f, 0.0f);
w_ecs_define_component(int, font_alignment, W_RENDERING_TEXT_ALIGN_LEFT);
w_ecs_define_component(float, font_max_width, 0);


/****************************
*  image asset components  *
****************************/
enum W_RENDERING_PIXEL_FORMAT {
	W_RENDERING_PIXEL_FORMAT_UNKNOWN = 0,

	// 8 bit per pixel (no alpha)
	W_RENDERING_PIXEL_FORMAT_UNCOMPRESSED_GRAYSCALE = 1,
	// 8*2 bpp (2 channels)
	W_RENDERING_PIXEL_FORMAT_UNCOMPRESSED_GRAY_ALPHA,
	// 16 bpp
	W_RENDERING_PIXEL_FORMAT_UNCOMPRESSED_R5G6B5,
	// 24 bpp
	W_RENDERING_PIXEL_FORMAT_UNCOMPRESSED_R8G8B8,
	// 16 bpp (1 bit alpha)
	W_RENDERING_PIXEL_FORMAT_UNCOMPRESSED_R5G5B5A1,
	// 16 bpp (4 bit alpha)
	W_RENDERING_PIXEL_FORMAT_UNCOMPRESSED_R4G4B4A4,
	// 32 bpp
	W_RENDERING_PIXEL_FORMAT_UNCOMPRESSED_R8G8B8A8,
	// 32 bpp (1 channel - float)
	W_RENDERING_PIXEL_FORMAT_UNCOMPRESSED_R32,
	// 32*3 bpp (3 channels - float)
	W_RENDERING_PIXEL_FORMAT_UNCOMPRESSED_R32G32B32,
	// 32*4 bpp (4 channels - float)
	W_RENDERING_PIXEL_FORMAT_UNCOMPRESSED_R32G32B32A32,
	// 16 bpp (1 channel - half float)
	W_RENDERING_PIXEL_FORMAT_UNCOMPRESSED_R16,
	// 16*3 bpp (3 channels - half float)
	W_RENDERING_PIXEL_FORMAT_UNCOMPRESSED_R16G16B16,
	// 16*4 bpp (4 channels - half float)
	W_RENDERING_PIXEL_FORMAT_UNCOMPRESSED_R16G16B16A16,

	// 4 bpp (no alpha)
	W_RENDERING_PIXEL_FORMAT_COMPRESSED_DXT1_RGB,
	// 4 bpp (1 bit alpha)
	W_RENDERING_PIXEL_FORMAT_COMPRESSED_DXT1_RGBA,
	// 8 bpp
	W_RENDERING_PIXEL_FORMAT_COMPRESSED_DXT3_RGBA,
	// 8 bpp
	W_RENDERING_PIXEL_FORMAT_COMPRESSED_DXT5_RGBA,
	// 4 bpp
	W_RENDERING_PIXEL_FORMAT_COMPRESSED_ETC1_RGB,
	// 4 bpp
	W_RENDERING_PIXEL_FORMAT_COMPRESSED_ETC2_RGB,
	// 8 bpp
	W_RENDERING_PIXEL_FORMAT_COMPRESSED_ETC2_EAC_RGBA,
	// 4 bpp
	W_RENDERING_PIXEL_FORMAT_COMPRESSED_PVRT_RGB,
	// 4 bpp
	W_RENDERING_PIXEL_FORMAT_COMPRESSED_PVRT_RGBA,
	// 8 bpp
	W_RENDERING_PIXEL_FORMAT_COMPRESSED_ASTC_4X4_RGBA,
	// 2 bpp
	W_RENDERING_PIXEL_FORMAT_COMPRESSED_ASTC_8X8_RGBA
};

// data components
w_ecs_define_component(w_vec2i, image_dimensions, 0, 0);
w_ecs_define_component(int, image_mipmaps, 1);
w_ecs_define_component(int, image_pixel_format, W_RENDERING_PIXEL_FORMAT_UNKNOWN);
w_ecs_define_component(uint64_t, image_data_handle, WM_MANAGED_ALLOC_INVALID_HANDLE);
w_ecs_define_component(uint64_t, image_data_size, 0);

// lifecycle components
w_ecs_define_component(int, image_load_err, -1);
w_ecs_define_tag(req_image_hot);
w_ecs_define_tag(req_image_warm);
w_ecs_define_tag(req_image_cold);

// reference components
w_ecs_define_tag(image_asset);
w_ecs_define_component(w_entity_id, image_asset_entity, W_ENTITY_INVALID);
w_ecs_define_component(w_string_table_id, image_asset_name_string_id, W_STRING_TABLE_INVALID_ID);

/******************************
*  texture asset components  *
******************************/

// data components
w_ecs_define_component(w_vec2i, texture_dimensions, 0, 0);
w_ecs_define_component(int, texture_mipmaps, 1);
w_ecs_define_component(int, texture_pixel_format, W_RENDERING_PIXEL_FORMAT_UNKNOWN);
w_ecs_define_component(uint64_t, backend_texture_handle, 0);

// lifecycle components
w_ecs_define_component(int, texture_upload_err, -1);
w_ecs_define_tag(req_texture_hot);
w_ecs_define_tag(req_texture_warm);
w_ecs_define_tag(req_texture_cold);

// reference components
w_ecs_define_tag(texture_asset);
w_ecs_define_component(w_entity_id, texture_asset_entity, W_ENTITY_INVALID);
w_ecs_define_component(w_string_table_id, texture_asset_name_string_id, W_STRING_TABLE_INVALID_ID);

#endif /* WHISKER_RENDERING_COMPONENTS_H */

