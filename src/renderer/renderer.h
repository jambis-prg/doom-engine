#ifndef RENDERER_H_INCLUDED
#define RENDERER_H_INCLUDED

#include "typedefs.h"
#include "../core/player.h"
#include "assets/asset.h"

#define FOV PI_2
#define H_FOV PI_4

typedef struct _portal_wall_desc
{
    bool draw_upper_wall, draw_lower_wall, draw_ceil, draw_floor;
    int16_t x1, x2, light_level;
    float world_front_z1, world_back_z1, world_front_z2, world_back_z2, rw_normal_angle, rw_distance, upper_tex_alt, lower_tex_alt, rw_offset, rw_center_angle;
    char *upper_wall_texture;
    char *lower_wall_texture;
    char *ceil_texture;
    char *floor_texture;
} portal_wall_desc_t;


typedef struct _solid_wall_desc
{
    bool draw_wall, draw_ceil, draw_floor;
    int16_t x1, x2, light_level;
    float world_front_z1, world_front_z2, rw_normal_angle, rw_distance, middle_texture_alt, rw_offset, rw_center_angle;
    char *wall_texture;
    char *ceil_texture;
    char *floor_texture;
} solid_wall_desc_t;

bool r_init(uint16_t scrn_w, uint16_t scrn_h);

void r_begin_draw(const player_t *player);

void r_draw_pixel(int x, int y, uint32_t color);
void r_draw_vertical_line(int16_t x, int16_t y1, int16_t y2, const char *wall_texture, int16_t light_level, uint32_t color);
void r_draw_wall_col(const char *texture_name, float texture_column, int16_t x, int16_t y1, int16_t y2, float texture_alt, float inv_scale, int16_t light_level);
void r_draw_flat(const char *texture_name, int16_t x, int16_t y1, int16_t y2, float world_z, int16_t light_level);
void r_draw_portal_wall_range(portal_wall_desc_t *portal_wall_desc);
void r_draw_solid_wall_range(solid_wall_desc_t *solid_wall_desc);
void r_draw_sprite(int16_t x, int16_t z, image_t *sprite, float rw_scale, float rw_distance);
void r_end_draw();

int16_t r_angle_to_x(float angle);
float r_scale_from_global_angle(int16_t x, float normal_angle, float distance);


uint16_t r_get_width();
uint16_t r_get_height();

void r_shutdown();

#endif