#include "bsp.h"
#include "renderer/renderer.h"
#include <math.h>
#include "utils.h"
#include <string.h>
#include "logger.h"
#include "assets/asset.h"

#define SUB_SECTOR_IDENTIFIER 0x8000

#define LINE_TWO_SIDED 0x04
#define LINE_DONT_PEG_TOP 0x08
#define LINE_DONT_PEG_BOTTOM 0x10

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

static int16_t camera_x, camera_y, camera_z;
static float camera_angle;

static bool bsp_point_on_side(node_t *node)
{
    int16_t dx = camera_x - node->x_partition;
    int16_t dy = camera_y - node->y_partition;
    return (dx * node->dy_partition - dy * node->dx_partition) <= 0;
}

static bool bsp_check_box(bbox_t *bbox)
{
    int16_t			box_x;
    int16_t			box_y;
    // Find the corners of the box
    // that define the edges from current viewpoint.
    if (camera_x <= bbox->left)
	    box_x = 0;
    else if (camera_x < bbox->right)
	    box_x = 1;
    else
	    box_x = 2;
		
    if (camera_y >= bbox->top)
	    box_y = 0;
    else if (camera_y > bbox->bottom)
	    box_y = 1;
    else
	    box_y = 2;
		
    int16_t box_pos = (box_y << 2) + box_x;
    if (box_pos == 5) return true;
	
    int16_t x1 = *((int16_t*)bbox + check_coord[box_pos][0]);
    int16_t y1 = *((int16_t*)bbox + check_coord[box_pos][1]);
    int16_t x2 = *((int16_t*)bbox + check_coord[box_pos][2]);
    int16_t y2 = *((int16_t*)bbox + check_coord[box_pos][3]);
    
    // check clip list for an open space
    int16_t dx1 = x1 - camera_x;
    int16_t dy1 = y1 - camera_y;
    int16_t dx2 = x2 - camera_x;
    int16_t dy2 = y2 - camera_y;
    float angle1 = atan2(dy1, dx1) - camera_angle;
    float angle2 = atan2(dy2, dx2) - camera_angle;
	
    float span = u_normalize_angle(angle1 - angle2);

    if (span >= PI) return true;
    
    float tspan1 = u_normalize_angle(angle1 + H_FOV);
    float tspan2 = u_normalize_angle(H_FOV - angle2);
    return (tspan1 <= FOV || tspan1 < span + FOV) && (tspan2 <= FOV || tspan2 < span + FOV); 
}

static bool bsp_add_segment_to_fov(vertex_t a, vertex_t b, int16_t *x1, int16_t *x2, float *rw_angle)
{
    float angle1 = atan2(a.y - camera_y, a.x - camera_x);
    float angle2 = atan2(b.y - camera_y, b.x - camera_x);

    float span = u_normalize_angle(angle1 - angle2);
    if (span >= PI) return false;

    *rw_angle = angle1;

    angle1 -= camera_angle;
    angle2 -= camera_angle;

    float tspan1 = u_normalize_angle(angle1 + H_FOV);
    if (tspan1 > FOV)
    {
        if (tspan1 >= span + FOV)
             return false;
        angle1 = H_FOV;
    }

    float tspan2 = u_normalize_angle(H_FOV - angle2);
    if (tspan2 > FOV)
    {
        if (tspan2 >= span + FOV)
            return false;
        angle2 = -H_FOV;
    }

    *x1 = r_angle_to_x(angle1);
    *x2 = r_angle_to_x(angle2);

    return true; 
}

static void bsp_draw_portal_wall_range(bsp_t *bsp, seg_t *seg, int16_t front_sector_id, int16_t back_sector_id, int16_t x1, int16_t x2, float rw_angle)
{
    sector_t *front_sector = &bsp->sectors[front_sector_id];
    sector_t *back_sector = &bsp->sectors[back_sector_id];
    linedef_t *line = &bsp->linedefs[seg->linedef_id];
    sidedef_t *side = NULL;
    if (seg->direction)
        side = &bsp->sidedefs[line->back_sidedef_id];
    else
        side = &bsp->sidedefs[line->front_sidedef_id];

    char *upper_wall_texture = side->upper_texture_name;
    char *lower_wall_texture = side->lower_texture_name;
    char *ceil_texture = front_sector->ceil_texture_name;
    char *floor_texture = front_sector->floor_texture_name;
    int16_t light_level = front_sector->light_level;

    int16_t world_front_z1 = front_sector->ceil_z - camera_z;
    int16_t world_back_z1 = back_sector->ceil_z - camera_z;

    int16_t world_front_z2 = front_sector->floor_z - camera_z;
    int16_t world_back_z2 = back_sector->floor_z - camera_z;

    bool draw_wall = false;
    bool draw_ceil = false;
    bool draw_floor = false;
    bool draw_upper_wall = false;
    bool draw_lower_wall = false;

    if (strcmp(front_sector->ceil_texture_name, back_sector->ceil_texture_name) == 0 && strcmp(front_sector->ceil_texture_name, "F_SKY1") == 0)
        world_front_z1 = world_back_z1;

    if (world_front_z1 != world_back_z1 || front_sector->light_level != back_sector->light_level 
        || strcmp(ceil_texture, back_sector->ceil_texture_name) != 0)
    {
        draw_upper_wall = (upper_wall_texture[0] != '-') && world_back_z1 < world_front_z1;
        draw_ceil = world_front_z1 >= 0;
    }

    if (world_front_z2 != world_back_z2 || front_sector->light_level != back_sector->light_level
        || strcmp(floor_texture, back_sector->floor_texture_name) != 0)
    {
        draw_lower_wall = (lower_wall_texture[0] != '-') && world_back_z2 > world_front_z2;
        draw_floor = world_front_z2 <= 0;    
    }

    if (!draw_upper_wall && !draw_ceil && !draw_lower_wall && !draw_floor) return;

    float rw_normal_angle = u_convert_bams_to_radians(seg->angle) + PI_2;
    float offset_angle = rw_normal_angle - rw_angle;

    vertex_t *start_vertex = &bsp->vertexes[seg->start_vertex];
    float dx = camera_x - start_vertex->x;
    float dy = camera_y - start_vertex->y;
    float hypotenuse = sqrt(dx * dx + dy * dy);
    float rw_distance = hypotenuse * cos(offset_angle);

    float upper_tex_alt = world_front_z1;
    if (draw_upper_wall)
    {
        image_t *upper_texture = a_get_texture_by_name(upper_wall_texture);
        if (upper_texture == NULL) return;

        if ((line->flags & LINE_DONT_PEG_TOP) == 0)
        {
            float v_top = back_sector->ceil_z + upper_texture->height;
            upper_tex_alt = v_top - camera_z;
        }

        upper_tex_alt += side->y_offset;
    }

    float lower_tex_alt = world_front_z1;
    if (draw_lower_wall)
    {
        image_t *lower_texture = a_get_texture_by_name(lower_wall_texture);
        if (lower_texture == NULL) return;

        if ((line->flags & LINE_DONT_PEG_BOTTOM) == 0)
            lower_tex_alt = world_back_z2;

        lower_tex_alt += side->y_offset;
    }

    float rw_offset = hypotenuse * sin(offset_angle);
    rw_offset += seg->offset + side->x_offset;
    float rw_center_angle = rw_normal_angle - camera_angle;

    portal_wall_desc_t desc = {
        .draw_upper_wall = draw_upper_wall,
        .draw_lower_wall = draw_lower_wall,
        .draw_ceil = draw_ceil,
        .draw_floor = draw_floor,
        .x1 = x1,
        .x2 = x2,
        .light_level = light_level,
        .rw_normal_angle = rw_normal_angle,
        .rw_distance = rw_distance,
        .upper_tex_alt = upper_tex_alt,
        .lower_tex_alt = lower_tex_alt,
        .rw_offset = rw_offset,
        .rw_center_angle = rw_center_angle,
        .world_front_z1 = world_front_z1,
        .world_back_z1 = world_back_z1,
        .world_front_z2 = world_front_z2,
        .world_back_z2 = world_back_z2,
        .upper_wall_texture = upper_wall_texture,
        .lower_wall_texture = lower_wall_texture,
        .ceil_texture = ceil_texture,
        .floor_texture = floor_texture
    };

    r_draw_portal_wall_range(&desc);
}

static void bsp_draw_solid_wall_range(bsp_t *bsp, seg_t *seg, int16_t front_sector_id, int16_t x1, int16_t x2, float rw_angle)
{
    sector_t *front_sector = &bsp->sectors[front_sector_id];
    linedef_t *line = &bsp->linedefs[seg->linedef_id];
    sidedef_t *side = NULL;
    if (seg->direction)
        side = &bsp->sidedefs[line->back_sidedef_id];
    else
        side = &bsp->sidedefs[line->front_sidedef_id];

    char *wall_texture = side->mid_texture_name;
    char *ceil_texture = front_sector->ceil_texture_name;
    char *floor_texture = front_sector->floor_texture_name;
    int16_t light_level = front_sector->light_level;

    int16_t world_front_z1 = front_sector->ceil_z - camera_z;
    int16_t world_front_z2 = front_sector->floor_z - camera_z;

    bool draw_wall = wall_texture[0] != '-';
    bool draw_ceil = world_front_z1 > 0;
    bool draw_floor = world_front_z2 < 0;

    float rw_normal_angle = u_convert_bams_to_radians(seg->angle) + PI_2;
    float offset_angle = rw_normal_angle - rw_angle;

    vertex_t *start_vertex = &bsp->vertexes[seg->start_vertex];
    float dx = camera_x - start_vertex->x;
    float dy = camera_y - start_vertex->y;
    float hypotenuse = sqrt(dx * dx + dy * dy);
    float rw_distance = hypotenuse * cosf(offset_angle);
    
    float v_top = 0, middle_texture_alt = world_front_z1;
    if (line->flags & LINE_DONT_PEG_BOTTOM)
    {
        image_t *texture = a_get_texture_by_name(wall_texture);

        if (texture == NULL) 
        {
            DOOM_LOG_ERROR("Nao foi possivel achar a textura %.8s", wall_texture);
            return;
        }

        v_top = front_sector->floor_z + texture->height;
        middle_texture_alt = v_top - camera_z;
    }

    middle_texture_alt += side->y_offset;

    float rw_offset = hypotenuse * sin(offset_angle);
    rw_offset += seg->offset + side->x_offset;
    float rw_center_angle = rw_normal_angle - camera_angle;

    solid_wall_desc_t desc = {
        .draw_wall = draw_wall,
        .draw_ceil = draw_ceil,
        .draw_floor = draw_floor,
        .x1 = x1,
        .x2 = x2,
        .light_level = light_level,
        .rw_normal_angle = rw_normal_angle,
        .rw_distance = rw_distance,
        .middle_texture_alt = middle_texture_alt,
        .rw_offset = rw_offset,
        .rw_center_angle = rw_center_angle,
        .world_front_z1 = world_front_z1,
        .world_front_z2 = world_front_z2,
        .wall_texture = wall_texture,
        .ceil_texture = ceil_texture,
        .floor_texture = floor_texture
    };

    r_draw_solid_wall_range(&desc);
}

static void bsp_clip_portal_walls(bsp_t *bsp, seg_t *seg, int16_t front_sector_id, int16_t back_sector_id, int16_t x_start, int16_t x_end, float rw_angle)
{
    if (bsp->screen_range.size <= 0)
    {
        bsp->running_traverse = false;
        return;
    }

    bsp->screen_range.curr = bsp->screen_range.head;
    linked_list_t curr_wall = l_create_list_range(x_start, x_end);
    linked_list_t intersection = l_intersection(&bsp->screen_range, &curr_wall);
    if (intersection.size > 0)
    {
        if (intersection.size != curr_wall.size)
        {
            int16_t x = x_start, last_element = x_start;
            for (; intersection.curr != intersection.tail; l_next(&intersection))
            {
                int16_t curr_element = intersection.curr->next->element;
                if (curr_element - last_element > 1)
                {
                    bsp_draw_portal_wall_range(bsp, seg, front_sector_id, back_sector_id, x, last_element, rw_angle);
                    x = curr_element;
                }

                last_element = curr_element;
            }
            
            // Desenha até o último elemento da interseção
            bsp_draw_portal_wall_range(bsp, seg, front_sector_id, back_sector_id, x, intersection.curr->element, rw_angle);
        }
        else
            bsp_draw_portal_wall_range(bsp, seg, front_sector_id, back_sector_id, x_start, x_end - 1, rw_angle);
        
        l_delete_list(&intersection);
    }

    l_delete_list(&curr_wall);
}

static void bsp_clip_solid_wall(bsp_t *bsp, seg_t *seg, int16_t front_sector, int16_t x_start, int16_t x_end, float rw_angle)
{
    if (bsp->screen_range.size > 0)
    {
        bsp->screen_range.curr = bsp->screen_range.head;
        linked_list_t curr_wall = l_create_list_range(x_start, x_end);
        linked_list_t intersection = l_intersection_remove(&bsp->screen_range, &curr_wall);
        if (intersection.size > 0)
        {
            if (intersection.size != curr_wall.size)
            {
                int16_t x = x_start, last_element = x_start;
                for (; intersection.curr != intersection.tail; l_next(&intersection))
                {
                    int16_t curr_element = intersection.curr->next->element;
                    if (curr_element - last_element > 1)
                    {
                        bsp_draw_solid_wall_range(bsp, seg, front_sector, x, last_element, rw_angle);
                        x = curr_element;
                    }
                    
                    last_element = curr_element;
                }
                
                // Desenha até o último elemento da interseção
                bsp_draw_solid_wall_range(bsp, seg, front_sector, x, intersection.curr->element, rw_angle);
            }
            else
                bsp_draw_solid_wall_range(bsp, seg, front_sector, x_start, x_end - 1, rw_angle);
            
            l_delete_list(&intersection);
        }

        l_delete_list(&curr_wall);
    }
    else
        bsp->running_traverse = false;
}

static void bsp_render_subsector(bsp_t *bsp, int16_t subsector_id)
{
    subsector_t *sub = &bsp->subsectors[subsector_id];
    for (uint16_t i = 0; i < sub->seg_count; i++)
    {
        seg_t *seg = &bsp->segs[sub->first_seg_id + i];
        int16_t x1, x2;
        float rw_angle;
        if (bsp_add_segment_to_fov(bsp->vertexes[seg->start_vertex], bsp->vertexes[seg->end_vertex], &x1, &x2, &rw_angle))
        {
            if (x1 == x2) continue;
            
            int16_t front_sidedef, back_sidedef;
            
            if (seg->direction)
            {
                front_sidedef = bsp->linedefs[seg->linedef_id].back_sidedef_id;
                back_sidedef = bsp->linedefs[seg->linedef_id].front_sidedef_id;
            }
            else
            {
                front_sidedef = bsp->linedefs[seg->linedef_id].front_sidedef_id;
                back_sidedef = bsp->linedefs[seg->linedef_id].back_sidedef_id;
            }
            
            int16_t front_sector_id = bsp->sidedefs[front_sidedef].sector_id;
            if ((LINE_TWO_SIDED & bsp->linedefs[seg->linedef_id].flags) != 0)
            {
                int16_t back_sector_id = bsp->sidedefs[back_sidedef].sector_id;
                
                sector_t *front_sector = &bsp->sectors[front_sector_id];
                sector_t *back_sector = &bsp->sectors[back_sector_id];
                
                if (front_sector->ceil_z != back_sector->ceil_z || 
                    front_sector->floor_z != back_sector->floor_z)
                {
                    bsp_clip_portal_walls(bsp, seg, front_sector_id, back_sector_id, x1, x2, rw_angle);
                }
                    
                    
                if (strcmp(front_sector->ceil_texture_name, back_sector->ceil_texture_name) == 0 &&
                strcmp(back_sector->floor_texture_name, front_sector->floor_texture_name) == 0 &&
                bsp->sidedefs[front_sidedef].mid_texture_name[0] == '-') 
                {
                    continue;
                }
                
                bsp_clip_portal_walls(bsp, seg, front_sector_id, back_sector_id, x1, x2, rw_angle);
            }
            else
                bsp_clip_solid_wall(bsp, seg, front_sector_id, x1, x2, rw_angle);
        }
    }
}

static void bsp_render_traverse(bsp_t *bsp, int16_t node_id)
{
    if (!bsp->running_traverse) return;

    if (node_id & SUB_SECTOR_IDENTIFIER)
    {
        node_id = (node_id == -1) ? 0 : node_id & (~SUB_SECTOR_IDENTIFIER);
        bsp_render_subsector(bsp, node_id);
        return;
    }

    node_t *node = &bsp->nodes[node_id];

    if (bsp_point_on_side(node))
    {
        bsp_render_traverse(bsp, node->left_child);
        if (bsp_check_box(&node->right_bbox))
            bsp_render_traverse(bsp, node->right_child);
    }
    else
    {
        bsp_render_traverse(bsp, node->right_child);
        if (bsp_check_box(&node->left_bbox))
            bsp_render_traverse(bsp, node->left_child);
    }
}

bsp_t bsp_create(wad_reader_t *wdr, const char* level_name)
{
    bsp_t bsp = {0};
    uint32_t level_idx = 0;
    if (fh_get_value(&wdr->file_hash, level_name, &level_idx))
    {
        uint32_t nodes_size = 0;
        bsp.nodes = (node_t*)wdr_get_lump_data(wdr, level_idx + NODES_INDEX, 0, &nodes_size);
        nodes_size /= sizeof(node_t);
        bsp.root_id = nodes_size - 1;

        bsp.sectors = (sector_t*)wdr_get_lump_data(wdr, level_idx + SECTORS_INDEX, 0, NULL);
        bsp.subsectors = (subsector_t*)wdr_get_lump_data(wdr, level_idx + SUBSECTORS_INDEX, 0, NULL);
        bsp.segs = (seg_t*)wdr_get_lump_data(wdr, level_idx + SEGS_INDEX, 0, NULL);
        bsp.vertexes = (vertex_t*)wdr_get_lump_data(wdr, level_idx + VERTEXES_INDEX, 0, NULL);
        bsp.linedefs = (linedef_t*)wdr_get_lump_data(wdr, level_idx + LINEDEFS_INDEX, 0, NULL);
        bsp.sidedefs = (sidedef_t*)wdr_get_lump_data(wdr, level_idx + SIDEDEFS_INDEX, 0, NULL);

        uint32_t entities_count = 0;
        bsp.entities = (entity_t*)wdr_get_lump_data(wdr, level_idx + ENTITIES_INDEX, 0, &entities_count);
        entities_count /= sizeof(entity_t);
        bsp.entities_count = entities_count;

        camera_x = bsp.entities[0].pos_x;
        camera_y = bsp.entities[0].pos_y;
        camera_z = bsp_get_sub_sector_height(&bsp);
    }

    return bsp;
}

int16_t bsp_get_sub_sector_height(bsp_t *bsp)
{
    int16_t sub_sector_id = bsp->root_id;

    while((sub_sector_id & SUB_SECTOR_IDENTIFIER) == 0)
    {
        node_t *node = &bsp->nodes[sub_sector_id];
        sub_sector_id = bsp_point_on_side(node) ? node->left_child : node->right_child;
    }

    sub_sector_id = (sub_sector_id == -1) ? 0 : sub_sector_id & (~SUB_SECTOR_IDENTIFIER);
    subsector_t *subsector = &bsp->subsectors[sub_sector_id];
    seg_t *seg = &bsp->segs[subsector->first_seg_id];

    int16_t front_sidedef = seg->direction ? bsp->linedefs[seg->linedef_id].back_sidedef_id 
                                                            : 
                                            bsp->linedefs[seg->linedef_id].front_sidedef_id;

    int16_t front_sector_id = bsp->sidedefs[front_sidedef].sector_id;
    return bsp->sectors[front_sector_id].floor_z;
}

int16_t bsp_get_sub_sector_height_for_ent(bsp_t *bsp, int16_t x, int16_t y)
{
    int16_t sub_sector_id = bsp->root_id;

    while((sub_sector_id & SUB_SECTOR_IDENTIFIER) == 0)
    {
        node_t *node = &bsp->nodes[sub_sector_id];
        int16_t dx = x - node->x_partition;
        int16_t dy = y - node->y_partition;
        sub_sector_id = (dx * node->dy_partition - dy * node->dx_partition) <= 0 ? node->left_child : node->right_child;
    }

    sub_sector_id = (sub_sector_id == -1) ? 0 : sub_sector_id & (~SUB_SECTOR_IDENTIFIER);
    subsector_t *subsector = &bsp->subsectors[sub_sector_id];
    seg_t *seg = &bsp->segs[subsector->first_seg_id];

    int16_t front_sidedef = seg->direction ? bsp->linedefs[seg->linedef_id].back_sidedef_id 
                                                            : 
                                            bsp->linedefs[seg->linedef_id].front_sidedef_id;

    int16_t front_sector_id = bsp->sidedefs[front_sidedef].sector_id;
    return bsp->sectors[front_sector_id].floor_z;
}

vec3f_t bsp_get_player_spawn(bsp_t *bsp)
{
    int16_t sector_height = bsp_get_sub_sector_height(bsp);
    return (vec3f_t) { camera_x, camera_y, camera_z };
}

void bsp_update(bsp_t *bsp, vec3f_t pos, float angle)
{
    camera_x = (int16_t)pos.x;
    camera_y = (int16_t)pos.y;
    camera_z = (int16_t)pos.z;
    camera_angle = angle;
}

void bsp_render(bsp_t *bsp)
{
    bsp->running_traverse = true;
    bsp->screen_range = l_create_list_range(0, r_get_width());

    bsp_render_traverse(bsp, bsp->root_id);

    bsp->running_traverse = false;
    l_delete_list(&bsp->screen_range);
}

void bsp_render_sprites(bsp_t *bsp)
{

    vertex_t min, max;

    // Número de entidades carregadas
    // (Supondo que você tenha um campo bsp.num_entities)
    for (int i = 1; i < bsp->entities_count; i++) 
    {
        entity_t *ent = &bsp->entities[i];
        
        // Calcula vetor da câmera para a entidade
        float dx = ent->pos_x - camera_x;
        float dy = ent->pos_y - camera_y;
        float dist = sqrtf(dx*dx + dy*dy);

        // Calcula o ângulo relativo entre a câmera e o sprite
        float angle_to_sprite = u_normalize_angle(atan2f(dy, dx));
        float rw_angle = angle_to_sprite;
        
        angle_to_sprite -= camera_angle;

        float tspan1 = u_normalize_angle(angle_to_sprite + H_FOV);
        float tspan2 = u_normalize_angle(H_FOV - angle_to_sprite);
        if (tspan1 > FOV || tspan2 > FOV)
            continue;

        int16_t x = r_angle_to_x(angle_to_sprite);
        
        image_t *sprite = a_get_sprite_by_type(ent->type);
        if (sprite != NULL)
        {
            int16_t z1 = bsp_get_sub_sector_height_for_ent(bsp, ent->pos_x, ent->pos_y) - camera_z;
            float rw_scale = r_scale_from_global_angle(x, rw_angle, dist);
            r_draw_sprite(x, z1, sprite, rw_scale, dist);
        }
    }
}

void bsp_delete(bsp_t *bsp)
{
    if (bsp->entities != NULL)
        free(bsp->entities);

    if (bsp->nodes != NULL)
        free(bsp->nodes);

    if (bsp->subsectors != NULL)
        free(bsp->subsectors);

    if (bsp->sectors != NULL)
        free(bsp->sectors);

    if (bsp->segs != NULL)
        free(bsp->segs);

    if (bsp->vertexes != NULL)
        free(bsp->vertexes);

    if (bsp->linedefs != NULL)
        free(bsp->linedefs);

    if (bsp->sidedefs != NULL)
        free(bsp->sidedefs);
}
