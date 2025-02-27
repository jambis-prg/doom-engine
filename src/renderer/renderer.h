#ifndef RENDERER_H_INCLUDED
#define RENDERER_H_INCLUDED

#include "typedefs.h"

#define MAX_QUEUE 64

typedef struct _wall
{
    vec2i_t a, b;
    bool is_portal;
} wall_t;

typedef struct _sector
{
    uint32_t id;
    uint32_t first_wall_id, num_walls;
    float z_floor, z_ceil;
} sector_t;

typedef struct _queue_sector
{
    uint32_t arr[MAX_QUEUE];
    size_t n;
    uint32_t head, tail;
} queue_sector_t;

bool r_init(uint16_t scrn_w, uint16_t scrn_h);

void r_begin_draw();
void r_draw_sectors(sector_t *sectors, wall_t *walls, queue_sector_t *queue);
void r_end_draw();

void r_shutdown();

#endif