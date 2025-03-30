#ifndef ASSET_H_INCLUDED
#define ASSET_H_INCLUDED

#include "typedefs.h"
#include "wad/wad_reader.h"
#include "image.h"


void a_init(wad_reader_t *wdr);

image_t *a_load_sprites(const char *start_lump_name, const char *end_lump_name, uint32_t *count);
image_t *a_load_textures(const char *patch_lump_name, const char *texture_lump_name, uint32_t *count);
image_t *a_load_flat_textures(const char *start_lump_name, const char *end_lump_name, uint32_t *count);
image_t *a_load_patchs(const char *patch_lump_name, uint32_t *count);
texture_map_t *a_load_texture_maps(const char *texture_lump_name, uint32_t *count);
patch_colum_t *a_load_patch_columns(const char *patch_name, uint32_t *size_out, uint16_t *width_out, uint16_t *height_out, int16_t *left_offset_out, int16_t *top_offset_out);

image_t *a_get_sprite(uint32_t index);
image_t *a_get_sprite_by_type(int16_t type);
image_t *a_get_texture_by_name(const char *name);
image_t *a_get_flat_by_name(const char *name);

uint32_t a_get_palette_color(uint16_t color_index);


void a_shutdown();

#endif