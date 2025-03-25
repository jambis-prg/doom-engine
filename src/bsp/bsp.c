#include "bsp.h"

static bool is_back_side(node_t *node, uint16_t x, uint16_t y)
{
    uint16_t dx = x - node->x_partition;
    uint16_t dy = y - node->y_partition;
    return (dx * node->dy_partition - dy * node->dx_partition) <= 0;
}

static bool check_box(bbox_t *bbox)
{

}

bsp_node_t bsp_create(uint32_t nodes_len, node_t *nodes, subsector_t *subsectors, seg_t *segs)
{
    bsp_node_t bsp = {0};
    bsp.nodes = nodes;
    bsp.subsectors = subsectors;
    bsp.segs = segs;
    bsp.root_id = nodes_len - 1;
    return bsp;
}

void bsp_delete(bsp_node_t *bsp)
{
    if (bsp->nodes != NULL)
        free(bsp->nodes);

    if (bsp->subsectors != NULL)
        free(bsp->subsectors);

    if (bsp->segs != NULL)
        free(bsp->segs);
}
