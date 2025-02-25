#ifndef RENDERER_H_INCLUDED
#define RENDERER_H_INCLUDED

#include "typedefs.h"

typedef struct _wall
{
    vec2i_t a, b;
    bool is_portal;
} wall_t;

typedef struct _sector
{
    uint32_t id;
    wall_t *walls;
    uint32_t walls_count;
    float z_floor, z_ceil;
} sector_t;

bool r_init(uint16_t scrn_w, uint16_t scrn_h);

void r_begin_draw();
void r_end_draw();

void r_shutdown();

#endif