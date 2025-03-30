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
    int16_t top, bottom, left, right;
} bbox_t;

typedef struct _node
{
    int16_t x_partition, y_partition,
            dx_partition, dy_partition;

    bbox_t right_bbox, left_bbox;

    int16_t right_child, left_child;
} node_t;

typedef struct _subsector
{
    int16_t seg_count, first_seg_id;
} subsector_t;

// Os ângulos de doom são dados em BAMs para segmentos
// enquanto para entidades é dado em graus

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

typedef struct _sector
{
    int16_t floor_z, ceil_z;

    char floor_texture_name[8];
    char ceil_texture_name[8];
    
    int16_t light_level, type, tag;
} sector_t;

typedef struct _sidedef
{
    int16_t x_offset, y_offset;
    
    char upper_texture_name[8];
    char lower_texture_name[8];
    char mid_texture_name[8];

    int16_t sector_id;
} sidedef_t;

typedef struct _palette
{
    unsigned char r, g, b;
} palette_t;

typedef struct _patch_colum
{
    uint8_t top_delta, length;
    uint8_t padding_pre; // unused
    uint8_t *data; // size_bytes = length
    uint8_t padding_post; // unused
} patch_colum_t;

typedef struct _patch_header
{
    uint16_t width, height;
    int16_t left_offset, top_ofsset;
    uint32_t *column_offset; // size_bytes = 4 * width
} patch_header_t;

typedef struct _texture_header
{
    uint32_t texture_count;
    uint32_t *texture_data_offset;
} texture_header_t;

typedef struct _patch_map
{
    int16_t x_offset, y_offset;
    uint16_t patch_name_index;
    int16_t step_dir, color_map; // unused
} patch_map_t;

typedef struct _texture_map
{
    char name[8];
    uint32_t flags;
    uint16_t width, height;
    uint32_t column_dir; // unused
    uint16_t patch_count;
    patch_map_t *patch_maps;
} texture_map_t;

#endif