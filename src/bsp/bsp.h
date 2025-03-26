#ifndef BSP_H_INCLUDED
#define BSP_H_INCLUDED

#include "typedefs.h"

typedef struct _bsp
{
    int16_t root_id;
    node_t *nodes;
    subsector_t *subsectors;
    seg_t *segs;
    vertex_t *vertexes;
} bsp_t;

bsp_t bsp_create(uint32_t nodes_len, node_t *nodes, subsector_t *subsectors, seg_t *segs, vertex_t *vertexes);
void bsp_delete(bsp_t *bsp);

#endif