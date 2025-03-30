#ifndef BSP_H_INCLUDED
#define BSP_H_INCLUDED

#include "typedefs.h"
#include "linked_list.h"
#include "wad/wad_reader.h"

typedef struct _bsp
{
    int16_t root_id, entities_count;
    node_t *nodes;
    subsector_t *subsectors;
    sector_t *sectors;
    seg_t *segs;
    vertex_t *vertexes;
    linedef_t *linedefs;
    sidedef_t *sidedefs;
    entity_t *entities;
    bool running_traverse;
    linked_list_t screen_range;
} bsp_t;

bsp_t bsp_create(wad_reader_t *wdr, const char* level_name);
int16_t bsp_get_sub_sector_height(bsp_t *bsp);
vec3f_t bsp_get_player_spawn(bsp_t *bsp);
void bsp_update(bsp_t *bsp, vec3f_t pos, float angle);
void bsp_render(bsp_t *bsp);
void bsp_render_sprites(bsp_t *bsp);
void bsp_delete(bsp_t *bsp);

#endif