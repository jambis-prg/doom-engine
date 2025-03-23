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



#endif