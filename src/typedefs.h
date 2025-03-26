#ifndef TYPEDEFS_H_INCLUDED
#define TYPEDEFS_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define PI 3.14159265359f
#define TAU (2.0f * PI)
#define PI_2 (PI / 2.0f)
#define PI_4 (PI / 4.0f)

typedef struct _vec2i
{
    int x, y;
} vec2i_t;

typedef struct _vec2f
{
    float x, y;
} vec2f_t;

typedef struct _vec3f
{
    float x, y, z;
} vec3f_t;

#define v2f_to_v2i(_v) ({ __typeof__(_v) __v = (_v); (vec2i_t) { __v.x, __v.y }; })
#define v2i_to_v2f(_v) ({ __typeof__(_v) __v = (_v); (vec2f_t) { __v.x, __v.y }; })

typedef struct _vertex
{
    int16_t x, y;
} vertex_t;

typedef struct _linedef
{
    int16_t start_vertex, 
            end_vertex, 
            flags, 
            line_type,
            sector_tag,
            front_sidedef_id,
            back_sidedef_id;
} linedef_t;

typedef struct _bbox
{
    int16_t left, right, top, bottom;
} bbox_t;

typedef struct _node
{
    int16_t x_partition, y_partition,
            dx_partition, dy_partition,
            right_child, left_child;
    bbox_t right_bbox, left_bbox;
} node_t;

typedef struct _subsector
{
    int16_t seg_count, first_seg_id;
} subsector_t;

// Os ângulos de doom são dados em um formato diferente
// de graus e radianos que é específico do jogo, logo
// toda e qualquer operação que envolva ângulos precisa ser
// feita com isso em mente
typedef struct _seg
{
    int16_t start_vertex, end_vertex, 
            angle, linedef_id,
            direction, offset;
} seg_t;

typedef struct _entity
{
    int16_t pos_x, pos_y, angle,
            type, flags;
} entity_t;

typedef struct _sector2
{
    int16_t floor_z, ceil_z, light_level, type, tag;
    char floor_texture_name[8];
    char ceil_texture_name[8];
} sector2_t;

typedef struct _sidedef
{
    int16_t x_offset, y_offset, sector_id;
    char upper_texture_name[8];
    char lower_texture_name[8];
    char mid_texture_name[8];
} sidedef_t;
#endif