#ifndef IMAGE_H_INCLUDED
#define IMAGE_H_INCLUDED

#include "typedefs.h"

typedef struct _image
{
    uint16_t width, height;
    int16_t left_offset, top_offset;
    uint32_t *data;
} image_t;

image_t i_create_image(const patch_colum_t *columns, uint32_t columns_size, uint16_t width, uint16_t height, int16_t left_offset, int16_t top_offset);
image_t i_create_texture(const texture_map_t *texture_map, const image_t *patches);
image_t i_create_flat(uint8_t *data);
void i_delete_image(image_t *image);

#endif