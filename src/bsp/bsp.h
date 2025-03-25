#ifndef BSP_H_INCLUDED
#define BSP_H_INCLUDED

#include "typedefs.h"

typedef struct _bsp_node
{
    int16_t root_id;
    node_t *nodes;
    subsector_t *subsectors;
    seg_t *segs;
} bsp_node_t;

bsp_node_t bsp_create(uint32_t nodes_len, node_t *nodes, subsector_t *subsectors, seg_t *segs);
void bsp_delete(bsp_node_t *bsp);

#endif