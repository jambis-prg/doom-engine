#include "bsp.h"

#define SUB_SECTOR_IDENTIFIER 0x8000

#define FRAC_UNIT 65536 

#define FOV PI/2.f
#define H_FOV FOV/2

// TODO: Fazer com que as operações com ângulos fiquem normalizados
// Entre os valores de 0 a 2pi

//
// bsp_check_box
// Checks BSP node/subtree bounding box.
// Returns true
//  if some part of the bbox might be visible.
//
static uint8_t check_coord[12][4] =
{
    {3,0,2,1},
    {3,0,2,0},
    {3,1,2,0},
    {0},
    {2,0,2,1},
    {0,0,0,0},
    {3,1,3,0},
    {0},
    {2,0,3,1},
    {2,1,3,1},
    {2,1,3,0}
};

static bool bsp_point_on_side(node_t *node, int16_t x, int16_t y)
{
    if (!node->dx_partition)
    {
	    if (x <= node->x_partition)
	        return node->dy_partition > 0;
	
	    return node->dy_partition < 0;
    }

    if (!node->dy_partition)
    {
        if (y <= node->y_partition)
            return node->dx_partition < 0;
        
        return node->dx_partition > 0;
    }
	
    int16_t dx = (x - node->x_partition);
    int16_t dy = (y - node->y_partition);
	
    // Try to quickly decide by looking at sign bits.
    if ( (node->dy_partition ^ node->dx_partition ^ dx ^ dy)&0x8000 )
        return ((node->dy_partition ^ dx) & 0x8000) != 0;

    float left = ((float)(node->dy_partition) / FRAC_UNIT) * ((float)dx / FRAC_UNIT);
    float right = ((float)dy / FRAC_UNIT) * ((float)(node->dx_partition) / FRAC_UNIT);
	
    // front side = 0
    // back side = 1
    return (right >= left);	
}

static bool bsp_check_box(bbox_t *bbox, int16_t x, int16_t y, float angle)
{
    int16_t			box_x;
    int16_t			box_y;
    // Find the corners of the box
    // that define the edges from current viewpoint.
    if (x <= bbox->left)
	    box_x = 0;
    else if (x < bbox->right)
	    box_x = 1;
    else
	    box_x = 2;
		
    if (y >= bbox->top)
	    box_y = 0;
    else if (y > bbox->bottom)
	    box_y = 1;
    else
	    box_y = 2;
		
    int16_t box_pos = (box_y << 2) + box_x;
    if (box_pos == 5) return true;
	
    int16_t x1 = *(int16_t*)(bbox + check_coord[box_pos][0]);
    int16_t y1 = *(int16_t*)(bbox + check_coord[box_pos][1]);
    int16_t x2 = *(int16_t*)(bbox + check_coord[box_pos][2]);
    int16_t y2 = *(int16_t*)(bbox + check_coord[box_pos][3]);
    
    // check clip list for an open space
    int16_t dx1 = x1 - x;
    int16_t dy1 = y1 - y;
    int16_t dx2 = x2 - x;
    int16_t dy2 = y2 - y;
    float angle1 = atan2(dy1, dx1) - angle;
    float angle2 = atan2(dy2, dx2) - angle;
	
    float span = angle1 - angle2;

    // Sitting on a line?
    if (span >= PI) return true;
    
    float tspan1 = angle1 + H_FOV;
    float tspan2 = H_FOV - angle2;
    return (tspan1 <= FOV || tspan1 < span + FOV) && (tspan2 <= FOV || tspan2 < span + FOV); 
}

static void add_segment_to_fov(vertex_t a, vertex_t b, int16_t x, int16_t y, float angle)
{
    float angle1 = atan2(a.y - y, a.x - x);
    float angle2 = atan2(b.y - y, b.x - x);

    float span = angle1 - angle2;
    if (span >= PI) return false;

    angle1 -= angle;
    angle2 -= angle;

    float tspan1 = angle1 + H_FOV;
    float tspan2 = H_FOV - angle2;

    return (tspan1 <= FOV || tspan1 < span + FOV) && (tspan2 <= FOV || tspan2 < span + FOV); 
}

static void bsp_traverse(const bsp_t *bsp, int16_t node_id, uint16_t x, uint16_t y, float angle)
{
    if (node_id & SUB_SECTOR_IDENTIFIER)
    {
        node_id = (node_id == -1) ? 0 : node_id & (~SUB_SECTOR_IDENTIFIER);

        subsector_t *sub = &bsp->subsectors[node_id];

        for (uint16_t i = 0; i < sub->seg_count; i++)
        {
            seg_t *seg = &bsp->segs[sub->first_seg_id + i];
            if (bsp_add_segment_to_fov(bsp->vertexes[seg->start_vertex], bsp->vertexes[seg->end_vertex], x, y, angle))
            {
                // Draw subsector
            }
        }
    }

    node_t *node = &bsp->nodes[node_id];

    if (bsp_point_on_side(node, x, y))
    {
        // render (back)
        if (bsp_check_box(&node->right_bbox, x, y, angle))
            // render (front)
    }
    else
    {
        // render (front)
        if (bsp_check_box(&node->left_bbox, x, y, angle))
            // render (back)
    }
}

bsp_t bsp_create(uint32_t nodes_len, node_t *nodes, subsector_t *subsectors, seg_t *segs, vertex_t *vertexes)
{
    bsp_t bsp = {0};
    bsp.nodes = nodes;
    bsp.subsectors = subsectors;
    bsp.segs = segs;
    bsp.vertexes = vertexes;
    bsp.root_id = nodes_len - 1;
    return bsp;
}

void bsp_delete(bsp_t *bsp)
{
    if (bsp->nodes != NULL)
        free(bsp->nodes);

    if (bsp->subsectors != NULL)
        free(bsp->subsectors);

    if (bsp->segs != NULL)
        free(bsp->segs);
}
