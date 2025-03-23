#ifndef RENDERER_H_INCLUDED
#define RENDERER_H_INCLUDED

#include "typedefs.h"
#include "../core/player.h"

#define MAX_QUEUE 64

typedef struct _wall
{
    vec2i_t a, b;
    uint8_t texture_id;
    bool is_portal;
} wall_t;

typedef struct _sector
{
    uint8_t ceil_texture_id, floor_texture_id;
    uint32_t first_wall_id, num_walls;
    float z_floor, z_ceil;
    vec2f_t center;
} sector_t;

typedef struct _queue_sector
{
    uint32_t arr[MAX_QUEUE];
    size_t size;
    uint32_t front;
} queue_sector_t;

typedef struct _texture
{
    uint32_t width, height;

    union
    {
        uint32_t *buffer;
        uint32_t color;
    } data;
} texture_t;

bool r_init(uint16_t scrn_w, uint16_t scrn_h);

void r_begin_draw(const player_t *player, texture_t *textures);
void r_draw_sectors(sector_t *sectors, wall_t *walls, queue_sector_t *queue);
void r_draw_pixel(int x, int y, uint32_t color);
void r_draw_floor();
void r_end_draw();

texture_t r_create_texture(const char* filename, uint8_t width, uint8_t height);


void r_shutdown();

#endif