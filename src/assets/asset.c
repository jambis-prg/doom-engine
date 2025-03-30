#include "asset.h"
#include <string.h>
#include "logger.h"
#include <ctype.h>
#include "core/timer.h"

// Entities Types
#define SHOTGUN_ENTITY_TYPE 2001

#define ARMOR_TYPE 2018
#define MEGAARMOR_TYPE 2019
#define HEALT_BONUS_TYPE 2014
#define ARMOR_BONUS_TYPE 2015

#define AMMO_CLIP_TYPE 2007
#define BOX_OF_AMMO_TYPE 2048
#define SHELLS_TYPE 2008
#define BOX_OF_SHELLS_TYPE 2049
#define EXPLODING_BARREL_TYPE 2035

// Entities Indexes
#define SHOTGUN_ENTITY_IDX 424

#define ARMOR_IDX 433
#define MEGAARMOR_IDX 435
#define HEALTH_BONUS_IDX 467
#define ARMOR_BONUS_IDX 475

#define EXPLODING_BAR1_IDX 437


typedef struct _asset
{
    wad_reader_t *wdr;
    palette_t *palette;
    int16_t palette_size;
    uint32_t sprites_count, textures_count, flats_count;
    image_t *sprites, *textures, *flats;
    file_hash_t textures_hash, flats_hash;
} asset_t;

static asset_t asset_manager;

void a_init(wad_reader_t *wdr)
{
    asset_manager.wdr = wdr;

    uint32_t index;
    if (fh_get_value(&wdr->file_hash, "PLAYPAL", &index))
    {
        uint32_t size = 0;
        asset_manager.palette = (palette_t*)wdr_get_lump_data(wdr, index, 0, &size);
        asset_manager.palette_size = size / sizeof(palette_t);
    }

    asset_manager.sprites = a_load_sprites("S_START", "S_END", &asset_manager.sprites_count);
    asset_manager.textures = a_load_textures("PNAMES", "TEXTURE1", &asset_manager.textures_count);
    asset_manager.flats = a_load_flat_textures("F1_START", "F1_END", &asset_manager.flats_count);

    free(asset_manager.palette);
}

image_t *a_load_sprites(const char *start_lump_name, const char *end_lump_name, uint32_t *count)
{
    image_t *sprites = NULL;
    uint32_t index_1, index_2;
    *count = 0;
    if (fh_get_value(&asset_manager.wdr->file_hash, start_lump_name, &index_1) && fh_get_value(&asset_manager.wdr->file_hash, end_lump_name, &index_2))
    {
        *count = (index_2 - index_1 - 1);
        sprites = (image_t*) malloc(*count * sizeof(image_t));
        if (sprites != NULL)
        {
            for (uint32_t i = index_1 + 1, j = 0; i < index_2; i++, j++)
            {
                uint32_t size = 0;
                uint16_t width, height;
                int16_t left_offset, top_offset;
                patch_colum_t *patch_columns = a_load_patch_columns(
                    asset_manager.wdr->directories[i].name, &size, 
                    &width, &height, 
                    &left_offset, &top_offset
                );
                
                if (patch_columns != NULL)
                {
                    sprites[j] = i_create_image(
                        patch_columns, size, 
                        width, height, 
                        left_offset, top_offset
                    );
                    wdr_delete_patch_columns(patch_columns, size);
                }
                else
                    DOOM_LOG_DEBUG("Nao foi possivel carregar a imagem %s", asset_manager.wdr->directories[i].name);
            }
        }
    }

    return sprites;
}

image_t *a_load_textures(const char *patch_lump_name, const char *texture_lump_name, uint32_t *count)
{
    image_t *textures = NULL;

    uint32_t pacths_count = 0;
    image_t *patchs = a_load_patchs(patch_lump_name, &pacths_count);

    if (patchs != NULL)
    {
        uint32_t texture_map_count = 0;
        texture_map_t *texture_maps = a_load_texture_maps("TEXTURE1", &texture_map_count);

        if (texture_maps != NULL)
        {
            *count = texture_map_count;
            textures = (image_t*)malloc(texture_map_count * sizeof(image_t));
            if (textures != NULL)
            {
                asset_manager.textures_hash = fh_create_hash(2 * texture_map_count);

                if (asset_manager.textures_hash.buckets != NULL)
                {
                    for (uint32_t i = 0; i < texture_map_count; i++)
                    {
                        textures[i] = i_create_texture(&texture_maps[i], patchs);
                        fh_insert_hash(&asset_manager.textures_hash, texture_maps[i].name, i);
                    }
                }
                else
                {
                    free(textures);
                    textures = NULL;
                }
            }
            wdr_delete_texture_map(texture_maps, texture_map_count);
        }
        
        for (uint32_t i = 0; i < pacths_count; i++)
            i_delete_image(&patchs[i]);
        free(patchs);
    }

    return textures;
}

image_t *a_load_flat_textures(const char *start_lump_name, const char *end_lump_name, uint32_t *count)
{
    image_t *flat = NULL;
    uint32_t index_1, index_2;
    *count = 0;
    if (fh_get_value(&asset_manager.wdr->file_hash, start_lump_name, &index_1) && fh_get_value(&asset_manager.wdr->file_hash, end_lump_name, &index_2))
    {
        *count = (index_2 - index_1 - 1);
        flat = (image_t*) malloc(*count * sizeof(image_t));
        if (flat != NULL)
        {
            asset_manager.flats_hash = fh_create_hash(*count * 2);
            if (asset_manager.flats_hash.buckets == NULL)
            {
                free(flat);
                return NULL;
            }

            for (uint32_t i = index_1 + 1, j = 0; i < index_2; i++, j++)
            {
                uint8_t *data = wdr_get_lump_data(asset_manager.wdr, i, 0, NULL);
                if (data == NULL)
                {
                    for (uint32_t k = 0; k < j; k++)
                        free(flat[k].data);
                    free(flat);

                    fh_delete_hash(&asset_manager.flats_hash);
                    return NULL;
                }
                
                flat[j] = i_create_flat(data);
                free(data);
                
                if (flat[j].data == NULL)
                {
                    for (uint32_t k = 0; k < j; k++)
                        free(flat[k].data);
                    free(flat);

                    fh_delete_hash(&asset_manager.flats_hash);
                    return NULL;
                }

                fh_insert_hash(&asset_manager.flats_hash, asset_manager.wdr->directories[i].name, j);
            }
        }
    }

    return flat;
}

image_t *a_load_patchs(const char *patch_lump_name, uint32_t *count)
{
    uint32_t patch_index;
    image_t *patchs = NULL;
    if (fh_get_value(&asset_manager.wdr->file_hash, patch_lump_name, &patch_index))
    {
        uint32_t patch_size = 0;
        char *patch_names = (char *)wdr_get_lump_data(asset_manager.wdr, patch_index, 4, &patch_size);
        *count = patch_size / 8;
        patchs = (image_t *)malloc(*count * sizeof(image_t));

        if (patchs != NULL)
        {
            for (uint32_t i = 0, j = 0; i < patch_size; i += 8, j++)
            {
                char name[8];
                memset(name, 0, sizeof(name));
                strncpy(name, patch_names + i, 8);
                for (uint8_t i = 0; i < 8; i++)
                    name[i] = toupper(name[i]);
                uint32_t size = 0;
                uint16_t width, height;
                int16_t left_offset, top_offset;
                patch_colum_t *patch_columns = a_load_patch_columns(name, &size, &width, &height, &left_offset, &top_offset);
    
                if (patch_columns != NULL)
                {
                    patchs[j] = i_create_image(
                        patch_columns, size, 
                        width, height, 
                        left_offset, top_offset
                    );
                    wdr_delete_patch_columns(patch_columns, size);
                }
                else
                    DOOM_LOG_DEBUG("%d - Nao foi possivel carregar o patch %s", j, name);
            }
        }
    }

    return patchs;
}

texture_map_t *a_load_texture_maps(const char *texture_lump_name, uint32_t *count)
{
    uint32_t texture_map_index = 0;
    texture_map_t *texture_map = NULL;
    if (fh_get_value(&asset_manager.wdr->file_hash, texture_lump_name, &texture_map_index))
    {
        texture_header_t header;
        wdr_get_lump_header(asset_manager.wdr, &header, texture_map_index, sizeof(header.texture_count));
        *count = header.texture_count;
        header.texture_data_offset = wdr_get_lump_data(asset_manager.wdr, texture_map_index, sizeof(header.texture_count), NULL);

        if (header.texture_data_offset != NULL)
        {
            texture_map = wdr_get_texture_map(asset_manager.wdr, &header, texture_map_index);
            free(header.texture_data_offset);
        }
    }

    return texture_map;
}

patch_colum_t *a_load_patch_columns(const char *patch_name, uint32_t *size_out, uint16_t *width_out, uint16_t *height_out, int16_t *left_offset_out, int16_t *top_offset_out)
{
    uint32_t index;
    if (fh_get_value(&asset_manager.wdr->file_hash, patch_name, &index))
    {
        patch_header_t header;
        uint32_t header_len = sizeof(header) - sizeof(header.column_offset);
        wdr_get_lump_header(asset_manager.wdr, &header, index, header_len);
        *left_offset_out = header.left_offset;
        *top_offset_out = header.top_ofsset;
        *width_out = header.width;
        *height_out = header.height;
        header.column_offset = wdr_get_lump_data(asset_manager.wdr, index, header_len, NULL);
        
        if (header.column_offset != NULL)
        {
            uint32_t size = 0;
            patch_colum_t *patch_columns = wdr_get_patch_columns(asset_manager.wdr, &header, index, &size);
            
            free(header.column_offset);
            *size_out = size;
            return patch_columns;
        }
    }

    return NULL;
}

image_t *a_get_sprite(uint32_t index)
{
    return &asset_manager.sprites[index];
}

image_t *a_get_sprite_by_type(int16_t type)
{
    uint64_t animation_ticks = t_get_animation_tick();
    switch (type)
    {
    case SHOTGUN_ENTITY_TYPE:
        return &asset_manager.sprites[SHOTGUN_ENTITY_IDX];
    case EXPLODING_BARREL_TYPE:
        return &asset_manager.sprites[EXPLODING_BAR1_IDX];
    case ARMOR_TYPE:
        return &asset_manager.sprites[ARMOR_IDX + (animation_ticks % 2)];
    case HEALT_BONUS_TYPE:
    {
        uint32_t offset = (animation_ticks % 6);
        if (offset >= 4) 
            offset = 2 - (offset - 4);
        return &asset_manager.sprites[HEALTH_BONUS_IDX + offset];
    }
    case ARMOR_BONUS_TYPE:
    {
        uint32_t offset = (animation_ticks % 6);
        if (offset >= 4) 
            offset = 2 - (offset - 4);
        return &asset_manager.sprites[ARMOR_BONUS_IDX + offset];
    }
    case MEGAARMOR_TYPE:
        return &asset_manager.sprites[MEGAARMOR_IDX + (animation_ticks % 2)];
    default:
        return NULL;
    }

    return NULL;
}

image_t *a_get_texture_by_name(const char *name)
{
    uint32_t index = 0;
    if (fh_get_value(&asset_manager.textures_hash, name, &index))
        return &asset_manager.textures[index];
    
    return NULL;
}

image_t *a_get_flat_by_name(const char *name)
{
    uint32_t index = 0;
    if (fh_get_value(&asset_manager.flats_hash, name, &index))
        return &asset_manager.flats[index];
    
    return NULL;
}

uint32_t a_get_palette_color(uint16_t color_index)
{
    palette_t *palette = &asset_manager.palette[color_index];
    return (0xFF << 24) | (palette->b << 16) | (palette->g << 8) | palette->r;
}

void a_shutdown()
{
    fh_delete_hash(&asset_manager.textures_hash);
    fh_delete_hash(&asset_manager.flats_hash);

    for (uint32_t i = 0; i < asset_manager.textures_count; i++)
        i_delete_image(&asset_manager.textures[i]);
    free(asset_manager.textures);

    for (uint32_t i = 0; i < asset_manager.flats_count; i++)
        i_delete_image(&asset_manager.flats[i]);
    free(asset_manager.flats);

    for (uint32_t i = 0; i < asset_manager.sprites_count; i++)
        i_delete_image(&asset_manager.sprites[i]);
    free(asset_manager.sprites);
}
