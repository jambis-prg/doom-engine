#include "image.h"
#include "asset.h"
#include <string.h>

image_t i_create_image(const patch_colum_t *columns, uint32_t columns_size, uint16_t width, uint16_t height, int16_t left_offset, int16_t top_offset)
{
    image_t image;
    image.width = width;
    image.height = height;
    image.left_offset = left_offset;
    image.top_offset = top_offset;

    image.data = (uint32_t*)malloc(width * height * sizeof(uint32_t));
    
    if (image.data != NULL)
    {
        memset(image.data, 0, width * height * sizeof(uint32_t));
        uint16_t x = 0;
        for (uint32_t i = 0; i < columns_size; i++)
        {
            if (columns[i].top_delta != 0xFF)
            {
                for (uint16_t y = 0; y < columns[i].length; y++)
                    image.data[(y + columns[i].top_delta) * width + x] = a_get_palette_color(columns[i].data[y]);
            }
            else { x++; }
        }
    }

    return image;
}

image_t i_create_texture(const texture_map_t *texture_map, const image_t *patches)
{
    image_t image;
    image.width = texture_map->width;
    image.height = texture_map->height;
    image.left_offset = 0;
    image.top_offset = 0;
    
    image.data = (uint32_t*)malloc(image.width * image.height * sizeof(uint32_t));

    if (image.data != NULL)
    {
        memset(image.data, 0, image.width * image.height * sizeof(uint32_t));
        for (uint32_t i = 0; i < texture_map->patch_count; i++)
        {
            patch_map_t *patch_map = &texture_map->patch_maps[i];
            image_t *patch = &patches[patch_map->patch_name_index];
            
            if (patch->data != NULL)
            {
                uint16_t x_start = patch_map->x_offset < 0 ? 0 : patch_map->x_offset;
                uint16_t x_end = (patch_map->x_offset + patch->width) > image.width ? image.width : (patch_map->x_offset + patch->width);

                uint16_t y_start = patch_map->y_offset < 0 ? 0 : patch_map->y_offset;
                uint16_t y_end = (patch_map->y_offset + patch->height) > image.height ? image.height : (patch_map->y_offset + patch->height);
                for (uint16_t x = x_start; x < x_end; x++)
                    for (uint16_t y = y_start; y < y_end; y++)
                        image.data[y * image.width + x] = patch->data[(y - patch_map->y_offset) * patch->width + (x - patch_map->x_offset)];
            }
        }
    }

    return image;
}

image_t i_create_flat(uint8_t *data)
{
    image_t image;
    image.width = 64;
    image.height = 64;
    image.left_offset = 0;
    image.top_offset = 0;

    image.data = (uint32_t*)malloc(4096 * sizeof(uint32_t));
    if (image.data != NULL)
    {
        for (uint32_t i = 0; i < 4096; i++)
            image.data[i] = a_get_palette_color(data[i]);
    }
    return image;
}

void i_delete_image(image_t *image)
{
    if (image->data)
        free(image->data);
}
